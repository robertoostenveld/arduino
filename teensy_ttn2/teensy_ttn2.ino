#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <String.h>

// the LoRa keys and IDs are secret and shoudl not be disclosed
//static const PROGMEM u1_t NWKSKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//static const u1_t PROGMEM APPSKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//static const u4_t DEVADDR = 0x00000000;
#include "secret.h"

#define BUTTON_PIN 20
int button = 0;

#define LED_R 21
#define LED_G 22
#define LED_B 23
int color = NULL;

#define ONE_WIRE_BUS 15
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature dallasTemperature(&oneWire);
float temp = 0.0;

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

#define DATALEN 32
// static uint8_t datatx[] = "Hello, world!";
unsigned char datatx[DATALEN]; // transmit
unsigned char datarx[DATALEN]; // receive
static osjob_t sendjob;
static osjob_t recvjob;
static osjob_t tempjob;
static osjob_t ledjob;

// Schedule TX every this many seconds (might become longer due to duty cycle limitations).
const unsigned TX_INTERVAL = 10;
const unsigned RX_INTERVAL = 2;
const unsigned TEMP_INTERVAL = 2;
const unsigned LED_INTERVAL = 150;  // in miliseconds

// Pin mapping
const lmic_pinmap lmic_pins = {
  .nss = 10,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 9,
  .dio = {2, 5, 6},
};

void onEvent (ev_t ev) {
  Serial.print(os_getTime());
  Serial.print(": ");
  switch (ev) {
    case EV_SCAN_TIMEOUT:
      Serial.println(F("EV_SCAN_TIMEOUT"));
      break;
    case EV_BEACON_FOUND:
      Serial.println(F("EV_BEACON_FOUND"));
      break;
    case EV_BEACON_MISSED:
      Serial.println(F("EV_BEACON_MISSED"));
      break;
    case EV_BEACON_TRACKED:
      Serial.println(F("EV_BEACON_TRACKED"));
      break;
    case EV_JOINING:
      Serial.println(F("EV_JOINING"));
      break;
    case EV_JOINED:
      Serial.println(F("EV_JOINED"));
      break;
    case EV_RFU1:
      Serial.println(F("EV_RFU1"));
      break;
    case EV_JOIN_FAILED:
      Serial.println(F("EV_JOIN_FAILED"));
      break;
    case EV_REJOIN_FAILED:
      Serial.println(F("EV_REJOIN_FAILED"));
      break;
    case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      if (LMIC.dataLen) {
        // data received in rx slot after tx
        Serial.print(F("Data Received: "));
        Serial.write(LMIC.frame + LMIC.dataBeg, LMIC.dataLen);
        Serial.println();
        // deal with the received data
        bzero(datarx, DATALEN);
        memcpy(datarx, LMIC.frame + LMIC.dataBeg, min(LMIC.dataLen, DATALEN - 1));
      }
      // Schedule next transmission
      os_setTimedCallback(&sendjob, os_getTime() + ms2osticks(TX_INTERVAL), send_message);
      break;
    case EV_LOST_TSYNC:
      Serial.println(F("EV_LOST_TSYNC"));
      break;
    case EV_RESET:
      Serial.println(F("EV_RESET"));
      break;
    case EV_RXCOMPLETE:
      // data received in ping slot
      Serial.println(F("EV_RXCOMPLETE"));
      break;
    case EV_LINK_DEAD:
      Serial.println(F("EV_LINK_DEAD"));
      break;
    case EV_LINK_ALIVE:
      Serial.println(F("EV_LINK_ALIVE"));
      break;
    default:
      Serial.println(F("Unknown event"));
      break;
  }
}

void switch_led() {
  // Serial.print("color = ");
  // Serial.println(color);

  digitalWrite(LED_R, LOW);    // turn the LED off by making the voltage LOW
  digitalWrite(LED_G, LOW);    // turn the LED off by making the voltage LOW
  digitalWrite(LED_B, LOW);    // turn the LED off by making the voltage LOW
  switch (color) {
    case LED_R:
      digitalWrite(LED_R, HIGH);   // turn the LED on (HIGH is the voltage level)
      break;
    case LED_G:
      digitalWrite(LED_G, HIGH);   // turn the LED on (HIGH is the voltage level)
      break;
    case LED_B:
      digitalWrite(LED_B, HIGH);   // turn the LED on (HIGH is the voltage level)
      break;
    default:
      break;
  }
}

