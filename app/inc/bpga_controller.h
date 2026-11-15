#ifndef BPGA_CONTROLLER_H
#define BPGA_CONTROLLER_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "bpga_modbus_map.h"

#define BPGA_CONTROLLER_PERIOD_MS 20U
#define BPGA_CONTROLLER_INIT_TIME_MS 1000U

typedef struct {
    bpga_modbus_system_state_t system_state;
    uint16_t system_substate;
    uint16_t status_flags;
    uint16_t control_flags;
    uint16_t active_profile_slot;
    uint16_t active_profile_revision;
    uint16_t last_transition_reason;
    uint32_t test_elapsed_ms;
    uint32_t rpm_max;
    uint32_t voltage_max_mv;
    uint32_t current_max_ma;
    uint32_t energy_wh;
    uint32_t sample_counter;
} bpga_controller_snapshot_t;

BaseType_t bpgaControllerInit(void);
BaseType_t bpgaControllerGetSnapshot(bpga_controller_snapshot_t *output);
void bpgaControllerTask(void *parameters);

#endif
