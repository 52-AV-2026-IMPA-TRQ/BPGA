#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "bpga_modbus_map.h"

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

/**
 * @brief Read identification block (16 input registers at 0x0000-0x000F).
 *
 * Thread-safety: Call serially until polling cycle is incorporated.
 *
 * @param[out] output Pointer to identification structure.
 * @return true if read and decoded successfully, false otherwise.
 *         output is not modified on error.
 */
bool bpga_modbus_master_read_identification(
    bpga_modbus_identification_t *output
);

/**
 * @brief Read snapshot block (32 input registers at 0x0010-0x002F).
 *
 * Thread-safety: Call serially until polling cycle is incorporated.
 *
 * @param[out] output Pointer to snapshot structure.
 * @return true if read and decoded successfully, false otherwise.
 *         output is not modified on error.
 */
bool bpga_modbus_master_read_snapshot(
    bpga_modbus_snapshot_t *output
);

/**
 * @brief Read diagnostics block (16 input registers at 0x0040-0x004F).
 *
 * Thread-safety: Call serially until polling cycle is incorporated.
 *
 * @param[out] output Pointer to diagnostics structure.
 * @return true if read and decoded successfully, false otherwise.
 *         output is not modified on error.
 */
bool bpga_modbus_master_read_diagnostics(
    bpga_modbus_diagnostics_t *output
);

#ifdef __cplusplus
}
#endif
