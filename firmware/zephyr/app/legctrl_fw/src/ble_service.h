/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * BLE Service - LegCtrl Protocol
 * Service UUID: 12345678-1234-1234-1234-123456789abc
 * Command Characteristic: 12345678-1234-1234-1234-123456789abd (Write)
 * Telemetry Characteristic: 12345678-1234-1234-1234-123456789abe (Notify)
 */

#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include <zephyr/types.h>

/* Protocol message types */
#define MSG_TYPE_CMD_ARM           0x01
#define MSG_TYPE_CMD_DISARM        0x02
#define MSG_TYPE_CMD_SET_SERVO_CH0 0x03
#define MSG_TYPE_CMD_PING          0x04
#define MSG_TYPE_TELEMETRY         0x10

/* System states */
#define STATE_DISARMED 0x00
#define STATE_ARMED    0x01
#define STATE_FAULT    0x02

/* Error codes */
#define ERR_NONE              0x00
#define ERR_DEADMAN_TIMEOUT   0x01
#define ERR_LOW_BATTERY       0x02
#define ERR_I2C_FAULT         0x03
#define ERR_INVALID_CMD       0x04
#define ERR_UNKNOWN           0xFF

/* Protocol header */
struct protocol_header {
	uint8_t version;
	uint8_t msg_type;
	uint8_t payload_len;
	uint8_t reserved;
} __packed;

/* Command payloads */
struct cmd_set_servo_ch0 {
	uint16_t pulse_us;
} __packed;

/* Telemetry payload */
struct telemetry_payload {
	uint8_t state;
	uint8_t error_code;
	uint16_t last_cmd_age_ms;
	uint16_t battery_mv;
	uint16_t reserved;
} __packed;

/* Full telemetry message */
struct telemetry_msg {
	struct protocol_header hdr;
	struct telemetry_payload payload;
} __packed;

/**
 * @brief Initialize BLE service
 * @return 0 on success, negative errno on failure
 */
int ble_service_init(void);

/**
 * @brief Send telemetry notification
 * @return 0 on success, negative errno on failure
 */
int ble_service_send_telemetry(void);

/**
 * @brief Get current system state
 * @return Current state (STATE_DISARMED, STATE_ARMED, STATE_FAULT)
 */
uint8_t ble_service_get_state(void);

/**
 * @brief Get current error code
 * @return Current error code
 */
uint8_t ble_service_get_error_code(void);

/**
 * @brief Get time since last command (ms)
 * @return Time in milliseconds
 */
uint16_t ble_service_get_last_cmd_age_ms(void);

/**
 * @brief Update deadman timer (called periodically)
 */
void ble_service_update_deadman(void);

#endif /* BLE_SERVICE_H */
