#ifndef BMS_FAULT_H
#define BMS_FAULT_H

#include "main.h"
#include <stdbool.h>
#include <stdint.h>

/* Fault Codes */
typedef enum {
    FAULT_NONE = 0,
    FAULT_OVERVOLTAGE,
    FAULT_UNDERVOLTAGE,
    FAULT_OVERTEMP,
    FAULT_COMMS,
    FAULT_UNKNOWN
} FaultCode_t;

bool BMS_CheckForFaults(FaultCode_t *faultCode);
void BMS_TriggerFault(FaultCode_t code);

#endif
