/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * LegCtrl Firmware - Main Entry Point
 * XIAO nRF52840 - Zephyr RTOS
 */

#include <zephyr/kernel.h>
#include <version.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/device.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* PCA9685 PWM device */
#define PWM_DEV_NODE DT_NODELABEL(pca9685)

/* Servo PWM parameters
 * Standard servo PWM: 50Hz (20ms period)
 * Pulse width range: 1000-2000μs (typ. for 180° servos)
 * Safe initial value: 1500μs (center position)
 */
#define SERVO_PWM_PERIOD_NS    (20000000U)  /* 20ms = 20,000,000ns */
#define SERVO_PULSE_MIN_NS     (1000000U)   /* 1000μs = 1,000,000ns */
#define SERVO_PULSE_MAX_NS     (2000000U)   /* 2000μs = 2,000,000ns */
#define SERVO_PULSE_CENTER_NS  (1500000U)   /* 1500μs = 1,500,000ns (safe initial) */

/* Servo channel 0 */
#define SERVO_CHANNEL_0        0

int main(void)
{
	const struct device *pwm_dev;
	int ret;
	uint32_t pwm_freq_hz = 50; /* 50Hz for servos */

	LOG_INF("===========================================");
	LOG_INF("LegCtrl Firmware - XIAO nRF52840");
	LOG_INF("===========================================");
	LOG_INF("Zephyr version: %s", KERNEL_VERSION_STRING);
	LOG_INF("Board: %s", CONFIG_BOARD);

	/* Initialize PCA9685 PWM controller */
	pwm_dev = DEVICE_DT_GET(PWM_DEV_NODE);
	if (!device_is_ready(pwm_dev)) {
		LOG_ERR("PCA9685 PWM device is not ready");
		return -1;
	}
	LOG_INF("PCA9685 PWM device ready");

	/* Set servo channel 0 to safe center position (1500μs)
	 * This prevents sudden movement on startup
	 * 
	 * pwm_set parameters:
	 * - device: PWM device
	 * - channel: PWM channel (0-15 for PCA9685)
	 * - period: PWM period in nanoseconds (20ms = 20,000,000ns for 50Hz)
	 * - pulse: Pulse width in nanoseconds (1500μs = 1,500,000ns)
	 * - flags: PWM polarity flags (0 for normal polarity)
	 */
	ret = pwm_set(pwm_dev, SERVO_CHANNEL_0, 
		      SERVO_PWM_PERIOD_NS, SERVO_PULSE_CENTER_NS, 
		      0);

	if (ret < 0) {
		LOG_ERR("Failed to set PWM for servo channel 0: %d", ret);
		return -1;
	}

	LOG_INF("Servo channel 0 initialized:");
	LOG_INF("  - PWM frequency: 50Hz (period: 20ms)");
	LOG_INF("  - Pulse width: 1500us (center position)");
	LOG_INF("  - Safe range: 1000-2000us");
	LOG_INF("System started successfully");
	LOG_INF("===========================================");

	/* Main loop */
	while (1) {
		k_sleep(K_SECONDS(5));
		LOG_INF("Heartbeat: Servo active at center position");
	}

	return 0;
}
