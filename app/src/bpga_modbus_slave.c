#include "bpga_modbus_slave.h"
#include "bpga_modbus_map.h"
#include "bpga_measurement.h"
#include "bpga_controller.h"
#include "sapi.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define BPGA_MODBUS_REQUEST_SIZE 8U
#define BPGA_MODBUS_RECEIVE_BUFFER_SIZE 32U
#define BPGA_MODBUS_RESPONSE_SIZE (5U + 2U * BPGA_MODBUS_MAX_READ_REGISTERS)
#define BPGA_MODBUS_MAX_READ_REGISTERS 125U
#define BPGA_MODBUS_EXCEPTION_ILLEGAL_FUNCTION 0x01U
#define BPGA_MODBUS_EXCEPTION_ILLEGAL_ADDRESS 0x02U
#define BPGA_MODBUS_RX_LED_PULSE_MS 100U
#define BPGA_MODBUS_ERROR_LED_PULSE_MS 400U
#define BPGA_MODBUS_USB_BAUD_RATE 115200U
#define BPGA_MODBUS_USB_INTERVAL_MS 500U
#define BPGA_MODBUS_USB_LINE_SIZE 128U
#define BPGA_MODBUS_UART_DIAG_INTERVAL_MS 200U

typedef struct {
    gpioMap_t pin;
    TickType_t started;
    bool active;
} diagnosticLed;

typedef struct {
    diagnosticLed receive;
    diagnosticLed shortFrame;
    diagnosticLed longFrame;
} diagnosticLeds;

typedef struct {
    uint32_t overrun;
    uint32_t parity;
    uint32_t framing;
    uint32_t breakInt;
    uint32_t rxError;
    TickType_t lastLog;
} uartDiagnostics;

typedef struct {
    uint8_t line[BPGA_MODBUS_USB_LINE_SIZE];
    size_t length;
    size_t position;
    TickType_t lastStart;
    bool started;
} diagnosticOutput;

static const uint16_t identification[BPGA_MODBUS_IDENTIFICATION_COUNT] = {
    [BPGA_MODBUS_IR_DEVICE_MAGIC - BPGA_MODBUS_IDENTIFICATION_START] =
        BPGA_MODBUS_DEVICE_MAGIC,
    [BPGA_MODBUS_IR_PROTOCOL_VERSION - BPGA_MODBUS_IDENTIFICATION_START] =
        BPGA_MODBUS_VERSION_ENCODE(BPGA_MODBUS_PROTOCOL_VERSION_MAJOR,
                                  BPGA_MODBUS_PROTOCOL_VERSION_MINOR),
    [BPGA_MODBUS_IR_REGISTER_MAP_VERSION - BPGA_MODBUS_IDENTIFICATION_START] =
        BPGA_MODBUS_VERSION_ENCODE(BPGA_MODBUS_REGISTER_MAP_VERSION_MAJOR,
                                  BPGA_MODBUS_REGISTER_MAP_VERSION_MINOR),
    [BPGA_MODBUS_IR_DEVICE_TYPE - BPGA_MODBUS_IDENTIFICATION_START] = 1U,
};

static void pulseLed(diagnosticLed *led, TickType_t now)
{
    led->started = now;
    led->active = true;
    gpioWrite(led->pin, ON);
}

static void updateLed(diagnosticLed *led, TickType_t now, TickType_t pulseTicks)
{
    if (led->active && (TickType_t)(now - led->started) >= pulseTicks) {
        gpioWrite(led->pin, OFF);
        led->active = false;
    }
}

static uint16_t modbusCrc(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFFU;
    for (size_t i = 0U; i < length; ++i) {
        crc ^= data[i];
        for (unsigned int bit = 0U; bit < 8U; ++bit) {
            crc = (crc & 1U) != 0U ? (uint16_t)((crc >> 1U) ^ 0xA001U)
                                   : (uint16_t)(crc >> 1U);
        }
    }
    return crc;
}

static void appendText(diagnosticOutput *output, const char *text)
{
    while (*text != '\0') {
        output->line[output->length++] = (uint8_t)*text++;
    }
}

static void appendDecimal(diagnosticOutput *output, uint32_t value)
{
    char digits[10];
    size_t count = 0U;

    do {
        digits[count++] = (char)('0' + value % 10U);
        value /= 10U;
    } while (value != 0U);

    while (count != 0U) {
        output->line[output->length++] = (uint8_t)digits[--count];
    }
}

