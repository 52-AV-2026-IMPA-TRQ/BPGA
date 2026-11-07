#include "bpga_modbus_master.h"
#include "sdkconfig.h"

#if defined(CONFIG_BPGA_MODBUS_ENABLED) && CONFIG_BPGA_MODBUS_ENABLED

#include "bpga_modbus_map.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_modbus_master.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define BPGA_MODBUS_MUTEX_TIMEOUT_MS 1000

static const char *TAG = "BPGA_MODBUS_MASTER";

static bpga_modbus_master_status_t status = {
    .state = BPGA_MODBUS_MASTER_STOPPED,
    .enabled = true,
    .slave_address = CONFIG_BPGA_MODBUS_SLAVE_ADDRESS,
    .baud_rate = CONFIG_BPGA_MODBUS_BAUD_RATE,
    .uart_port = CONFIG_BPGA_MODBUS_UART_PORT,
    .tx_gpio = CONFIG_BPGA_MODBUS_UART_TX_GPIO,
    .rx_gpio = CONFIG_BPGA_MODBUS_UART_RX_GPIO,
    .rts_gpio = CONFIG_BPGA_MODBUS_UART_RTS_GPIO,
};

static void *modbus_handle;
static bpga_modbus_master_cache_t cache;
static SemaphoreHandle_t master_mutex;

static bool master_lock(void)
{
    if (master_mutex == NULL) {
        return false;
    }
    if (xSemaphoreTake(master_mutex, pdMS_TO_TICKS(BPGA_MODBUS_MUTEX_TIMEOUT_MS)) != pdTRUE) {
        ESP_LOGW(TAG, "Timed out waiting for Modbus mutex");
        return false;
    }
    return true;
}

static void master_unlock(void)
{
    xSemaphoreGive(master_mutex);
}

static bool master_is_ready_unlocked(void)
{
    return status.initialized && status.state == BPGA_MODBUS_MASTER_READY;
}

static const mb_parameter_descriptor_t device_parameters[] = {
    {
        .cid = 0,
        .param_key = "device_magic",
        .param_units = "",
        .mb_slave_addr = CONFIG_BPGA_MODBUS_SLAVE_ADDRESS,
        .mb_param_type = MB_PARAM_INPUT,
        .mb_reg_start = BPGA_MODBUS_IR_DEVICE_MAGIC,
        .mb_size = 1,
        .param_type = PARAM_TYPE_U16,
        .param_size = sizeof(uint16_t),
        .access = PAR_PERMS_READ,
    },
};

static uart_parity_t get_uart_parity(void)
{
#if CONFIG_BPGA_MODBUS_PARITY_NONE
    return UART_PARITY_DISABLE;
#elif CONFIG_BPGA_MODBUS_PARITY_ODD
    return UART_PARITY_ODD;
#else
    return UART_PARITY_EVEN;
#endif
}

static bool validate_configuration(void)
{
    if (CONFIG_BPGA_MODBUS_UART_PORT < 0 || CONFIG_BPGA_MODBUS_UART_PORT >= UART_NUM_MAX) {
        ESP_LOGE(TAG, "Invalid UART port: %d", CONFIG_BPGA_MODBUS_UART_PORT);
        return false;
    }
    if (CONFIG_BPGA_MODBUS_UART_TX_GPIO < 0 ||
        CONFIG_BPGA_MODBUS_UART_RX_GPIO < 0 ||
        CONFIG_BPGA_MODBUS_UART_RTS_GPIO < 0) {
        ESP_LOGE(TAG, "TX, RX, and RTS GPIO pins must not be negative");
        return false;
    }
    if (!GPIO_IS_VALID_OUTPUT_GPIO(CONFIG_BPGA_MODBUS_UART_TX_GPIO) ||
        !GPIO_IS_VALID_GPIO(CONFIG_BPGA_MODBUS_UART_RX_GPIO) ||
        !GPIO_IS_VALID_OUTPUT_GPIO(CONFIG_BPGA_MODBUS_UART_RTS_GPIO)) {
        ESP_LOGE(TAG, "Invalid UART GPIO pins");
        return false;
    }
    if (CONFIG_BPGA_MODBUS_UART_TX_GPIO == CONFIG_BPGA_MODBUS_UART_RX_GPIO ||
        CONFIG_BPGA_MODBUS_UART_TX_GPIO == CONFIG_BPGA_MODBUS_UART_RTS_GPIO ||
        CONFIG_BPGA_MODBUS_UART_RX_GPIO == CONFIG_BPGA_MODBUS_UART_RTS_GPIO) {
        ESP_LOGE(TAG, "UART GPIO pins must be distinct");
        return false;
    }
    if (!bpga_modbus_is_valid_slave_address(CONFIG_BPGA_MODBUS_SLAVE_ADDRESS)) {
        ESP_LOGE(TAG, "Slave address must be 1-247, got %d", CONFIG_BPGA_MODBUS_SLAVE_ADDRESS);
        return false;
    }
    if (CONFIG_BPGA_MODBUS_BAUD_RATE <= 0) {
        ESP_LOGE(TAG, "Baud rate must be positive, got %d", CONFIG_BPGA_MODBUS_BAUD_RATE);
        return false;
    }
    return true;
}

