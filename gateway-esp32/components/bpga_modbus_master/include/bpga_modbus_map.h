#ifndef BPGA_MODBUS_MAP_H
#define BPGA_MODBUS_MAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * GENERAL CONSTANTS
 * ============================================================================ */

#define BPGA_MODBUS_DEVICE_MAGIC UINT16_C(0x4250)

#define BPGA_MODBUS_PROTOCOL_VERSION_MAJOR UINT8_C(1)
#define BPGA_MODBUS_PROTOCOL_VERSION_MINOR UINT8_C(0)

#define BPGA_MODBUS_REGISTER_MAP_VERSION_MAJOR UINT8_C(1)
#define BPGA_MODBUS_REGISTER_MAP_VERSION_MINOR UINT8_C(0)

#define BPGA_MODBUS_COMMAND_CONTROL_KEY UINT16_C(0xA55A)
#define BPGA_MODBUS_NO_ACTIVE_PROFILE UINT16_C(0xFFFF)

/* Macro to encode version: major in high byte, minor in low byte */
#define BPGA_MODBUS_VERSION_ENCODE(major, minor) \
    ((uint16_t)(((uint8_t)(major) << 8U) | (uint8_t)(minor)))

/* ============================================================================
 * MODBUS FUNCTION CODES (FC)
 * ============================================================================ */

/* FC decimal 3 = 0x03 hex: Read Holding Registers */
#define BPGA_MODBUS_FC_READ_HOLDING_REGISTERS UINT8_C(0x03)

/* FC decimal 4 = 0x04 hex: Read Input Registers */
#define BPGA_MODBUS_FC_READ_INPUT_REGISTERS UINT8_C(0x04)

/* FC decimal 6 = 0x06 hex: Write Single Register */
#define BPGA_MODBUS_FC_WRITE_SINGLE_REGISTER UINT8_C(0x06)

/* FC decimal 16 = 0x10 hex: Write Multiple Registers */
#define BPGA_MODBUS_FC_WRITE_MULTIPLE_REGISTERS UINT8_C(0x10)

/* ============================================================================
 * INPUT REGISTERS (Read-only)
 *
 * Note: References 30001 and 40001 (Modbus SCADA convention) are documentary
 * only and must NOT be transmitted in the PDU. PDU addresses are zero-based.
 * ============================================================================ */

