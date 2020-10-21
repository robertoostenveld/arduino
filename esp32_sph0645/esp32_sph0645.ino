/*
 * Sketch for ESP32 board, like the NodeMCU 32S or the Adafruit Huzzah32
 * connected to a SPH0645 I2S microphone
 * 
 * See https://diyi0t.com/i2s-sound-tutorial-for-esp32/
 * and https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html
 *
 */

#include "driver/i2s.h"

const i2s_port_t I2S_PORT = I2S_NUM_0;

void setup() {
  Serial.begin(115200);
  esp_err_t err;

  // The I2S config as per the example
  const i2s_config_t i2s_config = {
      .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),  // receive, not transfer
      .sample_rate = 48000,                               // sampling rate
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,       // could only get it to work with 32bits
      .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,       // use right channel
      .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,           // interrupt level 1
      .dma_buf_count = 4,                                 // number of buffers
      .dma_buf_len = 8                                    // 8 samples per buffer (minimum)
  };

  // The pin config as per the setup
  const i2s_pin_config_t NOTUSED_pin_config = {
      .bck_io_num = 26,                   // Serial Clock (BCLK on the SPH0645)
      .ws_io_num = 25,                    // Word Select  (LRCL on the SPH0645)
      .data_out_num = I2S_PIN_NO_CHANGE,  // not used     (only for speakers)
      .data_in_num = 33                   // Serial Data  (DOUT on the SPH0645)
  };

  const i2s_pin_config_t pin_config = {
      .bck_io_num = 32,                   // Serial Clock (BCLK on the SPH0645)
      .ws_io_num = 22,                    // Word Select  (LRCL on the SPH0645)
      .data_out_num = I2S_PIN_NO_CHANGE,  // not used     (only for speakers)
      .data_in_num = 14                   // Serial Data  (DOUT on the SPH0645)
  };

  // Configuring the I2S driver and pins.
  // This function must be called before any I2S driver read/write operations.
  err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("Failed installing driver: %d\n", err);
    while (true);
  }
  Serial.println("I2S driver installed.");

  err = i2s_set_pin(I2S_PORT, &pin_config);
  if (err != ESP_OK) {
    Serial.printf("Failed setting pin: %d\n", err);
    while (true);
  }
  Serial.println("I2S pins set.");
}

void loop() {

  // Read a single sample and log it for the Serial Plotter.
  uint32_t sample = 0;
  int bytes_read = i2s_pop_sample(I2S_PORT, (char *)&sample, portMAX_DELAY); // no timeout
  if (bytes_read > 0) {
    Serial.println(sample);
  }
  
}