void blink_led(osjob_t* j) {
  switch_led();
  if (color) {
    // schedule the LED to switch off
    color = NULL;
    os_setTimedCallback(j, os_getTime() + ms2osticks(LED_INTERVAL), blink_led);
  }
}

void update_button() {
  // this function is triggered by an interrupt
  delay(50); // debounce
  if (digitalRead(BUTTON_PIN) == HIGH) {
    color = LED_B;
    blink_led(&ledjob);
    button = 1;
    Serial.print("button = ");
    Serial.println(button);
  }
}

void receive_message(osjob_t* j) {
  if (datarx[0]) {
    Serial.println("message = [");
    unsigned int i = 0;
    while (datarx[i])
      Serial.println((int)datarx[i++]);
    Serial.println("]");
    bzero(datarx, DATALEN);
    // blink
    color = LED_B;
    switch_led();
  }
  os_setTimedCallback(j, os_getTime() + sec2osticks(RX_INTERVAL), receive_message);
}

void update_temp(osjob_t* j) {
  dallasTemperature.requestTemperatures();     // Send the command to get temperatures
  temp = dallasTemperature.getTempCByIndex(0); // Get the temperature of the first sensor
  // Serial.print("temp = ");
  // Serial.println(temp);
  // the measurement takes quite some time, hence we blink afterwards (otherwise the blink duration would be incorrect)
  color = LED_G;
  blink_led(&ledjob);
  // schedule the next measurement
  os_setTimedCallback(j, os_getTime() + sec2osticks(TEMP_INTERVAL), update_temp);
}

void send_message(osjob_t* j) {

  color = LED_R;
  blink_led(&ledjob);

  if (button) {
    String str = String("Button pressed");
    str.toCharArray((char *)datatx, DATALEN);
  }
  else {
    String str = String("Temperature = ");
    str += String(temp);
    str.toCharArray((char *)datatx, DATALEN);
  }

  // reset the button status
  button = 0;

  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.println(F("OP_TXRXPEND, not sending"));
  } else {
    // Prepare upstream data transmission at the next possible time.
    LMIC_setTxData2(1, datatx, sizeof(datatx) - 1, 0);
    Serial.println(F("Packet queued"));
  }
  // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {
  // setup the button
  pinMode(BUTTON_PIN, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), update_button, RISING);

  // setup the temperature sensor
  dallasTemperature.begin();

  // setup the message content
  bzero(datatx, DATALEN);
  bzero(datarx, DATALEN);

  // setup the RGB led
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }
  Serial.println(F("Starting"));

#ifdef VCC_ENABLE
  // For Pinoccio Scout boards
  pinMode(VCC_ENABLE, OUTPUT);
  digitalWrite(VCC_ENABLE, HIGH);
  delay(1000);
#endif

  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();

  // Set static session parameters. Instead of dynamically establishing a session
  // by joining the network, precomputed session parameters are to be provided.
#ifdef PROGMEM
  // On AVR, these values are stored in flash and only copied to RAM
  // once. Copy them to a temporary buffer here, LMIC_setSession will
  // copy them into a buffer of its own again.
  uint8_t appskey[sizeof(APPSKEY)];
  uint8_t nwkskey[sizeof(NWKSKEY)];
  memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
  memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
  LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
#else
  // If not running an AVR with PROGMEM, just use the arrays directly
  LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
#endif

  // Set up the channels used by the Things Network, which corresponds
  // to the defaults of most gateways. Without this, only three base
  // channels from the LoRaWAN specification are used, which certainly
  // works, so it is good for debugging, but can overload those
  // frequencies, so be sure to configure the full frequency range of
  // your network here (unless your network autoconfigures them).
  // Setting up channels should happen after LMIC_setSession, as that
  // configures the minimal channel set.
  LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
  LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band
  // TTN defines an additional channel at 869.525Mhz using SF9 for class B
  // devices' ping slots. LMIC does not have an easy way to define set this
  // frequency and support for class B is spotty and untested, so this
  // frequency is not configured here.

  // Disable link check validation
  LMIC_setLinkCheckMode(FALSE);

  // Set adaptive data rate
  LMIC_setAdrMode(FALSE);
  // Set data rate and transmit power (note: txpow seems to be ignored by the library)
  LMIC_setDrTxpow(DR_SF7, 20);

  // Start the periodic jobs
  update_temp(&tempjob);
  send_message(&sendjob);
  receive_message(&recvjob);
}

void loop() {
  os_runloop();
}