typedef enum {
    /* Identification Block: 0x0000-0x000F (16 registers) */
    BPGA_MODBUS_IR_DEVICE_MAGIC = 0x0000,
    BPGA_MODBUS_IR_PROTOCOL_VERSION = 0x0001,
    BPGA_MODBUS_IR_REGISTER_MAP_VERSION = 0x0002,
    BPGA_MODBUS_IR_FIRMWARE_VERSION_MAJOR = 0x0003,
    BPGA_MODBUS_IR_FIRMWARE_VERSION_MINOR = 0x0004,
    BPGA_MODBUS_IR_FIRMWARE_VERSION_PATCH = 0x0005,
    BPGA_MODBUS_IR_DEVICE_TYPE = 0x0006,
    BPGA_MODBUS_IR_DEVICE_CAPABILITIES = 0x0007,
    BPGA_MODBUS_IR_BOOT_COUNT_HI = 0x0008,
    BPGA_MODBUS_IR_BOOT_COUNT_LO = 0x0009,
    BPGA_MODBUS_IR_RESERVED_ID_BEGIN = 0x000A,
    BPGA_MODBUS_IR_RESERVED_ID_END = 0x000F,

    /* State/Snapshot Block: 0x0010-0x002F (32 registers) */
    BPGA_MODBUS_IR_SNAPSHOT_SEQUENCE_BEGIN = 0x0010,
    BPGA_MODBUS_IR_SYSTEM_STATE = 0x0011,
    BPGA_MODBUS_IR_SYSTEM_SUBSTATE = 0x0012,
    BPGA_MODBUS_IR_STATUS_FLAGS = 0x0013,
    BPGA_MODBUS_IR_CONTROL_FLAGS = 0x0014,
    BPGA_MODBUS_IR_ACTIVE_PROFILE_SLOT = 0x0015,
    BPGA_MODBUS_IR_ACTIVE_PROFILE_REVISION = 0x0016,
    BPGA_MODBUS_IR_LAST_TRANSITION_REASON = 0x0017,
    BPGA_MODBUS_IR_UPTIME_HI = 0x0018,
    BPGA_MODBUS_IR_UPTIME_LO = 0x0019,
    BPGA_MODBUS_IR_LOOP_TIME_US = 0x001A,
    BPGA_MODBUS_IR_LOOP_TIME_MAX_US = 0x001B,
    BPGA_MODBUS_IR_RESERVED_STATE_BEGIN = 0x001C,
    BPGA_MODBUS_IR_RESERVED_STATE_END = 0x001F,

    /* Telemetry Block: 0x0020-0x002F (16 registers, part of snapshot) */
    BPGA_MODBUS_IR_RPM_HI = 0x0020,
    BPGA_MODBUS_IR_RPM_LO = 0x0021,
    BPGA_MODBUS_IR_VOLTAGE_MV_HI = 0x0022,
    BPGA_MODBUS_IR_VOLTAGE_MV_LO = 0x0023,
    BPGA_MODBUS_IR_CURRENT_MA_HI = 0x0024,
    BPGA_MODBUS_IR_CURRENT_MA_LO = 0x0025,
    BPGA_MODBUS_IR_POWER_W_HI = 0x0026,
    BPGA_MODBUS_IR_POWER_W_LO = 0x0027,
    BPGA_MODBUS_IR_EXCITATION_PERMILLE = 0x0028,
    BPGA_MODBUS_IR_LOAD_PERMILLE = 0x0029,
    BPGA_MODBUS_IR_RPM_QUALITY = 0x002A,
    BPGA_MODBUS_IR_VOLTAGE_QUALITY = 0x002B,
    BPGA_MODBUS_IR_CURRENT_QUALITY = 0x002C,
    BPGA_MODBUS_IR_SAMPLE_AGE_MS = 0x002D,
    BPGA_MODBUS_IR_MEASUREMENT_FLAGS = 0x002E,
    BPGA_MODBUS_IR_SNAPSHOT_SEQUENCE_END = 0x002F,

    /* Totals/Accumulators Block: 0x0030-0x003F (16 registers) */
    BPGA_MODBUS_IR_TEST_ELAPSED_MS_HI = 0x0030,
    BPGA_MODBUS_IR_TEST_ELAPSED_MS_LO = 0x0031,
    BPGA_MODBUS_IR_RPM_MAX_HI = 0x0032,
    BPGA_MODBUS_IR_RPM_MAX_LO = 0x0033,
    BPGA_MODBUS_IR_VOLTAGE_MAX_MV_HI = 0x0034,
    BPGA_MODBUS_IR_VOLTAGE_MAX_MV_LO = 0x0035,
    BPGA_MODBUS_IR_CURRENT_MAX_MA_HI = 0x0036,
    BPGA_MODBUS_IR_CURRENT_MAX_MA_LO = 0x0037,
    BPGA_MODBUS_IR_ENERGY_WH_HI = 0x0038,
    BPGA_MODBUS_IR_ENERGY_WH_LO = 0x0039,
    BPGA_MODBUS_IR_SAMPLE_COUNTER_HI = 0x003A,
    BPGA_MODBUS_IR_SAMPLE_COUNTER_LO = 0x003B,
    BPGA_MODBUS_IR_RESERVED_TOTALS_BEGIN = 0x003C,
    BPGA_MODBUS_IR_RESERVED_TOTALS_END = 0x003F,

    /* Diagnostics/Alarms Block: 0x0040-0x004F (16 registers) */
    BPGA_MODBUS_IR_ACTIVE_WARNINGS = 0x0040,
    BPGA_MODBUS_IR_ACTIVE_ALARMS = 0x0041,
    BPGA_MODBUS_IR_LATCHED_ALARMS = 0x0042,
    BPGA_MODBUS_IR_PRIMARY_FAULT_CODE = 0x0043,
    BPGA_MODBUS_IR_LAST_FAULT_CODE = 0x0044,
    BPGA_MODBUS_IR_LAST_FAULT_UPTIME_MS_HI = 0x0045,
    BPGA_MODBUS_IR_LAST_FAULT_UPTIME_MS_LO = 0x0046,
    BPGA_MODBUS_IR_MODBUS_VALID_REQUESTS = 0x0047,
    BPGA_MODBUS_IR_MODBUS_CRC_ERRORS = 0x0048,
    BPGA_MODBUS_IR_MODBUS_EXCEPTIONS = 0x0049,
    BPGA_MODBUS_IR_WATCHDOG_RESET_COUNT = 0x004A,
    BPGA_MODBUS_IR_RESERVED_DIAGNOSTICS_BEGIN = 0x004B,
    BPGA_MODBUS_IR_RESERVED_DIAGNOSTICS_END = 0x004F,
} bpga_modbus_input_register_t;

#define BPGA_MODBUS_INPUT_REGISTER_FIRST UINT16_C(0x0000)
#define BPGA_MODBUS_INPUT_REGISTER_LAST UINT16_C(0x004F)
#define BPGA_MODBUS_INPUT_REGISTER_COUNT UINT16_C(0x0050)

/* Input Register Blocks */
#define BPGA_MODBUS_IDENTIFICATION_START UINT16_C(0x0000)
#define BPGA_MODBUS_IDENTIFICATION_COUNT UINT16_C(16)

#define BPGA_MODBUS_SNAPSHOT_START UINT16_C(0x0010)
#define BPGA_MODBUS_SNAPSHOT_END UINT16_C(0x002F)
#define BPGA_MODBUS_SNAPSHOT_COUNT UINT16_C(32)

#define BPGA_MODBUS_TOTALS_START UINT16_C(0x0030)
#define BPGA_MODBUS_TOTALS_COUNT UINT16_C(16)

#define BPGA_MODBUS_DIAGNOSTICS_START UINT16_C(0x0040)
#define BPGA_MODBUS_DIAGNOSTICS_COUNT UINT16_C(16)

/* ============================================================================
 * HOLDING REGISTERS (Read-Write)
 *
 * Note: References 40001 onwards (Modbus SCADA convention) are documentary
 * only and must NOT be transmitted in the PDU. PDU addresses are zero-based.
 * ============================================================================ */