bool bpga_modbus_master_init(void)
{
    if (master_mutex == NULL) {
        master_mutex = xSemaphoreCreateMutex();
        if (master_mutex == NULL) {
            status.state = BPGA_MODBUS_MASTER_ERROR;
            status.last_error = ESP_ERR_NO_MEM;
            ESP_LOGE(TAG, "Failed to create Modbus mutex");
            return false;
        }
    }
    if (!master_lock()) {
        return false;
    }
    if (status.initialized) {
        bool ready = master_is_ready_unlocked();
        master_unlock();
        return ready;
    }
    if (modbus_handle != NULL) {
        ESP_LOGE(TAG, "Previous Modbus controller has not been deleted");
        master_unlock();
        return false;
    }

    status.state = BPGA_MODBUS_MASTER_STARTING;
    status.last_error = 0;
    if (!validate_configuration()) {
        status.last_error = ESP_ERR_INVALID_ARG;
        status.state = BPGA_MODBUS_MASTER_ERROR;
        master_unlock();
        return false;
    }

    mb_communication_info_t comm_info = {
        .ser_opts.port = CONFIG_BPGA_MODBUS_UART_PORT,
        .ser_opts.mode = MB_RTU,
        .ser_opts.baudrate = CONFIG_BPGA_MODBUS_BAUD_RATE,
        .ser_opts.parity = get_uart_parity(),
        .ser_opts.uid = 0,
        .ser_opts.response_tout_ms = CONFIG_BPGA_MODBUS_RESPONSE_TIMEOUT_MS,
        .ser_opts.data_bits = UART_DATA_8_BITS,
        .ser_opts.stop_bits = UART_STOP_BITS_1,
    };

    esp_err_t err = mbc_master_create_serial(&comm_info, &modbus_handle);
    if (err == ESP_OK && modbus_handle == NULL) {
        err = ESP_ERR_INVALID_STATE;
    }
    if (err == ESP_OK) {
        err = uart_set_pin(CONFIG_BPGA_MODBUS_UART_PORT, CONFIG_BPGA_MODBUS_UART_TX_GPIO,
                           CONFIG_BPGA_MODBUS_UART_RX_GPIO, CONFIG_BPGA_MODBUS_UART_RTS_GPIO,
                           UART_PIN_NO_CHANGE);
    }
    if (err == ESP_OK && CONFIG_BPGA_MODBUS_UART_RTS_GPIO != -1) {
        err = uart_set_mode(CONFIG_BPGA_MODBUS_UART_PORT, UART_MODE_RS485_HALF_DUPLEX);
    }
    if (err == ESP_OK) {
        err = mbc_master_set_descriptor(modbus_handle, device_parameters,
                                        sizeof(device_parameters) / sizeof(device_parameters[0]));
    }
    if (err == ESP_OK) {
        err = mbc_master_start(modbus_handle);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Modbus master initialization failed: %s", esp_err_to_name(err));
        status.last_error = err;
        status.state = BPGA_MODBUS_MASTER_ERROR;
        if (modbus_handle != NULL) {
            esp_err_t delete_err = mbc_master_delete(modbus_handle);
            if (delete_err == ESP_OK) {
                modbus_handle = NULL;
            } else {
                ESP_LOGE(TAG, "Modbus controller deletion failed: %s", esp_err_to_name(delete_err));
            }
        }
        master_unlock();
        return false;
    }

    status.initialized = true;
    status.state = BPGA_MODBUS_MASTER_READY;
    ESP_LOGI(TAG, "Modbus master initialized");
    master_unlock();
    return true;
}

