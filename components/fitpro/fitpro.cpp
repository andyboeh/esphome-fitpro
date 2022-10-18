#include "fitpro.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

#ifdef USE_ESP32

namespace esphome {
namespace fitpro {

static const char *const TAG = "fitpro";

void fitpro::dump_config() {
  ESP_LOGCONFIG(TAG, "FitPro");
}

void fitpro::setup() {
  std::string addr = this->parent_->address_str();
  if(this->status_) {
      this->status_->publish_state("Setting up...");
  }
}

void fitpro::loop() {
    if((millis() - this->last_command_) > CMD_THROTTLE_INTERVAL_MS) {
        if(this->next_cmd_ != FITPRO_CMD_NONE)
             ESP_LOGD(TAG, "Sending next command: %d", this->next_cmd_);
        if(this->next_cmd_ == FITPRO_CMD_INIT_STAGE_1) {
            auto now = this->time_->now();
            if(!now.is_valid()) {
                ESP_LOGD(TAG, "Time is still invalid, waiting...");
                this->last_command_ = millis();
                return;
            }
        }
        switch(this->next_cmd_) {
            case FITPRO_CMD_INIT_STAGE_1:
                this->send_data(FITPRO_CMD_GROUP_GENERAL, FITPRO_CMD_INIT1, 0x2);
                this->next_cmd_ = FITPRO_CMD_INIT_STAGE_2;
                this->last_command_ = millis();
                break;
            case FITPRO_CMD_INIT_STAGE_2: {
                auto now = this->time_->now();
                uint32_t ts = ((now.year - 2000) << 26 | now.month << 22 | now.day_of_month << 17 | now.hour << 12 | now.minute << 6);
                std::vector<uint8_t> vec;
                vec.push_back(((ts >> 24) & 0xff));
                vec.push_back(((ts >> 16) & 0xff));
                vec.push_back(((ts >> 8) & 0xff));
                vec.push_back(((ts >> 0) & 0xff));
                this->send_data(FITPRO_CMD_GROUP_GENERAL, FITPRO_CMD_SET_DATE_TIME, vec);
                this->next_cmd_ = FITPRO_CMD_INIT_STAGE_3;
                this->last_command_ = millis();
                break;
            }
            case FITPRO_CMD_INIT_STAGE_3:
                this->send_data(FITPRO_CMD_GROUP_REQUEST_DATA, FITPRO_CMD_INIT1);
                this->next_cmd_ = FITPRO_CMD_INIT_STAGE_4;
                this->last_command_ = millis();
                break;
            case FITPRO_CMD_INIT_STAGE_4:
                this->send_data(FITPRO_CMD_GROUP_REQUEST_DATA, FITPRO_CMD_INIT2);
                this->next_cmd_ = FITPRO_CMD_INIT_STAGE_5;
                this->last_command_ = millis();
                break;
            case FITPRO_CMD_INIT_STAGE_5:
                this->send_data(FITPRO_CMD_GROUP_GENERAL, FITPRO_CMD_SET_LANGUAGE, FITPRO_LANG_ENGLISH);
                this->next_cmd_ = FITPRO_CMD_INIT_STAGE_6;
                this->last_command_ = millis();
                break;
            case FITPRO_CMD_INIT_STAGE_6:
                this->send_data(FITPRO_CMD_GROUP_GENERAL, FITPRO_CMD_INIT3, 1);
                this->next_cmd_ = FITPRO_CMD_INIT_STAGE_7;
                this->last_command_ = millis();
                break;
            case FITPRO_CMD_INIT_STAGE_7:
                this->send_data(FITPRO_CMD_GROUP_REQUEST_DATA, 1);
                this->next_cmd_ = FITPRO_CMD_INIT_STAGE_8;
                this->last_command_ = millis();
                break;
            case FITPRO_CMD_INIT_STAGE_8:
                this->send_data(FITPRO_CMD_GROUP_REQUEST_DATA, 0xf);
                this->next_cmd_ = FITPRO_CMD_INIT_STAGE_9;
                this->last_command_ = millis();
                break;
            case FITPRO_CMD_INIT_STAGE_9:
                this->send_data(FITPRO_CMD_GROUP_REQUEST_DATA, FITPRO_CMD_GET_HW_INFO);
                this->next_cmd_ = FITPRO_CMD_INIT_STAGE_10;
                this->last_command_ = millis();
                break;
            case FITPRO_CMD_INIT_STAGE_10:
                this->send_data(FITPRO_CMD_GROUP_BAND_INFO, FITPRO_CMD_RX_BAND_INFO);
                this->next_cmd_ = FITPRO_CMD_NONE;
                this->last_command_ = millis();
                break;
            case FITPRO_CMD_FIND_DEVICE:
                this->send_data(FITPRO_CMD_GROUP_GENERAL, FITPRO_CMD_FIND_BAND, 1);
                this->next_cmd_ = FITPRO_CMD_NONE;
                this->last_command_ = millis();
                break;
            case FITPRO_CMD_NONE:
            default:
                break;
        }
    }
}

bool fitpro::discover_characteristics() {
    bool result = true;
    esphome::ble_client::BLECharacteristic *chr;
    esphome::ble_client::BLEDescriptor *desc;

    if(!this->char_handle_rx_) {
        chr = this->parent_->get_characteristic(UUID_UART_SERVICE, UUID_CHARACTERISTIC_RX);
        if(chr == nullptr) {
            ESP_LOGE(TAG, "Required data characteristic %s not found.", UUID_CHARACTERISTIC_RX.to_string().c_str());
            result = false;
        } else {
            this->char_handle_rx_ = chr->handle;
        }
    }

    if(!this->char_handle_tx_) {
        chr = this->parent_->get_characteristic(UUID_UART_SERVICE, UUID_CHARACTERISTIC_TX);
        if(chr == nullptr) {
            ESP_LOGE(TAG, "Required data characteristic %s not found.", UUID_CHARACTERISTIC_TX.to_string().c_str());
            result = false;
        } else {
            this->char_handle_tx_ = chr->handle;
        }
    }

    if(!this->char_handle_battery_) {
        chr = this->parent_->get_characteristic(UUID_BATTERY_SERVICE, UUID_CHARACTERISTIC_BATTERY);
        if(chr == nullptr) {
            ESP_LOGE(TAG, "Required data characteristic %s not found.", UUID_CHARACTERISTIC_BATTERY.to_string().c_str());
            result = false;
        } else {
            this->char_handle_battery_ = chr->handle;
        }
    }

    if(!this->cccd_rx_) {
        desc = this->parent_->get_config_descriptor(this->char_handle_rx_);
        if(desc == nullptr) {
            ESP_LOGW(TAG, "No config descriptor for status handle 0x%x. Will not be able to receive notifications",
                this->char_handle_rx_);
            result = false;
        } else if(desc-> uuid.get_uuid().len != ESP_UUID_LEN_16 ||
                  desc->uuid.get_uuid().uuid.uuid16 != ESP_GATT_UUID_CHAR_CLIENT_CONFIG) {
            ESP_LOGW(TAG, "Config descriptor 0x%x (uuid %s) is not a client config char uuid", this->char_handle_rx_,
                  desc->uuid.to_string().c_str());
            result = false;
        } else {
            this->cccd_rx_ = desc->handle;
        }
    }

    if(!this->cccd_battery_) {
        desc = this->parent_->get_config_descriptor(this->char_handle_battery_);
        if(desc == nullptr) {
            ESP_LOGW(TAG, "No config descriptor for status handle 0x%x. Will not be able to receive notifications",
                this->char_handle_battery_);
            result = false;
        } else if(desc-> uuid.get_uuid().len != ESP_UUID_LEN_16 ||
                  desc->uuid.get_uuid().uuid.uuid16 != ESP_GATT_UUID_CHAR_CLIENT_CONFIG) {
            ESP_LOGW(TAG, "Config descriptor 0x%x (uuid %s) is not a client config char uuid", this->char_handle_battery_,
                  desc->uuid.to_string().c_str());
            result = false;
        } else {
            this->cccd_battery_ = desc->handle;
        }
    }

    return result;
}

void fitpro::enable_notifications() {
      auto status = esp_ble_gattc_register_for_notify(this->parent()->get_gattc_if(),
                                                      this->parent_->get_remote_bda(), this->char_handle_rx_);
      if(status) {
        ESP_LOGW(TAG, "Registering for notification failed on %s", UUID_CHARACTERISTIC_RX.to_string().c_str());
      }


      status = esp_ble_gattc_register_for_notify(this->parent_->get_gattc_if(),
                                                      this->parent_->get_remote_bda(), this->char_handle_battery_);
      if(status) {
        ESP_LOGW(TAG, "Registering for notification failed on %s", UUID_CHARACTERISTIC_BATTERY.to_string().c_str());
      }
}

void fitpro::write_notify_config_descriptor(bool enable) {
    if(!this->cccd_rx_ || !this->cccd_battery_) {
        ESP_LOGW(TAG, "No descriptor found for notify of handle 0x%x", this->cccd_rx_);
        return;
    }

    uint16_t notify_en = enable ? 1 : 0;
    auto status = esp_ble_gattc_write_char_descr(this->parent_->get_gattc_if(), this->parent_->get_conn_id(), 
                                                 this->cccd_rx_, sizeof(notify_en), (uint8_t *) &notify_en, ESP_GATT_WRITE_TYPE_RSP,
                                                 ESP_GATT_AUTH_REQ_NONE);

    if(status) {
        ESP_LOGW(TAG, "esp_ble_gattc_write_char_descr error, status=%d", status);
    }


    status = esp_ble_gattc_write_char_descr(this->parent_->get_gattc_if(), this->parent_->get_conn_id(), 
                                                 this->cccd_battery_, sizeof(notify_en), (uint8_t *) &notify_en, ESP_GATT_WRITE_TYPE_RSP,
                                                 ESP_GATT_AUTH_REQ_NONE);

    if(status) {
        ESP_LOGW(TAG, "esp_ble_gattc_write_char_descr error, status=%d", status);
    }
}

void fitpro::send_data(uint8_t cmd_group, uint8_t cmd) {
    this->send_data(cmd_group, cmd, std::vector<uint8_t>());
}

void fitpro::send_data(uint8_t cmd_group, uint8_t cmd, uint8_t val) {
    this->send_data(cmd_group, cmd, std::vector<uint8_t>(1, val));
}

void fitpro::send_data(uint8_t cmd_group, uint8_t cmd, std::vector<uint8_t> val) {
    uint8_t data[8 + val.size()];

    data[0] = 0xcd;
    data[1] = (((8 + val.size() - 3) >> 8) & 0xff);
    data[2] = ((8 + val.size() - 3) & 0xff);
    data[3] = cmd_group;
    data[4] = 0x01;
    data[5] = cmd;
    data[6] = ((val.size() >> 8 ) & 0xff);
    data[7] = (val.size() & 0xff);
    //data_template.insert(data_template.end(), val.begin(), val.end());
    for(int i=0; i<val.size(); i++)
        data[i+8] = val[i];
    
    auto status = esp_ble_gattc_write_char(this->parent_->get_gattc_if(), this->parent_->get_conn_id(), 
          this->char_handle_tx_, (8 + val.size()), data, ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);
    if(status) {
          ESP_LOGW(TAG, "[%s] esp_ble_gattc_write_char failed, status=%d", this->parent_->address_str().c_str(),
               status);
    }
    for(int i=0; i < 8 + val.size(); i++) {
        ESP_LOGD(TAG, "[%s] write data: handle=0x%x, value[%d]=0x%x", this->parent_->address_str().c_str(),
               this->char_handle_tx_, i, data[i]);
    }
}

void fitpro::send_ack(uint8_t cmd_group, uint8_t len_hi, uint8_t len_low, uint8_t cmd) {
    ESP_LOGD(TAG, "send_ack: 0x%x 0x%x 0x%x 0x%x", cmd_group, len_hi, len_low, cmd);
    uint16_t size = ((len_hi << 8) | len_low) + 3;
    uint8_t data[8];
    
    data[0] = 0xdc;
    data[1] = 0x00;
    data[2] = 0x05;
    data[3] = cmd_group;
    data[4] = 0x01;
    data[5] = (size << 8) & 0xff;
    data[6] = size & 0xff;
    data[7] = 0x01;

    auto status = esp_ble_gattc_write_char(this->parent_->get_gattc_if(), this->parent_->get_conn_id(), 
          this->char_handle_tx_, 8, data, ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);
    if(status) {
          ESP_LOGW(TAG, "[%s] esp_ble_gattc_write_char failed, status=%d", this->parent_->address_str().c_str(),
               status);
    }
    for(int i=0; i < 8; i++) {
        ESP_LOGD(TAG, "[%s] write data: handle=0x%x, value[%d]=0x%x", this->parent_->address_str().c_str(),
               this->char_handle_tx_, i, data[i]);
    }
    this->last_command_ = millis();
    
}

void fitpro::trigger_find_device() {
  if(this->node_state == espbt::ClientState::ESTABLISHED && this->next_cmd_ == FITPRO_CMD_NONE)
    this->next_cmd_ = FITPRO_CMD_FIND_DEVICE;
}

void fitpro::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
  if(this->parent_->get_gattc_if() != gattc_if) // Event is not for us
    return;

