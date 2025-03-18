#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>

static const struct device *display = DEVICE_DT_GET(DT_NODELABEL(sharp_display));

static const struct gpio_dt_spec intb = GPIO_DT_SPEC_GET(DT_NODELABEL(sharp_display), intb_gpios);
static const struct gpio_dt_spec gsp = GPIO_DT_SPEC_GET(DT_NODELABEL(sharp_display), gsp_gpios);
static const struct gpio_dt_spec gck = GPIO_DT_SPEC_GET(DT_NODELABEL(sharp_display), gck_gpios);
static const struct gpio_dt_spec gen = GPIO_DT_SPEC_GET(DT_NODELABEL(sharp_display), gen_gpios);
static const struct gpio_dt_spec bsp = GPIO_DT_SPEC_GET(DT_NODELABEL(sharp_display), bsp_gpios);
static const struct gpio_dt_spec bck = GPIO_DT_SPEC_GET(DT_NODELABEL(sharp_display), bck_gpios);
static const struct gpio_dt_spec vb = GPIO_DT_SPEC_GET(DT_NODELABEL(sharp_display), vb_gpios);
static const struct gpio_dt_spec va = GPIO_DT_SPEC_GET(DT_NODELABEL(sharp_display), va_gpios);

static const struct gpio_dt_spec gpios[] = {
	intb,
	gsp,
	gck,
	gen,
	bsp,
	bck,
	vb,
	va,
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(sharp_display), rgb_gpios, 0),
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(sharp_display), rgb_gpios, 1),
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(sharp_display), rgb_gpios, 2),
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(sharp_display), rgb_gpios, 3),
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(sharp_display), rgb_gpios, 4),
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(sharp_display), rgb_gpios, 5),
};

int main(void)
{
	int ret;

	if (!gpio_is_ready_dt(&gsp)) {
		return 0;
	}

	for (int i = 0; i < 14; i++) {
		ret = gpio_pin_configure_dt(&gpios[i], GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return 0;
		}
	}

	while (1) {
		for (int i = 0; i < 14; i++) {
			ret = gpio_pin_toggle_dt(&gpios[i]);
			if (ret < 0) {
				return 0;
			}
		}
		k_busy_wait(1);
	}
	return 0;
}
