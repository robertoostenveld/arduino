/*
   Sketch for ESP32 board, like the NodeMCU 32S, LOLIN32 Lite, or the Adafruit Huzzah32
   connected to a SPH0645 I2S microphone

   See https://diyi0t.com/i2s-sound-tutorial-for-esp32/
   and https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html

*/

#if not defined(ESP32)
#error This is a sketch for an ESP32 board, like the NodeMCU 32S, LOLIN32 Lite, or the Adafruit Huzzah32
#endif

#include <WiFi.h>
#include <WiFiUdp.h>
#include <driver/i2s.h>
#include <endian.h>
#include <math.h>

#include "secret.h"
#include "RunningStat.h"

#ifndef htonl
#define htonl htobe32
#endif

#ifndef htons
#define htons htobe16
#endif

#ifndef ntohl
#define htons be32toh
#endif

#ifndef ntohs
#define htons be16toh
#endif

#define USE_DHCP

#ifdef USE_DHCP
IPAddress localAddress(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional
#endif

// this is for the remote UDP server
IPAddress sendAddress(192, 168, 1, 21);
const unsigned int sendPort = 4000;

// this is for the local UDP server
WiFiUDP Udp;
const unsigned int recvPort = 4001;

const unsigned int nMessage = 720;
const unsigned int nBuffer = 64;
const float decay = 0.1;
unsigned long lastBlink = 0;
unsigned long failedPackets = 0;
unsigned long previous = 0;
bool connected = false;
const unsigned int sampleRate = 22050;
float runningMean = -1;

struct message_t {
  uint32_t version = 1;
  uint32_t id = 0;
  uint32_t counter;
  uint32_t samples;
  int16_t data[nMessage];
} message __attribute__((packed));

struct response_t {
  uint32_t version = 1;
  uint32_t id = 0;
  uint32_t counter;
} response __attribute__((packed));

const i2s_port_t I2S_PORT = I2S_NUM_0;

RunningStat shortstat;
RunningStat longstat;

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.println("");
      Serial.println("WiFi connected.");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      Udp.begin(WiFi.localIP(), recvPort);
      connected = true;
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection.");
      connected = false;
      break;
    default: break;
  }
}

void print_binary(uint32_t val) {
  for (int l = 31; l >= 24; l--) {
    uint8_t b = (val >> l) & 0x01;
    Serial.print(b);
  }
  Serial.print(' ');
  for (int l = 23; l >= 16; l--) {
    uint8_t b = (val >> l) & 0x01;
    Serial.print(b);
  }
  Serial.print(' ');
  for (int l = 15; l >= 8; l--) {
    uint8_t b = (val >> l) & 0x01;
    Serial.print(b);
  }
  Serial.print(' ');
  for (int l = 7; l >= 0; l--) {
    uint8_t b = (val >> l) & 0x01;
    Serial.print(b);
  }
  Serial.println();
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

#ifndef USE_DHCP
  if (!WiFi.config(localAddress, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
    while (true);
  }
#endif

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
    .dma_buf_count = 16,                                // number of buffers, 128 max.
    .dma_buf_len = nBuffer                              // samples per buffer (minimum is 8)
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

  err = i2s_set_pin(I2S_PORT, &NODEMCU32_pin_config);
  if (err != ESP_OK) {
    Serial.printf("Failed setting pin: %d\n", err);
    while (true);
  }
  Serial.println("I2S pins set.");

  // use the last digit of the IP address as identifier
  message.id = WiFi.localIP()[3];

} // setup

void loop() {
  esp_err_t err;
  uint32_t bytes_read;
  uint32_t buffer[nBuffer];

  if ((millis() - lastBlink) > 2000) {
    digitalWrite(LED_BUILTIN, LOW);
    lastBlink = millis();
  }
  else if ((millis() - lastBlink) > 1000) {
    digitalWrite(LED_BUILTIN, HIGH);
  }

  size_t samples = nMessage - message.samples;

  if (samples > nBuffer) {
    // do not read more than nBuffer at a time
    samples = nBuffer;
  }

  err = i2s_read(I2S_PORT, buffer, samples * 4, &bytes_read, 0);

  if (err == ESP_OK) {
    for (unsigned int sample; sample < bytes_read / 4; sample++) {

      uint32_t value = buffer[sample];
      value = value >> 14;  // convert to 18 bit
      value = value >> 3;   // convert to 15 bit

      if (runningMean < 0) {
        runningMean = value;
      }
      else {
        runningMean = decay * value + (1 - decay) * runningMean;
      }

      int16_t demeaned = ((float)value - (float)runningMean);

      message.data[message.samples] = htons(demeaned);
      message.samples++;

      shortstat.Push(demeaned);
      longstat.Push(demeaned);
    }

    if (message.samples == nMessage) {

      /*
        Serial.print(message.version); Serial.print(", ");
        Serial.print(message.counter); Serial.print(", ");
        Serial.print(message.samples); Serial.print(", ");
        Serial.print(ntohs(message.data[0])); Serial.println();

        Serial.print(shortstat.Min());
        Serial.print(", ");
        Serial.print(shortstat.Mean());
        Serial.print(", ");
        Serial.println(shortstat.Max());
      */

      if (connected) {
        Udp.beginPacket(sendAddress, sendPort);
        Udp.write((uint8_t *)(&message), sizeof(message));
        Udp.endPacket();
      }

      shortstat.Clear();
      message.counter++;
      message.samples = 0;
    }
  }

  // receive incoming UDP packets
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    // Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
    int len = Udp.read((char *) &response, sizeof(response));
    if (len == sizeof(response)) {
      if (response.counter != previous+1) {
        failedPackets++;
        Serial.print("missing response: ");
        Serial.print(previous);
        Serial.print(", ");
        Serial.print(response.counter);
        Serial.println();
      }
      previous = response.counter;
    }
  }

} // loop
