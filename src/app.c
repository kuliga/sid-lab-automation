/*
 * Copyright (c) 2024, Jan Kuliga
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include "platform.h"

LOG_MODULE_REGISTER(sid, LOG_LEVEL_DBG);

enum thrmcpl_id {
	THRMCPL0,
	THRMCPL1,
	THRMCPL2,
};

enum emctrl_id {
	VALVE0	= 0,
	VALVE1	= 1,
	VALVE2	= 2,
	HEATER0	= 7,
};

static volatile unsigned displayed_info_flag = 0;
static struct sensor_value thrmcpl_vals[2];

int main(void)
{
	int ret = -1;
	uint16_t adc_buf;
	const struct adc_sequence adc_seq = {
		.buffer		= &adc_buf,
		.buffer_size	= sizeof(adc_buf),
		.channels	= BIT(adc_chan2_cfg.channel_id),
		.resolution	= DT_PROP(DT_NODELABEL(pressure_sensor0), zephyr_resolution),
		.calibrate	= true,
	};
	uint8_t lcdbuf[64];

	ret = platform_init();
	if (ret) {
		LOG_ERR("platform_init(): failed with %d\n", ret);
		return ret;
	}

	snprintk(lcdbuf, sizeof(lcdbuf), "Hello world from %s", CONFIG_BOARD);
	(void) auxdisplay_write(lcd_dev, lcdbuf, strlen(lcdbuf));
	k_sleep(K_MSEC(1000));

	k_sleep(K_MSEC(2000));
	LOG_WRN("START\n");

	// 1= zamyka
	// 0= otwarty
	// NORMALNIE OTWARTY
	ret = gpio_pin_set(emctrl_gpio_dev, VALVE0, 1);
	k_sleep(K_MSEC(200));
	ret = gpio_pin_set(emctrl_gpio_dev, VALVE1, 0);
	k_sleep(K_MSEC(200));

once_again:

	//while (1) {
	for (int i = 0; i < 60; ++i) {
		int32_t val_mv = 0;

		for (int i = 0; i < 2; ++i) {
			int ret;
			struct sensor_value val;
			ret = sensor_sample_fetch_chan(thrmcpl_devs[i], SENSOR_CHAN_AMBIENT_TEMP);
			if (ret < 0) {
				LOG_WRN("thermocouple%d: Could not fetch temperature (%d)\n", i, ret);
			}
	
			ret = sensor_channel_get(thrmcpl_devs[i], SENSOR_CHAN_AMBIENT_TEMP, &val);
			if (ret < 0) {
				LOG_WRN("thermocouple%d: Could not get temperature (%d)\n", i, ret);
			}
			thrmcpl_vals[i] = val;
	
			printf("Temperature%d: %.2f C\n", i, sensor_value_to_double(&val));
		}

		ret = adc_get_mv_reading(adc_dev, &adc_seq, &adc_chan2_cfg, &val_mv);
		if (ret) {
			LOG_ERR(" (value in mV not available)");
		} else {
			LOG_INF("adc reading = %"PRId32" mV", val_mv);
		}
		printf("mp3v5050v: %.2f +- %.2f kPa\n", mp3v5050v_get_pressure(adc_dev, val_mv),
			mp3v5050v_get_pressure_error());

		if (displayed_info_flag == 2) {
			snprintk(lcdbuf, sizeof(lcdbuf), "pressure:%.2f",
				 mp3v5050v_get_pressure(adc_dev, val_mv));
		} else {
			snprintk(lcdbuf, sizeof(lcdbuf), "thrmcpl%d: %.2f", displayed_info_flag,
				 sensor_value_to_double(&thrmcpl_vals[displayed_info_flag]));
		}
		k_sleep(K_MSEC(2000));
	//	(void) auxdisplay_clear(lcd_dev);
	//	(void) auxdisplay_cursor_position_set(lcd_dev, AUXDISPLAY_POSITION_ABSOLUTE, 0, 0);
	//	(void) auxdisplay_write(lcd_dev, lcdbuf, strlen(lcdbuf));


		//ret = gpio_pin_toggle(emctrl_gpio_dev, VALVE0);
		//k_sleep(K_MSEC(3000));
		//ret = gpio_pin_toggle(emctrl_gpio_dev, VALVE1);
		//k_sleep(K_MSEC(300));
	//	ret = gpio_pin_set(emctrl_gpio_dev, VALVE0, 0);
	//	k_sleep(K_MSEC(2000));
	//`	ret = gpio_pin_toggle(emctrl_gpio_dev, VALVE1);
	//`	k_sleep(K_MSEC(3000));
	}
	ret = gpio_pin_set(emctrl_gpio_dev, VALVE0, 0);
	k_sleep(K_MSEC(200));
	LOG_WRN("finished cycle\n");
	goto once_again;
	LOG_WRN("FINISH\n");
	return 0;
}

void button_callback(struct input_event *evt)
{
	if (evt->type != INPUT_EV_KEY || evt->code != INPUT_KEY_0) {
		LOG_ERR("spurious input event");
		return;
	}

	if (evt->value == 0) {
		displayed_info_flag  = (displayed_info_flag + 1) % 3u;
		LOG_INF("button pressed: %u", displayed_info_flag);
	} else {
		LOG_INF("button released");
	}
}
