/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * BLE Service Implementation - LegCtrl Protocol
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/device.h>

#include "ble_service.h"

LOG_MODULE_REGISTER(ble_service, LOG_LEVEL_INF);

/* PCA9685 PWM device */
#define PWM_DEV_NODE DT_NODELABEL(pca9685)

/* Servo PWM parameters */
#define SERVO_PWM_PERIOD_NS    (20000000U)  /* 20ms = 20,000,000ns */
#define SERVO_PULSE_MIN_NS     (500000U)    /* 500μs */
#define SERVO_PULSE_MAX_NS     (2500000U)   /* 2500μs */
#define SERVO_PULSE_CENTER_NS  (1500000U)   /* 1500μs */

/* Servo channel */
#define SERVO_CHANNEL_0        0

/* Deadman timeout (ms) */
#define DEADMAN_TIMEOUT_MS     200

/* Service UUID: 12345678-1234-1234-1234-123456789abc */
#define BT_UUID_LEGCTRL_SERVICE \
	BT_UUID_DECLARE_128( \
		0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12, 0x34, 0x12, \
		0x34, 0x12, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12)

/* Command Characteristic UUID: 12345678-1234-1234-1234-123456789abd */
#define BT_UUID_LEGCTRL_CMD \
	BT_UUID_DECLARE_128( \
		0xbd, 0x9a, 0x78, 0x56, 0x34, 0x12, 0x34, 0x12, \
		0x34, 0x12, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12)

/* Telemetry Characteristic UUID: 12345678-1234-1234-1234-123456789abe */
#define BT_UUID_LEGCTRL_TELEMETRY \
	BT_UUID_DECLARE_128( \
		0xbe, 0x9a, 0x78, 0x56, 0x34, 0x12, 0x34, 0x12, \
		0x34, 0x12, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12)

/* System state */
static struct {
	uint8_t state;
	uint8_t error_code;
	int64_t last_cmd_timestamp;
	bool telemetry_enabled;
	struct bt_conn *conn;
} sys_state = {
	.state = STATE_DISARMED,
	.error_code = ERR_NONE,
	.last_cmd_timestamp = 0,
	.telemetry_enabled = false,
	.conn = NULL,
};

/* Workqueue for command processing */
static struct k_work_q cmd_work_q;
static K_THREAD_STACK_DEFINE(cmd_work_q_stack, 1024);

/* Command work item */
struct cmd_work_item {
	struct k_work work;
	uint8_t msg_type;
	uint8_t data[8];  /* Max payload size */
	uint8_t data_len;
};

static struct cmd_work_item cmd_work;

/* PWM device */
static const struct device *pwm_dev;

/* Forward declarations */
static void cmd_work_handler(struct k_work *work);
static void process_cmd_arm(void);
static void process_cmd_disarm(void);
static void process_cmd_set_servo_ch0(uint16_t pulse_us);
static void process_cmd_ping(void);

/* Connection callbacks */
static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("Connection failed (err %u)", err);
		return;
	}

	LOG_INF("BLE Connected");
	sys_state.conn = bt_conn_ref(conn);
	
	/* Reset to DISARMED on new connection */
	sys_state.state = STATE_DISARMED;
	sys_state.error_code = ERR_NONE;
	sys_state.last_cmd_timestamp = 0;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("BLE Disconnected (reason %u)", reason);
	
	if (sys_state.conn) {
		bt_conn_unref(sys_state.conn);
		sys_state.conn = NULL;
	}
	
	/* Go to FAULT state on disconnect */
	sys_state.state = STATE_FAULT;
	sys_state.error_code = ERR_NONE;
	sys_state.telemetry_enabled = false;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

/* Command characteristic write callback */
static ssize_t cmd_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	const uint8_t *data = buf;
	
	LOG_HEXDUMP_INF(data, len, "CMD received:");
	
	/* Validate minimum header size */
	if (len < sizeof(struct protocol_header)) {
		LOG_ERR("Command too short: %u bytes", len);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}
	
	struct protocol_header *hdr = (struct protocol_header *)data;
	
	/* Validate version */
	if (hdr->version != 0x01) {
		LOG_ERR("Invalid version: 0x%02x", hdr->version);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}
	
	/* Validate payload length */
	if (len != sizeof(struct protocol_header) + hdr->payload_len) {
		LOG_ERR("Payload length mismatch: expected %u, got %u",
			sizeof(struct protocol_header) + hdr->payload_len, len);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}
	
	/* Validate payload fits in work item buffer */
	if (hdr->payload_len > sizeof(cmd_work.data)) {
		LOG_ERR("Payload too large: %u bytes (max %u)", 
			hdr->payload_len, (uint8_t)sizeof(cmd_work.data));
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}
	
	/* Queue command for processing in workqueue */
	cmd_work.msg_type = hdr->msg_type;
	cmd_work.data_len = hdr->payload_len;
	
	if (hdr->payload_len > 0) {
		memcpy(cmd_work.data, data + sizeof(struct protocol_header), 
		       hdr->payload_len);
	}
	
	k_work_submit_to_queue(&cmd_work_q, &cmd_work.work);
	
	return len;
}

