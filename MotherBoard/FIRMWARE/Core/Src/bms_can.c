/**
  ******************************************************************************
  * @file           : bms_can.c
  * @brief          : Implements CAN transmit helpers for BMS firmware
  ******************************************************************************
  */

#include "bms_can.h"
#include "stdio.h"

/* External CAN handle from main.c */
extern CAN_HandleTypeDef hcan1;

/**
  * @brief  Send BMS status message (pack voltage + SOC)
  */
HAL_StatusTypeDef BMS_CAN_SendStatus(uint32_t packVoltage_mV, uint8_t SOC_percent)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint8_t TxData[8] = {0};
    uint32_t TxMailbox;

    // Scale voltage to 0.1 V units (decivolts)
    // e.g. 235,000 mV → 2350 (0.1 V units)
    uint16_t packVoltage_dV = packVoltage_mV / 100;

    TxHeader.StdId = BMS_CAN_ID_STATUS;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.DLC = 8;
    TxHeader.TransmitGlobalTime = DISABLE;

    TxData[0] = (packVoltage_dV >> 8) & 0xFF;
    TxData[1] = packVoltage_dV & 0xFF;
    TxData[2] = SOC_percent;

    HAL_StatusTypeDef status = HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox);

#if 1
    if (status == HAL_OK)
        printf("CAN TX: STATUS | V=%.1fV | SOC=%d%%\r\n", packVoltage_dV / 10.0, SOC_percent);
    else
        printf("CAN TX ERROR: STATUS\r\n");
#endif

    return status;
}


/**
  * @brief  Send BMS fault message
  */
HAL_StatusTypeDef BMS_CAN_SendFault(uint8_t faultCode)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint8_t TxData[8] = {0};
    uint32_t TxMailbox;

    // Example layout:
    // Byte0: Fault Code
    // Bytes1–7: Reserved
    TxHeader.StdId = BMS_CAN_ID_FAULT;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.DLC = 8;
    TxHeader.TransmitGlobalTime = DISABLE;

    TxData[0] = faultCode;

    HAL_StatusTypeDef status = HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox);

#if 1
    if (status == HAL_OK)
        printf("CAN TX: FAULT | Code=%d\r\n", faultCode);
    else
        printf("CAN TX ERROR: FAULT\r\n");
#endif

    return status;
}