typedef enum {
    /* Configuration Block: 0x0000-0x000F (16 registers) */
    BPGA_MODBUS_HR_MODBUS_SLAVE_ADDRESS = 0x0000,
    BPGA_MODBUS_HR_MODBUS_BAUD_CODE = 0x0001,
    BPGA_MODBUS_HR_MODBUS_FORMAT_CODE = 0x0002,
    BPGA_MODBUS_HR_TELEMETRY_PERIOD_MS = 0x0003,
    BPGA_MODBUS_HR_COMM_WATCHDOG_MS = 0x0004,
    BPGA_MODBUS_HR_OPERATING_MODE = 0x0005,
    BPGA_MODBUS_HR_RESERVED_CONFIG_BEGIN = 0x0006,
    BPGA_MODBUS_HR_RESERVED_CONFIG_END = 0x000F,

    /* Profile Selection Block: 0x0010-0x001F (16 registers) */
    BPGA_MODBUS_HR_REQUESTED_PROFILE_SLOT = 0x0010,
    BPGA_MODBUS_HR_PROFILE_EXPECTED_REVISION = 0x0011,
    BPGA_MODBUS_HR_PROFILE_SELECTION_FLAGS = 0x0012,
    BPGA_MODBUS_HR_PROFILE_SELECTION_RESULT = 0x0013,
    BPGA_MODBUS_HR_RESERVED_PROFILE_BEGIN = 0x0014,
    BPGA_MODBUS_HR_RESERVED_PROFILE_END = 0x0017,
    BPGA_MODBUS_HR_RESERVED_PROFILE_EXPANSION_BEGIN = 0x0018,
    BPGA_MODBUS_HR_RESERVED_PROFILE_EXPANSION_END = 0x001F,

    /* Setpoints Block: 0x0020-0x003F (32 registers) */
    BPGA_MODBUS_HR_RPM_SETPOINT_HI = 0x0020,
    BPGA_MODBUS_HR_RPM_SETPOINT_LO = 0x0021,
    BPGA_MODBUS_HR_VOLTAGE_SETPOINT_MV_HI = 0x0022,
    BPGA_MODBUS_HR_VOLTAGE_SETPOINT_MV_LO = 0x0023,
    BPGA_MODBUS_HR_CURRENT_LIMIT_MA_HI = 0x0024,
    BPGA_MODBUS_HR_CURRENT_LIMIT_MA_LO = 0x0025,
    BPGA_MODBUS_HR_EXCITATION_SETPOINT = 0x0026,
    BPGA_MODBUS_HR_LOAD_SETPOINT = 0x0027,
    BPGA_MODBUS_HR_TEST_DURATION_MS_HI = 0x0028,
    BPGA_MODBUS_HR_TEST_DURATION_MS_LO = 0x0029,
    BPGA_MODBUS_HR_RAMP_TIME_MS = 0x002A,
    BPGA_MODBUS_HR_SETPOINT_FLAGS = 0x002B,
    BPGA_MODBUS_HR_RESERVED_SETPOINTS_BEGIN = 0x002C,
    BPGA_MODBUS_HR_RESERVED_SETPOINTS_END = 0x002F,
    BPGA_MODBUS_HR_RESERVED_CONTROL_EXPANSION_BEGIN = 0x0030,
    BPGA_MODBUS_HR_RESERVED_CONTROL_EXPANSION_END = 0x003F,

    /* Command Request Block: 0x0040-0x0045 (6 registers) */
    BPGA_MODBUS_HR_COMMAND_SEQUENCE = 0x0040,
    BPGA_MODBUS_HR_COMMAND_CODE = 0x0041,
    BPGA_MODBUS_HR_COMMAND_ARGUMENT_0 = 0x0042,
    BPGA_MODBUS_HR_COMMAND_ARGUMENT_1 = 0x0043,
    BPGA_MODBUS_HR_COMMAND_ARGUMENT_2 = 0x0044,
    BPGA_MODBUS_HR_COMMAND_CONTROL = 0x0045,

    /* Reserved Command Block: 0x0046-0x0047 */
    BPGA_MODBUS_HR_RESERVED_COMMAND_BEGIN = 0x0046,
    BPGA_MODBUS_HR_RESERVED_COMMAND_END = 0x0047,

    /* Command Response Block: 0x0048-0x004B (4 registers) */
    BPGA_MODBUS_HR_COMMAND_ACK_SEQUENCE = 0x0048,
    BPGA_MODBUS_HR_COMMAND_STATUS = 0x0049,
    BPGA_MODBUS_HR_COMMAND_RESULT = 0x004A,
    BPGA_MODBUS_HR_COMMAND_DETAIL = 0x004B,

    /* Reserved Command Result Block: 0x004C-0x004F */
    BPGA_MODBUS_HR_RESERVED_COMMAND_RESULT_BEGIN = 0x004C,
    BPGA_MODBUS_HR_RESERVED_COMMAND_RESULT_END = 0x004F,
} bpga_modbus_holding_register_t;