/* Telemetry CCC changed callback */
static void telemetry_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	sys_state.telemetry_enabled = (value == BT_GATT_CCC_NOTIFY);
	LOG_INF("Telemetry notifications %s", 
		sys_state.telemetry_enabled ? "enabled" : "disabled");
}

/* GATT Service definition */
BT_GATT_SERVICE_DEFINE(legctrl_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_LEGCTRL_SERVICE),
	
	/* Command Characteristic */
	BT_GATT_CHARACTERISTIC(BT_UUID_LEGCTRL_CMD,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_WRITE,
			       NULL, cmd_write, NULL),
	
	/* Telemetry Characteristic */
	BT_GATT_CHARACTERISTIC(BT_UUID_LEGCTRL_TELEMETRY,
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE,
			       NULL, NULL, NULL),
	BT_GATT_CCC(telemetry_ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

/* Index of telemetry characteristic value attribute in legctrl_svc.attrs[] */
#define TELEMETRY_ATTR_IDX 4

/* Command work handler - runs in workqueue context */
static void cmd_work_handler(struct k_work *work)
{
	struct cmd_work_item *item = CONTAINER_OF(work, struct cmd_work_item, work);
	
	LOG_INF("Processing command: msg_type=0x%02x", item->msg_type);
	
	switch (item->msg_type) {
	case MSG_TYPE_CMD_ARM:
		process_cmd_arm();
		break;
		
	case MSG_TYPE_CMD_DISARM:
		process_cmd_disarm();
		break;
		
	case MSG_TYPE_CMD_SET_SERVO_CH0:
		if (item->data_len == sizeof(struct cmd_set_servo_ch0)) {
			/* Extract pulse_us with explicit little-endian handling */
			uint16_t pulse_us = (uint16_t)item->data[0] | 
					    ((uint16_t)item->data[1] << 8);
			process_cmd_set_servo_ch0(pulse_us);
		} else {
			LOG_ERR("Invalid SET_SERVO_CH0 payload length: %u", item->data_len);
		}
		break;
		
	case MSG_TYPE_CMD_PING:
		process_cmd_ping();
		break;
		
	default:
		LOG_ERR("Unknown command: 0x%02x", item->msg_type);
		sys_state.error_code = ERR_INVALID_CMD;
		break;
	}
}

/* Process ARM command */
static void process_cmd_arm(void)
{
	LOG_INF("CMD_ARM");
	
	if (sys_state.state == STATE_DISARMED || sys_state.state == STATE_FAULT) {
		sys_state.state = STATE_ARMED;
		sys_state.error_code = ERR_NONE;
		sys_state.last_cmd_timestamp = k_uptime_get();
		LOG_INF("State: ARMED");
	}
}

/* Process DISARM command */
static void process_cmd_disarm(void)
{
	LOG_INF("CMD_DISARM");
	
	sys_state.state = STATE_DISARMED;
	sys_state.error_code = ERR_NONE;
	sys_state.last_cmd_timestamp = 0;
	
	/* Stop PWM output */
	if (pwm_dev && device_is_ready(pwm_dev)) {
		pwm_set(pwm_dev, SERVO_CHANNEL_0, SERVO_PWM_PERIOD_NS, 0, 0);
		LOG_INF("Servo output stopped");
	}
	
	LOG_INF("State: DISARMED");
}

/* Process SET_SERVO_CH0 command */
static void process_cmd_set_servo_ch0(uint16_t pulse_us)
{
	LOG_INF("CMD_SET_SERVO_CH0: pulse_us=%u", pulse_us);
	
	/* Only process in ARMED state */
	if (sys_state.state != STATE_ARMED) {
		LOG_WRN("Command ignored - not in ARMED state");
		return;
	}
	
	/* Update last command timestamp */
	sys_state.last_cmd_timestamp = k_uptime_get();
	
	/* Clamp pulse width to safe range */
	if (pulse_us < 500) {
		LOG_WRN("Pulse width %u us clamped to 500 us", pulse_us);
		pulse_us = 500;
	} else if (pulse_us > 2500) {
		LOG_WRN("Pulse width %u us clamped to 2500 us", pulse_us);
		pulse_us = 2500;
	}
	
	/* Set PWM */
	if (pwm_dev && device_is_ready(pwm_dev)) {
		uint32_t pulse_ns = pulse_us * 1000U;
		int ret = pwm_set(pwm_dev, SERVO_CHANNEL_0, 
				  SERVO_PWM_PERIOD_NS, pulse_ns, 0);
		
		if (ret < 0) {
			LOG_ERR("Failed to set PWM: %d", ret);
			sys_state.error_code = ERR_I2C_FAULT;
		} else {
			LOG_INF("Servo set to %u us", pulse_us);
		}
	}
}

/* Process PING command */
static void process_cmd_ping(void)
{
	LOG_DBG("CMD_PING");
	
	/* Only update timestamp in ARMED state */
	if (sys_state.state == STATE_ARMED) {
		sys_state.last_cmd_timestamp = k_uptime_get();
	}
}

/* Send telemetry notification */
int ble_service_send_telemetry(void)
{
	if (!sys_state.telemetry_enabled || !sys_state.conn) {
		return -ENOTCONN;
	}
	
	struct telemetry_msg msg;
	
	/* Fill header */
	msg.hdr.version = 0x01;
	msg.hdr.msg_type = MSG_TYPE_TELEMETRY;
	msg.hdr.payload_len = sizeof(struct telemetry_payload);
	msg.hdr.reserved = 0x00;
	
	/* Fill payload */
	msg.payload.state = sys_state.state;
	msg.payload.error_code = sys_state.error_code;
	msg.payload.last_cmd_age_ms = ble_service_get_last_cmd_age_ms();
	/* TODO: Replace stub battery voltage with actual ADC reading */
	msg.payload.battery_mv = 7400;  /* Stub value: 7.4V */
	msg.payload.reserved = 0;
	
	/* Send notification */
	int ret = bt_gatt_notify(sys_state.conn, &legctrl_svc.attrs[TELEMETRY_ATTR_IDX], 
				 &msg, sizeof(msg));
	
	if (ret < 0) {
		LOG_ERR("Failed to send telemetry: %d", ret);
		return ret;
	}
	
	LOG_DBG("Telemetry sent: state=%u, error=%u, age=%u ms",
		msg.payload.state, msg.payload.error_code, msg.payload.last_cmd_age_ms);
	
	return 0;
}

/* Get current system state */
uint8_t ble_service_get_state(void)
{
	return sys_state.state;
}

/* Get current error code */
uint8_t ble_service_get_error_code(void)
{
	return sys_state.error_code;
}

/* Get time since last command */
uint16_t ble_service_get_last_cmd_age_ms(void)
{
	if (sys_state.last_cmd_timestamp == 0) {
		return 0;
	}
	
	int64_t age = k_uptime_get() - sys_state.last_cmd_timestamp;
	
	/* Clamp to uint16_t max */
	if (age > 65535) {
		return 65535;
	}
	
	return (uint16_t)age;
}

/* Update deadman timer */
void ble_service_update_deadman(void)
{
	/* Only check deadman in ARMED state */
	if (sys_state.state != STATE_ARMED) {
		return;
	}
	
	uint16_t age_ms = ble_service_get_last_cmd_age_ms();
	
	if (age_ms >= DEADMAN_TIMEOUT_MS) {
		LOG_WRN("Deadman timeout: %u ms >= %u ms", age_ms, DEADMAN_TIMEOUT_MS);
		sys_state.state = STATE_FAULT;
		sys_state.error_code = ERR_DEADMAN_TIMEOUT;
		
		/* Stop PWM output */
		if (pwm_dev && device_is_ready(pwm_dev)) {
			pwm_set(pwm_dev, SERVO_CHANNEL_0, SERVO_PWM_PERIOD_NS, 0, 0);
			LOG_INF("Servo output stopped due to deadman timeout");
		}
	}
}

/* Advertising data */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
		      0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12, 0x34, 0x12,
		      0x34, 0x12, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12),
};

