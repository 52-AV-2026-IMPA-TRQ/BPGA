#ifndef BPGA_MEASUREMENT_H
#define BPGA_MEASUREMENT_H

#include <stdbool.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#define BPGA_STATUS_RPM_VALID        (1UL << 0)
#define BPGA_STATUS_VOLTAGE_VALID    (1UL << 1)
#define BPGA_STATUS_CURRENT_VALID    (1UL << 2)
#define BPGA_STATUS_SIMULATED        (1UL << 8)

typedef enum {
    BPGA_SOURCE_SIMULATED = 0,
    BPGA_SOURCE_HARDWARE = 1
} bpga_data_source_t;

typedef enum {
    BPGA_SIM_STOPPED = 0,
    BPGA_SIM_ACCELERATING,
    BPGA_SIM_NO_LOAD,
    BPGA_SIM_LOADING,
    BPGA_SIM_STABLE,
    BPGA_SIM_UNLOADING,
    BPGA_SIM_DECELERATING
} bpga_sim_state_t;

typedef struct {
    uint32_t sequence;
    uint32_t timestamp_ms;
    uint32_t rpm;
    uint32_t voltage_mV;
    uint32_t current_mA;
    uint32_t status_flags;
    bpga_data_source_t source;
    bpga_sim_state_t simulator_state;
} bpga_measurement_t;

BaseType_t bpgaMeasurementInit(void);
QueueHandle_t bpgaMeasurementQueue(void);
BaseType_t bpgaMeasurementGetSnapshot(bpga_measurement_t *measurement);
void bpgaMeasurementTask(void *parameters);

#endif
