#include "bms_fault.h"
#include "bq79600.h"
#include "bms_can.h"
#include <stdio.h>

// Example thresholds (adjust for your chemistry)
#define CELL_OVERVOLTAGE_MV 4250
#define CELL_UNDERVOLTAGE_MV 2500
#define TEMP_OVERTEMP_C     60
#define COMM_TIMEOUT_MS 1000 //1 second timeout

static uint32_t lastCommTime = 0;
static bool commInitialized = false;

extern uint16_t cellVoltages[ACTIVECHANNELS]; // from stackVoltageRead()
extern float boardTemperatures[2];            // example

bool BMS_CheckForFaults(FaultCode_t *faultCode)
{
    // Check voltages
    for (int i = 0; i < ACTIVECHANNELS; i++) {
        if (cellVoltages[i] > CELL_OVERVOLTAGE_MV) {
            *faultCode = FAULT_OVERVOLTAGE;
            return true;
        }
        if (cellVoltages[i] < CELL_UNDERVOLTAGE_MV) {
            *faultCode = FAULT_UNDERVOLTAGE;
            return true;
        }
    }

    // Check temperatures
    for (int i = 0; i < 2; i++) {
        if (boardTemperatures[i] > TEMP_OVERTEMP_C) {
            *faultCode = FAULT_OVERTEMP;
            return true;
        }
    }

    // ------------------------------------------------------------------
    // Communication timeout check
    // ------------------------------------------------------------------
    if (commInitialized) {
        uint32_t now = HAL_GetTick();
        if ((now - lastCommTime) > COMM_TIMEOUT_MS) {
            *faultCode = FAULT_COMMS;
            return true;
        }
    }

    *faultCode = FAULT_NONE;
    return false;
}

void BMS_RecordCommSuccess(void)
{
    lastCommTime = HAL_GetTick();
    commInitialized = true;
}

void BMS_TriggerFault(FaultCode_t code)
{
    printf("FAULT DETECTED: Code %d\r\n", code);

    // Broadcast over CAN
    BMS_CAN_SendFault(code);

    if (code == FAULT_COMMS) {
    // Disable outputs (same as FAULT state)
    HAL_GPIO_WritePin(CHARGE_EN_LV_GPIO_Port, CHARGE_EN_LV_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DISCHARGE_EN_LV_GPIO_Port, DISCHARGE_EN_LV_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(CHARGE_SAFE_LV_GPIO_Port, CHARGE_SAFE_LV_Pin, GPIO_PIN_SET);
    }
}
