#include "unity.h"
#include "bpga_modbus_map.h"
#include <string.h>

/* ============================================================================
 * TEST: Combines Modbus words into uint32
 * ============================================================================ */

TEST_CASE("combina palabras Modbus en uint32", "[bpga_modbus_map]")
{
    uint16_t high = 0x1234U;
    uint16_t low = 0x5678U;
    uint32_t result = bpga_modbus_u32_from_words(high, low);
    TEST_ASSERT_EQUAL_HEX32(0x12345678U, result);
}

/* ============================================================================
 * TEST: Splits uint32 into Modbus words
 * ============================================================================ */

TEST_CASE("separa uint32 en palabras Modbus", "[bpga_modbus_map]")
{
    uint32_t value = 0x89ABCDEFU;
    uint16_t high = 0U;
    uint16_t low = 0U;

    bpga_modbus_u32_to_words(value, &high, &low);

    TEST_ASSERT_EQUAL_HEX16(0x89ABU, high);
    TEST_ASSERT_EQUAL_HEX16(0xCDEFU, low);
}

/* ============================================================================
 * TEST: Splitting uint32 tolerates NULL pointers
 * ============================================================================ */

TEST_CASE("separar uint32 tolera punteros nulos", "[bpga_modbus_map]")
{
    uint32_t value = 0x89ABCDEFU;
    uint16_t high = 0U;

    /* Test with low = NULL */
    bpga_modbus_u32_to_words(value, &high, NULL);
    TEST_ASSERT_EQUAL_HEX16(0x89ABU, high);

    /* Test with high = NULL */
    uint16_t low = 0U;
    bpga_modbus_u32_to_words(value, NULL, &low);
    TEST_ASSERT_EQUAL_HEX16(0xCDEFU, low);

    /* Test with both NULL (should not crash) */
    bpga_modbus_u32_to_words(value, NULL, NULL);
}

/* ============================================================================
 * TEST: Validates snapshot sequence
 * ============================================================================ */

TEST_CASE("valida secuencia de snapshot", "[bpga_modbus_map]")
{
    /* Valid: begin=end and even */
    TEST_ASSERT_TRUE(bpga_modbus_snapshot_sequence_is_valid(2, 2));
    TEST_ASSERT_TRUE(bpga_modbus_snapshot_sequence_is_valid(0, 0));
    TEST_ASSERT_TRUE(bpga_modbus_snapshot_sequence_is_valid(4, 4));

    /* Invalid: odd sequence */
    TEST_ASSERT_FALSE(bpga_modbus_snapshot_sequence_is_valid(3, 3));
    TEST_ASSERT_FALSE(bpga_modbus_snapshot_sequence_is_valid(1, 1));

    /* Invalid: begin != end */
    TEST_ASSERT_FALSE(bpga_modbus_snapshot_sequence_is_valid(2, 4));
    TEST_ASSERT_FALSE(bpga_modbus_snapshot_sequence_is_valid(2, 3));
}

/* ============================================================================
 * TEST: Validates slave address
 * ============================================================================ */

TEST_CASE("valida dirección de servidor", "[bpga_modbus_map]")
{
    /* Valid addresses: 1-247 */
    TEST_ASSERT_TRUE(bpga_modbus_is_valid_slave_address(1));
    TEST_ASSERT_TRUE(bpga_modbus_is_valid_slave_address(100));
    TEST_ASSERT_TRUE(bpga_modbus_is_valid_slave_address(247));

    /* Invalid addresses */
    TEST_ASSERT_FALSE(bpga_modbus_is_valid_slave_address(0));
    TEST_ASSERT_FALSE(bpga_modbus_is_valid_slave_address(248));
    TEST_ASSERT_FALSE(bpga_modbus_is_valid_slave_address(255));
}

/* ============================================================================
 * TEST: Validates profile slot
 * ============================================================================ */

