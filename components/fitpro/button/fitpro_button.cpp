#include "fitpro_button.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace fitpro {

static const char *const TAG = "fitpro.button";

void FitProButton::dump_config() { LOG_BUTTON("", "FitPro Button", this); }
void FitProButton::press_action() {
  ESP_LOGI(TAG, "Triggering find device...");
  if(this->parent_)
    this->parent_->trigger_find_device();
}

}  // namespace fitpro
}  // namespace esphome
