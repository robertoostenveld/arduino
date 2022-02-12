/*
   Sketch for ESP32 board, like the NodeMCU 32S or the Adafruit Huzzah32
   connected to a INMP411 I2S microphone

   See https://diyi0t.com/i2s-sound-tutorial-for-esp32/
   and https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html

*/

#include <WiFi.h>
#include <WiFiUdp.h>
#include <driver/i2s.h>
#include <endian.h>
#include <math.h>
#include <limits.h>

#include "secret.h"
#include "RunningStat.h"

#define USE_DHCP
#define CONNECT_LOLIN32
#define PRINT_RANGE
#define DO_RECONNECT

#ifndef USE_DHCP
IPAddress localAddress(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional
#endif

// IPAddress serverAddress(192, 168, 1, 21);
IPAddress serverAddress(192, 168, 1, 34);

const unsigned int serverPort = 4000;
const unsigned int recvPort = 4001;

WiFiUDP Udp;    // this is for the local UDP server
WiFiClient Tcp;

unsigned long blinkTime = 250;
unsigned long lastBlink = 0;
unsigned int reconnectInterval = 30000;
unsigned long lastConnect = 0;
bool connected = false;
const unsigned int sampleRate = 22050;
const unsigned int nMessage = 512; // it can be up to 720
const unsigned int nBuffer = 64;
bool meanInitialized = 0;
const double alpha = 10. / sampleRate; // if the sampling time dT is much smaller than the time constant T, then alpha=1/(T*sampleRate) and T=1/(alpha*sampleRate)
double signalMean = sqrt(-1); // initialize as not-a-number
double signalScale = 1000;

struct message_t {
  uint32_t version = 1;
  uint32_t id = 0;
  uint32_t counter;
  uint32_t samples;
  int16_t data[nMessage];
} message __attribute__((packed));

esp_err_t err;
uint32_t bytes_read;
int32_t buffer[nBuffer];

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

#ifdef CONNECT_LOLIN32
#define LED_BUILTIN 22
  pinMode(32, OUTPUT); digitalWrite(32, HIGH);  // L/R
  pinMode(35, OUTPUT); digitalWrite(35, HIGH);  // VDD

  const i2s_pin_config_t pin_config = {
    .bck_io_num = 25,                   // Serial Clock (SCK on the INMP441)
    .ws_io_num = 33,                    // Word Select  (WS on the INMP441)
    .data_out_num = I2S_PIN_NO_CHANGE,  // not used     (only for speakers)
    .data_in_num = 26                   // Serial Data  (SD on the INMP441)
  };
#endif

#ifdef CONNECT_NODEMCU32
  const i2s_pin_config_t pin_config = {
    .bck_io_num = 13,                   // Serial Clock (SCK on the INMP441)
    .ws_io_num = 15,                    // Word Select  (WS on the INMP441)
    .data_out_num = I2S_PIN_NO_CHANGE,  // not used     (only for speakers)
    .data_in_num = 21                   // Serial Data  (SD on the INMP441)
  };
#endif


#ifdef CONNECT_HUZZAH32
  const i2s_pin_config_t pin_config = {
    .bck_io_num = 32,                   // Serial Clock (BCLK on the SPH0645)
    .ws_io_num = 22,                    // Word Select  (LRCL on the SPH0645)
    .data_out_num = I2S_PIN_NO_CHANGE,  // not used     (only for speakers)
    .data_in_num = 14                   // Serial Data  (DOUT on the SPH0645)
  };
#endif

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
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,        // channel to use
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,           // interrupt level 1
    .dma_buf_count = 16,                                // number of buffers, 128 max.
    .dma_buf_len = nBuffer                              // samples per buffer (minimum is 8)
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

  // use the last number of the IP address as identifier
  message.id = WiFi.localIP()[3];

  for (int i; i < sampleRate; i++) {
    err = i2s_read(I2S_PORT, buffer, 4, &bytes_read, 0);
  }

} // setup

void loop() {

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

      double value = buffer[sample];

      // compute a smooth running mean with https://en.wikipedia.org/wiki/Exponential_smoothing
      if (isnan(signalMean)) {
        signalMean = value;
      }
      else {
        signalMean = alpha * value + (1 - alpha) * signalMean;
      }
      value -= signalMean;
      value /= signalScale;

      // https://en.wikipedia.org/wiki/Sigmoid_function
      value /= 32767;
      value /= (1 + fabs(value));
      value *= 32767;

#ifdef PRINT_HEADER
      Serial.println(value);
#endif

      message.data[message.samples] = value; // this casts the value to an int16
      message.samples++;

      shortstat.Push(value);
      longstat.Push(value);
    }

    if (message.samples == nMessage) {

#ifdef PRINT_RANGE
      Serial.print(shortstat.Min());
      Serial.print(", ");
      Serial.print(shortstat.Mean());
      Serial.print(", ");
      Serial.println(shortstat.Max());
#endif

#ifdef PRINT_FREQUENCY
      Serial.println(shortstat.ZeroCrossingRate() * sampleRate);
#endif

#ifdef PRINT_VOLUME
      Serial.println(10*log10(shortstat.Variance()));
#endif

#ifdef PRINT_HEADER
      Serial.print(message.version); Serial.print(", ");
      Serial.print(message.counter); Serial.print(", ");
      Serial.print(message.samples); Serial.print(", ");
      Serial.print(message.data[0]); Serial.println();
#endif

#ifdef DO_RECONNECT
      if  ((millis() - lastConnect) > reconnectInterval) {
        // reconnect, but don't try to reconnect too often
        lastConnect = millis();

        // turn the status LED on
        digitalWrite(LED_BUILTIN, LOW);
        lastBlink = millis();

        if (!connected) {
          connected = Tcp.connect(serverAddress, serverPort);
          if (connected) {
            Serial.print("Connected to ");
            Serial.println(serverAddress);
          }
          else {
            Serial.print("Failed to connect to ");
            Serial.println(serverAddress);
          }
        }
      }
#endif

      if (connected) {
        blinkTime = 1000;
        int count = Tcp.write((uint8_t *)(&message), sizeof(message));
        connected = (count == sizeof(message));
      }
      else {
        blinkTime = 250;
      }

      shortstat.Clear();
      message.counter++;
      message.samples = 0;
    }
  }


} // loop
