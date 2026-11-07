#ifndef BPGA_MODBUS_SLAVE_H
#define BPGA_MODBUS_SLAVE_H

#include "FreeRTOS.h"
#include "task.h"

#define BPGA_MODBUS_SLAVE_ADDRESS 1U
#define BPGA_MODBUS_SLAVE_BAUD_RATE 115200U

BaseType_t bpgaModbusSlaveInit(void);
void bpgaModbusSlaveTask(void *parameters);

#endif