static void appendHexByte(diagnosticOutput *output, uint8_t value)
{
    static const char digits[] = "0123456789ABCDEF";

    output->line[output->length++] = (uint8_t)digits[value >> 4U];
    output->line[output->length++] = (uint8_t)digits[value & 0x0FU];
}

static void appendHexWord(diagnosticOutput *output, uint16_t value)
{
    static const char digits[] = "0123456789ABCDEF";

    output->line[output->length++] = (uint8_t)digits[(value >> 12U) & 0x0FU];
    output->line[output->length++] = (uint8_t)digits[(value >> 8U) & 0x0FU];
    output->line[output->length++] = (uint8_t)digits[(value >> 4U) & 0x0FU];
    output->line[output->length++] = (uint8_t)digits[value & 0x0FU];
}

static void queueDiagnostic(diagnosticOutput *output, const uint8_t *request,
                            size_t length, bool overflow, TickType_t now,
                            TickType_t intervalTicks)
{
    if (output->position < output->length ||
        (output->started && (TickType_t)(now - output->lastStart) < intervalTicks)) {
        return;
    }

    output->length = 0U;
    output->position = 0U;
    if (overflow) {
        appendText(output, "RX overflow\r\n");
        return;
    }

    appendText(output, "RX len=");
    appendDecimal(output, length);
    appendText(output, " data=");
    for (size_t i = 0U; i < length; ++i) {
        if (i != 0U) {
            appendText(output, " ");
        }
        appendHexByte(output, request[i]);
    }
    if (length == BPGA_MODBUS_REQUEST_SIZE) {
        uint16_t receivedCrc = (uint16_t)request[6] | ((uint16_t)request[7] << 8U);
        appendText(output, modbusCrc(request, BPGA_MODBUS_REQUEST_SIZE - 2U) == receivedCrc ?
                           " CRC=OK" : " CRC=BAD");
    }
    appendText(output, "\r\n");
}

static void transmitDiagnostic(diagnosticOutput *output)
{
    if (output->position < output->length && uartTxReady(UART_USB)) {
        if (output->position == 0U) {
            output->lastStart = xTaskGetTickCount();
            output->started = true;
        }
        uartTxWrite(UART_USB, output->line[output->position++]);
    }
}

static void sendResponse(uint8_t *response, size_t length)
{
    uint16_t crc = modbusCrc(response, length);
    response[length] = (uint8_t)crc;
    response[length + 1U] = (uint8_t)(crc >> 8U);
    uartWriteByteArray(UART_485, response, (uint32_t)(length + 2U));
}

