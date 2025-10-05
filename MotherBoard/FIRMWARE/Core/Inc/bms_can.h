/**
  ******************************************************************************
  * @file           : bms_can.h
  * @brief          : CAN interface helper for BMS state machine
  ******************************************************************************
  */

#ifndef BMS_CAN_H
#define BMS_CAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>

/* Message IDs (adjust as needed for your CAN layout) */
#define BMS_CAN_ID_STATUS  0x100  // Pack voltage / SOC
#define BMS_CAN_ID_FAULT   0x101  // Fault codes
#define BMS_CAN_ID_BALANCE 0x102  // Balancing status

/* Function Prototypes -------------------------------------------------------*/

/**
  * @brief  Send BMS status message over CAN
  * @param  packVoltage_mV: Total pack voltage in millivolts
  * @param  SOC_percent: State of charge (0–100)
  * @retval HAL_StatusTypeDef
  */
HAL_StatusTypeDef BMS_CAN_SendStatus(uint16_t packVoltage_mV, uint8_t SOC_percent);

/**
  * @brief  Send a BMS fault message over CAN
  * @param  faultCode: Fault identifier (e.g., 1 = comms error, 2 = overvoltage)
  * @retval HAL_StatusTypeDef
  */
HAL_StatusTypeDef BMS_CAN_SendFault(uint8_t faultCode);

#ifdef __cplusplus
}
#endif

#endif /* BMS_CAN_H */
