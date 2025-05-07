#include <lvgl.h>
#include <lvgl_display.h>
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led.h>
#include <zephyr/dt-bindings/gpio/nordic-nrf-gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

// nPM1300.
#include <zephyr/drivers/mfd/npm1300.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/dt-bindings/regulator/npm1300.h>

// sht40.
#include <zephyr/drivers/sensor/sht4x.h>

// static const struct device *regulators =
//     DEVICE_DT_GET(DT_NODELABEL(npm1300_regulators));
// static const struct device *ldo2 = DEVICE_DT_GET(DT_NODELABEL(npm1300_ldo2));

// BLE.
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0xaa, 0xfe),
    BT_DATA_BYTES(BT_DATA_SVC_DATA16, 0xaa, 0xfe, /* Eddystone UUID */
                  0x10,                           /* Eddystone-URL frame type */
                  0x00, /* Calibrated Tx power at 0m */
                  0x00, /* URL Scheme Prefix http://www. */
                  'z', 'e', 'p', 'h', 'y', 'r', 'p', 'r', 'o', 'j', 'e', 'c',
                  't', 0x08) /* .org */
};

/* Set Scan Response data */
static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct device *leds = DEVICE_DT_GET(DT_NODELABEL(npm1300_leds));

LOG_MODULE_REGISTER(main, CONFIG_DISPLAY_LOG_LEVEL);

#define DISPLAY_H DT_PROP(DT_NODELABEL(sharp_display), height)
#define DISPLAY_W DT_PROP(DT_NODELABEL(sharp_display), width)

static const struct device *display_dev =
    DEVICE_DT_GET(DT_NODELABEL(sharp_display));

// static const struct gpio_dt_spec vddio_en =
//     GPIO_DT_SPEC_GET(DT_NODELABEL(vddio_en), gpios);
// static const struct gpio_dt_spec vbus_en =
//     GPIO_DT_SPEC_GET(DT_NODELABEL(vbus_en), gpios);
static const struct gpio_dt_spec dispv5en =
    GPIO_DT_SPEC_GET(DT_NODELABEL(dispv5en), gpios);

static const struct gpio_dt_spec led0 =
    GPIO_DT_SPEC_GET(DT_NODELABEL(led1), gpios);

// Buttons.
static const struct gpio_dt_spec button0 =
    GPIO_DT_SPEC_GET(DT_NODELABEL(button0), gpios);
static const struct gpio_dt_spec button1 =
    GPIO_DT_SPEC_GET(DT_NODELABEL(button1), gpios);

static struct gpio_callback button_cb_data;

// Callbacks.
static void button_pressed(const struct device *dev, struct gpio_callback *cb,
                           uint32_t pins) {
  LOG_INF("Button pressed: pins %d", pins);
}

static void bt_ready(int err) {
  char addr_s[BT_ADDR_LE_STR_LEN];
  bt_addr_le_t addr = {0};
  size_t count = 1;

  if (err) {
    LOG_ERR("Bluetooth init failed (err %d)\n", err);
    return;
  }

  LOG_INF("Bluetooth initialized\n");

  /* Start advertising */
  err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad), sd,
                        ARRAY_SIZE(sd));
  if (err) {
    LOG_ERR("Advertising failed to start (err %d)\n", err);
    return;
  }

  /* For connectable advertising you would use
   * bt_le_oob_get_local().  For non-connectable non-identity
   * advertising an non-resolvable private address is used;
   * there is no API to retrieve that.
   */

  bt_id_get(&addr, &count);
  bt_addr_le_to_str(&addr, addr_s, sizeof(addr_s));

  LOG_INF("Beacon started, advertising as %s\n", addr_s);
}

