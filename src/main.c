#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/gpio/nordic-nrf-gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_DISPLAY_LOG_LEVEL);

#include "disp_vcom.h"
#include "display_sharp_ls0xxb7_bitbang_nrf_macros.h"

#define DISPLAY_H DT_PROP(DT_NODELABEL(sharp_display), height)
#define DISPLAY_W DT_PROP(DT_NODELABEL(sharp_display), width)

static const struct device *display =
    DEVICE_DT_GET(DT_NODELABEL(sharp_display));

static const struct gpio_dt_spec vddio_en =
    GPIO_DT_SPEC_GET(DT_NODELABEL(vddio_en), gpios);
static const struct gpio_dt_spec vbus_en =
    GPIO_DT_SPEC_GET(DT_NODELABEL(vbus_en), gpios);

// static const struct gpio_dt_spec vcom_vb =
//     GPIO_DT_SPEC_GET(DT_NODELABEL(sharp_display), vb_gpios);
// static const struct gpio_dt_spec va =
//     GPIO_DT_SPEC_GET(DT_NODELABEL(sharp_display), va_gpios);

// Led0.
static const struct gpio_dt_spec led0 =
    GPIO_DT_SPEC_GET(DT_NODELABEL(led0), gpios);

static uint8_t buf[(DISPLAY_W * DISPLAY_H) / 8] = {0};

int main(void) {
  int ret;

  gpio_pin_configure_dt(&vddio_en, GPIO_OUTPUT_HIGH);
  gpio_pin_configure_dt(&vbus_en, GPIO_OUTPUT_HIGH);
  gpio_pin_configure_dt(&led0, GPIO_OUTPUT);

  // Power up.
  gpio_pin_set_dt(&vddio_en, 1);
  k_busy_wait(1000);
  gpio_pin_set_dt(&vbus_en, 1);
  k_busy_wait(1000);

  // Start alternating VCOM/VB and VA signals.
  disp_vcom_init();

  struct display_buffer_descriptor desc = {
      .width = DISPLAY_W,
      .height = DISPLAY_H,
      .pitch = DISPLAY_W,
      .buf_size = sizeof(buf),
  };

  struct display_capabilities capabilities;
  // struct display_info info;

  while (1) {
    // send_frame();
    display_write(display, 0, 0, &desc, buf);
    k_msleep(500);
    // LOG_INF("Frame sent");
  }

  return 0;
}