  switch (event) {
    case ESP_GATTC_OPEN_EVT: {
      ESP_LOGD(TAG, "ESP_GATTC_OPEN_EVT");
      if(this->status_) {
        this->status_->publish_state("Connecting");
      }
      break;
    }
    case ESP_GATTC_DISCONNECT_EVT: {
      ESP_LOGD(TAG, "ESP_GATTC_DISCONNECT_EVT");
      if(this->status_) {
        this->status_->publish_state("Disconnected");
      }
      if(this->battery_sensor_) {
        this->battery_sensor_->publish_state(NAN);
      }
      break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT: {
      ESP_LOGD(TAG, "ESP_GATTC_SEARCH_COMPL_EVT");
      if(!this->discover_characteristics()) {
        ESP_LOGE(TAG, "Some required characteristics were not found.");
      } else {
        this->enable_notifications();
      }
      break;
    }
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
      // This event means that ESP received the request to enable notifications on the client side. But we also have to
      // tell the server that we want it to send notifications. Normally BLEClient parent would handle this
      // automatically, but as soon as we set our status to Established, the parent is going to purge all the
      // service/char/descriptor handles, and then get_config_descriptor() won't work anymore. There's no way to disable
      // the BLEClient parent behavior, so our only option is to write the handle anyway, and hope a double-write
      // doesn't break anything.

      ESP_LOGD(TAG, "ESP_GATTC_REG_FOR_NOTIFY_EVT");
      if(this->node_state != espbt::ClientState::ESTABLISHED) {
          this->node_state = espbt::ClientState::ESTABLISHED;
          this->write_notify_config_descriptor(true);
          this->last_command_ = millis();
      }
      ESP_LOGD(TAG, "status: %d, handle=0x%x", param->reg_for_notify.status, param->reg_for_notify.handle);
      if(param->reg_for_notify.handle == char_handle_rx_) {
          this->next_cmd_ = FITPRO_CMD_INIT_STAGE_1;
          if(this->status_) {
              this->status_->publish_state("Connected");
          }
      }
      break;
    }
    case ESP_GATTC_UNREG_FOR_NOTIFY_EVT: {
      ESP_LOGD(TAG, "ESP_GATTC_UNREG_FOR_NOTIFY_EVT");
      break;
    }
    case ESP_GATTC_WRITE_DESCR_EVT: {
      ESP_LOGD(TAG, "ESP_GATTC_WRITE_DESCR_EVT");
      break;
    }
    case ESP_GATTC_WRITE_CHAR_EVT: {
      ESP_LOGD(TAG, "ESP_GATTC_WRITE_CHAR_EVT");
      break;
    }
    case ESP_GATTC_NOTIFY_EVT: {
      ESP_LOGD(TAG, "ESP_GATTC_NOTIFY_EVT");
      if(param->notify.conn_id != this->parent_->get_conn_id())
          break;
      
      if(param->notify.handle != this->char_handle_rx_ && param->notify.handle != this->char_handle_battery_)
          break;
      for(int i=0; i<param->notify.value_len; i++) {
        ESP_LOGD(TAG, "[%s] ESP_GATTC_NOTIFY_EVT: handle=0x%x, value[%d]=0x%x", this->parent_->address_str().c_str(),
               param->notify.handle, i, param->notify.value[i]);
      }
      if(param->notify.handle == this->char_handle_rx_) {
        if(param->notify.value[0] == 0xcd) {
        // This requires an ACK to be sent
          if(param->notify.value_len <= 5)
            return;
        
          this->send_ack(param->notify.value[3], param->notify.value[1], param->notify.value[2], param->notify.value[5]);
        }
      }
      if(param->notify.handle == this->char_handle_battery_) {
        float val = param->notify.value[0];
        if(this->battery_sensor_) {
            this->battery_sensor_->publish_state(val);
        } 
      }
    }
    default:
      break;
  }
}

}  // namespace fitpro 
}  // namespace esphome

#endif

