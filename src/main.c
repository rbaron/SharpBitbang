#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/gpio/nordic-nrf-gpio.h>
#include <zephyr/kernel.h>

#include "disp_vcom.h"

#define DISPLAY_H DT_PROP(DT_NODELABEL(sharp_display), height)
#define DISPLAY_W DT_PROP(DT_NODELABEL(sharp_display), width)

#define D_GPIO_PORT(name, idx) \
  DT_PROP_BY_PHANDLE_IDX(DT_NODELABEL(sharp_display), name##_gpios, idx, port)
#define D_GPIO_PIN(name, idx) \
  DT_GPIO_PIN_BY_IDX(DT_NODELABEL(sharp_display), name##_gpios, idx)

#define D_NRF_REG(name, idx) CONCAT(NRF_P, D_GPIO_PORT(name, idx))

// Fast GPIO macros.
// To manipulate a pin defined as intb-gpios, use:
// GSET_IDX(intb, 0);
// To manipulate a pin defined as rgb-gpios[2], use:
// GSET_IDX(rgb, 2);
#define GSET_IDX(name, idx) \
  D_NRF_REG(name, idx)->OUTSET = (1 << D_GPIO_PIN(name, idx))
#define GCLR_IDX(name, idx) \
  D_NRF_REG(name, idx)->OUTCLR = (1 << D_GPIO_PIN(name, idx))
#define GTOG_IDX(name, idx) \
  D_NRF_REG(name, idx)->OUT ^= (1 << D_GPIO_PIN(name, idx))
#define GVAL_IDX(name, idx, val) \
  do {                           \
    if (val) {                   \
      GSET_IDX(name, idx);       \
    } else {                     \
      GCLR_IDX(name, idx);       \
    }                            \
  } while (0)

// Convenience macros for the first pin of each phandle-array group.
#define GSET(name) GSET_IDX(name, 0)
#define GCLR(name) GCLR_IDX(name, 0)
#define GTOG(name) GTOG_IDX(name, 0)
#define GVAL(name, val) GVAL_IDX(name, 0, val)

// RGB pins connected to &gpio0. Indexed into rgb-gpios.
#define RGB_P0_MASK                                       \
  (1 << D_GPIO_PIN(rgb, 1)) | (1 << D_GPIO_PIN(rgb, 3)) | \
      (1 << D_GPIO_PIN(rgb, 5))

// RGB pins connected to &gpio1. Indexed into rgb-gpios.
#define RGB_P1_MASK                                       \
  (1 << D_GPIO_PIN(rgb, 0)) | (1 << D_GPIO_PIN(rgb, 2)) | \
      (1 << D_GPIO_PIN(rgb, 4))

// `val` is a 6-bit value for a half-line: The least significant 2 bits are red,
// next 2 are green, and most significant 2 are blue values.
#define SET_RGB_P0(val)                                            \
  do {                                                             \
    NRF_P0->OUTCLR = RGB_P0_MASK;                                  \
    NRF_P0->OUTSET = (((val >> 1) & 0x01) << D_GPIO_PIN(rgb, 1)) | \
                     (((val >> 3) & 0x01) << D_GPIO_PIN(rgb, 3)) | \
                     (((val >> 4) & 0x01) << D_GPIO_PIN(rgb, 5));  \
  } while (0)

#define SET_RGB_P1(val)                                            \
  do {                                                             \
    NRF_P1->OUTCLR = RGB_P1_MASK;                                  \
    NRF_P1->OUTSET = (((val >> 0) & 0x01) << D_GPIO_PIN(rgb, 0)) | \
                     (((val >> 2) & 0x01) << D_GPIO_PIN(rgb, 2)) | \
                     (((val >> 1) & 0x01) << D_GPIO_PIN(rgb, 4));  \
  } while (0)

// This is used in the tightest, innermost loop, so we want to keep it fast.
// We can likely make up some time by connecting all rgb pins to the same port
// in a next shield revision.
#define SET_RGB(val) \
  do {               \
    SET_RGB_P0(val); \
    SET_RGB_P1(val); \
  } while (0)

static const struct device *display =
    DEVICE_DT_GET(DT_NODELABEL(sharp_display));

static const struct gpio_dt_spec intb =
    GPIO_DT_SPEC_GET(DT_NODELABEL(sharp_display), intb_gpios);
static const struct gpio_dt_spec gsp =
    GPIO_DT_SPEC_GET(DT_NODELABEL(sharp_display), gsp_gpios);
static const struct gpio_dt_spec gck =
    GPIO_DT_SPEC_GET(DT_NODELABEL(sharp_display), gck_gpios);
static const struct gpio_dt_spec gen =
    GPIO_DT_SPEC_GET(DT_NODELABEL(sharp_display), gen_gpios);
static const struct gpio_dt_spec bsp =
    GPIO_DT_SPEC_GET(DT_NODELABEL(sharp_display), bsp_gpios);
static const struct gpio_dt_spec bck =
    GPIO_DT_SPEC_GET(DT_NODELABEL(sharp_display), bck_gpios);

static const struct gpio_dt_spec vddio_en =
    GPIO_DT_SPEC_GET(DT_NODELABEL(vddio_en), gpios);
static const struct gpio_dt_spec vbus_en =
    GPIO_DT_SPEC_GET(DT_NODELABEL(vbus_en), gpios);

// Led0.
static const struct gpio_dt_spec led0 =
    GPIO_DT_SPEC_GET(DT_NODELABEL(led0), gpios);

static const struct gpio_dt_spec gpios[] = {
    intb,
    gsp,
    gck,
    gen,
    bsp,
    bck,
    GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(sharp_display), rgb_gpios, 0),
    GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(sharp_display), rgb_gpios, 1),
    GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(sharp_display), rgb_gpios, 2),
    GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(sharp_display), rgb_gpios, 3),
    GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(sharp_display), rgb_gpios, 4),
    GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(sharp_display), rgb_gpios, 5),
};