/* Initialize BLE service */
int ble_service_init(void)
{
	int ret;
	
	/* Get PWM device */
	pwm_dev = DEVICE_DT_GET(PWM_DEV_NODE);
	if (!device_is_ready(pwm_dev)) {
		LOG_ERR("PCA9685 PWM device is not ready");
		return -ENODEV;
	}
	LOG_INF("PWM device ready");
	
	/* Initialize workqueue */
	k_work_queue_init(&cmd_work_q);
	k_work_queue_start(&cmd_work_q, cmd_work_q_stack,
			   K_THREAD_STACK_SIZEOF(cmd_work_q_stack),
			   K_PRIO_PREEMPT(5), NULL);
	
	/* Initialize command work item */
	k_work_init(&cmd_work.work, cmd_work_handler);
	
	/* Enable Bluetooth */
	ret = bt_enable(NULL);
	if (ret) {
		LOG_ERR("Bluetooth init failed (err %d)", ret);
		return ret;
	}
	LOG_INF("Bluetooth initialized");
	
	/* Start advertising */
	ret = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (ret) {
		LOG_ERR("Advertising failed to start (err %d)", ret);
		return ret;
	}
	LOG_INF("BLE advertising started");
	LOG_INF("Device name: %s", CONFIG_BT_DEVICE_NAME);
	
	return 0;
}
