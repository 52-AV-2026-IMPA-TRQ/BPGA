#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BPGA_MODBUS_MASTER_DISABLED = 0,
    BPGA_MODBUS_MASTER_STOPPED,
    BPGA_MODBUS_MASTER_STARTING,
    BPGA_MODBUS_MASTER_READY,
    BPGA_MODBUS_MASTER_ERROR
} bpga_modbus_master_state_t;

typedef struct {
    bpga_modbus_master_state_t state;
    bool enabled;
    bool initialized;
    uint16_t slave_address;
    uint32_t baud_rate;
    int uart_port;
    int tx_gpio;
    int rx_gpio;
    int rts_gpio;
    int last_error;
} bpga_modbus_master_status_t;

bool bpga_modbus_master_init(void);

void bpga_modbus_master_deinit(void);

bool bpga_modbus_master_is_ready(void);

bpga_modbus_master_status_t bpga_modbus_master_get_status(void);

#ifdef __cplusplus
}
#endif
