#include "growatt_rs232.h"
#include "esphome/core/log.h"

namespace esphome {
namespace growatt_rs232 {

static const char *const TAG = "growatt_rs232";

static const float TWO_DEC_UNIT = 0.01;
static const float ONE_DEC_UNIT = 0.1;
static const float ZERO_DEC_UNIT = 1;

static const uint8_t START_MARKER = 0x57;

/* Build read command 0x31, 0x35, 0x30, 0x30 represents update internal of (1000ms)*/
static const uint8_t GROWATT_INIT_COMMAND[] = {0x3F, 0x23, 0x7E, 0x34, 0x41, 0x7E, 0x32,
                                               0x59, 0x31, 0x30, 0x30, 0x30, 0x23, 0x3F};

static const uint8_t GROWATT_START_COMMAND[] = {0x3F, 0x23, 0x7E, 0x34, 0x41, 0x7E, 0x32,
                                                0x59, 0x31, 0x35, 0x30, 0x30, 0x23, 0x3F};

void Growatt_RS232::setup() {
  ESP_LOGD(TAG, "Starting Growatt innitalisation sequence");
  this->start_requesting_data_();
}

void Growatt_RS232::update() {
  this->receive_data_frame_();

  /* Timeout reached when trying to read byte, restarting initalisation */
  if (this->receive_timeout_reached_()) {
    ESP_LOGW(TAG, "Timeout: growatt not responding (powered down after sunset perhaps?)");
    this->start_requesting_data_();
  }
}

void Growatt_RS232::dump_config() {
  ESP_LOGCONFIG(TAG, "Growatt RS232:");
  ESP_LOGCONFIG(TAG, "  Receive timeout: %.1fs", this->receive_timeout_ / 1e3f);
}

void Growatt_RS232::reset_data_frame_() {
  this->data_frame_bytes_read_ = 0;
  memset(this->inverter_data_frame_, 0, sizeof(this->inverter_data_frame_));
}

void Growatt_RS232::start_requesting_data_() {
  ESP_LOGD(TAG, "Request to init controler");
  this->write_array(GROWATT_INIT_COMMAND, sizeof(GROWATT_INIT_COMMAND));

  /**
   * Check if command was received property. Inverter (serial) is powered
   * down when no DC voltage is active (e.g. at night)
   */
  ESP_LOGD(TAG, "Initalisation command returned bytes: %i", this->available());
  if (this->available() <= 5) {
    return;
  }

  /* Reset last received timer */
  this->last_read_time_ = millis();

  /* Request interval data */
  this->write_array(GROWATT_START_COMMAND, sizeof(GROWATT_START_COMMAND));

  /* Reset partially complete data frame */
  this->reset_data_frame_();
}

void Growatt_RS232::receive_data_frame_() {
  /**
   * The loop is not called in sync with the incoming data-frames, hence
   * we need buffer the incoming frame until this frame is completed. Since we
   * are requesting data-frames to be sent every 1 second which is (way) higher
   * than request_interval setting a number of data-frames get discarded.
   */
  byte incoming_byte;

  while (this->available()) {
    /* We have received data, refresh keep-alive timer. */
    this->last_read_time_ = millis();

    if (this->read_byte(&incoming_byte) == false) {
      ESP_LOGE(TAG, "Unable to read available serial bytes from bus");
    }

    /* Find out if we are currently builing an data-frame */
    if (this->inverter_data_frame_[0] == START_MARKER) {
      /* Write to new data-frame and advance to next position */
      this->inverter_data_frame_[this->data_frame_bytes_read_] = incoming_byte;
      this->data_frame_bytes_read_++;

      /* Check if data-frame is complete */
      if (data_frame_bytes_read_ == sizeof(this->inverter_data_frame_)) {
        this->publish_data_frame_();
        /* Mark incoming data-frame complete */
        this->reset_data_frame_();
      }
    }
    /* Check for incoming start marker (e.g new data-frame) */
    else if (incoming_byte == START_MARKER) {
      this->data_frame_bytes_read_ = 1;
      this->inverter_data_frame_[0] = START_MARKER;
    } else {
      /* Discard un-classified byte */
    }
  }
}

void Growatt_RS232::publish_data_frame_() {
  ESP_LOGD(TAG, "Processing completed data-frame");
  if (this->request_interval_reached_() == false) {
    /* Ignore completed data-frame since no window to send data-frame yet */
    ESP_LOGD(TAG, "Discard data-frame since it's ready before next available publish window");
    return;
  }

  auto publish_1_byte_sensor_state = [&](sensor::Sensor *sensor, size_t i) -> void {
    if (sensor == nullptr)
      return;
    float value = this->inverter_data_frame_[i];
    sensor->publish_state(value);
  };

  auto publish_2_byte_sensor_state = [&](sensor::Sensor *sensor, size_t i, float unit) -> void {
    if (sensor == nullptr)
      return;
    float value = encode_uint16(this->inverter_data_frame_[i], this->inverter_data_frame_[i + 1]) * unit;
    sensor->publish_state(value);
  };

  auto publish_4_byte_sensor_state = [&](sensor::Sensor *sensor, size_t i, float unit) -> void {
    if (sensor == nullptr)
      return;
    float value = encode_uint32(this->inverter_data_frame_[i], this->inverter_data_frame_[i + 1],
                                this->inverter_data_frame_[i + 2], this->inverter_data_frame_[i + 3]) *
                  unit;
    sensor->publish_state(value);
  };

  publish_2_byte_sensor_state(this->pv1_sensor_, 1, ONE_DEC_UNIT);
  publish_2_byte_sensor_state(this->pv2_sensor_, 5, ONE_DEC_UNIT);
  publish_2_byte_sensor_state(this->grid_voltage_sensor_, 7, ONE_DEC_UNIT);
  publish_2_byte_sensor_state(this->grid_frequency_sensor_, 9, TWO_DEC_UNIT);
  publish_2_byte_sensor_state(this->pv_active_power_sensor_, 11, ONE_DEC_UNIT);

  publish_2_byte_sensor_state(this->inverter_module_temp_sensor_, 13, ONE_DEC_UNIT);

  publish_1_byte_sensor_state(this->inverter_status_sensor_, 15);
  publish_1_byte_sensor_state(this->inverter_fault_code_sensor_, 16);

  publish_2_byte_sensor_state(this->today_production_sensor_, 21, ONE_DEC_UNIT);
  publish_4_byte_sensor_state(this->total_energy_production_sensor_, 23, ONE_DEC_UNIT);
  publish_4_byte_sensor_state(this->total_generation_time_sensor_, 27, ZERO_DEC_UNIT);

  /* Tag time of newly published entries */
  this->last_publish_time_ = millis();
}
}  // namespace growatt_rs232
}  // namespace esphome
