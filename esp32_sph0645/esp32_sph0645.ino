/*
   Sketch for ESP32 board, like the NodeMCU 32S or the Adafruit Huzzah32
   connected to a SPH0645 I2S microphone

   See https://diyi0t.com/i2s-sound-tutorial-for-esp32/
   and https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html

*/

#include <WiFi.h>
#include <driver/i2s.h>
#include <endian.h>
#include "secret.h"

long lastBlink = 0;
bool connected = false;
const unsigned int sampleRate = 22050;
uint32_t buffer[360], count = 0;

struct message_t {
  uint32_t version = 1;
  uint32_t samples;
  uint8_t data[360 * 4]; // this can hold up to 360 uint32 samples, or more with compression
} message __attribute__((packed));

const i2s_port_t I2S_PORT = I2S_NUM_0;

WiFiUDP udp;

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.println("");
      Serial.println("WiFi connected.");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      udp.begin(WiFi.localIP(), udpPort);
      connected = true;
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection.");
      connected = false;
      break;
    default: break;
  }
}

void setup() {
  esp_err_t err;

  Serial.begin(115200);

  // initialize status LED
  pinMode(LED_BUILTIN, OUTPUT);
  lastBlink = millis();

  // delete old config
  WiFi.disconnect(true);

  //register event handler
  WiFi.onEvent(WiFiEvent);

  if (!WiFi.config(localAddress, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
    while (true);
  }

  //Initiate connection
  Serial.println("Connecting to WiFi network: " + String(ssid));
  WiFi.begin(ssid, password);
  Serial.println("Waiting for WIFI connection...");

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
  }

  // The I2S config as per the example
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),  // receive, not transfer
    .sample_rate = sampleRate,                          // sampling rate
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,       // could only get it to work with 32bits
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,        // channel to use
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,           // interrupt level 1
    .dma_buf_count = 4,                                 // number of buffers
    .dma_buf_len = 32                                   // samples per buffer (minimum is 8)
  };

  // setup for the NODEMCU32
  const i2s_pin_config_t NODEMCU32_pin_config = {
    .bck_io_num = 26,                   // Serial Clock (BCLK on the SPH0645)
    .ws_io_num = 25,                    // Word Select  (LRCL on the SPH0645)
    .data_out_num = I2S_PIN_NO_CHANGE,  // not used     (only for speakers)
    .data_in_num = 33                   // Serial Data  (DOUT on the SPH0645)
  };

  // setup for the HUZZAH32
  const i2s_pin_config_t HUZZAH32_pin_config = {
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

  err = i2s_set_pin(I2S_PORT, &HUZZAH32_pin_config);
  if (err != ESP_OK) {
    Serial.printf("Failed setting pin: %d\n", err);
    while (true);
  }
  Serial.println("I2S pins set.");
} // setup

void loop() {
  uint32_t bytes_read;
  esp_err_t err;

  if ((millis() - lastBlink) > 2000) {
    digitalWrite(LED_BUILTIN, LOW);
    lastBlink = millis();
  }
  else if ((millis() - lastBlink) > 1000) {
    digitalWrite(LED_BUILTIN, HIGH);
  }

  // The Data Format is I2S, 24 bit, 2’s compliment, MSB first. The Data Precision is 18 bits, unused bits are zeros.
  err = i2s_read(I2S_PORT, buffer, 360 * 4, &bytes_read, portMAX_DELAY);

  if (err != ESP_OK) {
    Serial.printf("Failed reading from I2S: %d\n", err);
  }
  else {
    count++;

    for (int i; i < bytes_read / 4; i++) {
      buffer[i] = (buffer[i] >> 14) << 14; // the ESP32 is little-endian, the I2S data stream is big-endian
    }

    /*
        Serial.println(buffer[0]);
        uint32_t tmp = buffer[0];
        for (int i = 0; i < 32; i++) {
          Serial.print(tmp >> 31);
          tmp = tmp << 1;
        }
        Serial.println();
    */

    message.samples = bytes_read / 4;
    compress(buffer, message.samples, message.data);

    if (connected) {
      udp.beginPacket(udpAddress, udpPort);
      udp.write((uint8_t *)(&message), sizeof(message));
      udp.endPacket();
    }
  }

} // loop

uint32_t compress(uint32_t *data, uint32_t nsamples, uint8_t *dest) {
  uint32_t nbytes = 0;
  uint32_t *ptr = (uint32_t *)dest;
  for (int i; i < nsamples; i++) {
    ptr[i] = data[i];
    nbytes += 4;
  }
  return nbytes;
} // compress


uint32_t decompress(uint32_t *dest, uint32_t nsamples, uint8_t *data) {
  uint32_t nbytes = 0;
  uint32_t *ptr = (uint32_t *)data;
  for (int i; i < nsamples; i++) {
    dest[i] = ptr[i];
    nbytes += 4;
  }
  return nbytes;
} // decompress
