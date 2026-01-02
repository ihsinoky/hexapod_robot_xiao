/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * LegCtrl Firmware - Main Entry Point
 * XIAO nRF52840 - Zephyr RTOS
 */

#include <zephyr/kernel.h>
#include <zephyr/version.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
	LOG_INF("===========================================");
	LOG_INF("LegCtrl Firmware - XIAO nRF52840");
	LOG_INF("===========================================");
	LOG_INF("Zephyr version: %s", KERNEL_VERSION_STRING);
	LOG_INF("Board: %s", CONFIG_BOARD);
	LOG_INF("System started successfully");
	LOG_INF("===========================================");

	/* Main loop */
	while (1) {
		k_sleep(K_SECONDS(5));
		LOG_INF("Heartbeat: System running");
	}

	return 0;
}
