#include "bpga_measurement.h"
#include <string.h>

#define BPGA_MEASUREMENT_QUEUE_LENGTH  1U

static QueueHandle_t measurementQueue;
static SemaphoreHandle_t snapshotMutex;
static bpga_measurement_t latestMeasurement;

BaseType_t bpgaMeasurementInit(void)
{
    memset(&latestMeasurement, 0, sizeof(latestMeasurement));
    measurementQueue = xQueueCreate(BPGA_MEASUREMENT_QUEUE_LENGTH,
                                    sizeof(bpga_measurement_t));
    snapshotMutex = xSemaphoreCreateMutex();

    if ((measurementQueue == NULL) || (snapshotMutex == NULL)) {
        return pdFAIL;
    }
    return pdPASS;
}

QueueHandle_t bpgaMeasurementQueue(void)
{
    return measurementQueue;
}

BaseType_t bpgaMeasurementGetSnapshot(bpga_measurement_t *measurement)
{
    if ((measurement == NULL) || (snapshotMutex == NULL)) {
        return pdFAIL;
    }

    if (xSemaphoreTake(snapshotMutex, pdMS_TO_TICKS(10U)) != pdTRUE) {
        return pdFAIL;
    }
    *measurement = latestMeasurement;
    xSemaphoreGive(snapshotMutex);
    return pdPASS;
}

void bpgaMeasurementTask(void *parameters)
{
    bpga_measurement_t received;
    (void)parameters;

    for (;;) {
        if (xQueueReceive(measurementQueue, &received, portMAX_DELAY) == pdTRUE) {
            if (xSemaphoreTake(snapshotMutex, portMAX_DELAY) == pdTRUE) {
                latestMeasurement = received;
                xSemaphoreGive(snapshotMutex);
            }
        }
    }
}
