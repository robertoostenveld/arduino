#ifndef _COLORSPACE_H_
#define _COLORSPACE_H_

typedef struct {
    double r;       // percent
    double g;       // percent
    double b;       // percent
} rgb;

typedef struct {
    double h;       // angle in degrees
    double s;       // percent
    double v;       // percent
} hsv;

hsv rgb2hsv(rgb);
rgb hsv2rgb(hsv);

#endif

