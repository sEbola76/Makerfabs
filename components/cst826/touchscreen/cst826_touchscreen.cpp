#include "cst826_touchscreen.h"

namespace esphome {
namespace cst826 {

void CST826Touchscreen::continue_setup_() {
  if (this->interrupt_pin_ != nullptr) {
    this->interrupt_pin_->setup();
    this->attach_interrupt_(this->interrupt_pin_, gpio::INTERRUPT_FALLING_EDGE);
  }
  if (!this->skip_probe_) {
    if (this->read_byte(REG_CHIP_ID, &this->chip_id_)) {
      switch (this->chip_id_) {
        case CST820_CHIP_ID:
        case CST826_CHIP_ID:
        case CST716_CHIP_ID:
        case CST816S_CHIP_ID:
        case CST816D_CHIP_ID:
        case CST816T_CHIP_ID:
          break;
        default:
          this->mark_failed();
          this->status_set_error(str_sprintf("Unknown chip ID 0x%02X", this->chip_id_).c_str());
          return;
      }
      this->write_byte(REG_IRQ_CTL, IRQ_EN_MOTION);  
    }
  }
  
  if (this->x_raw_max_ == this->x_raw_min_) {
    this->x_raw_max_ = this->display_->get_native_width();
  }
  if (this->y_raw_max_ == this->y_raw_min_) {
    this->y_raw_max_ = this->display_->get_native_height();
  }
  ESP_LOGCONFIG(TAG, "CST826 Touchscreen setup complete");
}

void CST826Touchscreen::setup() {
  ESP_LOGCONFIG(TAG, "Setting up CST826 Touchscreen...");
  if (this->reset_pin_ != nullptr) {
    this->reset_pin_->setup();
    this->reset_pin_->digital_write(true);
    delay(5);
    this->reset_pin_->digital_write(false);
    delay(5);
    this->reset_pin_->digital_write(true);
    this->set_timeout(30, [this] { this->continue_setup_(); });
  } else {
    this->continue_setup_();
  }
}

void CST826Touchscreen::update_touches() {
  uint8_t data[13];
  //uint8_t data[7];
  if (!this->read_bytes(REG_STATUS, data, sizeof data)) {
    this->status_set_warning();
    return;
  }
  uint8_t num_of_touches = data[REG_TOUCH_NUM] & 3;
  if (num_of_touches == 0) {
    return;
  }
  // int event = data[1] >> 6;
  // if (event != 0) {
  //   ESP_LOGV(TAG, "No touch");
  //   return;
  // }

  //int x = (int)data[2] + (int)(data[1] & 0x0f) * 256;
  //int y = (int)data[4] + (int)(data[3] & 0x0f) * 256;
  uint16_t x = encode_uint16(data[REG_XPOS_HIGH] & 0xF, data[REG_XPOS_LOW]);
  uint16_t y = encode_uint16(data[REG_YPOS_HIGH] & 0xF, data[REG_YPOS_LOW]);
  ESP_LOGV(TAG, "Read touch %d/%d", x, y);
  this->add_raw_touch_position_(0, x, y);
}

void CST826Touchscreen::dump_config() {
  ESP_LOGCONFIG(TAG, "CST826 Touchscreen:");
  LOG_I2C_DEVICE(this);
  LOG_PIN("  Interrupt Pin: ", this->interrupt_pin_);
  LOG_PIN("  Reset Pin: ", this->reset_pin_);
  ESP_LOGCONFIG(TAG, "  X Raw Min: %d, X Raw Max: %d", this->x_raw_min_, this->x_raw_max_);
  ESP_LOGCONFIG(TAG, "  Y Raw Min: %d, Y Raw Max: %d", this->y_raw_min_, this->y_raw_max_);
  const char *name;
  switch (this->chip_id_) {
    case CST820_CHIP_ID:
      name = "CST820";
      break;
    case CST826_CHIP_ID:
      name = "CST826";
      break;
    case CST816S_CHIP_ID:
      name = "CST816S";
      break;
    case CST816D_CHIP_ID:
      name = "CST816D";
      break;
    case CST716_CHIP_ID:
      name = "CST716";
      break;
    case CST816T_CHIP_ID:
      name = "CST816T";
      break;
    default:
      name = "Unknown";
      break;
  }
  ESP_LOGCONFIG(TAG, "  Chip type: %s", name);
}

}  // namespace cst826
}  // namespace esphome