#ifndef PLATFORM_H
#define PLATFORM_H

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/pwm.h>

/*
 * Variables and functions defined by the platform, to be used by the app.
 */
extern const struct device *const thrmcpl_devs[2];
extern const struct device *const adc_dev;
extern const struct adc_channel_cfg adc_chan2_cfg;
extern const struct device *const lcd_dev;
extern const struct device *const button_dev;
extern const struct device *const rtc_dev;
extern const struct device *const emctrl_gpio_dev;
extern const struct pwm_dt_spec dcpump_pwm_dt;

int platform_init(void);

int adc_get_mv_reading(const struct device *const dev, const struct adc_sequence *const seq,
		       const struct adc_channel_cfg *const cfg, int32_t *val_mv);


static inline double mp3v5050v_get_pressure(const struct device *const adc_dev, int32_t val_mv)
{
	return 56 * ((float)val_mv / adc_ref_internal(adc_dev)) - 52;
}

static inline double mp3v5050v_get_pressure_error(void)
{
	return 1.25f * 1; // TODO: read the temperature to use proper temperature multiplier value
}
/*
 * Functions to be defined by the app.
 */
void button_callback(struct input_event *evt);

#endif /* PLATFORM_H */