int main(void) {
  NRF_NFCT->PADCONFIG = 0;

  // gpio_pin_configure_dt(&vddio_en, GPIO_OUTPUT_HIGH);
  // gpio_pin_configure_dt(&vbus_en, GPIO_OUTPUT_HIGH);
  LOG_INF("Okay0!");
  gpio_pin_configure_dt(&led0, GPIO_OUTPUT);
  gpio_pin_configure_dt(&dispv5en, GPIO_OUTPUT_HIGH);

  LOG_INF("Okay1!");

  // Configure buttons.
  int ret = 0;
  ret = gpio_pin_configure_dt(&button0, GPIO_INPUT);
  if (ret != 0) {
    LOG_ERR("Error %d: failed to configure %s pin %d\n", ret,
            button0.port->name, button0.pin);
    return 0;
  }

  ret = gpio_pin_interrupt_configure_dt(&button0, GPIO_INT_EDGE_TO_ACTIVE);
  if (ret != 0) {
    LOG_ERR("Error %d: failed to configure %s pin %d\n", ret,
            button0.port->name, button0.pin);
    return 0;
  }

  ret = gpio_pin_configure_dt(&button1, GPIO_INPUT);
  if (ret != 0) {
    LOG_ERR("Error %d: failed to configure %s pin %d\n", ret,
            button1.port->name, button1.pin);
    return 0;
  }
  ret = gpio_pin_interrupt_configure_dt(&button1, GPIO_INT_EDGE_TO_ACTIVE);
  if (ret != 0) {
    LOG_ERR("Error %d: failed to configure %s pin %d\n", ret,
            button1.port->name, button1.pin);
    return 0;
  }

  gpio_init_callback(&button_cb_data, button_pressed,
                     BIT(button0.pin) | BIT(button1.pin));

  ret = gpio_add_callback(button0.port, &button_cb_data);
  if (ret != 0) {
    LOG_ERR("Error %d: failed to add callback for %s pin %d\n", ret,
            button0.port->name, button0.pin);
    return 0;
  }

  // if (!device_is_ready(regulators)) {
  //   LOG_ERR("Error: Regulator device is not ready\n");
  //   return 0;
  // }

  // if (!device_is_ready(ldo2)) {
  //   LOG_ERR("Error: LDO2 device is not ready\n");
  //   return 0;
  // }

  // ret = bt_enable(bt_ready);
  // if (ret) {
  //   LOG_ERR("Bluetooth init failed (err %d)\n", ret);
  // }

  // Power up.
  // gpio_pin_set_dt(&vddio_en, 1);
  // k_busy_wait(1000);
  // gpio_pin_set_dt(&vbus_en, 1);
  // k_busy_wait(1000);
  gpio_pin_set_dt(&dispv5en, 1);
  k_busy_wait(1000);

  if (!device_is_ready(display_dev)) {
    LOG_ERR("Device not ready, aborting test");
    return 0;
  }

  // Rotation.
  // lv_disp_set_rotation(lv_disp_get_default(), LV_DISP_ROT_180);

  lv_obj_t *scr = lv_scr_act();

  // Square.
  lv_obj_t *square = lv_obj_create(scr);
  lv_obj_set_style_bg_opa(square, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_size(square, 180, 128);
  lv_obj_align(square, LV_ALIGN_CENTER, 0, 0);

  // Text.
  lv_obj_t *label = lv_label_create(scr);
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(label, 110);
  lv_label_set_text(label, "Hello, world");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

#if CONFIG_SHARP_LS0XXB7_DISPLAY_MODE_MONOCHROME
  lv_obj_set_style_bg_color(square, lv_color_black(), LV_PART_MAIN);
#elif CONFIG_SHARP_LS0XXB7_DISPLAY_MODE_COLOR
  // Paint background white.
  static lv_style_t style_bg_white;
  lv_style_init(&style_bg_white);
  lv_style_set_bg_color(&style_bg_white, lv_color_white());
  // lv_style_set_bg_color(&style_bg_white, lv_color_hex(0x0000ff));
  lv_style_set_bg_opa(&style_bg_white, LV_OPA_COVER);
  lv_obj_add_style(scr, &style_bg_white, LV_PART_MAIN);

  // Square.
  static lv_style_t style;
  lv_style_init(&style);
  lv_style_set_bg_color(&style, lv_color_hex(0x00ff00));
  lv_style_set_border_color(&style, lv_color_hex(0x0000ff));
  lv_style_set_border_width(&style, 4);
  lv_obj_add_style(square, &style, LV_PART_MAIN);
  lv_obj_update_layout(square);

  // Paint text red.
  static lv_style_t style_red;
  lv_style_init(&style_red);
  lv_style_set_text_color(&style_red, lv_color_hex(0xff0000));
  lv_obj_add_style(label, &style_red, LV_PART_MAIN);
#endif  // CONFIG_SHARP_LS0XXB7_DISPLAY_MODE_COLOR

  LOG_INF("Okay2!");
  // lv_disp_set_rotation(NULL, LV_DISP_ROT_180);

  // SHT40.
  // const struct device *const sht = DEVICE_DT_GET_ANY(sensirion_sht4x);
  // struct sensor_value temp, hum;
  // if (!device_is_ready(sht)) {
  //   LOG_ERR("Device %s is not ready.\n", sht->name);
  //   return 0;
  // }

  int x = 100;
  char buf[32];
  while (1) {
    lv_task_handler();
    k_msleep(1000);

    // Update counter.
    snprintf(buf, sizeof(buf), "x: %d", x);
    lv_label_set_text(label, buf);
    x += 1;

    // Toggle LED.
    gpio_pin_toggle_dt(&led0);

    // if (sensor_sample_fetch(sht)) {
    //   printf("Failed to fetch sample from SHT4X device\n");
    //   return 0;
    // }

    // sensor_channel_get(sht, SENSOR_CHAN_AMBIENT_TEMP, &temp);
    // sensor_channel_get(sht, SENSOR_CHAN_HUMIDITY, &hum);

    // LOG_INF("Temperature: %d.%06d C", temp.val1, temp.val2);
    // LOG_INF("Humidity: %d.%06d %%", hum.val1, hum.val2);

    led_on(leds, 0);
    k_msleep(500);
    led_off(leds, 0);

    // Enable regulator.
    // int ret = 0;
    // if (x % 2 == 0) {
    //   ret = regulator_disable(ldo2);
    // } else {
    //   ret = regulator_enable(ldo2);
    // }
    // if (ret < 0) {
    //   LOG_ERR("Error togglign regulator: %d", ret);
    // }
  }

  return 0;
}