static void buildInputRegisterImage(uint16_t *registers, size_t register_count)
{
    if ((registers == NULL) || (register_count < BPGA_MODBUS_INPUT_REGISTER_COUNT)) {
        return;
    }

    memset(registers, 0, register_count * sizeof(uint16_t));

    registers[BPGA_MODBUS_IR_DEVICE_MAGIC] = identification[BPGA_MODBUS_IR_DEVICE_MAGIC - BPGA_MODBUS_IDENTIFICATION_START];
    registers[BPGA_MODBUS_IR_PROTOCOL_VERSION] = identification[BPGA_MODBUS_IR_PROTOCOL_VERSION - BPGA_MODBUS_IDENTIFICATION_START];
    registers[BPGA_MODBUS_IR_REGISTER_MAP_VERSION] = identification[BPGA_MODBUS_IR_REGISTER_MAP_VERSION - BPGA_MODBUS_IDENTIFICATION_START];
    registers[BPGA_MODBUS_IR_DEVICE_TYPE] = identification[BPGA_MODBUS_IR_DEVICE_TYPE - BPGA_MODBUS_IDENTIFICATION_START];

    bpga_measurement_t snapshot = {0};
    if (bpgaMeasurementGetSnapshot(&snapshot) != pdPASS) {
        return;
    }

    uint16_t sequence_pair = (uint16_t)(snapshot.sequence << 1U);
    registers[BPGA_MODBUS_IR_SNAPSHOT_SEQUENCE_BEGIN] = sequence_pair;
    registers[BPGA_MODBUS_IR_SNAPSHOT_SEQUENCE_END] = sequence_pair;

    bpga_controller_snapshot_t controller = {0};
    if (bpgaControllerGetSnapshot(&controller) != pdPASS) {
        controller.system_state = BPGA_MODBUS_SYSTEM_INVALID;
        controller.system_substate = (uint16_t)snapshot.simulator_state;
    }

    registers[BPGA_MODBUS_IR_SYSTEM_STATE] = (uint16_t)controller.system_state;
    registers[BPGA_MODBUS_IR_SYSTEM_SUBSTATE] = controller.system_substate;
    registers[BPGA_MODBUS_IR_STATUS_FLAGS] = controller.status_flags;
    registers[BPGA_MODBUS_IR_CONTROL_FLAGS] = controller.control_flags;
    registers[BPGA_MODBUS_IR_ACTIVE_PROFILE_SLOT] = controller.active_profile_slot;
    registers[BPGA_MODBUS_IR_ACTIVE_PROFILE_REVISION] = controller.active_profile_revision;
    registers[BPGA_MODBUS_IR_LAST_TRANSITION_REASON] = controller.last_transition_reason;

    registers[BPGA_MODBUS_IR_TEST_ELAPSED_MS_HI] = (uint16_t)(controller.test_elapsed_ms >> 16U);
    registers[BPGA_MODBUS_IR_TEST_ELAPSED_MS_LO] = (uint16_t)(controller.test_elapsed_ms & 0xFFFFU);
    registers[BPGA_MODBUS_IR_RPM_MAX_HI] = (uint16_t)(controller.rpm_max >> 16U);
    registers[BPGA_MODBUS_IR_RPM_MAX_LO] = (uint16_t)(controller.rpm_max & 0xFFFFU);
    registers[BPGA_MODBUS_IR_VOLTAGE_MAX_MV_HI] = (uint16_t)(controller.voltage_max_mv >> 16U);
    registers[BPGA_MODBUS_IR_VOLTAGE_MAX_MV_LO] = (uint16_t)(controller.voltage_max_mv & 0xFFFFU);
    registers[BPGA_MODBUS_IR_CURRENT_MAX_MA_HI] = (uint16_t)(controller.current_max_ma >> 16U);
    registers[BPGA_MODBUS_IR_CURRENT_MAX_MA_LO] = (uint16_t)(controller.current_max_ma & 0xFFFFU);
    registers[BPGA_MODBUS_IR_ENERGY_WH_HI] = (uint16_t)(controller.energy_wh >> 16U);
    registers[BPGA_MODBUS_IR_ENERGY_WH_LO] = (uint16_t)(controller.energy_wh & 0xFFFFU);
    registers[BPGA_MODBUS_IR_SAMPLE_COUNTER_HI] = (uint16_t)(controller.sample_counter >> 16U);
    registers[BPGA_MODBUS_IR_SAMPLE_COUNTER_LO] = (uint16_t)(controller.sample_counter & 0xFFFFU);

    registers[BPGA_MODBUS_IR_UPTIME_HI] = (uint16_t)(snapshot.timestamp_ms >> 16U);
    registers[BPGA_MODBUS_IR_UPTIME_LO] = (uint16_t)(snapshot.timestamp_ms & 0xFFFFU);

    registers[BPGA_MODBUS_IR_RPM_HI] = (uint16_t)(snapshot.rpm >> 16U);
    registers[BPGA_MODBUS_IR_RPM_LO] = (uint16_t)(snapshot.rpm & 0xFFFFU);

    registers[BPGA_MODBUS_IR_VOLTAGE_MV_HI] = (uint16_t)(snapshot.voltage_mV >> 16U);
    registers[BPGA_MODBUS_IR_VOLTAGE_MV_LO] = (uint16_t)(snapshot.voltage_mV & 0xFFFFU);

    registers[BPGA_MODBUS_IR_CURRENT_MA_HI] = (uint16_t)(snapshot.current_mA >> 16U);
    registers[BPGA_MODBUS_IR_CURRENT_MA_LO] = (uint16_t)(snapshot.current_mA & 0xFFFFU);

    uint64_t power_64 = ((uint64_t)snapshot.voltage_mV * (uint64_t)snapshot.current_mA) / 1000000U;
    registers[BPGA_MODBUS_IR_POWER_W_HI] = (uint16_t)(power_64 >> 16U);
    registers[BPGA_MODBUS_IR_POWER_W_LO] = (uint16_t)(power_64 & 0xFFFFU);

    TickType_t now_ticks = xTaskGetTickCount();
    uint32_t now_ms = (uint32_t)(now_ticks * portTICK_PERIOD_MS);
    uint32_t age_ms = now_ms - snapshot.timestamp_ms;
    registers[BPGA_MODBUS_IR_SAMPLE_AGE_MS] = (age_ms > UINT16_MAX) ? UINT16_MAX : (uint16_t)age_ms;
}

