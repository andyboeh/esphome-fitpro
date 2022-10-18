#pragma once

#include "esphome/core/component.h"
#include "esphome/components/button/button.h"
#include "esphome/components/fitpro/fitpro.h"

namespace esphome {
namespace fitpro {

class FitProButton : public button::Button, public Component {
 public:
  void dump_config() override;
  void set_fitpro_parent(fitpro *parent) { this->parent_ = parent; }

 protected:
  void press_action() override;

  fitpro *parent_{nullptr};
};

}  // namespace fitpro
}  // namespace esphome