#define BPGA_MODBUS_HOLDING_REGISTER_FIRST UINT16_C(0x0000)
#define BPGA_MODBUS_HOLDING_REGISTER_LAST UINT16_C(0x004F)
#define BPGA_MODBUS_HOLDING_REGISTER_COUNT UINT16_C(0x0050)

/* Command Blocks */
#define BPGA_MODBUS_COMMAND_REQUEST_START UINT16_C(0x0040)
#define BPGA_MODBUS_COMMAND_REQUEST_END UINT16_C(0x0045)
#define BPGA_MODBUS_COMMAND_REQUEST_COUNT UINT16_C(6)

#define BPGA_MODBUS_COMMAND_RESULT_START UINT16_C(0x0048)
#define BPGA_MODBUS_COMMAND_RESULT_END UINT16_C(0x004B)
#define BPGA_MODBUS_COMMAND_RESULT_COUNT UINT16_C(4)

/* ============================================================================
 * CAPABILITY MASKS
 * ============================================================================ */

#define BPGA_MODBUS_CAP_RPM UINT16_C(0x0001)
#define BPGA_MODBUS_CAP_VOLTAGE UINT16_C(0x0002)
#define BPGA_MODBUS_CAP_CURRENT UINT16_C(0x0004)
#define BPGA_MODBUS_CAP_EXCITATION_CONTROL UINT16_C(0x0008)
#define BPGA_MODBUS_CAP_GENERATOR_PROFILES UINT16_C(0x0010)
#define BPGA_MODBUS_CAP_LATCHED_ALARMS UINT16_C(0x0020)
#define BPGA_MODBUS_CAP_EDU_CIAA_SIMULATION UINT16_C(0x0040)
#define BPGA_MODBUS_CAP_REMOTE_RESTART UINT16_C(0x0080)

/* ============================================================================
 * STATUS FLAGS
 * ============================================================================ */

#define BPGA_MODBUS_STATUS_INITIALIZED UINT16_C(0x0001)
#define BPGA_MODBUS_STATUS_READY UINT16_C(0x0002)
#define BPGA_MODBUS_STATUS_TEST_ACTIVE UINT16_C(0x0004)
#define BPGA_MODBUS_STATUS_GENERATOR_MOVING UINT16_C(0x0008)
#define BPGA_MODBUS_STATUS_PROFILE_VALID UINT16_C(0x0010)
#define BPGA_MODBUS_STATUS_MEASUREMENTS_VALID UINT16_C(0x0020)
#define BPGA_MODBUS_STATUS_SENSORS_VALID UINT16_C(0x0040)
#define BPGA_MODBUS_STATUS_SIMULATION_ACTIVE UINT16_C(0x0080)
#define BPGA_MODBUS_STATUS_WARNING_ACTIVE UINT16_C(0x0100)
#define BPGA_MODBUS_STATUS_ALARM_ACTIVE UINT16_C(0x0200)
#define BPGA_MODBUS_STATUS_ALARM_LATCHED UINT16_C(0x0400)
#define BPGA_MODBUS_STATUS_EMERGENCY_ACTIVE UINT16_C(0x0800)
#define BPGA_MODBUS_STATUS_STOP_REQUESTED UINT16_C(0x1000)
#define BPGA_MODBUS_STATUS_COMMAND_PENDING UINT16_C(0x2000)
#define BPGA_MODBUS_STATUS_MAINTENANCE_ACTIVE UINT16_C(0x4000)

/* ============================================================================
 * CONTROL FLAGS
 * ============================================================================ */

#define BPGA_MODBUS_CONTROL_GENERAL_ENABLE UINT16_C(0x0001)
#define BPGA_MODBUS_CONTROL_MOTOR_ENABLE UINT16_C(0x0002)
#define BPGA_MODBUS_CONTROL_GENERATOR_ENABLE UINT16_C(0x0004)
#define BPGA_MODBUS_CONTROL_EXCITATION_ENABLE UINT16_C(0x0008)
#define BPGA_MODBUS_CONTROL_LOAD_ENABLE UINT16_C(0x0010)
#define BPGA_MODBUS_CONTROL_SAFETY_OUTPUT_ACTIVE UINT16_C(0x0020)

/* ============================================================================
 * MEASUREMENT FLAGS
 * ============================================================================ */

#define BPGA_MODBUS_MEAS_RPM_VALID UINT16_C(0x0001)
#define BPGA_MODBUS_MEAS_VOLTAGE_VALID UINT16_C(0x0002)
#define BPGA_MODBUS_MEAS_CURRENT_VALID UINT16_C(0x0004)
#define BPGA_MODBUS_MEAS_POWER_VALID UINT16_C(0x0008)
#define BPGA_MODBUS_MEAS_RPM_OUT_OF_RANGE UINT16_C(0x0010)
#define BPGA_MODBUS_MEAS_VOLTAGE_OUT_OF_RANGE UINT16_C(0x0020)
#define BPGA_MODBUS_MEAS_CURRENT_OUT_OF_RANGE UINT16_C(0x0040)
#define BPGA_MODBUS_MEAS_RPM_SATURATED UINT16_C(0x0080)
#define BPGA_MODBUS_MEAS_VOLTAGE_SATURATED UINT16_C(0x0100)
#define BPGA_MODBUS_MEAS_CURRENT_SATURATED UINT16_C(0x0200)
#define BPGA_MODBUS_MEAS_SIMULATED UINT16_C(0x0400)
#define BPGA_MODBUS_MEAS_FILTER_STABLE UINT16_C(0x0800)

