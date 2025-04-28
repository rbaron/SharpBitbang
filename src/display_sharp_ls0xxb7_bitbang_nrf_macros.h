// These are convenience macros for the Sharp LS0XXB7 display driver for nRF
// SoCs. They are low-level, below-Zephyr fast GPIO macros for the nRF SoCs
// only.

#ifndef _DISPLAY_SHARP_LS0XXB7_BITBANG_NRF_MACROS_H_
#define _DISPLAY_SHARP_LS0XXB7_BITBANG_NRF_MACROS_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/gpio/nordic-nrf-gpio.h>

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
// P0:
// 0.00 - r1
// 0.02 - b1
// 0.03 - g1
#define SET_RGB_P0(val)                                            \
  do {                                                             \
    NRF_P0->OUTCLR = RGB_P0_MASK;                                  \
    NRF_P0->OUTSET = (((val >> 5) & 0x01) << D_GPIO_PIN(rgb, 1)) | \
                     (((val >> 1) & 0x01) << D_GPIO_PIN(rgb, 5)) | \
                     (((val >> 3) & 0x01) << D_GPIO_PIN(rgb, 3));  \
  } while (0)

// P1:
// 1.12 - r0
// 1.13 - g0
// 1.14 - b0
#define SET_RGB_P1(val)                                            \
  do {                                                             \
    NRF_P1->OUTCLR = RGB_P1_MASK;                                  \
    NRF_P1->OUTSET = (((val >> 4) & 0x01) << D_GPIO_PIN(rgb, 0)) | \
                     (((val >> 2) & 0x01) << D_GPIO_PIN(rgb, 2)) | \
                     (((val >> 0) & 0x01) << D_GPIO_PIN(rgb, 4));  \
  } while (0)

// This is used in the tightest, innermost loop, so we want to keep it fast.
// We can likely make up some time by connecting all rgb pins to the same port
// in a next shield revision.
#define SET_RGB(val)   \
  do {                 \
    SET_RGB_P0((val)); \
    SET_RGB_P1((val)); \
  } while (0)

// Convert 5-bit and 6-bit color values to 2 bit values by ignoring the least
// significant bits.
#define CVT_52_BITS(val) ((val) >> 3)
#define CVT_62_BITS(val) ((val) >> 4)

#endif  // _DISPLAY_SHARP_LS0XXB7_BITBANG_NRF_MACROS_H_