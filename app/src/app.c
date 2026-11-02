/*
 * BPGA
 * Banco de Prueba de Generadores Aeronauticos
 * EDU-CIAA-NXP + FreeRTOS
 */

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "sapi.h"

#define BPGA_BLINK_PERIOD_MS  500U
#define BPGA_BLINK_PRIORITY   (tskIDLE_PRIORITY + 1U)

static void bpgaBlinkTask(void *parameters)
{
    TickType_t lastWakeTime;
    (void)parameters;
    lastWakeTime = xTaskGetTickCount();

    for (;;) {
        gpioToggle(LEDB);
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(BPGA_BLINK_PERIOD_MS));
    }
}

int main(void)
{
    BaseType_t taskStatus;

    boardConfig();
    gpioWrite(LEDB, OFF);
    gpioWrite(LEDR, OFF);

    taskStatus = xTaskCreate(
        bpgaBlinkTask,
        "BPGA_LED",
        configMINIMAL_STACK_SIZE,
        NULL,
        BPGA_BLINK_PRIORITY,
        NULL
    );

    configASSERT(taskStatus == pdPASS);
    vTaskStartScheduler();

    for (;;) {
        gpioToggle(LEDR);
        for (volatile uint32_t delay = 0U; delay < 1000000U; ++delay) {
            __asm volatile ("nop");
        }
    }
}
