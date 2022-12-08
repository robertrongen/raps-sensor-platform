#ifndef _APPLICATION_H
#define _APPLICATION_H

#ifndef VERSION
#define VERSION "0.2"
#endif

#include <bcl.h>
#include <hx711.h> 

#define CLKPIN TWR_GPIO_P9
#define DTPIN TWR_GPIO_P8

uint64_t _radio_id = 0;

#endif
