// Zephyr bit-banging display driver for Sharp Memory LCDs with the 6-bit
// parallel interface.

// API docs:
// https://docs.zephyrproject.org/latest/doxygen/html/group__display__interface.html

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sharp_mip_parallel, CONFIG_DISPLAY_LOG_LEVEL);

#include <stdint.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

// Private mutable data for the driver.
struct sharp_mip_data {};

// Private const data for the driver.
struct sharp_mip_config {
  uint16_t height;
  uint16_t width;
  // GPIOs.
  struct gpio_dt_spec vcom_vb;
  struct gpio_dt_spec va;
};

static int sharp_mip_init(const struct device *dev) {
  const struct sharp_mip_config *config = dev->config;

  // Configure GPIOs.
  // gpio_pin_configure_dt(&config->vcom_vb, GPIO_OUTPUT);
  // gpio_pin_configure_dt(&config->va, GPIO_OUTPUT);
  LOG_DBG("Sharp MIP display initialized. Resolution: %dx%d", config->width,
          config->height);

  return 0;
}

static int sharp_mip_write(const struct device *dev, const uint16_t x,
                           const uint16_t y,
                           const struct display_buffer_descriptor *desc,
                           const void *buf) {
  struct sharp_mip_config *config = dev->config;

  // Send the data to the display.
  for (size_t i = 0; i < desc->buf_size; i++) {
    // Send the data byte.
    // gpio_pin_set_dt(&config->vcom_vb, buf[i]);
    // gpio_pin_set_dt(&config->va, buf[i]);
  }

  return 0;
}

static void sharp_mip_get_capabilities(
    const struct device *dev, struct display_capabilities *capabilities) {
  const struct sharp_mip_config *config = dev->config;

  capabilities->x_resolution = config->width;
  capabilities->y_resolution = config->height;
  // TODO: This is wasteful. We are allocating 16 bits per pixel here, and we
  // only need 6 bits per pixel. In 2025-03, Zephyr introduced
  // (https://github.com/zephyrproject-rtos/zephyr/pull/86821) the
  // PIXEL_FORMAT_L_8. It's intended for 8-bit grayscale, but I think we can use
  // it for 6-bit color here, saving us half the buffer size.
  capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565;
  capabilities->current_pixel_format = PIXEL_FORMAT_RGB_565;
  capabilities->screen_info = 0;
  // TODO: get from config.
  capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

struct display_driver_api sharp_mip_driver_api = {
    .write = sharp_mip_write,
    .get_capabilities = sharp_mip_get_capabilities,
};

#define SHARP_MIP_DEFINE(node_id)                                  \
  static struct sharp_mip_data data_##node_id;                     \
  static const struct sharp_mip_config config_##node_id = {        \
      .height = DT_PROP(node_id, height),                          \
      .width = DT_PROP(node_id, width),                            \
      .vcom_vb = GPIO_DT_SPEC_GET(node_id, vb_gpios),              \
      .va = GPIO_DT_SPEC_GET(node_id, va_gpios),                   \
  };                                                               \
  DEVICE_DT_DEFINE(node_id, sharp_mip_init, NULL, &data_##node_id, \
                   &config_##node_id, POST_KERNEL,                 \
                   CONFIG_DISPLAY_INIT_PRIORITY, &sharp_mip_driver_api);

DT_FOREACH_STATUS_OKAY(sharp_ls0xxb7_bitbang, SHARP_MIP_DEFINE);