#include "bpga_modbus_map.h"

/* Converts an absolute PDU address into an index relative to a block buffer. */
#define BPGA_MODBUS_BLOCK_OFFSET(address, block_start) \
    ((size_t)((address) - (block_start)))

#define BPGA_MODBUS_SNAPSHOT_REG(registers, address) \
    ((registers)[BPGA_MODBUS_BLOCK_OFFSET((address), BPGA_MODBUS_SNAPSHOT_START)])

#define BPGA_MODBUS_TOTALS_REG(registers, address) \
    ((registers)[BPGA_MODBUS_BLOCK_OFFSET((address), BPGA_MODBUS_TOTALS_START)])

#define BPGA_MODBUS_DIAGNOSTICS_REG(registers, address) \
    ((registers)[BPGA_MODBUS_BLOCK_OFFSET((address), BPGA_MODBUS_DIAGNOSTICS_START)])

/* ============================================================================
 * CONVERSION FUNCTIONS
 * ============================================================================ */

uint32_t bpga_modbus_u32_from_words(
    uint16_t high_word,
    uint16_t low_word
)
{
    return ((uint32_t)high_word << 16U) | (uint32_t)low_word;
}

void bpga_modbus_u32_to_words(
    uint32_t value,
    uint16_t *high_word,
    uint16_t *low_word
)
{
    if (high_word != NULL) {
        *high_word = (uint16_t)(value >> 16U);
    }
    if (low_word != NULL) {
        *low_word = (uint16_t)(value & 0xFFFFU);
    }
}

uint8_t bpga_modbus_version_major(uint16_t encoded)
{
    return (uint8_t)(encoded >> 8U);
}

uint8_t bpga_modbus_version_minor(uint16_t encoded)
{
    return (uint8_t)(encoded & 0xFFU);
}

/* ============================================================================
 * VALIDATION FUNCTIONS
 * ============================================================================ */

bool bpga_modbus_snapshot_sequence_is_valid(
    uint16_t sequence_begin,
    uint16_t sequence_end
)
{
    if (sequence_begin != sequence_end) {
        return false;
    }
    if ((sequence_begin & 1U) != 0U) {
        return false;
    }
    return true;
}

bool bpga_modbus_is_valid_profile_slot(uint16_t slot)
{
    return slot <= 9U;
}

bool bpga_modbus_is_valid_slave_address(uint16_t address)
{
    return address >= 1U && address <= 247U;
}

bool bpga_modbus_is_valid_permille(uint16_t value)
{
    return value <= 1000U;
}

/* ============================================================================
 * DECODING FUNCTIONS
 * ============================================================================ */

bool bpga_modbus_decode_identification(
    const uint16_t *registers,
    size_t register_count,
    bpga_modbus_identification_t *output
)
{
    if (registers == NULL || output == NULL) {
        return false;
    }

    if (register_count < BPGA_MODBUS_IDENTIFICATION_COUNT) {
        return false;
    }

    bpga_modbus_identification_t temp;

    temp.device_magic = registers[BPGA_MODBUS_IR_DEVICE_MAGIC];
    temp.protocol_version = registers[BPGA_MODBUS_IR_PROTOCOL_VERSION];
    temp.register_map_version = registers[BPGA_MODBUS_IR_REGISTER_MAP_VERSION];
    temp.firmware_version_major = registers[BPGA_MODBUS_IR_FIRMWARE_VERSION_MAJOR];
    temp.firmware_version_minor = registers[BPGA_MODBUS_IR_FIRMWARE_VERSION_MINOR];
    temp.firmware_version_patch = registers[BPGA_MODBUS_IR_FIRMWARE_VERSION_PATCH];
    temp.device_type = registers[BPGA_MODBUS_IR_DEVICE_TYPE];
    temp.capabilities = registers[BPGA_MODBUS_IR_DEVICE_CAPABILITIES];

    temp.boot_count = bpga_modbus_u32_from_words(
        registers[BPGA_MODBUS_IR_BOOT_COUNT_HI],
        registers[BPGA_MODBUS_IR_BOOT_COUNT_LO]
    );

    *output = temp;
    return true;
}

