/* BPGA - EDU-CIAA-NXP + FreeRTOS */

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "sapi.h"
#include "bpga_measurement.h"
#include "bpga_simulator.h"
#include "bpga_modbus_slave.h"

#define BPGA_HEARTBEAT_PRIORITY    (tskIDLE_PRIORITY + 1U)
#define BPGA_MEASUREMENT_PRIORITY  (tskIDLE_PRIORITY + 2U)
#define BPGA_SIMULATOR_PRIORITY    (tskIDLE_PRIORITY + 3U)
#define BPGA_MODBUS_PRIORITY       (tskIDLE_PRIORITY + 4U)

static void bpgaHeartbeatTask(void *parameters)
{
    TickType_t lastWake = xTaskGetTickCount();
    (void)parameters;
    for (;;) {
        gpioToggle(LEDB);
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(500U));
    }
}

static void bpgaFatalError(void)
{
    gpioWrite(LEDB, OFF);
    gpioWrite(LEDG, OFF);
    for (;;) {
        gpioToggle(LEDR);
        for (volatile uint32_t delay = 0U; delay < 1000000U; ++delay) {
            __asm volatile ("nop");
        }
    }
}

int main(void)
{
    BaseType_t status = pdPASS;

    boardConfig();
    gpioWrite(LEDB, OFF);
    gpioWrite(LEDG, OFF);
    gpioWrite(LEDR, OFF);

    status = bpgaMeasurementInit();
    if (status != pdPASS) {
        bpgaFatalError();
    }

    status = bpgaModbusSlaveInit();
    if (status != pdPASS) {
        bpgaFatalError();
    }

    status = xTaskCreate(bpgaMeasurementTask, "BPGA_DATA",
                         configMINIMAL_STACK_SIZE * 2U, NULL,
                         BPGA_MEASUREMENT_PRIORITY, NULL);
    if (status == pdPASS) {
        status = xTaskCreate(bpgaSimulatorTask, "BPGA_SIM",
                             configMINIMAL_STACK_SIZE * 2U, NULL,
                             BPGA_SIMULATOR_PRIORITY, NULL);
    }
    if (status == pdPASS) {
        status = xTaskCreate(bpgaHeartbeatTask, "BPGA_LED",
                             configMINIMAL_STACK_SIZE, NULL,
                             BPGA_HEARTBEAT_PRIORITY, NULL);
    }
    if (status == pdPASS) {
        status = xTaskCreate(bpgaModbusSlaveTask, "BPGA_MODBUS",
                             configMINIMAL_STACK_SIZE * 3U, NULL,
                             BPGA_MODBUS_PRIORITY, NULL);
    }
    if (status != pdPASS) {
        bpgaFatalError();
    }

    vTaskStartScheduler();
    bpgaFatalError();
    return 0;
}
