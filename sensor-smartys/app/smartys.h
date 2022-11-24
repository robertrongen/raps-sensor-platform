#ifndef _SMARTYS_APPLICATION_H
#define _SMARTYS_APPLICATION_H

#ifndef VERSION
#define VERSION "0.1"
#endif

#include <bcl.h>

typedef struct
{
    uint8_t channel;
    float value;
    bc_tick_t next_pub;

} event_param_t;

#endif // _APPLICATION_H
