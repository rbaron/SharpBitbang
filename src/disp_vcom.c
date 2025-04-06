// This is a lower-level, zephyrless module that uses timers, GPIOTE and (D)PPI
// to generate VCOM/VB and VA signals without CPU intervention. Based on sample
// code:
// https://github.com/zephyrproject-rtos/hal_nordic/blob/master/nrfx/samples/src/nrfx_gppi/fork/main.c

// Relevant dev academy course:
// https://academy.nordicsemi.com/courses/nrf-connect-sdk-intermediate/lessons/lesson-6-analog-to-digital-converter-adc/topic/exercise-3-interfacing-with-adc-using-nrfx-drivers-and-timer-ppi/

// nrfx docs:
// - Zephyr:
// https://docs.zephyrproject.org/latest/samples/boards/nordic/nrfx/README.html
// - nRF54L15:
// https://docs.nordicsemi.com/bundle/ps_nrf5340/page/dppi.html

#include "disp_vcom.h"

#include <hal/nrf_gpio.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_gpiote.h>
#include <nrfx_timer.h>
#include <zephyr/kernel.h>
#if defined(DPPI_PRESENT)
#include <nrfx_dppi.h>
#else
#include <nrfx_ppi.h>
#endif

#define D_GPIO_PORT(pin) \
  DT_PROP_BY_PHANDLE_IDX(DT_NODELABEL(sharp_display), pin##_gpios, 0, port)
#define D_GPIO_PIN(pin) DT_GPIO_PIN(DT_NODELABEL(sharp_display), pin##_gpios)
#define D_GPIO_ABS_PIN(pin) NRF_GPIO_PIN_MAP(D_GPIO_PORT(pin), D_GPIO_PIN(pin))

#define TIMER_INST_IDX 22
#define TIME_TO_WAIT_MS (1000 / DT_PROP(DT_NODELABEL(sharp_display), vcom_freq))

// GPIOTE30 is in the GPIO P0 domain (see figure 1 -- block diagram page 15 on
// nRF54L15 datasheet).
#define GPIOTE_INST_IDX_P0 30
// GPIOTE20 is in the GPIO P1 domain.
#define GPIOTE_INST_IDX_P1 20

#define OUTPUT_PIN_PRIMARY D_GPIO_ABS_PIN(vb)  // p0
#define OUTPUT_PIN_FORK D_GPIO_ABS_PIN(va)     // p1

int disp_vcom_init(void) {
  nrfx_err_t status;
  (void)status;

  uint8_t out_channel_primary, out_channel_fork;
  uint8_t gppi_channel_p0, gppi_channel_p1;

#if defined(__ZEPHYR__)
  IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_TIMER_INST_GET(TIMER_INST_IDX)),
              IRQ_PRIO_LOWEST, NRFX_TIMER_INST_HANDLER_GET(TIMER_INST_IDX), 0,
              0);

  // Not needed? See:
  // https://devzone.nordicsemi.com/f/nordic-q-a/110123/ncs-v2-6-0-instructions-to-use-gpiote-and-zephyr-gpio-api
  // IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_GPIOTE_INST_GET(GPIOTE_INST_IDX_P0)),
  //             IRQ_PRIO_LOWEST,
  //             NRFX_GPIOTE_INST_HANDLER_GET(GPIOTE_INST_IDX_p0), 0, 0);
