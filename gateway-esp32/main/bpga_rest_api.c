#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "esp_err.h"
#include "esp_log.h"
#include "bpga_modbus_master.h"
#include "bpga_modbus_map.h"
#include "bpga_rest_api.h"

/* Worst case is about 1.2 KB; truncation is detected anyway. */
#define BPGA_STATUS_JSON_SIZE 1792

static const char *TAG = "bpga-api";

/*
 * ESP-IDF runs all URI handlers in the single HTTP server task, so one static
 * buffer is safe and keeps the task stack small.
 */
static char s_json[BPGA_STATUS_JSON_SIZE];

static bool append(size_t *length, const char *format, ...)
{
    if (*length >= sizeof(s_json)) {
        return false;
    }

    va_list args;
    va_start(args, format);
    const int written = vsnprintf(s_json + *length, sizeof(s_json) - *length, format, args);
    va_end(args);

    if ((written < 0) || ((size_t)written >= (sizeof(s_json) - *length))) {
        *length = sizeof(s_json);
        return false;
    }
    *length += (size_t)written;
    return true;
}

static const char *json_bool(bool value)
{
    return value ? "true" : "false";
}

static bool build_status_json(void)
{
    /* Both getters copy under the master mutex and release it before returning. */
    const bpga_modbus_master_status_t master = bpga_modbus_master_get_status();
    bpga_modbus_master_cache_t cache = {0};
    const bool cache_ok = bpga_modbus_master_get_cache(&cache);
    if (!cache_ok) {
        cache = (bpga_modbus_master_cache_t){0};
    }

    const bool online = master.initialized && (master.state == BPGA_MODBUS_MASTER_READY);
    const bool data_valid = cache_ok && cache.identification_valid && cache.snapshot_valid &&
                            cache.totals_valid && cache.diagnostics_valid;
    int last_error = cache.last_error != 0 ? cache.last_error : master.last_error;
    if (!cache_ok && last_error == 0) {
        last_error = ESP_ERR_TIMEOUT;
    }

    /* Blocks never validated stay zeroed. */
    bpga_modbus_identification_t id = {0};
    bpga_modbus_snapshot_t snap = {0};
    bpga_modbus_totals_t tot = {0};
    bpga_modbus_diagnostics_t diag = {0};
    if (cache.identification_valid) {
        id = cache.identification;
    }
    if (cache.snapshot_valid) {
        snap = cache.snapshot;
    }
    if (cache.totals_valid) {
        tot = cache.totals;
    }
    if (cache.diagnostics_valid) {
        diag = cache.diagnostics;
    }

    const bool simulation = (snap.status_flags & BPGA_MODBUS_STATUS_SIMULATION_ACTIVE) != 0U;
    const bool meas_valid = (snap.status_flags & BPGA_MODBUS_STATUS_MEASUREMENTS_VALID) != 0U;

    size_t n = 0U;
    bool ok = append(&n,
        "{\"online\":%s,\"dataValid\":%s,\"lastError\":%d,"
        "\"validity\":{\"identification\":%s,\"snapshot\":%s,\"totals\":%s,\"diagnostics\":%s},",
        json_bool(online), json_bool(data_valid), last_error,
        json_bool(cache.identification_valid), json_bool(cache.snapshot_valid),
        json_bool(cache.totals_valid), json_bool(cache.diagnostics_valid));

    ok = ok && append(&n,
        "\"identification\":{\"deviceMagic\":%u,\"protocolVersion\":%u,"
        "\"registerMapVersion\":%u,\"deviceType\":%u},",
        (unsigned)id.device_magic, (unsigned)id.protocol_version,
        (unsigned)id.register_map_version, (unsigned)id.device_type);

    ok = ok && append(&n,
        "\"state\":{\"systemState\":%u,\"systemSubstate\":%u,\"statusFlags\":%u,"
        "\"controlFlags\":%u,\"simulationActive\":%s,\"measurementsValid\":%s},",
        (unsigned)snap.system_state, (unsigned)snap.system_substate,
        (unsigned)snap.status_flags, (unsigned)snap.control_flags,
        json_bool(simulation), json_bool(meas_valid));

    ok = ok && append(&n,
        "\"profile\":{\"slot\":%u,\"revision\":%u},",
        (unsigned)snap.active_profile_slot, (unsigned)snap.active_profile_revision);

    ok = ok && append(&n,
        "\"measurement\":{\"sequence\":%u,\"uptimeMs\":%" PRIu32 ",\"sampleAgeMs\":%u,"
        "\"rpm\":%" PRIu32 ",\"voltageMv\":%" PRIu32 ",\"currentMa\":%" PRIu32
        ",\"powerW\":%" PRIu32 "},",
        (unsigned)snap.sequence, snap.uptime_ms, (unsigned)snap.sample_age_ms,
        snap.rpm, snap.voltage_mv, snap.current_ma, snap.power_w);

    ok = ok && append(&n,
        "\"totals\":{\"testElapsedMs\":%" PRIu32 ",\"rpmMax\":%" PRIu32
        ",\"voltageMaxMv\":%" PRIu32 ",\"currentMaxMa\":%" PRIu32
        ",\"energyWh\":%" PRIu32 ",\"sampleCounter\":%" PRIu32 "},",
        tot.test_elapsed_ms, tot.rpm_max, tot.voltage_max_mv, tot.current_max_ma,
        tot.energy_wh, tot.sample_counter);

    ok = ok && append(&n,
        "\"diagnostics\":{\"activeWarnings\":%u,\"activeAlarms\":%u,\"latchedAlarms\":%u,"
        "\"primaryFaultCode\":%u,\"lastFaultCode\":%u,\"modbusValidRequests\":%u,"
        "\"modbusCrcErrors\":%u,\"modbusExceptions\":%u}}",
        (unsigned)diag.active_warnings, (unsigned)diag.active_alarms,
        (unsigned)diag.latched_alarms, (unsigned)diag.primary_fault_code,
        (unsigned)diag.last_fault_code, (unsigned)diag.modbus_valid_requests,
        (unsigned)diag.modbus_crc_errors, (unsigned)diag.modbus_exceptions);

    return ok;
}

static esp_err_t bpga_status_get_handler(httpd_req_t *req)
{
    if (!build_status_json()) {
        ESP_LOGE(TAG, "Status JSON truncated");
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "Status response too large");
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_sendstr(req, s_json);
}

esp_err_t bpga_rest_api_register(httpd_handle_t server, void *user_ctx)
{
    const httpd_uri_t status_get_uri = {
        .uri = "/api/v1/bpga/status",
        .method = HTTP_GET,
        .handler = bpga_status_get_handler,
        .user_ctx = user_ctx,
    };
    return httpd_register_uri_handler(server, &status_get_uri);
}