TEST_CASE("valida posición de perfil", "[bpga_modbus_map]")
{
    /* Valid slots: 0-9 */
    TEST_ASSERT_TRUE(bpga_modbus_is_valid_profile_slot(0));
    TEST_ASSERT_TRUE(bpga_modbus_is_valid_profile_slot(5));
    TEST_ASSERT_TRUE(bpga_modbus_is_valid_profile_slot(9));

    /* Invalid slots */
    TEST_ASSERT_FALSE(bpga_modbus_is_valid_profile_slot(10));
    TEST_ASSERT_FALSE(bpga_modbus_is_valid_profile_slot(11));
    TEST_ASSERT_FALSE(bpga_modbus_is_valid_profile_slot(0xFFFFU));
}

/* ============================================================================
 * TEST: Validates permille values
 * ============================================================================ */

TEST_CASE("valida valores por mil", "[bpga_modbus_map]")
{
    /* Valid: 0-1000 */
    TEST_ASSERT_TRUE(bpga_modbus_is_valid_permille(0));
    TEST_ASSERT_TRUE(bpga_modbus_is_valid_permille(500));
    TEST_ASSERT_TRUE(bpga_modbus_is_valid_permille(1000));

    /* Invalid */
    TEST_ASSERT_FALSE(bpga_modbus_is_valid_permille(1001));
    TEST_ASSERT_FALSE(bpga_modbus_is_valid_permille(2000));
    TEST_ASSERT_FALSE(bpga_modbus_is_valid_permille(0xFFFFU));
}

/* ============================================================================
 * TEST: Decodes identification block
 * ============================================================================ */

TEST_CASE("decodifica identificación", "[bpga_modbus_map]")
{
    uint16_t registers[16] = {
        0x4250U,  /* DEVICE_MAGIC = 0x4250 */
        0x0100U,  /* PROTOCOL_VERSION = 0x0100 */
        0x0100U,  /* REGISTER_MAP_VERSION = 0x0100 */
        0x0001U,  /* FIRMWARE_VERSION_MAJOR = 1 */
        0x0002U,  /* FIRMWARE_VERSION_MINOR = 2 */
        0x0003U,  /* FIRMWARE_VERSION_PATCH = 3 */
        0x0001U,  /* DEVICE_TYPE = 1 */
        0x00FFU,  /* CAPABILITIES = 0x00FF */
        0x0000U,  /* BOOT_COUNT_HI = 0 */
        0x0005U,  /* BOOT_COUNT_LO = 5 */
        0, 0, 0, 0, 0, 0  /* Reserved */
    };

    bpga_modbus_identification_t output = {0};
    bool result = bpga_modbus_decode_identification(registers, 16, &output);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_HEX16(0x4250U, output.device_magic);
    TEST_ASSERT_EQUAL_HEX16(0x0100U, output.protocol_version);
    TEST_ASSERT_EQUAL_HEX16(0x0100U, output.register_map_version);
    TEST_ASSERT_EQUAL_HEX16(0x0001U, output.firmware_version_major);
    TEST_ASSERT_EQUAL_HEX16(0x0002U, output.firmware_version_minor);
    TEST_ASSERT_EQUAL_HEX16(0x0003U, output.firmware_version_patch);
    TEST_ASSERT_EQUAL_HEX32(0x00000005U, output.boot_count);
}

/* ============================================================================
 * TEST: Rejects identification with short buffer
 * ============================================================================ */

TEST_CASE("rechaza identificación con buffer corto", "[bpga_modbus_map]")
{
    uint16_t registers[15] = {0};
    bpga_modbus_identification_t output;
    memset(&output, 0xA5, sizeof(output));
    bpga_modbus_identification_t original;
    memcpy(&original, &output, sizeof(original));

    bool result = bpga_modbus_decode_identification(registers, 15, &output);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_MEMORY(&original, &output, sizeof(output));
}

