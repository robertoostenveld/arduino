#ifndef VERSION_H
#define VERSION_H

static const char VERSION_LINE1[] = "Version 1.0";

static const char VERSION_LINE2[] = {
  __DATE__[4], __DATE__[5], '-',
  __DATE__[0], __DATE__[1], __DATE__[2], '-',
  __DATE__[9], __DATE__[10], ' ',
  __TIME__[0], __TIME__[1], ':',
  __TIME__[3], __TIME__[4],
  '\0'
};

#endif
