/**
  ******************************************************************************
  * @file           : bms_sm.c
  * @brief          : State Machine implementation for STM32-based BMS firmware
  *                   Uses BQ79600 low-level functions to perform wakeup,
  *                   addressing, measurement, and balancing.
  ******************************************************************************
  */

#include "bms_state_machine.h"
#include "bms_can.h"
#include "SOC.h"
#include "bms_fault.h"
#include "stdio.h"   // for printf()

/* USER CODE BEGIN Private defines */
// Optional debug verbosity
#define BMS_DEBUG 1
/* USER CODE END Private defines */

/* USER CODE BEGIN Private function prototypes */
static void BMS_GotoState(bms_ctx_t *ctx, bms_state_t nextState);
static void BMS_HandleFault(bms_ctx_t *ctx, const char *reason);
/* USER CODE END Private function prototypes */

/**
  * @brief  Initialize the BMS state machine
  * @param  ctx: Pointer to BMS context
  * @retval None
  */
void BMS_Init(bms_ctx_t *ctx)
{
    ctx->state = BMS_WAKE;         // Start by waking up the BQ stack
    ctx->retries = 0;
    ctx->lastTransition = HAL_GetTick();

#if BMS_DEBUG
    printf("BMS: Initialized. Starting in WAKE state.\r\n");
#endif
}

/**
  * @brief  Run one iteration of the BMS state machine
  * @param  ctx: Pointer to BMS context
  * @retval None
  */
