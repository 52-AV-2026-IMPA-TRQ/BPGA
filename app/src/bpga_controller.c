#include "bpga_controller.h"
#include "bpga_measurement.h"
#include "task.h"
#include <string.h>

static SemaphoreHandle_t controllerMutex;
static bpga_controller_snapshot_t controllerSnapshot;

BaseType_t bpgaControllerInit(void)
{
    memset(&controllerSnapshot, 0, sizeof(controllerSnapshot));
    controllerSnapshot.system_state = BPGA_MODBUS_SYSTEM_INITIALIZING;
    controllerSnapshot.status_flags = BPGA_MODBUS_STATUS_INITIALIZED;

    controllerMutex = xSemaphoreCreateMutex();
    return (controllerMutex != NULL) ? pdPASS : pdFAIL;
}

BaseType_t bpgaControllerGetSnapshot(bpga_controller_snapshot_t *output)
{
    if ((output == NULL) || (controllerMutex == NULL)) {
        return pdFAIL;
    }
    if (xSemaphoreTake(controllerMutex, pdMS_TO_TICKS(10U)) != pdTRUE) {
        return pdFAIL;
    }
    *output = controllerSnapshot;
    xSemaphoreGive(controllerMutex);
    return pdPASS;
}

static uint16_t buildStatusFlags(bpga_modbus_system_state_t state,
                                 const bpga_measurement_t *measurement)
{
    uint16_t flags = BPGA_MODBUS_STATUS_INITIALIZED;

    if (state == BPGA_MODBUS_SYSTEM_READY) {
        flags |= BPGA_MODBUS_STATUS_READY;
    }
    if (state == BPGA_MODBUS_SYSTEM_TEST_RUNNING) {
        flags |= BPGA_MODBUS_STATUS_TEST_ACTIVE;
    }
    if (measurement->source == BPGA_SOURCE_SIMULATED) {
        flags |= BPGA_MODBUS_STATUS_SIMULATION_ACTIVE;
    }
    if (((measurement->status_flags & BPGA_STATUS_RPM_VALID) != 0U) &&
        ((measurement->status_flags & BPGA_STATUS_VOLTAGE_VALID) != 0U) &&
        ((measurement->status_flags & BPGA_STATUS_CURRENT_VALID) != 0U)) {
        flags |= BPGA_MODBUS_STATUS_MEASUREMENTS_VALID;
    }
    return flags;
}

/* Temporary: READY after a fixed delay. To be replaced by real
 * initialization checks and interlocks. SAFE_STOPPED is reserved for a
 * future real safe stop. */
static void advanceState(bpga_controller_snapshot_t *snapshot,
                         TickType_t elapsedTicks)
{
    if ((snapshot->system_state == BPGA_MODBUS_SYSTEM_INITIALIZING) &&
        (elapsedTicks >= pdMS_TO_TICKS(BPGA_CONTROLLER_INIT_TIME_MS))) {
        snapshot->system_state = BPGA_MODBUS_SYSTEM_READY;
    }
}

void bpgaControllerTask(void *parameters)
{
    TickType_t lastWake = xTaskGetTickCount();
    const TickType_t startTick = lastWake;
    (void)parameters;

    for (;;) {
        bpga_measurement_t measurement;

        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(BPGA_CONTROLLER_PERIOD_MS));

        if ((bpgaMeasurementGetSnapshot(&measurement) != pdPASS) ||
            (xSemaphoreTake(controllerMutex, portMAX_DELAY) != pdTRUE)) {
            continue;
        }

        advanceState(&controllerSnapshot, (TickType_t)(xTaskGetTickCount() - startTick));
        controllerSnapshot.status_flags =
            buildStatusFlags(controllerSnapshot.system_state, &measurement);
        controllerSnapshot.system_substate = (uint16_t)measurement.simulator_state;

        if (measurement.rpm > controllerSnapshot.rpm_max) {
            controllerSnapshot.rpm_max = measurement.rpm;
        }
        if (measurement.voltage_mV > controllerSnapshot.voltage_max_mv) {
            controllerSnapshot.voltage_max_mv = measurement.voltage_mV;
        }
        if (measurement.current_mA > controllerSnapshot.current_max_ma) {
            controllerSnapshot.current_max_ma = measurement.current_mA;
        }
        controllerSnapshot.sample_counter++;

        xSemaphoreGive(controllerMutex);
    }
}
