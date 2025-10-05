/**
  ******************************************************************************
  * @file           : bms_sm.h
  * @brief          : State Machine logic for STM32-based BMS firmware
  *                   Handles wakeup, addressing, measurement, balancing, and fault
  *                   transitions for the BQ79600-based battery management system.
  ******************************************************************************
  */

#ifndef BMS_SM_H
#define BMS_SM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "bq79600.h"
#include <stdbool.h>
#include <stdint.h>

/* USER CODE BEGIN Includes */
// Additional includes for timing, debug printing, etc.
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/

/**
  * @brief Enumerates all operational states of the BMS
  */
typedef enum {
    BMS_INIT = 0,        // Initial boot / system setup
    BMS_WAKE,            // Wake up BQ devices in stack
    BMS_AUTO_ADDRESS,    // Auto address BQ stack
    BMS_MEASURE,         // Read voltage stack measurements
    BMS_BALANCE,         // Perform simple balancing
    BMS_IDLE,            // Waiting between operations
    BMS_FAULT,           // Fault state (e.g., SPI, hardware, or safety)
    BMS_SHUTDOWN         // Safe shutdown sequence
} bms_state_t;

/**
  * @brief Context structure storing BMS state and timing information
  */
typedef struct {
    bms_state_t state;       // Current state
    uint32_t lastTransition; // Timestamp of last state transition
    uint8_t retries;         // Retry counter for state operations
} bms_ctx_t;

/* Exported function prototypes ---------------------------------------------*/

/**
  * @brief Initializes the BMS state machine
  * @param ctx: Pointer to BMS context structure
  * @retval None
  */
void BMS_Init(bms_ctx_t *ctx);

/**
  * @brief Executes one iteration of the BMS state machine
  * @param ctx: Pointer to BMS context structure
  * @retval None
  */
void BMS_Update(bms_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* BMS_SM_H */
