/*
 * Copyright (c) 2021 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

/**
 * @file Sample app using the MAX6675 cold-junction-compensated K-thermocouple
 *	 to digital converter.
 *
 * This app will read and display the sensor temperature every second.
 */

int main(void)
{
	//const struct device *const dev = DEVICE_DT_GET_ONE(maxim_max6675);
	const struct device *const devs[] = {DEVICE_DT_GET(DT_NODELABEL(thermocouple0)), DEVICE_DT_GET(DT_NODELABEL(thermocouple1))};

	for (int i = 0; i < 2; ++i) {
		if (!device_is_ready(devs[i])) {
			printk("sensor %d: device not ready.\n", i);
			return 0;
		}
	}

	while (1) {
		for (int i = 0; i < 2; ++i) {
			int ret;
			struct sensor_value val;
			ret = sensor_sample_fetch_chan(devs[i], SENSOR_CHAN_AMBIENT_TEMP);
			if (ret < 0) {
				printf("Could not fetch temperature (%d)\n", ret);
				return 0;
			}
	
			ret = sensor_channel_get(devs[i], SENSOR_CHAN_AMBIENT_TEMP, &val);
			if (ret < 0) {
				printf("Could not get temperature (%d)\n", ret);
				return 0;
			}
	
			printf("Temperature%d: %.2f C\n", i, sensor_value_to_double(&val));
		}

		k_sleep(K_MSEC(1000));
	}
	return 0;
}
