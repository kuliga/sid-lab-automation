#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "platform.h"

LOG_MODULE_REGISTER(sid_platform, LOG_LEVEL_DBG);

const struct device *const thrmcpl_devs[] = {DEVICE_DT_GET(DT_NODELABEL(thermocouple0)),
 					    DEVICE_DT_GET(DT_NODELABEL(thermocouple1))};
const struct device *const adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc1));
const struct adc_channel_cfg adc_chan2_cfg =
 			ADC_CHANNEL_CFG_DT(DT_NODELABEL(pressure_sensor0));

const struct device *const lcd_dev = DEVICE_DT_GET(DT_NODELABEL(lcd0));
const struct device *const button_dev = DEVICE_DT_GET(DT_PARENT(DT_NODELABEL(user_button)));
INPUT_CALLBACK_DEFINE(button_dev, button_callback);

const struct device *const rtc_dev = DEVICE_DT_GET(DT_NODELABEL(rtc));

const struct device *const emctrl_gpio_dev = DEVICE_DT_GET(DT_NODELABEL(emctrl_gpio));
const struct pwm_dt_spec dcpump_pwm_dt = PWM_DT_SPEC_GET_BY_NAME(DT_PATH(zephyr_user), dcpump);

static int thermocouples_init(const struct device *const *devs, int ndevs);
static int adc_init(const struct device *const dev, const struct adc_channel_cfg *const chan_cfg);
static int lcd_init(const struct device *const dev);
static int rtc_init(const struct device *const dev);
static int emctrl_gpio_init(const struct device *const dev);

int platform_init(void)
{
	int ret;

	ret = thermocouples_init(thrmcpl_devs, ARRAY_SIZE(thrmcpl_devs));
	if (ret) {
		LOG_ERR("sid-platform: exit %d", ret);
		return 1;
	}

	ret = adc_init(adc_dev, &adc_chan2_cfg);
	if (ret) {
		LOG_ERR("sid-platform: exit %d", ret);
		return 1;
	}

	ret = rtc_init(rtc_dev);
	if (ret) {
		LOG_ERR("sid-platform: exit %d", ret);
		return 1;
	}

	ret = emctrl_gpio_init(emctrl_gpio_dev);
	if (ret) {
		LOG_ERR("sid-platform: exit %d", ret);
		return 1;
	}

	ret = pwm_is_ready_dt(&dcpump_pwm_dt);
	if (!ret) {
		LOG_ERR("sid-platform: exit %d", ret);
		return 1;
	}

	//ret = pwm_set_pulse_dt(&dcpump_pwm_dt, PWM_NSEC(0));
	ret = pwm_set_dt(&dcpump_pwm_dt, PWM_NSEC(1000), PWM_NSEC(850));
	if (ret) {
		LOG_ERR("sid-platform: exit %d", ret);
		return 1;
	}

	ret = lcd_init(lcd_dev);
	if (ret) {
		LOG_ERR("sid-platform: exit %d", ret);
		return 1;
	}

	return 0;
}

int adc_get_mv_reading(const struct device *const dev, const struct adc_sequence *const seq,
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

static int rtc_init(const struct device *const dev)
{
	if (!device_is_ready(dev)) {
		LOG_ERR("rtc: device is not ready");
		return -1;
	}

	return 0;
}

static int emctrl_gpio_init(const struct device *const dev)
{
	if (!device_is_ready(dev)) {
		LOG_ERR("emctrl_gpio: device is not ready");
		return -1;
	}

	for (int i = 0; i < 8; ++i) {
		int ret;

		ret = gpio_pin_configure(dev, i, GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW);
		if (ret) {
			LOG_ERR("emctrl_gpio: pin configure failed: %d", ret);
		}
	}

	return 0;
}