static void processRequest(const uint8_t *request, diagnosticLeds *leds)
{
    uint16_t receivedCrc = (uint16_t)request[6] | ((uint16_t)request[7] << 8U);
    if (modbusCrc(request, BPGA_MODBUS_REQUEST_SIZE - 2U) != receivedCrc) {
        TickType_t now = xTaskGetTickCount();
        pulseLed(&leds->shortFrame, now);
        pulseLed(&leds->longFrame, now);
        return;
    }

    if (request[0] != BPGA_MODBUS_SLAVE_ADDRESS) {
        return;
    }

    uint8_t response[BPGA_MODBUS_RESPONSE_SIZE];
    response[0] = BPGA_MODBUS_SLAVE_ADDRESS;
    if (request[1] != BPGA_MODBUS_FC_READ_INPUT_REGISTERS) {
        response[1] = request[1] | 0x80U;
        response[2] = BPGA_MODBUS_EXCEPTION_ILLEGAL_FUNCTION;
        sendResponse(response, 3U);
        return;
    }

    uint16_t start = ((uint16_t)request[2] << 8U) | request[3];
    uint16_t count = ((uint16_t)request[4] << 8U) | request[5];

    if (count == 0U || count > BPGA_MODBUS_MAX_READ_REGISTERS ||
        start < BPGA_MODBUS_INPUT_REGISTER_FIRST ||
        start > BPGA_MODBUS_INPUT_REGISTER_LAST ||
        (uint32_t)start + count > (uint32_t)BPGA_MODBUS_INPUT_REGISTER_LAST + 1U) {
        response[1] = BPGA_MODBUS_FC_READ_INPUT_REGISTERS | 0x80U;
        response[2] = BPGA_MODBUS_EXCEPTION_ILLEGAL_ADDRESS;
        sendResponse(response, 3U);
        return;
    }

    uint16_t registers[BPGA_MODBUS_INPUT_REGISTER_COUNT];
    buildInputRegisterImage(registers, BPGA_MODBUS_INPUT_REGISTER_COUNT);

    response[1] = BPGA_MODBUS_FC_READ_INPUT_REGISTERS;
    response[2] = (uint8_t)(count * 2U);
    for (uint16_t i = 0U; i < count; ++i) {
        uint16_t value = registers[start + i];
        response[3U + 2U * i] = (uint8_t)(value >> 8U);
        response[4U + 2U * i] = (uint8_t)value;
    }
    sendResponse(response, 3U + 2U * count);
}

BaseType_t bpgaModbusSlaveInit(void)
{
    gpioWrite(LED1, OFF);
    gpioWrite(LED2, OFF);
    gpioWrite(LED3, OFF);
    uartInit(UART_USB, BPGA_MODBUS_USB_BAUD_RATE);
    uartInit2(UART_485, BPGA_MODBUS_SLAVE_BAUD_RATE, UART_DATA_BITS_8,
              UART_PARITY_EVEN, UART_STOP_BITS_1);

    diagnosticOutput output = {0};
    appendText(&output, "UART RS-485 Config: LCR=0x");
    appendHexByte(&output, (uint8_t)(LPC_USART0->LCR & 0xFF));
    appendText(&output, " FDR=0x");
    appendHexByte(&output, (uint8_t)(LPC_USART0->FDR & 0xFF));
    appendText(&output, " RS485CTRL=0x");
    appendHexByte(&output, (uint8_t)(LPC_USART0->RS485CTRL & 0xFF));
    appendText(&output, " RS485DLY=0x");
    appendHexByte(&output, (uint8_t)(LPC_USART0->RS485DLY & 0xFF));
    appendText(&output, "\r\n");

    if (output.length > 0U) {
        uartWriteByteArray(UART_USB, output.line, (uint32_t)output.length);
    }

    return pdPASS;
}