void bpga_modbus_master_deinit(void)
{
    if (master_mutex == NULL || !master_lock()) {
        return;
    }
    if (modbus_handle != NULL) {
        esp_err_t err = mbc_master_delete(modbus_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Modbus controller deletion failed: %s", esp_err_to_name(err));
            status.last_error = err;
            status.state = BPGA_MODBUS_MASTER_ERROR;
            master_unlock();
            return;
        }
        modbus_handle = NULL;
    }
    status.initialized = false;
    status.state = BPGA_MODBUS_MASTER_STOPPED;
    master_unlock();
}

bool bpga_modbus_master_is_ready(void)
{
    if (!master_lock()) {
        return false;
    }
    bool ready = master_is_ready_unlocked();
    master_unlock();
    return ready;
}

bpga_modbus_master_status_t bpga_modbus_master_get_status(void)
{
    if (master_mutex == NULL) {
        return status;
    }
    if (!master_lock()) {
        const bpga_modbus_master_status_t unavailable = {
            .state = BPGA_MODBUS_MASTER_ERROR,
            .enabled = true,
            .last_error = ESP_FAIL,
        };
        return unavailable;
    }
    bpga_modbus_master_status_t result = status;
    master_unlock();
    return result;
}

bool bpga_modbus_master_get_cache(bpga_modbus_master_cache_t *output)
{
    if (output == NULL) {
        return false;
    }
    if (master_mutex == NULL) {
        *output = cache;
        return true;
    }
    if (!master_lock()) {
        return false;
    }
    *output = cache;
    master_unlock();
    return true;
}

void bpga_modbus_master_clear_cache(void)
{
    if (master_mutex == NULL) {
        cache = (bpga_modbus_master_cache_t){0};
        return;
    }
    if (!master_lock()) {
        return;
    }
    cache = (bpga_modbus_master_cache_t){0};
    master_unlock();
}

static void record_read_success(void)
{
    cache.last_success_us = (uint64_t)esp_timer_get_time();
    cache.successful_reads++;
    cache.last_error = 0;
}

static void record_read_failure(esp_err_t err)
{
    cache.last_error_us = (uint64_t)esp_timer_get_time();
    cache.last_error = err;
    cache.failed_reads++;
}

static esp_err_t read_input_registers_block(
    uint16_t reg_start,
    uint16_t reg_count,
    uint16_t *buffer
)
{
    if (!master_is_ready_unlocked()) {
        ESP_LOGW(TAG, "Modbus master not ready");
        return ESP_FAIL;
    }

    mb_param_request_t request = {
        .slave_addr = CONFIG_BPGA_MODBUS_SLAVE_ADDRESS,
        .command = BPGA_MODBUS_FC_READ_INPUT_REGISTERS,
        .reg_start = reg_start,
        .reg_size = reg_count,
    };

    esp_err_t err = mbc_master_send_request(modbus_handle, &request, buffer);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Modbus read at 0x%04X failed: %s", reg_start, esp_err_to_name(err));
    }

    return err;
}