// Simple example, draw stripes of RGB colors.
static inline void set_rgb(int y, int x) {
  if (y < DISPLAY_H / 3) {
    SET_RGB(0b000011);
  } else if (y < (DISPLAY_H * 2) / 3) {
    SET_RGB(0b001100);
  } else {
    SET_RGB(0b110000);
  }
}

// Sends a half-line: either MSB of LSB of 280 line pixels, depending on GCK:
// GCK = 0: MSB
// GCK = 1: LSB
static inline void send_line(int y) {
  GSET(bsp);
  GSET(bck);
  GCLR(bck);
  GCLR(bsp);

  for (int i = 3; i <= 141; i++) {
    set_rgb(y, i);
    GTOG(bck);
  }

  // Remaining 3 clock cycles.
  for (int i = 0; i < 3; i++) {
    GTOG(bck);
  }
}

static void send_frame(void) {
  GSET(intb);
  GSET(gsp);
  GSET(gck);
  GCLR(gck);

  send_line(0);

  GCLR(gsp);

  for (int i = 3; i <= 561; i++) {
    GTOG(gck);
    GSET(gen);

    send_line((i - 2) / 2);

    GCLR(gen);
  }

  // GCK edge 562 -- no data.
  GTOG(gck);
  GSET(gen);
  GCLR(gen);
  GTOG(gck);

  for (int i = 563; i <= 568; i++) {
    GTOG(gck);
    if (i == 566) {
      GCLR(intb);
      continue;
    }
  }

  GCLR(intb);
}

int main(void) {
  int ret;

  if (!gpio_is_ready_dt(&gsp)) {
    return 0;
  }

  if (!gpio_is_ready_dt(&vddio_en)) {
    return 0;
  }

  gpio_pin_configure_dt(&vddio_en, GPIO_OUTPUT_HIGH);
  gpio_pin_configure_dt(&vbus_en, GPIO_OUTPUT_HIGH);
  gpio_pin_configure_dt(&led0, GPIO_OUTPUT);

  for (int i = 0; i < 14; i++) {
    ret = gpio_pin_configure_dt(&gpios[i], GPIO_OUTPUT_LOW);
    if (ret < 0) {
      return 0;
    }
  }

  // Power up.
  gpio_pin_set_dt(&vddio_en, 1);
  k_busy_wait(1000);
  gpio_pin_set_dt(&vbus_en, 1);
  k_busy_wait(1000);

  // Start alternating VCOM/VB and VA signals.
  disp_vcom_init();

  while (1) {
    send_frame();
    k_msleep(500);
  }

  return 0;
}
