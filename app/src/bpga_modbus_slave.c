#include "bpga_modbus_slave.h"
#include "bpga_modbus_map.h"
#include "sapi.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BPGA_MODBUS_REQUEST_SIZE 8U
#define BPGA_MODBUS_RESPONSE_SIZE (5U + 2U * BPGA_MODBUS_IDENTIFICATION_COUNT)
#define BPGA_MODBUS_MAX_READ_REGISTERS 125U
#define BPGA_MODBUS_EXCEPTION_ILLEGAL_FUNCTION 0x01U
#define BPGA_MODBUS_EXCEPTION_ILLEGAL_ADDRESS 0x02U

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

static void sendResponse(uint8_t *response, size_t length)
{
    uint16_t crc = modbusCrc(response, length);
    response[length] = (uint8_t)crc;
    response[length + 1U] = (uint8_t)(crc >> 8U);
    uartWriteByteArray(UART_485, response, (uint32_t)(length + 2U));
}

static void processRequest(const uint8_t *request)
{
    if (request[0] != BPGA_MODBUS_SLAVE_ADDRESS) {
        return;
    }

    uint16_t receivedCrc = (uint16_t)request[6] | ((uint16_t)request[7] << 8U);
    if (modbusCrc(request, BPGA_MODBUS_REQUEST_SIZE - 2U) != receivedCrc) {
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
        start < BPGA_MODBUS_IDENTIFICATION_START ||
        (uint32_t)start + count >
            (uint32_t)BPGA_MODBUS_IDENTIFICATION_START + BPGA_MODBUS_IDENTIFICATION_COUNT) {
        response[1] = BPGA_MODBUS_FC_READ_INPUT_REGISTERS | 0x80U;
        response[2] = BPGA_MODBUS_EXCEPTION_ILLEGAL_ADDRESS;
        sendResponse(response, 3U);
        return;
    }

    response[1] = BPGA_MODBUS_FC_READ_INPUT_REGISTERS;
    response[2] = (uint8_t)(count * 2U);
    for (uint16_t i = 0U; i < count; ++i) {
        uint16_t value = identification[start - BPGA_MODBUS_IDENTIFICATION_START + i];
        response[3U + 2U * i] = (uint8_t)(value >> 8U);
        response[4U + 2U * i] = (uint8_t)value;
    }
    sendResponse(response, 3U + 2U * count);
}

BaseType_t bpgaModbusSlaveInit(void)
{
    uartInit2(UART_485, BPGA_MODBUS_SLAVE_BAUD_RATE, UART_DATA_BITS_8,
              UART_PARITY_EVEN, UART_STOP_BITS_1);
    return pdPASS;
}

void bpgaModbusSlaveTask(void *parameters)
{
    (void)parameters;
    uint8_t request[BPGA_MODBUS_REQUEST_SIZE];
    size_t length = 0U;
    bool overflow = false;
    TickType_t lastByte = 0U;
    /* One extra tick accounts for the unknown phase within the last tick. */
    const TickType_t silenceTicks =
        (TickType_t)(((2U * (uint32_t)configTICK_RATE_HZ + 999U) / 1000U) + 1U);
    const TickType_t idleTicks = pdMS_TO_TICKS(1U) > 0U ? pdMS_TO_TICKS(1U) : 1U;

    for (;;) {
        uint8_t byte;
        if (uartReadByte(UART_485, &byte)) {
            lastByte = xTaskGetTickCount();
            if (!overflow) {
                if (length < sizeof(request)) {
                    request[length++] = byte;
                } else {
                    length = 0U;
                    overflow = true;
                }
            }
        } else {
            if ((length != 0U || overflow) &&
                (TickType_t)(xTaskGetTickCount() - lastByte) >= silenceTicks) {
                if (!overflow && length == sizeof(request)) {
                    processRequest(request);
                }
                length = 0U;
                overflow = false;
            }
            vTaskDelay(idleTicks);
        }
    }
}