void BMS_Update(bms_ctx_t *ctx)
{
    HAL_StatusTypeDef status = HAL_OK;

    switch (ctx->state)
    {
    /* --------------------------------------------------------------
     * STATE: WAKE
     * --------------------------------------------------------------
     * Wake up the BQ79600 devices. This step toggles SPI pins manually
     * to send a WAKE ping, followed by a SEND_WAKE broadcast command.
     */
    case BMS_WAKE:
#if BMS_DEBUG
        printf("STATE: BMS_WAKE\r\n");
#endif
        status = BQ79600_WakeUp(TOTALBOARDS, false);
        if (status == HAL_OK)
            BMS_GotoState(ctx, BMS_AUTO_ADDRESS);
        else
            BMS_HandleFault(ctx, "Wakeup failed");
        break;

    /* --------------------------------------------------------------
     * STATE: AUTO ADDRESS
     * --------------------------------------------------------------
     * Automatically assign addresses to all BQ devices in the stack.
     * This ensures each device can be uniquely addressed by SPI.
     */
    case BMS_AUTO_ADDRESS:
#if BMS_DEBUG
        printf("STATE: BMS_AUTO_ADDRESS\r\n");
#endif
        status = SpiAutoAddress(TOTALBOARDS);
        if (status == HAL_OK)
            BMS_GotoState(ctx, BMS_MEASURE);
        else
            BMS_HandleFault(ctx, "Auto-addressing failed");
        break;


    /* --------------------------------------------------------------
     * STATE: MEASURE
     * --------------------------------------------------------------
     * Read cell voltages across the stack and print results.
     * On success, transition to IDLE state. On failure, fault.
     */
    case BMS_MEASURE:
    #if BMS_DEBUG
        printf("STATE: BMS_MEASURE\r\n");
    #endif
        status = stackVoltageRead(ACTIVECHANNELS);

        if (status == HAL_OK)
        {
            FaultCode_t fault;
            if (BMS_CheckForFaults(&fault)) {
                BMS_TriggerFault(fault);
                BMS_HandleFault(ctx, "Measurement fault detected");
                break;
            }

            // Continue as normal if no fault detected
            uint16_t avgVoltage = 3700 * 56; // placeholder
            uint8_t soc = getBatterySOC(avgVoltage / 56);
            BMS_CAN_SendStatus(avgVoltage, soc);
            BMS_GotoState(ctx, BMS_IDLE);
        }
        else
        {
            BMS_HandleFault(ctx, "Voltage read failed");
        }
        break;

/* --------------------------------------------------------------
 * STATE: IDLE
 * --------------------------------------------------------------
 * The system waits in this state between measurement or balancing cycles.
 * During this time, it monitors for faults and may periodically
 * perform CAN updates or transition back to MEASURE after a delay.
 */
	case BMS_IDLE:
#if BMS_DEBUG
	printf("STATE: BMS_IDLE\r\n");
#endif
{
	FaultCode_t fault;
	// Check for any runtime faults during idle
		if (BMS_CheckForFaults(&fault)) {
		BMS_TriggerFault(fault);
		BMS_HandleFault(ctx, "Idle fault detected");
		break;
	}

	// send periodic CAN keepalive
		static uint32_t lastCanSend = 0;
		if (HAL_GetTick() - lastCanSend > 2000) { // every 2 seconds
		BMS_CAN_SendStatus(0, 0); // TODO: replace with actual pack voltage/SOC vars
		lastCanSend = HAL_GetTick();
	}

	// After 1 second, transition back to measurement
		if (HAL_GetTick() - ctx->lastTransition > 1000) {
		BMS_GotoState(ctx, BMS_MEASURE);
		break;
	}

	// Stay in idle otherwise
		break;
}

    /* --------------------------------------------------------------
     * STATE: FAULT
     * --------------------------------------------------------------
     * A safety-related fault has been detected (SPI error, fault pin, etc.)
     * Immediately disable charge/discharge paths and assert CHARGE_SAFE_LV.
     */
    case BMS_FAULT:
#if BMS_DEBUG
        printf("STATE: BMS_FAULT\r\n");
#endif
        // Safety outputs
        HAL_GPIO_WritePin(CHARGE_EN_LV_GPIO_Port, CHARGE_EN_LV_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(DISCHARGE_EN_LV_GPIO_Port, DISCHARGE_EN_LV_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(CHARGE_SAFE_LV_GPIO_Port, CHARGE_SAFE_LV_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(FAN_EN_LV_GPIO_Port, FAN_EN_LV_Pin, GPIO_PIN_RESET);

        // Wait before attempting a safe restart (optional)
        HAL_Delay(500);
        // Could stay here indefinitely or attempt recovery
        BMS_GotoState(ctx, BMS_SHUTDOWN);
        break;

    /* --------------------------------------------------------------
     * STATE: SHUTDOWN
     * --------------------------------------------------------------
     * Final safe state. All high-power outputs are off.
     * Could be extended later to send CAN shutdown message.
     */
    case BMS_SHUTDOWN:
#if BMS_DEBUG
        printf("STATE: BMS_SHUTDOWN\r\n");
#endif
        HAL_GPIO_WritePin(CHARGE_EN_LV_GPIO_Port, CHARGE_EN_LV_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(DISCHARGE_EN_LV_GPIO_Port, DISCHARGE_EN_LV_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(FAN_EN_LV_GPIO_Port, FAN_EN_LV_Pin, GPIO_PIN_RESET);
        break;

    default:
        BMS_HandleFault(ctx, "Unknown state");
        break;
    }
}

/* ---------------------------------------------------------------------------
 * @brief  Transition helper for moving to a new state
 * --------------------------------------------------------------------------*/
static void BMS_GotoState(bms_ctx_t *ctx, bms_state_t nextState)
{
#if BMS_DEBUG
    printf("Transition: %d -> %d\r\n", ctx->state, nextState);
#endif
    ctx->state = nextState;
    ctx->lastTransition = HAL_GetTick();
    ctx->retries = 0;
}

/* ---------------------------------------------------------------------------
 * @brief  Fault handler helper function
 * --------------------------------------------------------------------------*/
static void BMS_HandleFault(bms_ctx_t *ctx, const char *reason)
{
#if BMS_DEBUG
    printf("FAULT: %s\r\n", reason);
#endif
    ctx->state = BMS_FAULT;
    ctx->retries = 0;
}
