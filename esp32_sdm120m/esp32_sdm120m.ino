/*
 * Read teh usage from a SDM120-Modbus energy meter using a NodeMCU-32S microcontroller
 * with an RS485 module with automatic flow direction control.
 *
 * This code is inspired by
 *   https://www.youtube.com/watch?v=wC4JxfGBhgo
 *   https://github.com/masteripul/sdm120
 * and moified to use a hardware serial interface.
 */

#include <ModbusMaster.h>

#define TXD2 16
#define RXD2 17
#define MODBUS_ADDR 1

ModbusMaster meter;

float readFloat(uint16_t address) {
  if (meter.readInputRegisters(address, 2) == meter.ku8MBSuccess) {
    uint32_t value =
      ((uint32_t)meter.getResponseBuffer(0) << 16) | meter.getResponseBuffer(1);

    float result;
    memcpy(&result, &value, sizeof(result));
    return result;
  }

  return NAN;  // read failed
}

void printValue(const char* label, uint16_t address) {
  float value = readFloat(address);
  Serial.printf("%-20s : %.2f\n", label, value);
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  meter.begin(MODBUS_ADDR, Serial2);

  Serial.println("Ready");
}

void loop() {
  Serial.println("--------------------------------");
  printValue("Voltage", 0x0000);
  printValue("Current", 0x0006);
  printValue("Active Power", 0x000C);
  printValue("Total Energy", 0x0156);
  printValue("Power Factor", 0x001E);
  printValue("Frequency", 0x0046);

  delay(1000);
}
