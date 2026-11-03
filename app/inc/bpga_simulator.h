#ifndef BPGA_SIMULATOR_H
#define BPGA_SIMULATOR_H

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#define BPGA_SIMULATOR_PERIOD_MS  20U
#define BPGA_SIMULATOR_CYCLE_MS   40000U

void bpgaSimulatorTask(void *parameters);

#endif