void bpgaModbusSlaveTask(void *parameters)
{
    (void)parameters;
    uint8_t request[BPGA_MODBUS_RECEIVE_BUFFER_SIZE];
    size_t length = 0U;
    bool overflow = false;
    TickType_t lastByte = 0U;
    diagnosticOutput output = {0};
    diagnosticLeds leds = {
        .receive = {LED1, 0U, false},
        .shortFrame = {LED2, 0U, false},
        .longFrame = {LED3, 0U, false},
    };
    uartDiagnostics uartDiag = {0};
    /* One extra tick accounts for the unknown phase within the last tick. */
    const TickType_t silenceTicks =
        (TickType_t)(((2U * (uint32_t)configTICK_RATE_HZ + 999U) / 1000U) + 1U);
    const TickType_t idleTicks = pdMS_TO_TICKS(1U) > 0U ? pdMS_TO_TICKS(1U) : 1U;
    const TickType_t receivePulseTicks =
        pdMS_TO_TICKS(BPGA_MODBUS_RX_LED_PULSE_MS) > 0U ?
            pdMS_TO_TICKS(BPGA_MODBUS_RX_LED_PULSE_MS) : 1U;
    const TickType_t errorPulseTicks =
        pdMS_TO_TICKS(BPGA_MODBUS_ERROR_LED_PULSE_MS) > 0U ?
            pdMS_TO_TICKS(BPGA_MODBUS_ERROR_LED_PULSE_MS) : 1U;
    const TickType_t diagnosticIntervalTicks =
        (TickType_t)(((BPGA_MODBUS_USB_INTERVAL_MS * (uint32_t)configTICK_RATE_HZ +
                       999U) / 1000U) + 1U);
    const TickType_t uartDiagIntervalTicks =
        pdMS_TO_TICKS(BPGA_MODBUS_UART_DIAG_INTERVAL_MS) > 0U ?
            pdMS_TO_TICKS(BPGA_MODBUS_UART_DIAG_INTERVAL_MS) : 1U;

    for (;;) {
        TickType_t now = xTaskGetTickCount();
        updateLed(&leds.receive, now, receivePulseTicks);
        updateLed(&leds.shortFrame, now, errorPulseTicks);
        updateLed(&leds.longFrame, now, errorPulseTicks);

        uint8_t lsr = (uint8_t)(LPC_USART0->LSR & 0xFF);
        uint8_t byte;

        if (lsr & UART_LSR_RDR) {
            if (uartReadByte(UART_485, &byte)) {
                if (lsr & (UART_LSR_OE | UART_LSR_PE | UART_LSR_FE | UART_LSR_BI | UART_LSR_RXFE)) {
                    if (lsr & UART_LSR_OE) uartDiag.overrun++;
                    if (lsr & UART_LSR_PE) uartDiag.parity++;
                    if (lsr & UART_LSR_FE) uartDiag.framing++;
                    if (lsr & UART_LSR_BI) uartDiag.breakInt++;
                    if (lsr & UART_LSR_RXFE) uartDiag.rxError++;

                    if ((now - uartDiag.lastLog) >= uartDiagIntervalTicks) {
                        diagnosticOutput diagOut = {0};
                        appendText(&diagOut, "UART LSR error=0x");
                        appendHexByte(&diagOut, lsr);
                        appendText(&diagOut, " (OVR:");
                        appendDecimal(&diagOut, uartDiag.overrun);
                        appendText(&diagOut, " PAR:");
                        appendDecimal(&diagOut, uartDiag.parity);
                        appendText(&diagOut, " FRM:");
                        appendDecimal(&diagOut, uartDiag.framing);
                        appendText(&diagOut, " BRK:");
                        appendDecimal(&diagOut, uartDiag.breakInt);
                        appendText(&diagOut, " RXE:");
                        appendDecimal(&diagOut, uartDiag.rxError);
                        appendText(&diagOut, ")\r\n");

                        uartWriteByteArray(UART_USB, diagOut.line, (uint32_t)diagOut.length);
                        uartDiag.lastLog = now;
                    }
                } else {
                    lastByte = xTaskGetTickCount();
                    pulseLed(&leds.receive, lastByte);
                    if (!overflow) {
                        if (length < sizeof(request)) {
                            request[length++] = byte;
                        } else {
                            length = 0U;
                            overflow = true;
                            pulseLed(&leds.longFrame, lastByte);
                        }
                    }
                }
            }
        } else {
            if ((length != 0U || overflow) &&
                (TickType_t)(xTaskGetTickCount() - lastByte) >= silenceTicks) {
                if (!overflow) {
                    if (length == BPGA_MODBUS_REQUEST_SIZE) {
                        processRequest(request, &leds);
                    } else if (length < BPGA_MODBUS_REQUEST_SIZE) {
                        pulseLed(&leds.shortFrame, xTaskGetTickCount());
                    } else {
                        pulseLed(&leds.longFrame, xTaskGetTickCount());
                    }
                }
                queueDiagnostic(&output, request, length, overflow,
                                xTaskGetTickCount(), diagnosticIntervalTicks);
                length = 0U;
                overflow = false;
            }
            transmitDiagnostic(&output);
            vTaskDelay(idleTicks);
        }
    }
}
