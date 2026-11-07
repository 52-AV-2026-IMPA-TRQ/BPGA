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

typedef struct {
    bool identification_valid;
    bool snapshot_valid;
    bool diagnostics_valid;

    bpga_modbus_identification_t identification;
    bpga_modbus_snapshot_t snapshot;
    bpga_modbus_diagnostics_t diagnostics;

    uint64_t last_success_us;
    uint64_t last_error_us;
    int last_error;

    uint32_t successful_reads;
    uint32_t failed_reads;
} bpga_modbus_master_cache_t;

typedef struct {
    bool enabled;
    bool running;
    uint32_t period_ms;
    uint32_t completed_cycles;
    uint32_t failed_cycles;
} bpga_modbus_poll_status_t;

/*
 * Before the first successful mutex creation, serialize all API calls,
 * including init. Afterwards, access is serialized internally.
 * The API must not be called from an ISR.
 */
bool bpga_modbus_master_init(void);

void bpga_modbus_master_deinit(void);

bool bpga_modbus_master_is_ready(void);

/* On lock failure, returns an ERROR status with last_error set to ESP_FAIL. */
bpga_modbus_master_status_t bpga_modbus_master_get_status(void);

/**
 * Copy the complete cache to output. Returns false if output is NULL or the
 * mutex cannot be acquired; output is unchanged on failure.
 */
bool bpga_modbus_master_get_cache(
    bpga_modbus_master_cache_t *output
);

/**
 * Invalidate all cached blocks and reset their data, counters, and timestamps.
 * If the mutex cannot be acquired, the cache is unchanged.
 */
void bpga_modbus_master_clear_cache(void);

/*
 * Polling only updates the Modbus cache; it does not change the simulator or
 * the active data source. These functions must not be called from an ISR.
 * Starting is idempotent; stopping is safe and repeatable.
 * If stopping times out, the task remains running until it can exit.
 * A cycle is not counted if its status cannot be locked.
 */
bool bpga_modbus_master_start_polling(void);

void bpga_modbus_master_stop_polling(void);

/* On lock failure, returns running=false without changing the polling state. */
bpga_modbus_poll_status_t bpga_modbus_master_get_poll_status(void);

/**
 * @brief Read identification block (16 input registers at 0x0000-0x000F).
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
