#include "bpga_gps.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "sapi.h"

#define BPGA_GPS_BAUD_RATE             9600U
#define BPGA_GPS_LINE_SIZE             96U
#define BPGA_GPS_BYTES_PER_CYCLE       32U
#define BPGA_GPS_POLL_PERIOD_MS        10U
#define BPGA_GPS_INITIAL_WINDOW_MS     120000U
#define BPGA_GPS_RETRY_DELAY_MS        300000U
#define BPGA_GPS_RETRY_WINDOW_MS       30000U

static uint8_t hexValue(char character)
{
    if ((character >= '0') && (character <= '9')) {
        return (uint8_t)(character - '0');
    }
    if ((character >= 'A') && (character <= 'F')) {
        return (uint8_t)(character - 'A' + 10);
    }
    if ((character >= 'a') && (character <= 'f')) {
        return (uint8_t)(character - 'a' + 10);
    }
    return 0xFFU;
}

static bool_t validateChecksum(const char *line)
{
    uint8_t checksum = 0U;
    uint8_t high;
    uint8_t low;
    const char *cursor;
    const char *asterisk;

    if ((line == NULL) || (line[0] != '$')) {
        return FALSE;
    }

    asterisk = strchr(line, '*');
    if ((asterisk == NULL) ||
        (asterisk[1] == '\0') ||
        (asterisk[2] == '\0')) {
        return FALSE;
    }

    for (cursor = line + 1; cursor < asterisk; ++cursor) {
        checksum ^= (uint8_t)*cursor;
    }

    high = hexValue(asterisk[1]);
    low = hexValue(asterisk[2]);

    if ((high > 0x0FU) || (low > 0x0FU)) {
        return FALSE;
    }

    return checksum == (uint8_t)((high << 4U) | low);
}

static bool_t parseTwoDigits(const char *text, uint8_t *value)
{
    if ((text == NULL) ||
        (value == NULL) ||
        (text[0] < '0') || (text[0] > '9') ||
        (text[1] < '0') || (text[1] > '9')) {
        return FALSE;
    }

    *value = (uint8_t)(
        ((uint8_t)(text[0] - '0') * 10U) +
        (uint8_t)(text[1] - '0')
    );

    return TRUE;
}

static bool_t isLeapYear(uint16_t year)
{
    return (bool_t)(
        ((year % 4U) == 0U) &&
        (((year % 100U) != 0U) || ((year % 400U) == 0U))
    );
}

static uint8_t daysInMonth(uint16_t year, uint8_t month)
{
    static const uint8_t days[] = {
        31U, 28U, 31U, 30U, 31U, 30U,
        31U, 31U, 30U, 31U, 30U, 31U
    };

    if ((month < 1U) || (month > 12U)) {
        return 0U;
    }

    if ((month == 2U) && isLeapYear(year)) {
        return 29U;
    }

    return days[month - 1U];
}

static uint8_t calculateWeekday(
    uint16_t year,
    uint8_t month,
    uint8_t day
)
{
    static const uint8_t offsets[] = {
        0U, 3U, 2U, 5U, 0U, 3U,
        5U, 1U, 4U, 6U, 2U, 4U
    };
    uint16_t adjustedYear = year;

    if (month < 3U) {
        adjustedYear--;
    }

    return (uint8_t)(
        (
            adjustedYear +
            adjustedYear / 4U -
            adjustedYear / 100U +
            adjustedYear / 400U +
            offsets[month - 1U] +
            day
        ) % 7U
    ) + 1U;
}