bool bpga_modbus_master_read_identification(
    bpga_modbus_identification_t *output
)
{
    if (output == NULL) {
        return false;
    }
    if (!master_lock()) {
        return false;
    }

    uint16_t registers[BPGA_MODBUS_IDENTIFICATION_COUNT];
    esp_err_t err = read_input_registers_block(
            BPGA_MODBUS_IDENTIFICATION_START,
            BPGA_MODBUS_IDENTIFICATION_COUNT,
            registers);
    if (err != ESP_OK) {
        record_read_failure(err);
        master_unlock();
        return false;
    }

    bpga_modbus_identification_t decoded;
    if (!bpga_modbus_decode_identification(
            registers,
            BPGA_MODBUS_IDENTIFICATION_COUNT,
            &decoded)) {
        ESP_LOGW(TAG, "Failed to decode identification block");
        record_read_failure(ESP_FAIL);
        master_unlock();
        return false;
    }

    *output = decoded;
    cache.identification = decoded;
    cache.identification_valid = true;
    record_read_success();
    master_unlock();
    return true;
}

bool bpga_modbus_master_read_snapshot(
    bpga_modbus_snapshot_t *output
)
{
    if (output == NULL) {
        return false;
    }
    if (!master_lock()) {
        return false;
    }

    uint16_t registers[BPGA_MODBUS_SNAPSHOT_COUNT];
    esp_err_t err = read_input_registers_block(
            BPGA_MODBUS_SNAPSHOT_START,
            BPGA_MODBUS_SNAPSHOT_COUNT,
            registers);
    if (err != ESP_OK) {
        record_read_failure(err);
        master_unlock();
        return false;
    }

    bpga_modbus_snapshot_t decoded;
    if (!bpga_modbus_decode_snapshot(
            registers,
            BPGA_MODBUS_SNAPSHOT_COUNT,
            &decoded)) {
        ESP_LOGW(TAG, "Failed to decode snapshot block");
        record_read_failure(ESP_FAIL);
        master_unlock();
        return false;
    }

    *output = decoded;
    cache.snapshot = decoded;
    cache.snapshot_valid = true;
    record_read_success();
    master_unlock();
    return true;
}

bool bpga_modbus_master_read_diagnostics(
    bpga_modbus_diagnostics_t *output
)
{
    if (output == NULL) {
        return false;
    }
    if (!master_lock()) {
        return false;
    }

    uint16_t registers[BPGA_MODBUS_DIAGNOSTICS_COUNT];
    esp_err_t err = read_input_registers_block(
            BPGA_MODBUS_DIAGNOSTICS_START,
            BPGA_MODBUS_DIAGNOSTICS_COUNT,
            registers);
    if (err != ESP_OK) {
        record_read_failure(err);
        master_unlock();
        return false;
    }

    bpga_modbus_diagnostics_t decoded;
    if (!bpga_modbus_decode_diagnostics(
            registers,
            BPGA_MODBUS_DIAGNOSTICS_COUNT,
            &decoded)) {
        ESP_LOGW(TAG, "Failed to decode diagnostics block");
        record_read_failure(ESP_FAIL);
        master_unlock();
        return false;
    }

    *output = decoded;
    cache.diagnostics = decoded;
    cache.diagnostics_valid = true;
    record_read_success();
    master_unlock();
    return true;
}

#else

bool bpga_modbus_master_init(void)
{
    return true;
}

void bpga_modbus_master_deinit(void)
{
}

bool bpga_modbus_master_is_ready(void)
{
    return false;
}

bpga_modbus_master_status_t bpga_modbus_master_get_status(void)
{
    const bpga_modbus_master_status_t status = {
        .state = BPGA_MODBUS_MASTER_DISABLED,
        .enabled = false,
        .initialized = false,
        .tx_gpio = -1,
        .rx_gpio = -1,
        .rts_gpio = -1,
    };
    return status;
}

bool bpga_modbus_master_get_cache(bpga_modbus_master_cache_t *output)
{
    if (output == NULL) {
        return false;
    }
    *output = (bpga_modbus_master_cache_t){0};
    return true;
}

void bpga_modbus_master_clear_cache(void)
{
}

bool bpga_modbus_master_read_identification(
    bpga_modbus_identification_t *output
)
{
    (void)output;
    return false;
}

bool bpga_modbus_master_read_snapshot(
    bpga_modbus_snapshot_t *output
)
{
    (void)output;
    return false;
}

bool bpga_modbus_master_read_diagnostics(
    bpga_modbus_diagnostics_t *output
)
{
    (void)output;
    return false;
}

#endif