/* ============================================================================
 * TEST: Decodes snapshot with valid sequence
 * ============================================================================ */

TEST_CASE("decodifica snapshot coherente", "[bpga_modbus_map]")
{
    uint16_t registers[32] = {
        0x0002U,  /* SNAPSHOT_SEQUENCE_BEGIN = 2 */
        0x0001U,  /* SYSTEM_STATE = 1 */
        0x0000U,  /* SYSTEM_SUBSTATE = 0 */
        0x000FU,  /* STATUS_FLAGS = 0x000F */
        0x0001U,  /* CONTROL_FLAGS = 1 */
        0x0001U,  /* ACTIVE_PROFILE_SLOT = 1 */
        0x0001U,  /* ACTIVE_PROFILE_REVISION = 1 */
        0x0000U,  /* LAST_TRANSITION_REASON = 0 */
        0x0000U,  /* UPTIME_HI = 0 */
        0x0064U,  /* UPTIME_LO = 100 */
        0x0100U,  /* LOOP_TIME_US = 256 */
        0x0200U,  /* LOOP_TIME_MAX_US = 512 */
        0, 0, 0, 0,  /* Reserved state */
        0x0010U,  /* RPM_HI = 16 */
        0x00FFU,  /* RPM_LO = 255 */
        0x0001U,  /* VOLTAGE_MV_HI = 1 */
        0x86A0U,  /* VOLTAGE_MV_LO = 34464 */
        0x0000U,  /* CURRENT_MA_HI = 0 */
        0x00A0U,  /* CURRENT_MA_LO = 160 */
        0x0000U,  /* POWER_W_HI = 0 */
        0x0032U,  /* POWER_W_LO = 50 */
        0x0200U,  /* EXCITATION_PERMILLE = 512 */
        0x0100U,  /* LOAD_PERMILLE = 256 */
        0x000AU,  /* RPM_QUALITY = 10 */
        0x000BU,  /* VOLTAGE_QUALITY = 11 */
        0x000CU,  /* CURRENT_QUALITY = 12 */
        0x001EU,  /* SAMPLE_AGE_MS = 30 */
        0x000FU,  /* MEASUREMENT_FLAGS = 15 */
        0x0002U   /* SNAPSHOT_SEQUENCE_END = 2 */
    };

    bpga_modbus_snapshot_t output = {0};
    bool result = bpga_modbus_decode_snapshot(registers, 32, &output);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT16(2, output.sequence);
    TEST_ASSERT_EQUAL(BPGA_MODBUS_SYSTEM_SAFE_STOPPED, output.system_state);
    TEST_ASSERT_EQUAL_UINT32(100U, output.uptime_ms);
    TEST_ASSERT_EQUAL_UINT32(0x001000FFU, output.rpm);
    TEST_ASSERT_EQUAL_UINT32(0x000186A0U, output.voltage_mv);
    TEST_ASSERT_EQUAL_UINT32(0x000000A0U, output.current_ma);
    TEST_ASSERT_EQUAL_UINT32(0x00000032U, output.power_w);
    TEST_ASSERT_EQUAL_UINT16(512, output.excitation_permille);
}

/* ============================================================================
 * TEST: Rejects snapshot with mismatched sequences
 * ============================================================================ */

TEST_CASE("rechaza snapshot con secuencias distintas", "[bpga_modbus_map]")
{
    uint16_t registers[32];
    memset(registers, 0, sizeof(registers));
    registers[0] = 2;
    registers[31] = 4;

    bpga_modbus_snapshot_t output;
    memset(&output, 0xA5, sizeof(output));
    bpga_modbus_snapshot_t original;
    memcpy(&original, &output, sizeof(original));

    bool result = bpga_modbus_decode_snapshot(registers, 32, &output);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_MEMORY(&original, &output, sizeof(output));
}

/* ============================================================================
 * TEST: Rejects snapshot with odd sequence
 * ============================================================================ */

