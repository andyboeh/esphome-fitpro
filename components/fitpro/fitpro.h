#pragma once

#include "esphome/core/component.h"
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/preferences.h"
#include "esphome/components/time/real_time_clock.h"

#ifdef USE_ESP32

#define CMD_THROTTLE_INTERVAL_MS  200

#include <esp_gattc_api.h>

namespace esphome {
namespace fitpro {

namespace espbt = esphome::esp32_ble_tracker;

static const espbt::ESPBTUUID UUID_BATTERY_SERVICE = espbt::ESPBTUUID::from_raw("0000180f-0000-1000-8000-00805f9b34fb");
static const espbt::ESPBTUUID UUID_CHARACTERISTIC_BATTERY = espbt::ESPBTUUID::from_raw("00002a19-0000-1000-8000-00805f9b34fb");
static const espbt::ESPBTUUID UUID_UART_SERVICE = espbt::ESPBTUUID::from_raw("6e400001-b5a3-f393-e0a9-e50e24dcca9d");
static const espbt::ESPBTUUID UUID_CHARACTERISTIC_RX = espbt::ESPBTUUID::from_raw("6e400003-b5a3-f393-e0a9-e50e24dcca9d");
static const espbt::ESPBTUUID UUID_CHARACTERISTIC_TX = espbt::ESPBTUUID::from_raw("6e400002-b5a3-f393-e0a9-e50e24dcca9d");

enum FITPRO_CMD {
  FITPRO_CMD_NONE,
  FITPRO_CMD_INIT_STAGE_1,
  FITPRO_CMD_INIT_STAGE_2,
  FITPRO_CMD_INIT_STAGE_3,
  FITPRO_CMD_INIT_STAGE_4,
  FITPRO_CMD_INIT_STAGE_5,
  FITPRO_CMD_INIT_STAGE_6,
  FITPRO_CMD_INIT_STAGE_7,
  FITPRO_CMD_INIT_STAGE_8,
  FITPRO_CMD_INIT_STAGE_9,
  FITPRO_CMD_INIT_STAGE_10,
  FITPRO_CMD_FIND_DEVICE,
};


static const uint8_t FITPRO_CMD_GROUP_GENERAL = 0x12;
static const uint8_t FITPRO_CMD_GROUP_BAND_INFO = 0x20;
static const uint8_t FITPRO_CMD_GROUP_REQUEST_DATA = 0x1a;

static const uint8_t FITPRO_CMD_INIT1 = 0xa;
static const uint8_t FITPRO_CMD_INIT2 = 0xc;
static const uint8_t FITPRO_CMD_INIT3 = 0xff;
static const uint8_t FITPRO_CMD_FIND_BAND = 0x0b;
static const uint8_t FITPRO_CMD_SET_LANGUAGE = 0x15;
static const uint8_t FITPRO_CMD_SET_DATE_TIME = 0x1;
static const uint8_t FITPRO_CMD_NOTIFICATION_MESSAGE = 0x12;
static const uint8_t FITPRO_CMD_NOTIFICATION_CALL = 0x11;
static const uint8_t FITPRO_CMD_NOTIFICATIONS_ENABLE = 0x7;

static const uint8_t FITPRO_CMD_RX_BAND_INFO = 0x2;
static const uint8_t FITPRO_CMD_GET_HW_INFO = 0x10;
static const uint8_t FITPRO_LANG_ENGLISH = 0x1;

class fitpro : public esphome::ble_client::BLEClientNode, public Component {
 public:
  void setup() override;
  void loop() override;
  void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                           esp_ble_gattc_cb_param_t *param) override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  void set_time(time::RealTimeClock *time) { time_ = time; }
  void set_battery_sensor(sensor::Sensor  *battery_sensor) { battery_sensor_ = battery_sensor; }
  void set_status_sensor(text_sensor::TextSensor *status) { status_ = status; }

 private:
  bool discover_characteristics();
  void write_notify_config_descriptor(bool enable);
  void enable_notifications();
  void send_data(uint8_t cmd_group, uint8_t cmd, uint8_t val);
  void send_data(uint8_t cmd_group, uint8_t cmd);
  void send_data(uint8_t cmd_group, uint8_t cmd, std::vector<uint8_t> val);
  void send_ack(uint8_t cmd_group, uint8_t len_hi, uint8_t len_low, uint8_t cmd);

  time::RealTimeClock *time_{nullptr};
  sensor::Sensor *battery_sensor_{nullptr};
  text_sensor::TextSensor *status_{nullptr};
  FITPRO_CMD next_cmd_{FITPRO_CMD_NONE};
  uint16_t char_handle_tx_;
  uint16_t char_handle_rx_;
  uint16_t char_handle_battery_;
  uint16_t cccd_rx_;
  uint16_t cccd_battery_;
  uint32_t last_command_{0};
};

}  // namespace senseu
}  // namespace esphome

#endif

