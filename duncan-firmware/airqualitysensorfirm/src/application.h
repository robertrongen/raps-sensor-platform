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
   uint32_t  SERVICE_INTERVAL_INTERVAL;
   uint32_t  BATTERY_UPDATE_INTERVAL;
   uint32_t  UPDATE_SERVICE_INTERVAL;      
   uint32_t  UPDATE_NORMAL_INTERVAL;
   uint32_t  BAROMETER_UPDATE_SERVICE_INTERVAL;
   uint32_t  BAROMETER_UPDATE_NORMAL_INTERVAL;
   uint32_t  TEMPERATURE_UPDATE_SERVICE_INTERVAL;
   uint32_t  TEMPERATURE_UPDATE_NORMAL_INTERVAL;
   uint32_t  HUMIDITY_UPDATE_SERVICE_INTERVAL;
   uint32_t  HUMIDITY_UPDATE_NORMAL_INTERVAL;
   uint32_t  CO2_UPDATE_SERVICE_INTERVAL;
   uint32_t  CO2_UPDATE_NORMAL_INTERVAL;
   uint32_t  VOC_LP_UPDATE_SERVICE_INTERVAL;
   uint32_t  VOC_LP_UPDATE_NORMAL_INTERVAL;
   uint32_t  TEMPERATURE_TAG_PUB_NO_CHANGE_INTERVAL;
   float_t TEMPERATURE_TAG_PUB_VALUE_CHANGE;
   uint32_t  HUMIDITY_TAG_PUB_NO_CHANGE_INTERVAL;
   float_t  HUMIDITY_TAG_PUB_VALUE_CHANGE;
   uint32_t  BAROMETER_TAG_PUB_NO_CHANGE_INTERVAL;
   float_t  BAROMETER_TAG_PUB_VALUE_CHANGE;
} all_settings_t;

#endif
