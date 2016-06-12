#include <JeeLib.h>
#include <Ports.h>

#define pirPin      9     // the number of the pushbutton pin
#define redLed      5     // the number of the LED pin
#define buttonPin   4     // the number of the pushbutton pin
#define greenLed    3     // the number of the LED pin

//uncomment this line for debugging through the serial interface
#define BLENDER_DEBUG

#define KAKU_GROUP 1
#define KAKU_ADDR  1

// this is used to save some energy while sleeping
ISR(WDT_vect) { 
  Sleepy::watchdogEvent(); 
}

int active  = LOW;
int armed   = LOW;
int engaged = LOW; 

int buttonState;              // the current value from the input pin
int pirState;                 // the current value from the input pin
int lastButtonState = HIGH;   // the previous value from the input pin
int lastPirState = LOW;       // the previous value from the input pin
int lastSwitchState = -1;    // the last known state of the kaku switch

long buttonDebounceTime = 0;  // the last time the button pin was toggled
long pirDebounceTime = 0;     // the last time the PIR pin was toggled
long lastSwitchTime = 0;

#define debounceDelay      5  // the debounce time; increase if the output flickers
#define armedDelay      5000
#define blinkTime        500

/****************************************************************************************/

void setup() {
#ifdef BLENDER_DEBUG
  Serial.begin(57600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.print("\n[blenderdefender / ");
  Serial.print(__DATE__);
  Serial.print(" / ");
  Serial.print(__TIME__);
  Serial.println("]");
#endif

  // these should start in the expected value at rest 
  digitalWrite(buttonPin, HIGH);
  digitalWrite(pirPin, LOW);

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(pirPin, INPUT);
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(13, OUTPUT);

  // start with the switch off
  kakuSwitch(0);


  // set initial LED state
  ledStatus();
}

/****************************************************************************************/

void ledStatus() {
#ifdef BLENDER_DEBUG
  Serial.print("active = ");
  Serial.print(active);
  Serial.print("\tarmed = ");
  Serial.print(armed);
  Serial.print("\tengaged = ");
  Serial.println(engaged);
  delay(100);
#endif

  if (engaged==HIGH) {
    digitalWrite(redLed,   HIGH);
    digitalWrite(greenLed, HIGH);
  }
  else if (active==LOW) {
    digitalWrite(redLed,   LOW);
    digitalWrite(greenLed, HIGH);
  }
  else if (active==HIGH && armed==LOW) {
    digitalWrite(redLed,   (int)((millis()-buttonDebounceTime)/blinkTime) % 2);
    digitalWrite(greenLed, LOW);
  }
  else if (active==HIGH && armed==HIGH) {
    digitalWrite(redLed,   HIGH);
    digitalWrite(greenLed, LOW);
  }

  // this emulates the blender
  digitalWrite(13, engaged);
}

/****************************************************************************************/

void loop() {
  int buttonValue = digitalRead(buttonPin);
  int pirValue    = digitalRead(pirPin);

  if (buttonValue != lastButtonState)
    buttonDebounceTime = millis();

  if (pirValue != lastPirState) 
    pirDebounceTime = millis();

  if ((millis() - buttonDebounceTime) > debounceDelay) {
    if (buttonValue != buttonState) {
      buttonState = buttonValue;
#ifdef BLENDER_DEBUG
      Serial.print("buttonState = ");
      Serial.println(buttonState);
#endif
      if (buttonState == LOW) {
        active  = !active;
        armed   = LOW;
        engaged = LOW;
      }
    }
  }

  if (active && (millis() - buttonDebounceTime) > armedDelay)
    armed = HIGH;

  if ((millis() - pirDebounceTime) > debounceDelay) {
    if (pirValue != pirState) {
#ifdef BLENDER_DEBUG
      Serial.print("pirState = ");
      Serial.println(pirState);
#endif
      pirState = pirValue;
      if ((pirState == HIGH) && active && armed) {
        engaged = HIGH;
      }
      else {
        engaged = LOW;
      }
    }
  }

  // update the LEDs
  ledStatus();

  if (active && armed && !engaged && (millis() - lastSwitchTime) > 60000) {
    // switch the blender off every minute to safe-guard that it remains off 
    lastSwitchTime = millis();
    kakuSwitch(0); 
  }

  if (lastSwitchState!=engaged) {
    // toggle the blender on or off
    kakuSwitch(engaged); 
    lastSwitchState = engaged;
    lastSwitchTime = millis();
  }

  // remember these for the next iteration of the loop
  lastButtonState  = buttonValue;
  lastPirState     = pirValue;

  // spend some time in sleep mode to save energy
  Sleepy::loseSomeTime(20);
}

/****************************************************************************************/

void kakuSwitch(int engaged) {
#ifdef BLENDER_DEBUG
  Serial.print("kakuSwitch ");
#endif
  rf12_initialize(0, RF12_433MHZ, 0);
#ifdef BLENDER_DEBUG
  Serial.println(engaged);
#endif
  // send it twice to improve the robustness
  kakuSend(KAKU_GROUP, KAKU_ADDR, engaged);
  kakuSend(KAKU_GROUP, KAKU_ADDR, engaged);
}

/****************************************************************************************/

// this is from RF12demo
static void kakuSend(char addr, byte device, byte on) {
  int cmd = 0x600 | ((device - 1) << 4) | ((addr - 1) & 0xF);
  if (on)
    cmd |= 0x800;
  for (byte i = 0; i < 4; ++i) {
    for (byte bit = 0; bit < 12; ++bit) {
      ookPulse(375, 1125);
      int on = bitRead(cmd, bit) ? 1125 : 375;
      ookPulse(on, 1500 - on);
    }
    ookPulse(375, 375);
    delay(11); // approximate
  }
}

/****************************************************************************************/

// this is from RF12demo
static void ookPulse(int on, int off) {
  rf12_onOff(1);
  delayMicroseconds(on + 150);
  rf12_onOff(0);
  delayMicroseconds(off - 200);
}


