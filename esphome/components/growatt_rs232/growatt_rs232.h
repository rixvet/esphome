#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/log.h"
#include "esphome/core/defines.h"

#include <dsmr/parser.h>
#include <dsmr/fields.h>

#include <vector>

namespace esphome {
namespace growatt_rs232 {

class Growatt_RS232 : public PollingComponent, public uart::UARTDevice {
 public:
  Growatt_RS232() : PollingComponent(1000) {}

  void setup() override;
  void update() override;

  void dump_config() override;

  bool receive_timeout_reached_() { return (millis() - this->last_read_time_) > this->receive_timeout_; }
  bool request_interval_reached_() { return (millis() - this->last_publish_time_) > this->request_interval_; }

  void set_request_interval(uint32_t request_interval) { this->request_interval_ = request_interval; }
  void set_receive_timeout(uint32_t timeout) { this->receive_timeout_ = timeout; }

  void set_pv1_sensor(sensor::Sensor *sensor) { this->pv1_sensor_ = sensor; }
  void set_pv2_sensor(sensor::Sensor *sensor) { this->pv2_sensor_ = sensor; }

  void set_grid_voltage_sensor(sensor::Sensor *sensor) { this->grid_voltage_sensor_ = sensor; }
  void set_grid_frequency_sensor(sensor::Sensor *sensor) { this->grid_frequency_sensor_ = sensor; }
  void set_grid_active_power_sensor(sensor::Sensor *sensor) { this->grid_active_power_sensor_ = sensor; }

  void set_inverter_module_temp_sensor(sensor::Sensor *sensor) { this->inverter_module_temp_sensor_ = sensor; }
  void set_inverter_status_sensor(sensor::Sensor *sensor) { this->inverter_status_sensor_ = sensor; }
  void set_inverter_fault_code_sensor(sensor::Sensor *sensor) { this->inverter_fault_code_sensor_ = sensor; }

  void set_today_production_sensor(sensor::Sensor *sensor) { this->today_production_sensor_ = sensor; }
  void set_total_energy_production_sensor(sensor::Sensor *sensor) { this->total_energy_production_sensor_ = sensor; }
  void set_total_generation_time_sensor(sensor::Sensor *sensor) { this->total_generation_time_sensor_ = sensor; }

 protected:
  void start_requesting_data_();
  void receive_data_frame_();
  void publish_data_frame_();
  void reset_data_frame_();

  uint32_t last_read_time_{0};
  uint32_t last_publish_time_{0};

  uint32_t request_interval_;
  uint32_t receive_timeout_;

  byte inverter_data_frame_[31];
  size_t data_frame_bytes_read_{0};

  sensor::Sensor *pv1_sensor_{nullptr};
  sensor::Sensor *pv2_sensor_{nullptr};

  sensor::Sensor *grid_voltage_sensor_{nullptr};
  sensor::Sensor *grid_frequency_sensor_{nullptr};
  sensor::Sensor *grid_active_power_sensor_{nullptr};

  sensor::Sensor *inverter_status_sensor_{nullptr};
  sensor::Sensor *inverter_module_temp_sensor_{nullptr};
  sensor::Sensor *inverter_fault_code_sensor_{nullptr};

  sensor::Sensor *pv_active_power_sensor_{nullptr};

  sensor::Sensor *today_production_sensor_{nullptr};
  sensor::Sensor *total_energy_production_sensor_{nullptr};
  sensor::Sensor *total_generation_time_sensor_{nullptr};
};

}  // namespace growatt_rs232
}  // namespace esphome