/* ============================================================================
 * ALARM MASKS
 * ============================================================================ */

#define BPGA_MODBUS_ALARM_EMERGENCY_STOP UINT16_C(0x0001)
#define BPGA_MODBUS_ALARM_OVERSPEED UINT16_C(0x0002)
#define BPGA_MODBUS_ALARM_OVERVOLTAGE UINT16_C(0x0004)
#define BPGA_MODBUS_ALARM_OVERCURRENT UINT16_C(0x0008)
#define BPGA_MODBUS_ALARM_RPM_SIGNAL_LOST UINT16_C(0x0010)
#define BPGA_MODBUS_ALARM_VOLTAGE_MEASUREMENT UINT16_C(0x0020)
#define BPGA_MODBUS_ALARM_CURRENT_MEASUREMENT UINT16_C(0x0040)
#define BPGA_MODBUS_ALARM_INVALID_PROFILE UINT16_C(0x0080)
#define BPGA_MODBUS_ALARM_STAGE_TIMEOUT UINT16_C(0x0100)
#define BPGA_MODBUS_ALARM_EXCITATION UINT16_C(0x0200)
#define BPGA_MODBUS_ALARM_ACTUATOR UINT16_C(0x0400)
#define BPGA_MODBUS_ALARM_INTERNAL UINT16_C(0x0800)
#define BPGA_MODBUS_ALARM_WATCHDOG UINT16_C(0x1000)
#define BPGA_MODBUS_ALARM_INVALID_CONFIGURATION UINT16_C(0x2000)
#define BPGA_MODBUS_ALARM_SAFETY UINT16_C(0x4000)


/* ============================================================================
 * SYSTEM STATE ENUMERATION
 * ============================================================================ */

typedef enum {
    BPGA_MODBUS_SYSTEM_INITIALIZING = 0,
    BPGA_MODBUS_SYSTEM_SAFE_STOPPED = 1,
    BPGA_MODBUS_SYSTEM_READY = 2,
    BPGA_MODBUS_SYSTEM_STARTING = 3,
    BPGA_MODBUS_SYSTEM_TEST_RUNNING = 4,
    BPGA_MODBUS_SYSTEM_CONTROLLED_STOP = 5,
    BPGA_MODBUS_SYSTEM_TEST_COMPLETED = 6,
    BPGA_MODBUS_SYSTEM_ALARM = 7,
    BPGA_MODBUS_SYSTEM_EMERGENCY = 8,
    BPGA_MODBUS_SYSTEM_MAINTENANCE = 9,
    BPGA_MODBUS_SYSTEM_SIMULATION = 10,
    BPGA_MODBUS_SYSTEM_INVALID = 0xFFFF,
} bpga_modbus_system_state_t;

/* ============================================================================
 * COMMAND CODE ENUMERATION
 * ============================================================================ */

typedef enum {
    BPGA_MODBUS_COMMAND_NONE = 0x0000,
    BPGA_MODBUS_COMMAND_PREPARE_BENCH = 0x0001,
    BPGA_MODBUS_COMMAND_START_TEST = 0x0002,
    BPGA_MODBUS_COMMAND_CONTROLLED_STOP = 0x0003,
    BPGA_MODBUS_COMMAND_CANCEL_PREPARATION = 0x0004,
    BPGA_MODBUS_COMMAND_APPLY_SETPOINTS = 0x0005,
    BPGA_MODBUS_COMMAND_LOAD_PROFILE = 0x0006,
    BPGA_MODBUS_COMMAND_ACK_WARNINGS = 0x0007,
    BPGA_MODBUS_COMMAND_RESET_LATCHED_ALARMS = 0x0008,
    BPGA_MODBUS_COMMAND_ENTER_MAINTENANCE = 0x0009,
    BPGA_MODBUS_COMMAND_EXIT_MAINTENANCE = 0x000A,
    BPGA_MODBUS_COMMAND_ENABLE_SIMULATION = 0x000B,
    BPGA_MODBUS_COMMAND_DISABLE_SIMULATION = 0x000C,
    BPGA_MODBUS_COMMAND_SAVE_CONFIGURATION = 0x000D,
    BPGA_MODBUS_COMMAND_RESTART_CONTROLLER = 0x000E,
} bpga_modbus_command_code_t;

/* ============================================================================
 * COMMAND STATUS ENUMERATION
 * ============================================================================ */

typedef enum {
    BPGA_MODBUS_COMMAND_STATUS_NONE = 0,
    BPGA_MODBUS_COMMAND_STATUS_RECEIVED = 1,
    BPGA_MODBUS_COMMAND_STATUS_EXECUTING = 2,
    BPGA_MODBUS_COMMAND_STATUS_COMPLETED = 3,
    BPGA_MODBUS_COMMAND_STATUS_REJECTED = 4,
    BPGA_MODBUS_COMMAND_STATUS_FAILED = 5,
} bpga_modbus_command_status_t;

/* ============================================================================
 * COMMAND RESULT ENUMERATION
 * ============================================================================ */