TEST_CASE("rechaza snapshot con secuencia impar", "[bpga_modbus_map]")
{
    uint16_t registers[32];
    memset(registers, 0, sizeof(registers));
    registers[0] = 3;
    registers[31] = 3;

    bpga_modbus_snapshot_t output;
    memset(&output, 0xA5, sizeof(output));
    bpga_modbus_snapshot_t original;
    memcpy(&original, &output, sizeof(original));

    bool result = bpga_modbus_decode_snapshot(registers, 32, &output);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_MEMORY(&original, &output, sizeof(output));
}

/* ============================================================================
 * TEST: Rejects snapshot with short buffer
 * ============================================================================ */

TEST_CASE("rechaza snapshot con buffer corto", "[bpga_modbus_map]")
{
    uint16_t registers[31];
    memset(registers, 0, sizeof(registers));

    bpga_modbus_snapshot_t output;
    memset(&output, 0xA5, sizeof(output));
    bpga_modbus_snapshot_t original;
    memcpy(&original, &output, sizeof(original));

    bool result = bpga_modbus_decode_snapshot(registers, 31, &output);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_MEMORY(&original, &output, sizeof(output));
}

/* ============================================================================
 * TEST: Decodes totals block
 * ============================================================================ */

TEST_CASE("decodifica acumulados", "[bpga_modbus_map]")
{
    uint16_t registers[16] = {
        0x0001U,  /* TEST_ELAPSED_MS_HI = 1 */
        0x0000U,  /* TEST_ELAPSED_MS_LO = 0 */
        0x0002U,  /* RPM_MAX_HI = 2 */
        0x0000U,  /* RPM_MAX_LO = 0 */
        0x0003U,  /* VOLTAGE_MAX_MV_HI = 3 */
        0x0000U,  /* VOLTAGE_MAX_MV_LO = 0 */
        0x0004U,  /* CURRENT_MAX_MA_HI = 4 */
        0x0000U,  /* CURRENT_MAX_MA_LO = 0 */
        0x0005U,  /* ENERGY_WH_HI = 5 */
        0x0000U,  /* ENERGY_WH_LO = 0 */
        0x0006U,  /* SAMPLE_COUNTER_HI = 6 */
        0x0000U,  /* SAMPLE_COUNTER_LO = 0 */
        0, 0, 0, 0  /* Reserved */
    };

    bpga_modbus_totals_t output = {0};
    bool result = bpga_modbus_decode_totals(registers, 16, &output);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT32(0x00010000U, output.test_elapsed_ms);
    TEST_ASSERT_EQUAL_UINT32(0x00020000U, output.rpm_max);
    TEST_ASSERT_EQUAL_UINT32(0x00030000U, output.voltage_max_mv);
    TEST_ASSERT_EQUAL_UINT32(0x00040000U, output.current_max_ma);
    TEST_ASSERT_EQUAL_UINT32(0x00050000U, output.energy_wh);
    TEST_ASSERT_EQUAL_UINT32(0x00060000U, output.sample_counter);
}

/* ============================================================================
 * TEST: Decodes diagnostics block
 * ============================================================================ */

