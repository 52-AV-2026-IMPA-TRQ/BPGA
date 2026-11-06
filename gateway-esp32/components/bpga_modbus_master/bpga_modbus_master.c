#include "bpga_modbus_master.h"
#include "sdkconfig.h"

#if defined(CONFIG_BPGA_MODBUS_ENABLED) && CONFIG_BPGA_MODBUS_ENABLED

#include "bpga_modbus_map.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_modbus_master.h"

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
    if (status.initialized) {
        return status.state == BPGA_MODBUS_MASTER_READY;
    }
    if (modbus_handle != NULL) {
        ESP_LOGE(TAG, "Previous Modbus controller has not been deleted");
        return false;
    }

    status.state = BPGA_MODBUS_MASTER_STARTING;
    status.last_error = 0;
    if (!validate_configuration()) {
        status.last_error = ESP_ERR_INVALID_ARG;
        status.state = BPGA_MODBUS_MASTER_ERROR;
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
        return false;
    }

    status.initialized = true;
    status.state = BPGA_MODBUS_MASTER_READY;
    ESP_LOGI(TAG, "Modbus master initialized");
    return true;
}

void bpga_modbus_master_deinit(void)
{
    if (modbus_handle != NULL) {
        esp_err_t err = mbc_master_delete(modbus_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Modbus controller deletion failed: %s", esp_err_to_name(err));
            status.last_error = err;
            status.state = BPGA_MODBUS_MASTER_ERROR;
            return;
        }
        modbus_handle = NULL;
    }
    status.initialized = false;
    status.state = BPGA_MODBUS_MASTER_STOPPED;
}

bool bpga_modbus_master_is_ready(void)
{
    return status.initialized && status.state == BPGA_MODBUS_MASTER_READY;
}

bpga_modbus_master_status_t bpga_modbus_master_get_status(void)
{
    return status;
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

#endif
