#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

/* Registers the read-only BPGA endpoints backed by the Modbus master cache. */
esp_err_t bpga_rest_api_register(httpd_handle_t server, void *user_ctx);