bool bpga_modbus_decode_snapshot(
    const uint16_t *registers,
    size_t register_count,
    bpga_modbus_snapshot_t *output
)
{
    if (registers == NULL || output == NULL) {
        return false;
    }

    if (register_count < BPGA_MODBUS_SNAPSHOT_COUNT) {
        return false;
    }

    uint16_t seq_begin = BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_SNAPSHOT_SEQUENCE_BEGIN);
    uint16_t seq_end = BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_SNAPSHOT_SEQUENCE_END);

    if (!bpga_modbus_snapshot_sequence_is_valid(seq_begin, seq_end)) {
        return false;
    }

    bpga_modbus_snapshot_t temp;

    temp.sequence = seq_begin;
    temp.system_state = (bpga_modbus_system_state_t)BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_SYSTEM_STATE);
    temp.system_substate = BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_SYSTEM_SUBSTATE);
    temp.status_flags = BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_STATUS_FLAGS);
    temp.control_flags = BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_CONTROL_FLAGS);
    temp.active_profile_slot = BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_ACTIVE_PROFILE_SLOT);
    temp.active_profile_revision = BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_ACTIVE_PROFILE_REVISION);
    temp.last_transition_reason = BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_LAST_TRANSITION_REASON);

    temp.uptime_ms = bpga_modbus_u32_from_words(
        BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_UPTIME_HI),
        BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_UPTIME_LO)
    );

    temp.loop_time_us = BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_LOOP_TIME_US);
    temp.loop_time_max_us = BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_LOOP_TIME_MAX_US);

    temp.rpm = bpga_modbus_u32_from_words(
        BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_RPM_HI),
        BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_RPM_LO)
    );

    temp.voltage_mv = bpga_modbus_u32_from_words(
        BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_VOLTAGE_MV_HI),
        BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_VOLTAGE_MV_LO)
    );

    temp.current_ma = bpga_modbus_u32_from_words(
        BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_CURRENT_MA_HI),
        BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_CURRENT_MA_LO)
    );

    temp.power_w = bpga_modbus_u32_from_words(
        BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_POWER_W_HI),
        BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_POWER_W_LO)
    );

    temp.excitation_permille = BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_EXCITATION_PERMILLE);
    temp.load_permille = BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_LOAD_PERMILLE);
    temp.rpm_quality = BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_RPM_QUALITY);
    temp.voltage_quality = BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_VOLTAGE_QUALITY);
    temp.current_quality = BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_CURRENT_QUALITY);
    temp.sample_age_ms = BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_SAMPLE_AGE_MS);
    temp.measurement_flags = BPGA_MODBUS_SNAPSHOT_REG(registers, BPGA_MODBUS_IR_MEASUREMENT_FLAGS);

    *output = temp;
    return true;
}

bool bpga_modbus_decode_totals(
    const uint16_t *registers,
    size_t register_count,
    bpga_modbus_totals_t *output
)
{
    if (registers == NULL || output == NULL) {
        return false;
    }

    if (register_count < BPGA_MODBUS_TOTALS_COUNT) {
        return false;
    }

    bpga_modbus_totals_t temp;

    temp.test_elapsed_ms = bpga_modbus_u32_from_words(
        BPGA_MODBUS_TOTALS_REG(registers, BPGA_MODBUS_IR_TEST_ELAPSED_MS_HI),
        BPGA_MODBUS_TOTALS_REG(registers, BPGA_MODBUS_IR_TEST_ELAPSED_MS_LO)
    );

    temp.rpm_max = bpga_modbus_u32_from_words(
        BPGA_MODBUS_TOTALS_REG(registers, BPGA_MODBUS_IR_RPM_MAX_HI),
        BPGA_MODBUS_TOTALS_REG(registers, BPGA_MODBUS_IR_RPM_MAX_LO)
    );

    temp.voltage_max_mv = bpga_modbus_u32_from_words(
        BPGA_MODBUS_TOTALS_REG(registers, BPGA_MODBUS_IR_VOLTAGE_MAX_MV_HI),
        BPGA_MODBUS_TOTALS_REG(registers, BPGA_MODBUS_IR_VOLTAGE_MAX_MV_LO)
    );

    temp.current_max_ma = bpga_modbus_u32_from_words(
        BPGA_MODBUS_TOTALS_REG(registers, BPGA_MODBUS_IR_CURRENT_MAX_MA_HI),
        BPGA_MODBUS_TOTALS_REG(registers, BPGA_MODBUS_IR_CURRENT_MAX_MA_LO)
    );

    temp.energy_wh = bpga_modbus_u32_from_words(
        BPGA_MODBUS_TOTALS_REG(registers, BPGA_MODBUS_IR_ENERGY_WH_HI),
        BPGA_MODBUS_TOTALS_REG(registers, BPGA_MODBUS_IR_ENERGY_WH_LO)
    );

    temp.sample_counter = bpga_modbus_u32_from_words(
        BPGA_MODBUS_TOTALS_REG(registers, BPGA_MODBUS_IR_SAMPLE_COUNTER_HI),
        BPGA_MODBUS_TOTALS_REG(registers, BPGA_MODBUS_IR_SAMPLE_COUNTER_LO)
    );

    *output = temp;
    return true;
}