static bool_t parseRmc(char *line, rtc_t *rtc)
{
    char *fields[10];
    size_t fieldCount = 0U;
    char *cursor;
    uint8_t day;
    uint8_t month;
    uint8_t yearTwoDigits;

    if ((line == NULL) || (rtc == NULL)) {
        return FALSE;
    }

    if ((strncmp(line, "$GPRMC,", 7U) != 0) &&
        (strncmp(line, "$GNRMC,", 7U) != 0)) {
        return FALSE;
    }

    if (!validateChecksum(line)) {
        return FALSE;
    }

    cursor = line;

    while ((fieldCount < 10U) && (cursor != NULL)) {
        fields[fieldCount++] = cursor;
        cursor = strchr(cursor, ',');

        if (cursor != NULL) {
            *cursor = '\0';
            cursor++;
        }
    }

    if ((fieldCount < 10U) ||
        (strlen(fields[1]) < 6U) ||
        (strcmp(fields[2], "A") != 0) ||
        (strlen(fields[9]) < 6U)) {
        return FALSE;
    }

    if (!parseTwoDigits(&fields[1][0], &rtc->hour) ||
        !parseTwoDigits(&fields[1][2], &rtc->min) ||
        !parseTwoDigits(&fields[1][4], &rtc->sec) ||
        !parseTwoDigits(&fields[9][0], &day) ||
        !parseTwoDigits(&fields[9][2], &month) ||
        !parseTwoDigits(&fields[9][4], &yearTwoDigits)) {
        return FALSE;
    }

    rtc->year = (uint16_t)(2000U + yearTwoDigits);
    rtc->month = month;
    rtc->mday = day;

    if ((rtc->hour > 23U) ||
        (rtc->min > 59U) ||
        (rtc->sec > 59U) ||
        (rtc->month < 1U) ||
        (rtc->month > 12U) ||
        (rtc->mday < 1U) ||
        (rtc->mday > daysInMonth(rtc->year, rtc->month))) {
        return FALSE;
    }

    rtc->wday = calculateWeekday(
        rtc->year,
        rtc->month,
        rtc->mday
    );

    return TRUE;
}

static bool_t synchronizeRtcInWindow(TickType_t windowTicks)
{
    TickType_t startTick = xTaskGetTickCount();
    char line[BPGA_GPS_LINE_SIZE];
    size_t length = 0U;
    bool_t reportedBytes = FALSE;
    bool_t reportedRmc = FALSE;

    while ((TickType_t)(xTaskGetTickCount() - startTick) < windowTicks) {
        uint32_t processed = 0U;
        uint8_t byte;

        while ((processed < BPGA_GPS_BYTES_PER_CYCLE) &&
               uartReadByte(UART_232, &byte)) {
            processed++;

            if (!reportedBytes) {
                uartWriteString(
                    UART_USB,
                    "GPS: UART recibe datos\r\n"
                );
                reportedBytes = TRUE;
            }

            if (byte == '$') {
                length = 0U;
                line[length++] = (char)byte;
                continue;
            }

            if (length == 0U) {
                continue;
            }

            if ((byte == '\r') || (byte == '\n')) {
                rtc_t received;
                rtc_t verified;

                line[length] = '\0';

                if (!reportedRmc &&
                    ((strncmp(line, "$GPRMC,", 7U) == 0) ||
                     (strncmp(line, "$GNRMC,", 7U) == 0)) &&
                    validateChecksum(line)) {
                    uartWriteString(
                        UART_USB,
                        "GPS: RMC recibida\r\n"
                    );
                    reportedRmc = TRUE;
                }

                if (parseRmc(line, &received) &&
                    rtcWrite(&received) &&
                    rtcRead(&verified) &&
                    (received.year == verified.year) &&
                    (received.month == verified.month) &&
                    (received.mday == verified.mday) &&
                    (received.wday == verified.wday) &&
                    (received.hour == verified.hour) &&
                    (received.min == verified.min) &&
                    (received.sec == verified.sec)) {
                    return TRUE;
                }

                length = 0U;
                continue;
            }

            if (length < (BPGA_GPS_LINE_SIZE - 1U)) {
                line[length++] = (char)byte;
            } else {
                length = 0U;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(BPGA_GPS_POLL_PERIOD_MS));
    }

    return FALSE;
}

void bpgaGpsTask(void *parameters)
{
    bool_t firstAttempt = TRUE;
    (void)parameters;

    uartWriteString(
        UART_USB,
        "GPS: tarea iniciada\r\n"
    );

    uartInit2(
        UART_232,
        BPGA_GPS_BAUD_RATE,
        UART_DATA_BITS_8,
        UART_PARITY_NONE,
        UART_STOP_BITS_1
    );

    Chip_RTC_Init(LPC_RTC);
    Chip_RTC_Enable(LPC_RTC, ENABLE);

    for (;;) {
        const TickType_t windowTicks = pdMS_TO_TICKS(
            firstAttempt
                ? BPGA_GPS_INITIAL_WINDOW_MS
                : BPGA_GPS_RETRY_WINDOW_MS
        );

        if (synchronizeRtcInWindow(windowTicks)) {
            uartWriteString(
                UART_USB,
                "GPS: RTC sincronizado UTC\r\n"
            );
            vTaskDelete(NULL);
        }

        firstAttempt = FALSE;
        vTaskDelay(pdMS_TO_TICKS(BPGA_GPS_RETRY_DELAY_MS));
    }
}