typedef enum {
    BPGA_MODBUS_COMMAND_RESULT_OK = 0,
    BPGA_MODBUS_COMMAND_RESULT_UNKNOWN_COMMAND = 1,
    BPGA_MODBUS_COMMAND_RESULT_INCOMPATIBLE_STATE = 2,
    BPGA_MODBUS_COMMAND_RESULT_ARGUMENT_OUT_OF_RANGE = 3,
    BPGA_MODBUS_COMMAND_RESULT_INVALID_PROFILE = 4,
    BPGA_MODBUS_COMMAND_RESULT_ALARM_ACTIVE = 5,
    BPGA_MODBUS_COMMAND_RESULT_EMERGENCY_ACTIVE = 6,
    BPGA_MODBUS_COMMAND_RESULT_NOT_READY = 7,
    BPGA_MODBUS_COMMAND_RESULT_DUPLICATE = 8,
    BPGA_MODBUS_COMMAND_RESULT_TIMEOUT = 9,
    BPGA_MODBUS_COMMAND_RESULT_INTERNAL_ERROR = 10,
} bpga_modbus_command_result_t;

/* ============================================================================
 * LOGICAL STRUCTURES
 *
 * These structures contain decoded data and must NOT be mapped directly
 * onto Modbus buffers. Use encode/decode functions instead.
 * ============================================================================ */

typedef struct {
    uint16_t device_magic;
    uint16_t protocol_version;
    uint16_t register_map_version;
    uint16_t firmware_version_major;
    uint16_t firmware_version_minor;
    uint16_t firmware_version_patch;
    uint16_t device_type;
    uint16_t capabilities;
    uint32_t boot_count;
} bpga_modbus_identification_t;

typedef struct {
    uint16_t sequence;
    bpga_modbus_system_state_t system_state;
    uint16_t system_substate;
    uint16_t status_flags;
    uint16_t control_flags;
    uint16_t active_profile_slot;
    uint16_t active_profile_revision;
    uint16_t last_transition_reason;
    uint32_t uptime_ms;
    uint16_t loop_time_us;
    uint16_t loop_time_max_us;
    uint32_t rpm;
    uint32_t voltage_mv;
    uint32_t current_ma;
    uint32_t power_w;
    uint16_t excitation_permille;
    uint16_t load_permille;
    uint16_t rpm_quality;
    uint16_t voltage_quality;
    uint16_t current_quality;
    uint16_t sample_age_ms;
    uint16_t measurement_flags;
} bpga_modbus_snapshot_t;

typedef struct {
    uint32_t test_elapsed_ms;
    uint32_t rpm_max;
    uint32_t voltage_max_mv;
    uint32_t current_max_ma;
    uint32_t energy_wh;
    uint32_t sample_counter;
} bpga_modbus_totals_t;

typedef struct {
    uint16_t active_warnings;
    uint16_t active_alarms;
    uint16_t latched_alarms;
    uint16_t primary_fault_code;
    uint16_t last_fault_code;
    uint32_t last_fault_uptime_ms;
    uint16_t modbus_valid_requests;
    uint16_t modbus_crc_errors;
    uint16_t modbus_exceptions;
    uint16_t watchdog_reset_count;
} bpga_modbus_diagnostics_t;

typedef struct {
    uint16_t sequence;
    bpga_modbus_command_code_t code;
    uint16_t argument_0;
    uint16_t argument_1;
    uint16_t argument_2;
} bpga_modbus_command_request_t;

typedef struct {
    uint16_t acknowledged_sequence;
    bpga_modbus_command_status_t status;
    bpga_modbus_command_result_t result;
    uint16_t detail;
} bpga_modbus_command_response_t;

/* ============================================================================
 * CONVERSION FUNCTIONS
 * ============================================================================ */

/* Combine two 16-bit words into a 32-bit value (high word first) */
uint32_t bpga_modbus_u32_from_words(
    uint16_t high_word,
    uint16_t low_word
);

/* Split a 32-bit value into two 16-bit words (high word first)
 * If high_word is NULL, high word is not written.
 * If low_word is NULL, low word is not written.
 */
void bpga_modbus_u32_to_words(
    uint32_t value,
    uint16_t *high_word,
    uint16_t *low_word
);

/* Extract major version from encoded version */
uint8_t bpga_modbus_version_major(uint16_t encoded);

/* Extract minor version from encoded version */
uint8_t bpga_modbus_version_minor(uint16_t encoded);

/* ============================================================================
 * VALIDATION FUNCTIONS
 * ============================================================================ */

/* Validate snapshot sequence markers (begin and end must match and be even) */
bool bpga_modbus_snapshot_sequence_is_valid(
    uint16_t sequence_begin,
    uint16_t sequence_end
);

/* Validate profile slot (0-9 are valid; 0xFFFF means no active profile) */
bool bpga_modbus_is_valid_profile_slot(uint16_t slot);

/* Validate Modbus slave address (1-247 are valid) */
bool bpga_modbus_is_valid_slave_address(uint16_t address);

/* Validate permille value (0-1000 are valid) */
bool bpga_modbus_is_valid_permille(uint16_t value);