bool bpga_modbus_decode_diagnostics(
    const uint16_t *registers,
    size_t register_count,
    bpga_modbus_diagnostics_t *output
)
{
    if (registers == NULL || output == NULL) {
        return false;
    }

    if (register_count < BPGA_MODBUS_DIAGNOSTICS_COUNT) {
        return false;
    }

    bpga_modbus_diagnostics_t temp;

    temp.active_warnings = BPGA_MODBUS_DIAGNOSTICS_REG(registers, BPGA_MODBUS_IR_ACTIVE_WARNINGS);
    temp.active_alarms = BPGA_MODBUS_DIAGNOSTICS_REG(registers, BPGA_MODBUS_IR_ACTIVE_ALARMS);
    temp.latched_alarms = BPGA_MODBUS_DIAGNOSTICS_REG(registers, BPGA_MODBUS_IR_LATCHED_ALARMS);
    temp.primary_fault_code = BPGA_MODBUS_DIAGNOSTICS_REG(registers, BPGA_MODBUS_IR_PRIMARY_FAULT_CODE);
    temp.last_fault_code = BPGA_MODBUS_DIAGNOSTICS_REG(registers, BPGA_MODBUS_IR_LAST_FAULT_CODE);

    temp.last_fault_uptime_ms = bpga_modbus_u32_from_words(
        BPGA_MODBUS_DIAGNOSTICS_REG(registers, BPGA_MODBUS_IR_LAST_FAULT_UPTIME_MS_HI),
        BPGA_MODBUS_DIAGNOSTICS_REG(registers, BPGA_MODBUS_IR_LAST_FAULT_UPTIME_MS_LO)
    );

    temp.modbus_valid_requests = BPGA_MODBUS_DIAGNOSTICS_REG(registers, BPGA_MODBUS_IR_MODBUS_VALID_REQUESTS);
    temp.modbus_crc_errors = BPGA_MODBUS_DIAGNOSTICS_REG(registers, BPGA_MODBUS_IR_MODBUS_CRC_ERRORS);
    temp.modbus_exceptions = BPGA_MODBUS_DIAGNOSTICS_REG(registers, BPGA_MODBUS_IR_MODBUS_EXCEPTIONS);
    temp.watchdog_reset_count = BPGA_MODBUS_DIAGNOSTICS_REG(registers, BPGA_MODBUS_IR_WATCHDOG_RESET_COUNT);

    *output = temp;
    return true;
}

bool bpga_modbus_decode_command_response(
    const uint16_t *registers,
    size_t register_count,
    bpga_modbus_command_response_t *output
)
{
    if (registers == NULL || output == NULL) {
        return false;
    }

    if (register_count < BPGA_MODBUS_COMMAND_RESULT_COUNT) {
        return false;
    }

    bpga_modbus_command_response_t temp;

    temp.acknowledged_sequence = registers[0];
    temp.status = (bpga_modbus_command_status_t)registers[1];
    temp.result = (bpga_modbus_command_result_t)registers[2];
    temp.detail = registers[3];

    *output = temp;
    return true;
}

/* ============================================================================
 * ENCODING FUNCTIONS
 * ============================================================================ */

bool bpga_modbus_encode_command_request(
    const bpga_modbus_command_request_t *request,
    uint16_t *registers,
    size_t register_count
)
{
    if (request == NULL || registers == NULL) {
        return false;
    }

    if (register_count < BPGA_MODBUS_COMMAND_REQUEST_COUNT) {
        return false;
    }

    registers[0] = request->sequence;
    registers[1] = (uint16_t)request->code;
    registers[2] = request->argument_0;
    registers[3] = request->argument_1;
    registers[4] = request->argument_2;
    registers[5] = BPGA_MODBUS_COMMAND_CONTROL_KEY;

    return true;
}
