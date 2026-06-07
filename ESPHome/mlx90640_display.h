#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/sensor/sensor.h"
#include <Wire.h>

// MLX90640 library from Melexis / sparkfun
// Must be added to esphome libraries section:
//   - https://github.com/sparkfun/SparkFun_MLX90640_Arduino_Example.git
#include <SparkFun_MLX90640_Arduino_Library.h>

namespace esphome {
namespace mlx90640_display {

static const char *const TAG = "mlx90640";

// Number of pixels in the MLX90640 array
static const int MLX90640_PIXEL_COUNT = 768; // 32 * 24
static const int MLX90640_COLS = 32;
static const int MLX90640_ROWS = 24;

class MLX90640DisplayComponent : public PollingComponent {
 public:
  // ---- Configuration setters (called from YAML) ----
  void set_sda_pin(uint8_t pin) { sda_pin_ = pin; }
  void set_scl_pin(uint8_t pin) { scl_pin_ = pin; }
  void set_i2c_frequency(uint32_t freq) { i2c_frequency_ = freq; }
  void set_min_temp(float t) { min_temp_ = t; }
  void set_max_temp(float t) { max_temp_ = t; }
  void set_refresh_rate(uint8_t r) { refresh_rate_ = r; }

  // Optional HA sensor outputs
  void set_min_temperature_sensor(sensor::Sensor *s) { min_sensor_ = s; }
  void set_max_temperature_sensor(sensor::Sensor *s) { max_sensor_ = s; }
  void set_mean_temperature_sensor(sensor::Sensor *s) { mean_sensor_ = s; }

  // ---- Lifecycle ----
  void setup() override {
    ESP_LOGI(TAG, "Initialising MLX90640 on SDA=%d SCL=%d at %uHz", sda_pin_, scl_pin_, i2c_frequency_);
    Wire.begin(sda_pin_, scl_pin_);
    Wire.setClock(i2c_frequency_);

    if (!camera_.begin() ) {
      ESP_LOGE(TAG, "MLX90640 not found — check wiring and I2C address!");
      mark_failed();
      return;
    }

    // Set the refresh rate (default 0x04 = 8 Hz)
    camera_.setRefreshRate(refresh_rate_);
    ESP_LOGI(TAG, "MLX90640 ready. Refresh rate register=0x%02X", refresh_rate_);

    // Pre-fill frame buffer with midpoint temp so display isn't garbage before first read
    for (int i = 0; i < MLX90640_PIXEL_COUNT; i++) {
      frame_[i] = (min_temp_ + max_temp_) / 2.0f;
    }
  }

  void update() override {
    if (is_failed()) return;

    if (camera_.getFrame(frame_) != 0) {
      ESP_LOGW(TAG, "MLX90640 getFrame failed — sensor busy?");
      return;
    }

    // Compute statistics and push to HA sensors
    float mn = frame_[0], mx = frame_[0], sum = 0.0f;
    for (int i = 0; i < MLX90640_PIXEL_COUNT; i++) {
      if (frame_[i] < mn) mn = frame_[i];
      if (frame_[i] > mx) mx = frame_[i];
      sum += frame_[i];
    }
    float mean = sum / MLX90640_PIXEL_COUNT;

    if (min_sensor_)  min_sensor_->publish_state(mn);
    if (max_sensor_)  max_sensor_->publish_state(mx);
    if (mean_sensor_) mean_sensor_->publish_state(mean);

    // Update auto-range if enabled (clamp to configured limits)
    min_detected_ = mn;
    max_detected_ = mx;

    ESP_LOGD(TAG, "Frame OK: min=%.1f max=%.1f mean=%.1f", mn, mx, mean);
  }

  // ---- Accessors used by the display lambda ----

  // Raw float temperature for pixel (col, row) — 0-indexed
  float get_pixel(int col, int row) const {
    return frame_[row * MLX90640_COLS + col];
  }

  // Full frame buffer pointer (32*24 floats)
  const float* get_frame() const { return frame_; }

  float get_min_temp()      const { return min_temp_; }
  float get_max_temp()      const { return max_temp_; }
  float get_min_detected()  const { return min_detected_; }
  float get_max_detected()  const { return max_detected_; }

  // ---- Color mapping: float temp → esphome::Color ----
  // Uses the classic "iron" / "inferno" palette:
  // black → blue → cyan → green → yellow → red → white
  static esphome::Color temp_to_color(float temp, float t_min, float t_max) {
    // Clamp & normalise to 0..1
    float v = (temp - t_min) / (t_max - t_min);
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;

    uint8_t r, g, b;

    // 5-stop gradient:  0.0=black  0.25=blue  0.5=cyan/green  0.75=yellow  1.0=red/white
    if (v < 0.25f) {
      // black → blue
      float t = v / 0.25f;
      r = 0;
      g = 0;
      b = (uint8_t)(t * 255);
    } else if (v < 0.5f) {
      // blue → cyan
      float t = (v - 0.25f) / 0.25f;
      r = 0;
      g = (uint8_t)(t * 255);
      b = 255;
    } else if (v < 0.75f) {
      // cyan → yellow (through green)
      float t = (v - 0.5f) / 0.25f;
      r = (uint8_t)(t * 255);
      g = 255;
      b = (uint8_t)((1.0f - t) * 255);
    } else {
      // yellow → red → white
      float t = (v - 0.75f) / 0.25f;
      r = 255;
      g = (uint8_t)((1.0f - t) * 255);
      b = (uint8_t)(t * 128); // slight white tinge at top
    }

    return esphome::Color(r, g, b);
  }

 protected:
  // Hardware
  uint8_t  sda_pin_{21};
  uint8_t  scl_pin_{22};
  uint32_t i2c_frequency_{400000};
  uint8_t  refresh_rate_{0x04};  // 8 Hz

  // Temperature range for colour mapping
  float min_temp_{15.0f};
  float max_temp_{40.0f};

  // Detected extremes (last frame)
  float min_detected_{0.0f};
  float max_detected_{0.0f};

  // Pixel data — 32*24 floats, degrees Celsius
  float frame_[MLX90640_PIXEL_COUNT]{};

  // MLX90640 driver
  SparkFun_MLX90640 camera_;

  // Optional HA sensor outputs
  sensor::Sensor *min_sensor_{nullptr};
  sensor::Sensor *max_sensor_{nullptr};
  sensor::Sensor *mean_sensor_{nullptr};
};

}  // namespace mlx90640_display
}  // namespace esphome