/* ============================================================================
 * DECODING FUNCTIONS
 *
 * All decoding functions:
 * - validate pointers
 * - validate minimum register count
 * - return bool (true on success, false on failure)
 * - do not modify output on error
 * - decode to temporary variable first, then copy to output
 * - ignore reserved registers
 * - interpret uint32 with high word first
 * ============================================================================ */

/* Decode identification block (requires >= 16 registers) */
bool bpga_modbus_decode_identification(
    const uint16_t *registers,
    size_t register_count,
    bpga_modbus_identification_t *output
);

/* Decode snapshot block (requires >= 32 registers)
 * Validates that snapshot sequence begin and end match and are even
 */
bool bpga_modbus_decode_snapshot(
    const uint16_t *registers,
    size_t register_count,
    bpga_modbus_snapshot_t *output
);

/* Decode totals block (requires >= 16 registers) */
bool bpga_modbus_decode_totals(
    const uint16_t *registers,
    size_t register_count,
    bpga_modbus_totals_t *output
);

/* Decode diagnostics block (requires >= 16 registers) */
bool bpga_modbus_decode_diagnostics(
    const uint16_t *registers,
    size_t register_count,
    bpga_modbus_diagnostics_t *output
);

/* Decode command response block (requires >= 4 registers)
 * Buffer starts at BPGA_MODBUS_COMMAND_RESULT_START
 */
bool bpga_modbus_decode_command_response(
    const uint16_t *registers,
    size_t register_count,
    bpga_modbus_command_response_t *output
);

/* ============================================================================
 * ENCODING FUNCTIONS
 * ============================================================================ */

/* Encode command request block (requires >= 6 registers)
 * Automatically adds BPGA_MODBUS_COMMAND_CONTROL_KEY to register [5]
 * Registers: [0]=sequence, [1]=code, [2..4]=arguments, [5]=control_key
 */
bool bpga_modbus_encode_command_request(
    const bpga_modbus_command_request_t *request,
    uint16_t *registers,
    size_t register_count
);

/* ============================================================================
 * STATIC ASSERTIONS
 * ============================================================================ */

#ifdef __cplusplus
#define BPGA_MODBUS_STATIC_ASSERT(condition, message) \
    static_assert(condition, message)
#else
#define BPGA_MODBUS_STATIC_ASSERT(condition, message) \
    _Static_assert(condition, message)
#endif

BPGA_MODBUS_STATIC_ASSERT(BPGA_MODBUS_INPUT_REGISTER_COUNT == 80,
    "Input register count must be 80");

BPGA_MODBUS_STATIC_ASSERT(BPGA_MODBUS_HOLDING_REGISTER_COUNT == 80,
    "Holding register count must be 80");

BPGA_MODBUS_STATIC_ASSERT(BPGA_MODBUS_SNAPSHOT_COUNT == 32,
    "Snapshot register count must be 32");

BPGA_MODBUS_STATIC_ASSERT(BPGA_MODBUS_COMMAND_REQUEST_COUNT == 6,
    "Command request register count must be 6");

BPGA_MODBUS_STATIC_ASSERT(BPGA_MODBUS_COMMAND_RESULT_COUNT == 4,
    "Command result register count must be 4");

BPGA_MODBUS_STATIC_ASSERT(
    (BPGA_MODBUS_IR_SNAPSHOT_SEQUENCE_END -
     BPGA_MODBUS_IR_SNAPSHOT_SEQUENCE_BEGIN + 1) == BPGA_MODBUS_SNAPSHOT_COUNT,
    "Snapshot sequence markers must span exactly SNAPSHOT_COUNT registers");

BPGA_MODBUS_STATIC_ASSERT(
    (BPGA_MODBUS_HR_COMMAND_CONTROL -
     BPGA_MODBUS_HR_COMMAND_SEQUENCE + 1) == BPGA_MODBUS_COMMAND_REQUEST_COUNT,
    "Command request markers must span exactly COMMAND_REQUEST_COUNT registers");

BPGA_MODBUS_STATIC_ASSERT(
    (BPGA_MODBUS_IR_SNAPSHOT_SEQUENCE_END - BPGA_MODBUS_SNAPSHOT_START) <
        BPGA_MODBUS_SNAPSHOT_COUNT,
    "Snapshot last field must be inside the snapshot block");

BPGA_MODBUS_STATIC_ASSERT(
    (BPGA_MODBUS_IR_SAMPLE_COUNTER_LO - BPGA_MODBUS_TOTALS_START) <
        BPGA_MODBUS_TOTALS_COUNT,
    "Totals last field must be inside the totals block");

BPGA_MODBUS_STATIC_ASSERT(
    (BPGA_MODBUS_IR_WATCHDOG_RESET_COUNT - BPGA_MODBUS_DIAGNOSTICS_START) <
        BPGA_MODBUS_DIAGNOSTICS_COUNT,
    "Diagnostics last field must be inside the diagnostics block");

BPGA_MODBUS_STATIC_ASSERT(
    (BPGA_MODBUS_HR_COMMAND_DETAIL - BPGA_MODBUS_COMMAND_RESULT_START) <
        BPGA_MODBUS_COMMAND_RESULT_COUNT,
    "Command detail must be inside the command result block");

