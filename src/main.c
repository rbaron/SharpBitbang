#include <lvgl.h>
// #include <lvgl_input_device.h>
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

static const struct device *display_dev =
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

// static uint8_t buf[(DISPLAY_W * DISPLAY_H) / 8] = {0};

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

  if (!device_is_ready(display_dev)) {
    LOG_ERR("Device not ready, aborting test");
    return 0;
  }

  // display_

  lv_obj_t *label = lv_label_create(lv_scr_act());
  lv_label_set_text(label, "Kello, LVGL!");
  lv_obj_align(label, LV_ALIGN_CENTER, -40, -10);
  // lv_obj_align(label, LV_ALIGN_TOP_MID, 10, 0);
  // lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  // display_blanking_off(display_dev);

  while (1) {
    lv_task_handler();
    k_sleep(K_MSEC(10));
  }

  // lv_obj_t *hello_world_label;
  // lv_obj_t *count_label;

  // // hello_world_label = lv_label_create(lv_screen_active());
  // hello_world_label = lv_label_create(lv_scr_act());
  // lv_label_set_text(hello_world_label, "Hello world!");
  // lv_obj_align(hello_world_label, LV_ALIGN_CENTER, 0, 0);

  // struct display_buffer_descriptor desc = {
  //     .width = DISPLAY_W,
  //     .height = DISPLAY_H,
  //     .pitch = DISPLAY_W,
  //     .buf_size = sizeof(buf),
  // };

  // // struct display_capabilities capabilities;
  // // struct display_info info;

  // while (1) {
  //   // send_frame();
  //   // display_write(display_dev, 0, 0, &desc, buf);
  //   k_msleep(500);
  //   // LOG_INF("Frame sent");

  //   // lv_label_set_text(count_label, count_str);

  //   lv_timer_handler();
  // }

  return 0;
}
