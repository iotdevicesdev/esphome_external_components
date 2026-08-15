#pragma once

#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace ggreg20_v3 {

// Sensor classes used by Python codegen
class DosePowerSensor : public sensor::Sensor {};
class EquivDoseSensor : public sensor::Sensor {};
class TotalDoseSensor : public sensor::Sensor {};
class CPMSensor : public sensor::Sensor {};
class CountSensor : public sensor::Sensor {};
class StatusSensor : public text_sensor::TextSensor {};

class GGreg20V3Component : public Component {
 public:
  // Setters (from YAML)
  void set_pin(InternalGPIOPin *pin) { this->pin_ = pin; }
  void set_measurement_period_sec(int period) { this->measurement_period_sec_ = period; }
  void set_dead_time_us(int dead_time) { this->dead_time_us_ = dead_time; }
  void set_int_logic(int logic) { this->int_logic_ = logic; }
  void set_dose_power_factor(float factor) { this->dose_power_factor_ = factor; }
  void set_equiv_dose_factor(float factor) { this->equiv_dose_factor_ = factor; }

  // Sensor object setters
  void set_dose_power_sensor(DosePowerSensor *s) { this->dose_power_sensor_ = s; }
  void set_equiv_dose_sensor(EquivDoseSensor *s) { this->equiv_dose_sensor_ = s; }
  void set_total_dose_sensor(TotalDoseSensor *s) { this->total_dose_sensor_ = s; }
  void set_cpm_sensor(CPMSensor *s) { this->cpm_sensor_ = s; }
  void set_count_sensor(CountSensor *s) { this->count_sensor_ = s; }
  void set_status_sensor(StatusSensor *s) { this->status_sensor_ = s; }

  // ESPHome lifecycle
  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return esphome::setup_priority::HARDWARE; }

  // ISR
  static void IRAM_ATTR gpio_interrupt(GGreg20V3Component *obj);

 protected:
  // Config
  InternalGPIOPin *pin_{nullptr};
  int measurement_period_sec_{60};
  int dead_time_us_{190};
  int int_logic_{0};  // 0 = Active-Low, 1 = Active-High
  float dose_power_factor_{0.00378f};
  float equiv_dose_factor_{0.00332f};

  // Runtime
  volatile uint32_t pulse_count_ = 0;
  uint32_t last_pulse_time_us_ = 0;
  uint32_t last_update_time_ms_ = 0;
  float total_accumulated_dose_ = 0.0f;
  bool first_measurement_ = true;
  uint32_t last_published_pulse_count_ = 0;

  // Sensors
  DosePowerSensor *dose_power_sensor_{nullptr};
  EquivDoseSensor *equiv_dose_sensor_{nullptr};
  TotalDoseSensor *total_dose_sensor_{nullptr};
  CPMSensor *cpm_sensor_{nullptr};
  CountSensor *count_sensor_{nullptr};
  StatusSensor *status_sensor_{nullptr};

  // Calculations
  void publish_data(uint32_t pulses);
  void update_instant_count();  // Updates pulse count display
};

}  // namespace ggreg20_v3
}  // namespace esphome
