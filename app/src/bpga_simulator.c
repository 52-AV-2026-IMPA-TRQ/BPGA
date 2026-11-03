#include "bpga_simulator.h"
#include "bpga_measurement.h"
#include "sapi.h"

#define BPGA_NOMINAL_RPM          3000U
#define BPGA_NOMINAL_VOLTAGE_MV   220000U
#define BPGA_NOMINAL_CURRENT_MA   3000U

static uint32_t ramp(uint32_t elapsed, uint32_t duration, uint32_t target)
{
    if (elapsed >= duration) {
        return target;
    }
    return (uint32_t)(((uint64_t)target * elapsed) / duration);
}

static uint32_t triangle(uint32_t elapsed, uint32_t period, uint32_t amplitude)
{
    uint32_t phase = elapsed % period;
    uint32_t half = period / 2U;
    if (phase < half) {
        return (uint32_t)(((uint64_t)amplitude * phase) / half);
    }
    return (uint32_t)(((uint64_t)amplitude * (period - phase)) / half);
}

static void buildMeasurement(uint32_t cycleMs, bpga_measurement_t *m)
{
    m->rpm = 0U;
    m->voltage_mV = 0U;
    m->current_mA = 0U;
    m->status_flags = BPGA_STATUS_SIMULATED;
    m->source = BPGA_SOURCE_SIMULATED;

    if (cycleMs < 2000U) {
        m->simulator_state = BPGA_SIM_STOPPED;
    } else if (cycleMs < 8000U) {
        uint32_t t = cycleMs - 2000U;
        m->simulator_state = BPGA_SIM_ACCELERATING;
        m->rpm = ramp(t, 6000U, BPGA_NOMINAL_RPM);
        if (t > 2000U) {
            m->voltage_mV = ramp(t - 2000U, 4000U, BPGA_NOMINAL_VOLTAGE_MV);
        }
    } else if (cycleMs < 12000U) {
        m->simulator_state = BPGA_SIM_NO_LOAD;
        m->rpm = BPGA_NOMINAL_RPM;
        m->voltage_mV = BPGA_NOMINAL_VOLTAGE_MV;
        m->current_mA = 150U;
    } else if (cycleMs < 18000U) {
        uint32_t t = cycleMs - 12000U;
        m->simulator_state = BPGA_SIM_LOADING;
        m->rpm = BPGA_NOMINAL_RPM;
        m->voltage_mV = BPGA_NOMINAL_VOLTAGE_MV - ramp(t, 6000U, 2500U);
        m->current_mA = 150U + ramp(t, 6000U, BPGA_NOMINAL_CURRENT_MA - 150U);
    } else if (cycleMs < 28000U) {
        uint32_t t = cycleMs - 18000U;
        m->simulator_state = BPGA_SIM_STABLE;
        m->rpm = BPGA_NOMINAL_RPM - 10U + triangle(t, 1000U, 20U);
        m->voltage_mV = BPGA_NOMINAL_VOLTAGE_MV - 1500U + triangle(t, 1200U, 3000U);
        m->current_mA = BPGA_NOMINAL_CURRENT_MA - 50U + triangle(t, 800U, 100U);
    } else if (cycleMs < 32000U) {
        uint32_t t = cycleMs - 28000U;
        m->simulator_state = BPGA_SIM_UNLOADING;
        m->rpm = BPGA_NOMINAL_RPM;
        m->voltage_mV = 218500U + ramp(t, 4000U, 1500U);
        m->current_mA = BPGA_NOMINAL_CURRENT_MA - ramp(t, 4000U, 2850U);
    } else if (cycleMs < 38000U) {
        uint32_t t = cycleMs - 32000U;
        m->simulator_state = BPGA_SIM_DECELERATING;
        m->rpm = BPGA_NOMINAL_RPM - ramp(t, 6000U, BPGA_NOMINAL_RPM);
        m->voltage_mV = BPGA_NOMINAL_VOLTAGE_MV - ramp(t, 6000U, BPGA_NOMINAL_VOLTAGE_MV);
        m->current_mA = 150U - ramp(t, 6000U, 150U);
    } else {
        m->simulator_state = BPGA_SIM_STOPPED;
    }

    if (m->rpm > 0U) {
        m->status_flags |= BPGA_STATUS_RPM_VALID;
    }
    if (m->voltage_mV > 0U) {
        m->status_flags |= BPGA_STATUS_VOLTAGE_VALID;
    }
    m->status_flags |= BPGA_STATUS_CURRENT_VALID;
}

void bpgaSimulatorTask(void *parameters)
{
    bpga_measurement_t measurement = {0};
    TickType_t lastWake = xTaskGetTickCount();
    uint32_t elapsedMs = 0U;
    QueueHandle_t queue = bpgaMeasurementQueue();
    (void)parameters;

    gpioWrite(LEDG, ON);

    for (;;) {
        measurement.sequence++;
        measurement.timestamp_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
        buildMeasurement(elapsedMs, &measurement);
        xQueueOverwrite(queue, &measurement);

        elapsedMs += BPGA_SIMULATOR_PERIOD_MS;
        if (elapsedMs >= BPGA_SIMULATOR_CYCLE_MS) {
            elapsedMs = 0U;
        }
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(BPGA_SIMULATOR_PERIOD_MS));
    }
}