#endif

  nrfx_gpiote_t const gpiote_inst_p0 = NRFX_GPIOTE_INSTANCE(GPIOTE_INST_IDX_P0);
  status = nrfx_gpiote_init(&gpiote_inst_p0,
                            NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY);
  NRFX_ASSERT(status == NRFX_SUCCESS);

  status = nrfx_gpiote_channel_alloc(&gpiote_inst_p0, &out_channel_primary);
  NRFX_ASSERT(status == NRFX_SUCCESS);

  nrfx_gpiote_t const gpiote_inst_p1 = NRFX_GPIOTE_INSTANCE(GPIOTE_INST_IDX_P1);
  status = nrfx_gpiote_init(&gpiote_inst_p1,
                            NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY);
  NRFX_ASSERT(status == NRFX_SUCCESS);

  status = nrfx_gpiote_channel_alloc(&gpiote_inst_p1, &out_channel_fork);
  NRFX_ASSERT(status == NRFX_SUCCESS);

  // Configure tasks. Both will toggle VB and VA.
  static const nrfx_gpiote_output_config_t output_config = {
      .drive = NRF_GPIO_PIN_S0S1,
      .input_connect = NRF_GPIO_PIN_INPUT_DISCONNECT,
      .pull = NRF_GPIO_PIN_NOPULL,
  };

  nrfx_gpiote_task_config_t task_config = {
      .task_ch = out_channel_primary,
      .polarity = NRF_GPIOTE_POLARITY_TOGGLE,
      .init_val = NRF_GPIOTE_INITIAL_VALUE_HIGH,
  };

  status = nrfx_gpiote_output_configure(&gpiote_inst_p0, OUTPUT_PIN_PRIMARY,
                                        &output_config, &task_config);
  NRFX_ASSERT(status == NRFX_SUCCESS);

  nrfx_gpiote_out_task_enable(&gpiote_inst_p0, OUTPUT_PIN_PRIMARY);

  task_config.task_ch = out_channel_fork;
  task_config.init_val = NRF_GPIOTE_INITIAL_VALUE_LOW;

  status = nrfx_gpiote_output_configure(&gpiote_inst_p1, OUTPUT_PIN_FORK,
                                        &output_config, &task_config);
  NRFX_ASSERT(status == NRFX_SUCCESS);

  nrfx_gpiote_out_task_enable(&gpiote_inst_p1, OUTPUT_PIN_FORK);

  // Set up timer.

  nrfx_timer_t timer_inst = NRFX_TIMER_INSTANCE(TIMER_INST_IDX);
  uint32_t base_frequency = NRF_TIMER_BASE_FREQUENCY_GET(timer_inst.p_reg);
  nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG(base_frequency);
  timer_config.bit_width = NRF_TIMER_BIT_WIDTH_32;
  timer_config.p_context = "Some context";

  status =
      nrfx_timer_init(&timer_inst, &timer_config, /*timer_event_handler=*/NULL);
  NRFX_ASSERT(status == NRFX_SUCCESS);

  nrfx_timer_clear(&timer_inst);

  uint32_t desired_ticks = nrfx_timer_ms_to_ticks(&timer_inst, TIME_TO_WAIT_MS);

  nrfx_timer_extended_compare(&timer_inst, NRF_TIMER_CC_CHANNEL0, desired_ticks,
                              NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK,
                              /*enable_int=*/false);

  nrfx_timer_extended_compare(&timer_inst, NRF_TIMER_CC_CHANNEL1, desired_ticks,
                              NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK,
                              /*enable_int=*/false);

  // Allocate GPPI channels.
  // According to the documentation, I should be able to use a single channel
  // and multiple subscribers, but I did not manage to get that to work. Indeed,
  // if both subscribers are on the same GPIOTE instance, things work. But using
  // different GPIOTE instances (the one for P0 and for P1), it did not.
  status = nrfx_gppi_channel_alloc(&gppi_channel_p0);
  NRFX_ASSERT(status == NRFX_SUCCESS);

  status = nrfx_gppi_channel_alloc(&gppi_channel_p1);
  NRFX_ASSERT(status == NRFX_SUCCESS);

  nrfx_gppi_channel_endpoints_setup(
      gppi_channel_p0,
      // Event register.
      nrfx_timer_compare_event_address_get(&timer_inst, NRF_TIMER_CC_CHANNEL0),
      // Task register.
      nrfx_gpiote_out_task_address_get(&gpiote_inst_p0, OUTPUT_PIN_PRIMARY));

  nrfx_gppi_channel_endpoints_setup(
      gppi_channel_p1,
      // Event register.
      nrfx_timer_compare_event_address_get(&timer_inst, NRF_TIMER_CC_CHANNEL1),
      // Task register.
      nrfx_gpiote_out_task_address_get(&gpiote_inst_p1, OUTPUT_PIN_FORK));

  nrfx_gppi_channels_enable(BIT(gppi_channel_p0) | BIT(gppi_channel_p1));

  nrfx_timer_enable(&timer_inst);

  return 0;
}