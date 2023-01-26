#include "esphome.h"
#include "esphome/components/esp32_ble/ble.h"
#include "esphome/components/esp32_ble_server/ble_characteristic.h"
# include <cstring>

static const char *const TAG = "ble_data";

class BLEDataComponent : public Component {
 public:
  BLEDataComponent(esp32_ble_server::BLEServer *server, template_::TemplateCover *cover) : Component() {
    this->server_ = server;
    this->cover_ = cover;
  }
  
  void setup() override {
  }
  
  void loop() override {
    switch (this->state_) {
    case RUNNING:
      return;

    case INIT: {
      if (this->server_->can_proceed()) {
        this->state_ = REGISTERING;
      }      
      break;
    }
    case REGISTERING: {
      this->service_ = this->server_->create_service(0x181C, true); // user data
      
      this->rpc_characteristic_ = this->service_->create_characteristic(0xE200, esp32_ble_server::BLECharacteristic::PROPERTY_WRITE);
      this->rpc_characteristic_->on_write([this](const std::vector<uint8_t> &data) {
        if (!data.empty()) {
          std::string str_data;
          str_data.assign(data.begin(), data.end());
          ESP_LOGD(TAG, "incoming BLE data %s", str_data.c_str());
          
          if(str_data.compare("open") == 0) {
            auto call = this->cover_->make_call();  
            call.set_command_open();
            call.perform();
          }
          if(str_data.compare("close") == 0) {
            auto call = this->cover_->make_call();  
            call.set_command_close();
            call.perform();
          }
        }
      });
      esp32_ble_server::BLEDescriptor *rpc_descriptor = new esp32_ble_server::BLE2902();
      this->rpc_characteristic_->add_descriptor(rpc_descriptor);
      
      this->is_open_characteristic_ = this->service_->create_characteristic(0xE201, esp32_ble_server::BLECharacteristic::PROPERTY_READ); // boolean
      this->is_close_characteristic_ = this->service_->create_characteristic(0xE202, esp32_ble_server::BLECharacteristic::PROPERTY_READ); // boolean
      this->state_ = STARTING_SERVICE;
      break;
    }
    case STARTING_SERVICE: {
      if (!this->service_->is_created()) {
        break;
      }
      if (this->service_->is_running()) {
        this->state_ = RUNNING;
        ESP_LOGD(TAG, "BLE data setup successfully");
      } else if (!this->service_->is_starting()) {
        this->service_->start();
      }
      break;
    }
  }
  }
  
  void set_is_open(bool value) {
    if(this->is_open_characteristic_) {
      std::string val_str = esphome::to_string(value);
      ESP_LOGD(TAG, "BLE data set is_open to %s", val_str.c_str());
      this->is_open_characteristic_->set_value(value);    
    }
  }

  void set_is_close(bool value) {
    if(this->is_close_characteristic_) {
      std::string val_str = esphome::to_string(value);
      ESP_LOGD(TAG, "BLE data set is_close to %s", val_str.c_str());
      this->is_close_characteristic_->set_value(value);    
    }
  }
  
 protected:
  esp32_ble_server::BLEServer *server_{nullptr};
  template_::TemplateCover *cover_{nullptr};
  std::shared_ptr<esp32_ble_server::BLEService> service_{nullptr};
  esphome::esp32_ble_server::BLECharacteristic *is_open_characteristic_{nullptr};
  esphome::esp32_ble_server::BLECharacteristic *is_close_characteristic_{nullptr};
  esphome::esp32_ble_server::BLECharacteristic *rpc_characteristic_{nullptr};
  
  enum State : uint8_t {
    INIT = 0x00,
    REGISTERING,
    STARTING_SERVICE,
    RUNNING,
  } state_{INIT};
};
