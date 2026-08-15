#include "ggreg20_v3_component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include <inttypes.h>
#include <string>

static const char *const TAG = "ggreg20_v3";

namespace esphome {
namespace ggreg20_v3 {

void IRAM_ATTR GGreg20V3Component::gpio_interrupt(GGreg20V3Component *obj) {
  uint32_t now_us = micros();

  // Dead-time filtering
  if (now_us - obj->last_pulse_time_us_ < static_cast<uint32_t>(obj->dead_time_us_)) {
    return;
  }

  obj->pulse_count_++;
  obj->last_pulse_time_us_ = now_us;
}

void GGreg20V3Component::setup() {
  this->pin_->setup();

  // Configure interrupt based on int_logic setting
  gpio::InterruptType interrupt_type;
  if (this->int_logic_ == 0) {
    // Active-Low: trigger on falling edge
    interrupt_type = gpio::INTERRUPT_FALLING_EDGE;
  } else {
    // Active-High: trigger on rising edge
    interrupt_type = gpio::INTERRUPT_RISING_EDGE;
  }
  
  this->pin_->attach_interrupt(GGreg20V3Component::gpio_interrupt, this, interrupt_type);

  this->last_update_time_ms_ = millis();

  // Fix for ESPHome GPIO dump_summary API change
  char pin_summary[64];
  this->pin_->dump_summary(pin_summary, sizeof(pin_summary));

  ESP_LOGCONFIG(TAG, "GGreg20_V3 Sensor setup complete. Pin: %s, Logic: %s", 
                pin_summary,
                this->int_logic_ == 0 ? "Active-Low" : "Active-High");
}

void GGreg20V3Component::loop() {
  const uint32_t period_ms = static_cast<uint32_t>(this->measurement_period_sec_) * 1000U;

  // Check for new pulses and update instant CPM
  uint32_t current_pulse_count;
  {
    InterruptLock lock;
    current_pulse_count = this->pulse_count_;
  }
  
  if (current_pulse_count != this->last_published_pulse_count_) {
    this->update_instant_count();
    this->last_published_pulse_count_ = current_pulse_count;
  }

  // Check if measurement period is complete
  if (millis() - this->last_update_time_ms_ >= period_ms) {
    uint32_t pulses;
    {
      InterruptLock lock;
      pulses = this->pulse_count_;
      this->pulse_count_ = 0;
    }

    this->publish_data(pulses);
    this->last_update_time_ms_ = millis();
    this->last_published_pulse_count_ = 0; // Reset for new period
  }
}

void GGreg20V3Component::publish_data(uint32_t pulses) {
  const float time_in_minutes = static_cast<float>(this->measurement_period_sec_) / 60.0f;

  // Check if this is the first measurement
  if (this->first_measurement_) {
    if (this->status_sensor_ != nullptr) {
      this->status_sensor_->publish_state("Wait, collecting data");
    }
    
    if (this->dose_power_sensor_ != nullptr) {
      this->dose_power_sensor_->publish_state(0.0f);
    }
    if (this->equiv_dose_sensor_ != nullptr) {
      this->equiv_dose_sensor_->publish_state(0.0f);
    }
    if (this->cpm_sensor_ != nullptr) {
      this->cpm_sensor_->publish_state(0.0f);
    }
    if (this->count_sensor_ != nullptr) {
      this->count_sensor_->publish_state(0.0f);
    }
    if (this->total_dose_sensor_ != nullptr) {
      this->total_dose_sensor_->publish_state(0.0f);
    }
    
    this->first_measurement_ = false;
    
    ESP_LOGD(TAG, "First measurement period - waiting for data collection");
    return;
  }

  // Normal measurement processing
  const float cpm = static_cast<float>(pulses) / time_in_minutes;
  const float dose_power = cpm * this->dose_power_factor_;
  const float equiv_dose = cpm * this->equiv_dose_factor_;
  
  const float dose_increment = equiv_dose * (time_in_minutes / 60.0f);
  this->total_accumulated_dose_ += dose_increment;

  std::string status;
  if (cpm > 315000.0f) {
    status = "Sensor Overflow Error";
  } else if (dose_power > 0.6f) {
    status = "Danger";
  } else if (dose_power > 0.3f) {
    status = "Warning";
  } else if (cpm >= 3.0f) {
    status = "Normal";
  } else {
    status = "Sensor Error (Low Count)";
  }

  if (this->dose_power_sensor_ != nullptr) {
    this->dose_power_sensor_->publish_state(dose_power);
  }
  if (this->equiv_dose_sensor_ != nullptr) {
    this->equiv_dose_sensor_->publish_state(equiv_dose);
  }
  if (this->total_dose_sensor_ != nullptr) {
    this->total_dose_sensor_->publish_state(this->total_accumulated_dose_);
  }
  if (this->cpm_sensor_ != nullptr) {
    this->cpm_sensor_->publish_state(cpm);
  }
  if (this->count_sensor_ != nullptr) {
    this->count_sensor_->publish_state(0.0f);
  }
  if (this->status_sensor_ != nullptr) {
    this->status_sensor_->publish_state(status);
  }

  // Fixed format string using PRIu32 macro
  ESP_LOGD(TAG, "Pulses: %" PRIu32 ", CPM: %.2f, Power: %.4f uSv/h, Equiv: %.4f uSv/h, Total: %.4f uSv, Status: %s",
           pulses, cpm, dose_power, equiv_dose, this->total_accumulated_dose_, status.c_str());
}

void GGreg20V3Component::update_instant_count() {
  if (this->first_measurement_) {
    return;
  }

  uint32_t active_pulses;
  {
    InterruptLock lock;
    active_pulses = this->pulse_count_;
  }

  if (this->count_sensor_ != nullptr) {
    this->count_sensor_->publish_state(static_cast<float>(active_pulses));
  }

  ESP_LOGV(TAG, "Instant pulse count: %" PRIu32, active_pulses);
}

}  // namespace ggreg20_v3
}  // namespace esphome
