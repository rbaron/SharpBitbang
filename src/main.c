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

static const struct gpio_dt_spec led0 =
    GPIO_DT_SPEC_GET(DT_NODELABEL(led0), gpios);

int main(void) {
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

  lv_obj_t *scr = lv_scr_act();

  // Text.
  lv_obj_t *label = lv_label_create(scr);
  lv_obj_set_width(label, 200);
  lv_label_set_text(label, "Hello, LVGL!");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

#if CONFIG_SHARP_LS0XXB7_DISPLAY_MODE_COLOR
  // Paint background white.
  static lv_style_t style_bg_white;
  lv_style_init(&style_bg_white);
  lv_style_set_bg_color(&style_bg_white, lv_color_white());
  lv_style_set_bg_opa(&style_bg_white, LV_OPA_COVER);
  lv_obj_add_style(scr, &style_bg_white, LV_PART_MAIN);

  // Square.
  static lv_style_t style;
  lv_style_init(&style);
  lv_style_set_bg_color(&style, lv_palette_main(LV_PALETTE_GREEN));
  lv_style_set_border_color(&style, lv_palette_lighten(LV_PALETTE_GREEN, 3));
  lv_style_set_border_width(&style, 3);
  lv_obj_t *square = lv_obj_create(scr);
  lv_obj_add_style(square, &style, LV_PART_MAIN);
  lv_obj_set_style_bg_color(square, lv_palette_main(LV_PALETTE_ORANGE),
                            LV_PART_MAIN);
  lv_obj_set_size(square, 100, 100);
  lv_obj_set_pos(square, 280 / 2 - 110, 280 / 2 - 50);
  lv_obj_update_layout(square);
  // TODO: There's a bug somewhere that causes center alignment not to work
  // (rect is shifted to the right).
  // lv_obj_align(square, LV_ALIGN_CENTER, 0, 0);

  // Paint text red.
  static lv_style_t style_red;
  lv_style_init(&style_red);
  lv_style_set_text_color(&style_red, lv_color_hex(0xff0000));
  lv_obj_add_style(label, &style_red, LV_PART_MAIN);
#endif  // CONFIG_SHARP_LS0XXB7_DISPLAY_MODE_COLOR

  while (1) {
    // lv_task_handler();
    lv_timer_handler();
    k_msleep(100);
  }

  return 0;
}