TEST_CASE("decodifica diagnósticos", "[bpga_modbus_map]")
{
    uint16_t registers[16] = {
        0x0001U,  /* ACTIVE_WARNINGS = 1 */
        0x0002U,  /* ACTIVE_ALARMS = 2 */
        0x0004U,  /* LATCHED_ALARMS = 4 */
        0x0008U,  /* PRIMARY_FAULT_CODE = 8 */
        0x0010U,  /* LAST_FAULT_CODE = 16 */
        0x0000U,  /* LAST_FAULT_UPTIME_MS_HI = 0 */
        0x0064U,  /* LAST_FAULT_UPTIME_MS_LO = 100 */
        0x0020U,  /* MODBUS_VALID_REQUESTS = 32 */
        0x0001U,  /* MODBUS_CRC_ERRORS = 1 */
        0x0002U,  /* MODBUS_EXCEPTIONS = 2 */
        0x0003U,  /* WATCHDOG_RESET_COUNT = 3 */
        0, 0, 0, 0, 0  /* Reserved */
    };

    bpga_modbus_diagnostics_t output = {0};
    bool result = bpga_modbus_decode_diagnostics(registers, 16, &output);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT16(1, output.active_warnings);
    TEST_ASSERT_EQUAL_UINT16(2, output.active_alarms);
    TEST_ASSERT_EQUAL_UINT16(4, output.latched_alarms);
    TEST_ASSERT_EQUAL_UINT32(100, output.last_fault_uptime_ms);
    TEST_ASSERT_EQUAL_UINT16(32, output.modbus_valid_requests);
    TEST_ASSERT_EQUAL_UINT16(1, output.modbus_crc_errors);
    TEST_ASSERT_EQUAL_UINT16(2, output.modbus_exceptions);
    TEST_ASSERT_EQUAL_UINT16(3, output.watchdog_reset_count);
}

/* ============================================================================
 * TEST: Block buffers are relative to the start of the requested block
 * ============================================================================ */

TEST_CASE("snapshot usa offsets relativos al bloque", "[bpga_modbus_map]")
{
    uint16_t registers[32] = {0};
    registers[0] = 6;
    registers[16] = 0x0001U;
    registers[17] = 0x0002U;
    registers[31] = 6;

    bpga_modbus_snapshot_t output = {0};

    TEST_ASSERT_TRUE(bpga_modbus_decode_snapshot(registers, 32, &output));
    TEST_ASSERT_EQUAL_UINT16(6, output.sequence);
    TEST_ASSERT_EQUAL_UINT32(0x00010002U, output.rpm);
}

TEST_CASE("acumulados usa offsets relativos al bloque", "[bpga_modbus_map]")
{
    uint16_t registers[16] = {0};
    registers[0] = 0x0001U;
    registers[1] = 0x0002U;
    registers[10] = 0x0003U;
    registers[11] = 0x0004U;

    bpga_modbus_totals_t output = {0};

    TEST_ASSERT_TRUE(bpga_modbus_decode_totals(registers, 16, &output));
    TEST_ASSERT_EQUAL_UINT32(0x00010002U, output.test_elapsed_ms);
    TEST_ASSERT_EQUAL_UINT32(0x00030004U, output.sample_counter);
}

TEST_CASE("diagnósticos usa offsets relativos al bloque", "[bpga_modbus_map]")
{
    uint16_t registers[16] = {0};
    registers[0] = 0x0007U;
    registers[5] = 0x0001U;
    registers[6] = 0x0002U;
    registers[10] = 0x0009U;

    bpga_modbus_diagnostics_t output = {0};

    TEST_ASSERT_TRUE(bpga_modbus_decode_diagnostics(registers, 16, &output));
    TEST_ASSERT_EQUAL_UINT16(7, output.active_warnings);
    TEST_ASSERT_EQUAL_UINT32(0x00010002U, output.last_fault_uptime_ms);
    TEST_ASSERT_EQUAL_UINT16(9, output.watchdog_reset_count);
}

/* ============================================================================
 * TEST: Encodes command request
 * ============================================================================ */

TEST_CASE("codifica solicitud de comando", "[bpga_modbus_map]")
{
    bpga_modbus_command_request_t request = {
        .sequence = 0x0001U,
        .code = BPGA_MODBUS_COMMAND_START_TEST,
        .argument_0 = 0x0100U,
        .argument_1 = 0x0200U,
        .argument_2 = 0x0300U,
    };

    uint16_t registers[6] = {0};
    bool result = bpga_modbus_encode_command_request(&request, registers, 6);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT16(0x0001U, registers[0]);
    TEST_ASSERT_EQUAL_UINT16(BPGA_MODBUS_COMMAND_START_TEST, registers[1]);
    TEST_ASSERT_EQUAL_UINT16(0x0100U, registers[2]);
    TEST_ASSERT_EQUAL_UINT16(0x0200U, registers[3]);
    TEST_ASSERT_EQUAL_UINT16(0x0300U, registers[4]);
    TEST_ASSERT_EQUAL_UINT16(BPGA_MODBUS_COMMAND_CONTROL_KEY, registers[5]);
}

