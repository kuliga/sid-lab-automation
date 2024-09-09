/*
 * Copyright (c) 2024, Jan Kuliga
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <stdio.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/logging/log.h>
#include <zephyr/drivers/auxdisplay.h>

#include <zephyr/input/input.h>

#include <zephyr/drivers/gpio.h>


LOG_MODULE_REGISTER(sid, LOG_LEVEL_DBG);

static const struct device *const thrmcpl_devs[] = {DEVICE_DT_GET(DT_NODELABEL(thermocouple0)),
						    DEVICE_DT_GET(DT_NODELABEL(thermocouple1))};
static const struct device *const adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc1));
static const struct adc_channel_cfg adc_chan2_cfg =
				ADC_CHANNEL_CFG_DT(DT_NODELABEL(pressure_sensor0));

static const struct device *const lcd_dev = DEVICE_DT_GET(DT_NODELABEL(lcd0));
static const struct device *const button_dev = DEVICE_DT_GET(DT_PARENT(DT_NODELABEL(user_button)));

static int thermocouples_init(const struct device *const *devs, int ndevs);
static int adc_init(const struct device *const dev, const struct adc_channel_cfg *const chan_cfg);
static int lcd_init(const struct device *const dev);
static int adc_get_mv_reading(const struct device *const dev, const struct adc_sequence *const seq,
			      const struct adc_channel_cfg *const cfg, int32_t *val_mv);
static void button_callback(struct input_event *evt);

static inline double mp3v5050v_get_pressure(const struct device *const adc_dev, int32_t val_mv)
{
	return 56 * ((float)val_mv / adc_ref_internal(adc_dev)) - 52;
}

static inline double mp3v5050v_get_pressure_error(void)
{
	return 1.25f * 1; // TODO: read the temperature to use proper temperature multiplier value
}

INPUT_CALLBACK_DEFINE(button_dev, button_callback);
static volatile unsigned displayed_info_flag = 0;

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

	ret = thermocouples_init(thrmcpl_devs, ARRAY_SIZE(thrmcpl_devs));
	if (ret) {
		LOG_ERR("sid: exit %d", ret);
		return 1;
	}

	ret = adc_init(adc_dev, &adc_chan2_cfg);
	if (ret) {
		LOG_ERR("sid: exit %d", ret);
		return 1;
	}

	ret = lcd_init(lcd_dev);
	if (ret) {
		LOG_ERR("sid: exit %d", ret);
		return 1;
	}

	while (1) {
		int32_t val_mv = 0;

		for (int i = 0; i < 2; ++i) {
			int ret;
			struct sensor_value val;
			ret = sensor_sample_fetch_chan(thrmcpl_devs[i], SENSOR_CHAN_AMBIENT_TEMP);
			if (ret < 0) {
				printf("Could not fetch temperature (%d)\n", ret);
				return 0;
			}
	
			ret = sensor_channel_get(thrmcpl_devs[i], SENSOR_CHAN_AMBIENT_TEMP, &val);
			if (ret < 0) {
				printf("Could not get temperature (%d)\n", ret);
				return 0;
			}
	
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

		k_sleep(K_MSEC(1000));
	}
	return 0;
}

static int thermocouples_init(const struct device *const *devs, int ndevs)
{
	for (int i = 0; i < ndevs; ++i) {
		if (!device_is_ready(devs[i])) {
			LOG_ERR("thermocouple %d: device is not ready", i);
			return -1;
		}
	}

	return 0;
}

static int adc_init(const struct device *const dev, const struct adc_channel_cfg *const chan_cfg)
{
	if (!device_is_ready(dev)) {
		LOG_ERR("adc: device is not ready");
		return -1;
	}

	if (adc_channel_setup(dev, chan_cfg)) {
		LOG_ERR("adc: could not setup the channel");
		return -1;
	}

	return 0;
}

static int lcd_init(const struct device *const dev)
{
	int ret;

	if (!device_is_ready(dev)) {
		LOG_ERR("lcd: device is not ready");
		return -1;
	}

	ret = auxdisplay_clear(dev);
	if (ret) {
		LOG_WRN("lcd: failed to clear the display");
	}

	return 0;
}

static int adc_get_mv_reading(const struct device *const dev, const struct adc_sequence *const seq,
			      const struct adc_channel_cfg *const cfg, int32_t *val_mv)
{
	int ret;
	int32_t val;

	ret = adc_read(dev, seq);
	if (ret) {
		LOG_ERR("failed to read adc sequence: %d", ret);
		return -1;
	}
	val = (int32_t)(*(uint16_t *)seq->buffer);

	ret = adc_raw_to_millivolts(adc_ref_internal(dev), cfg->gain, seq->resolution, &val);
	if (ret) {
		LOG_ERR("failed to convert the adc reading to milivolts: %d", ret);
		return -1;
	}
	*val_mv = val;

	return 0;
}

static void button_callback(struct input_event *evt)
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