BPGA_MODBUS_STATIC_ASSERT(BPGA_MODBUS_IR_BOOT_COUNT_LO == BPGA_MODBUS_IR_BOOT_COUNT_HI + 1,
    "Boot count LO must be HI + 1");

BPGA_MODBUS_STATIC_ASSERT(BPGA_MODBUS_IR_UPTIME_LO == BPGA_MODBUS_IR_UPTIME_HI + 1,
    "Uptime LO must be HI + 1");

BPGA_MODBUS_STATIC_ASSERT(BPGA_MODBUS_IR_LOOP_TIME_MAX_US == BPGA_MODBUS_IR_LOOP_TIME_US + 1,
    "Loop time registers must be consecutive");

BPGA_MODBUS_STATIC_ASSERT(BPGA_MODBUS_IR_RPM_LO == BPGA_MODBUS_IR_RPM_HI + 1,
    "RPM LO must be HI + 1");

BPGA_MODBUS_STATIC_ASSERT(BPGA_MODBUS_IR_VOLTAGE_MV_LO == BPGA_MODBUS_IR_VOLTAGE_MV_HI + 1,
    "Voltage MV LO must be HI + 1");

BPGA_MODBUS_STATIC_ASSERT(BPGA_MODBUS_IR_CURRENT_MA_LO == BPGA_MODBUS_IR_CURRENT_MA_HI + 1,
    "Current MA LO must be HI + 1");

BPGA_MODBUS_STATIC_ASSERT(BPGA_MODBUS_IR_POWER_W_LO == BPGA_MODBUS_IR_POWER_W_HI + 1,
    "Power W LO must be HI + 1");

BPGA_MODBUS_STATIC_ASSERT(BPGA_MODBUS_IR_TEST_ELAPSED_MS_LO == BPGA_MODBUS_IR_TEST_ELAPSED_MS_HI + 1,
    "Test elapsed MS LO must be HI + 1");

BPGA_MODBUS_STATIC_ASSERT(BPGA_MODBUS_IR_RPM_MAX_LO == BPGA_MODBUS_IR_RPM_MAX_HI + 1,
    "RPM max LO must be HI + 1");

BPGA_MODBUS_STATIC_ASSERT(BPGA_MODBUS_IR_VOLTAGE_MAX_MV_LO == BPGA_MODBUS_IR_VOLTAGE_MAX_MV_HI + 1,
    "Voltage max MV LO must be HI + 1");

BPGA_MODBUS_STATIC_ASSERT(BPGA_MODBUS_IR_CURRENT_MAX_MA_LO == BPGA_MODBUS_IR_CURRENT_MAX_MA_HI + 1,
    "Current max MA LO must be HI + 1");

BPGA_MODBUS_STATIC_ASSERT(BPGA_MODBUS_IR_ENERGY_WH_LO == BPGA_MODBUS_IR_ENERGY_WH_HI + 1,
    "Energy WH LO must be HI + 1");

BPGA_MODBUS_STATIC_ASSERT(BPGA_MODBUS_IR_SAMPLE_COUNTER_LO == BPGA_MODBUS_IR_SAMPLE_COUNTER_HI + 1,
    "Sample counter LO must be HI + 1");

BPGA_MODBUS_STATIC_ASSERT(BPGA_MODBUS_IR_LAST_FAULT_UPTIME_MS_LO == BPGA_MODBUS_IR_LAST_FAULT_UPTIME_MS_HI + 1,
    "Last fault uptime MS LO must be HI + 1");

BPGA_MODBUS_STATIC_ASSERT(BPGA_MODBUS_HR_RPM_SETPOINT_LO == BPGA_MODBUS_HR_RPM_SETPOINT_HI + 1,
    "RPM setpoint LO must be HI + 1");

BPGA_MODBUS_STATIC_ASSERT(BPGA_MODBUS_HR_VOLTAGE_SETPOINT_MV_LO == BPGA_MODBUS_HR_VOLTAGE_SETPOINT_MV_HI + 1,
    "Voltage setpoint MV LO must be HI + 1");

BPGA_MODBUS_STATIC_ASSERT(BPGA_MODBUS_HR_CURRENT_LIMIT_MA_LO == BPGA_MODBUS_HR_CURRENT_LIMIT_MA_HI + 1,
    "Current limit MA LO must be HI + 1");

BPGA_MODBUS_STATIC_ASSERT(BPGA_MODBUS_HR_TEST_DURATION_MS_LO == BPGA_MODBUS_HR_TEST_DURATION_MS_HI + 1,
    "Test duration MS LO must be HI + 1");

BPGA_MODBUS_STATIC_ASSERT(BPGA_MODBUS_IR_RESERVED_DIAGNOSTICS_END == BPGA_MODBUS_INPUT_REGISTER_LAST,
    "Input register diagnostics end must equal input register last");

BPGA_MODBUS_STATIC_ASSERT(BPGA_MODBUS_HR_RESERVED_COMMAND_RESULT_END == BPGA_MODBUS_HOLDING_REGISTER_LAST,
    "Holding register command result end must equal holding register last");

#undef BPGA_MODBUS_STATIC_ASSERT

#ifdef __cplusplus
}
#endif

#endif /* BPGA_MODBUS_MAP_H */
