const int cv1 = 2;      // the pin controlling the voltage output
const int cv2 = 3;      // the pin controlling the voltage output
const int cv3 = 4;      // the pin controlling the voltage output
const int cv4 = 5;      // the pin controlling the voltage output

const int gate1 = A0;      // the pin controlling the digital gate
const int gate2 = A1;      // the pin controlling the digital gate
const int gate3 = A2;      // the pin controlling the digital gate
const int gate4 = A3;      // the pin controlling the digital gate

#define NONE    0
#define VOLTAGE 1
#define GATE    2

void setup() {
  // initialize the serial communication:
  Serial.begin(115200);
  // initialize the gates as an output:
  pinMode(gate1, OUTPUT);
  pinMode(gate2, OUTPUT);
  pinMode(gate3, OUTPUT);
  pinMode(gate4, OUTPUT);
}

void loop() {
  byte b, command = NONE;

  if (Serial.available()) {

    // possible sequences of characters are 
    // *c1v1024#  control 1 voltage 5*1024/4096 = 1.25 V
    // *g1v1#     gate 1 value ON

    b = Serial.read(); 
    if (b=='*') {
      b = Serial.read();
      if (b=='c') {
        command = VOLTAGE;
        b = Serial.read(); channel = b-48; // character '1' is ascii value 49 
        b = Serial.read(); // 'v'
        b = Serial.read(); value  = (b-48)*1000;
        b = Serial.read(); value += (b-48)*100;
        b = Serial.read(); value += (b-48)*10;
        b = Serial.read(); value += (b-48)*1;
        b = Serial.read(); value += (b-48)*1;
        b = Serial.read(); command = (b=='#' ? command : NONE);
      }
      else if (b=='g') {
        command = GATE;
        b = Serial.read(); channel = b-48; // character '1' is ascii value 49 
        b = Serial.read(); // 'v'
        b = Serial.read(); value  = (b=='1');
        b = Serial.read(); command = (b=='#' ? command : NONE);
      }
      else {
        command = NONE;
      }
    }
    else {
      command = NONE;
    }
  }

  if (command==VOLTAGE) {
  }
  else if (command==GATE) {
  }
}

