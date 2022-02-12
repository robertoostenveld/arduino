/*
   Sketch for ESP8266 board, like the WEMOS D1 mini pro
   connected to a INMP411 I2S microphone

   The *internal GPIO number* which is NOT NECESSARIALY the same as the pin numbers, are as follows:
   I2SI_DATA     = GPIO12
   IS2I_BCK      = GPIO13
   I2SI_WS/LRCLK = GPIO14

   See https://github.com/esp8266/Arduino/blob/master/libraries/esp8266/examples/I2SInput/I2SInput.ino

*/

#include <ESP8266WiFi.h>
#include <i2s.h>

#include "secret.h"
#include "RunningStat.h"

IPAddress serverAddress(192, 168, 1, 21);
const unsigned int serverPort = 4000;
const unsigned int recvPort = 4001;

WiFiClient Tcp;

unsigned long lastBlink = 0;
unsigned long lastConnect = 0;
unsigned long blinkTime = 250;
bool connected = false;
const unsigned int sampleRate = 22050;
const unsigned int nMessage = 720;
float smoothedMean = 0;
bool meanInitialized = 0;
const float alpha = 10. / sampleRate; // if the sampling time dT is much smaller than the time constant T, then alpha=1/(T*sampleRate) and T=1/(alpha*sampleRate)

struct message_t {
  uint32_t version = 1;
  uint32_t id = 0;
  uint32_t counter;
  uint32_t samples;
  int16_t data[nMessage];
} message __attribute__((packed));

RunningStat shortstat;

void setup() {
  Serial.begin(115200);

  // initialize the status LED
  pinMode(LED_BUILTIN, OUTPUT);
  lastBlink = millis();

  // set A0 LOW, this is connected to the L/R selection
  pinMode(A0, OUTPUT);
  digitalWrite(A0, LOW);

  // set GPIO16 LOW and use that as GND, rather than the GND pin
  // this allows for a cleaner routing of the wires
  pinMode(16, OUTPUT);
  digitalWrite(16, LOW);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  i2s_rxtx_begin(true, false); // Enable I2S RX
  i2s_set_rate(sampleRate);

  // use the last part of the IP address as identifier
  message.id = WiFi.localIP()[3];

} // setup

void loop() {
  int16_t l, r;

  if ((millis() - lastBlink) > 2 * blinkTime) {
    digitalWrite(LED_BUILTIN, LOW); // status LED on
    lastBlink = millis();
  }
  else if ((millis() - lastBlink) > 1 * blinkTime) {
    digitalWrite(LED_BUILTIN, HIGH); // status LED off
  }

  while (i2s_rx_available() && message.samples < nMessage) {
    i2s_read_sample(&l, &r, true);
    float value = l;

    // compute a smooth running mean with https://en.wikipedia.org/wiki/Exponential_smoothing
    if (meanInitialized) {
      smoothedMean = alpha * value + (1 - alpha) * smoothedMean;
    }
    else {
      smoothedMean = value;
      meanInitialized = 1;
    }
    value -= smoothedMean;

    /*
           Serial.print(value);
           Serial.println();
    */

    message.data[message.samples] = value; // this casts the value back to an int16
    message.samples++;
    shortstat.Push(value);
  }

  if (message.samples == nMessage) {

    Serial.print(shortstat.Min());
    Serial.print(", ");
    Serial.print(shortstat.Mean());
    Serial.print(", ");
    Serial.println(shortstat.Max());
    /*
      Serial.print(message.version); Serial.print(", ");
      Serial.print(message.counter); Serial.print(", ");
      Serial.print(message.samples); Serial.print(", ");
      Serial.print(message.data[0]); Serial.println();
    */

    if  ((millis() - lastConnect) > 10000) {
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

    if (connected) {
      blinkTime = 1000;
      int count = Tcp.write((uint8_t *)(&message), sizeof(message));
      connected = (count == sizeof(message));
    }
    else {
      blinkTime = 250;
    }

    message.counter++;
    message.samples = 0;
    shortstat.Clear();
  }

} // loop
