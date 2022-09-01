#ifndef _BC_CP201T_H
#define _BC_CP201T_H

#include <bc_module_sensor.h>
#include <bc_scheduler.h>

typedef enum
{
    //! @brief Error event
    BC_CP201T_EVENT_ERROR = 0,

    //! @brief Update event
    BC_CP201T_EVENT_UPDATE = 1

} bc_cp201t_event_t;

//! @brief Initialize CP-201T thermistor
//! @param[in] channel Channel of Sensor Module that sensor is connected to
//! @return true When successfully initialized
//! @return false When not successfully initialized

bool bc_cp201t_init(bc_module_sensor_channel_t channel);

//! @brief Set callback function
//! @param[in] channel Channel of Sensor Module that sensor is connected to
//! @param[in] event_handler Function address
//! @param[in] event_param Optional event parameter (can be NULL)

void bc_cp201t_set_event_handler(bc_module_sensor_channel_t channel, void (*event_handler)(bc_module_sensor_channel_t, bc_cp201t_event_t, void *), void *event_param);

//! @brief Set measurement interval
//! @param[in] channel Channel of Sensor Module that sensor is connected to
//! @param[in] interval Measurement interval

void bc_cp201t_set_update_interval(bc_module_sensor_channel_t channel, bc_tick_t interval);

//! @brief Get measured temperature in degrees of Celsius
//! @param[in] channel Channel of Sensor Module that sensor is connected to
//! @param[in] celsius Pointer to variable where result will be stored
//! @return true When value is valid
//! @return false When value is invalid

bool bc_cp201t_get_temperature_celsius(bc_module_sensor_channel_t channel, float *celsius);

#endif // _BC_CP201T_H
