#ifndef _APPLICATION_H
#define _APPLICATION_H

#include <twr.h>
#include <bcl.h>

typedef struct
{
    uint8_t channel;
    float value;
    twr_tick_t next_pub;

} event_param_t;

typedef struct {
   uint64_t  SERVICE_INTERVAL_INTERVAL;
   uint64_t  BATTERY_UPDATE_INTERVAL;
   uint64_t  UPDATE_SERVICE_INTERVAL;      
   uint64_t  UPDATE_NORMAL_INTERVAL;
   uint64_t  BAROMETER_UPDATE_SERVICE_INTERVAL;
   uint64_t  BAROMETER_UPDATE_NORMAL_INTERVAL;
   uint64_t  TEMPERATURE_TAG_PUB_NO_CHANGE_INTERVAL;
   uint64_t  TEMPERATURE_TAG_PUB_VALUE_CHANGE;
   uint64_t  HUMIDITY_TAG_PUB_NO_CHANGE_INTERVAL;
   uint64_t  HUMIDITY_TAG_PUB_VALUE_CHANGE;
   uint64_t  LUX_METER_TAG_PUB_NO_CHANGE_INTERVAL;
   uint64_t  LUX_METER_TAG_PUB_VALUE_CHANGE;
   uint64_t  BAROMETER_TAG_PUB_NO_CHANGE_INTERVAL;
   uint64_t  BAROMETER_TAG_PUB_VALUE_CHANGE;
} all_settings_t;

#endif
