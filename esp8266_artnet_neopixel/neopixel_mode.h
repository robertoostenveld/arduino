#ifndef _MODE_H_
#define _MODE_H_

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArtnetWifi.h>
#include <Adafruit_NeoPixel.h>

#define ROUND(x)   (int(x + 0.5))
#define ABS(x)     (x * (x < 0 ? -1 : 1))
#define MIN(x, y)  (x < y ? x : y)
#define MAX(x, y)  (x > y ? x : y)
#define WRAP360(x) (x > 0 ? (x - int((x)/360)*360) : (x - int((x)/360 - 1)*360))  // between    0 and 360
#define WRAP180(x) (WRAP360(x) < 180 ? WRAP360(x) : WRAP360(x) - 360)             // between -180 and 180
#define BALANCE(l, x1, x2)  ((x1) * (1. - l) + (x2) * l)

#ifdef __cplusplus
extern "C" {
#endif

extern long tic_frame;

void map_hsv_to_rgb(int *, int *, int *);

void singleRed();
void singleGreen();
void singleBlue();
void singleYellow();
void singleWhite();
void fullBlack();

void mode0(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode1(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode2(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode3(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode4(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode5(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode6(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode7(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode8(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode9(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode10(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode11(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode12(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode13(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode14(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode15(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode16(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode17(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode18(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode19(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode20(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode21(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode22(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode23(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode24(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode25(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode26(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode27(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode28(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode29(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode30(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode31(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode32(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode33(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode34(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode35(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode36(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode37(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode38(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode39(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode40(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode41(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode42(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode43(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode44(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode45(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode46(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode47(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode48(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode49(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode50(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode51(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode52(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode53(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode54(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode55(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode56(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode57(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode58(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode59(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode60(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode61(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode62(uint16_t, uint16_t, uint8_t, uint8_t *);
void mode63(uint16_t, uint16_t, uint8_t, uint8_t *);

#ifdef __cplusplus
}
#endif

#endif