/* ============================================================================
 * TEST: Rejects command with short buffer
 * ============================================================================ */

TEST_CASE("rechaza solicitud con buffer corto", "[bpga_modbus_map]")
{
    bpga_modbus_command_request_t request = {
        .sequence = 0x0001U,
        .code = BPGA_MODBUS_COMMAND_START_TEST,
        .argument_0 = 0x0100U,
        .argument_1 = 0x0200U,
        .argument_2 = 0x0300U,
    };

    uint16_t registers[5];
    memset(registers, 0xFFU, sizeof(registers));
    uint16_t original[5];
    memcpy(original, registers, sizeof(registers));

    bool result = bpga_modbus_encode_command_request(&request, registers, 5);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_MEMORY(original, registers, sizeof(registers));
}

/* ============================================================================
 * TEST: Decodes command response
 * ============================================================================ */

TEST_CASE("decodifica respuesta de comando", "[bpga_modbus_map]")
{
    uint16_t registers[4] = {
        0x0001U,  /* ACK_SEQUENCE = 1 */
        0x0003U,  /* STATUS = COMPLETED */
        0x0000U,  /* RESULT = OK */
        0x0000U,  /* DETAIL = 0 */
    };

    bpga_modbus_command_response_t output = {0};
    bool result = bpga_modbus_decode_command_response(registers, 4, &output);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT16(1, output.acknowledged_sequence);
    TEST_ASSERT_EQUAL(BPGA_MODBUS_COMMAND_STATUS_COMPLETED, output.status);
    TEST_ASSERT_EQUAL(BPGA_MODBUS_COMMAND_RESULT_OK, output.result);
    TEST_ASSERT_EQUAL_UINT16(0, output.detail);
}

/* ============================================================================
 * TEST: Decoding functions reject NULL pointers
 * ============================================================================ */

TEST_CASE("funciones de decodificación rechazan punteros nulos", "[bpga_modbus_map]")
{
    uint16_t registers[32] = {0};
    bpga_modbus_identification_t id_out;
    bpga_modbus_snapshot_t snap_out;
    bpga_modbus_totals_t tot_out;
    bpga_modbus_diagnostics_t diag_out;
    bpga_modbus_command_response_t cmd_out;

    /* Test with NULL registers */
    TEST_ASSERT_FALSE(bpga_modbus_decode_identification(NULL, 16, &id_out));
    TEST_ASSERT_FALSE(bpga_modbus_decode_snapshot(NULL, 32, &snap_out));
    TEST_ASSERT_FALSE(bpga_modbus_decode_totals(NULL, 16, &tot_out));
    TEST_ASSERT_FALSE(bpga_modbus_decode_diagnostics(NULL, 16, &diag_out));
    TEST_ASSERT_FALSE(bpga_modbus_decode_command_response(NULL, 4, &cmd_out));

    /* Test with NULL output */
    TEST_ASSERT_FALSE(bpga_modbus_decode_identification(registers, 16, NULL));
    TEST_ASSERT_FALSE(bpga_modbus_decode_snapshot(registers, 32, NULL));
    TEST_ASSERT_FALSE(bpga_modbus_decode_totals(registers, 16, NULL));
    TEST_ASSERT_FALSE(bpga_modbus_decode_diagnostics(registers, 16, NULL));
    TEST_ASSERT_FALSE(bpga_modbus_decode_command_response(registers, 4, NULL));
}

