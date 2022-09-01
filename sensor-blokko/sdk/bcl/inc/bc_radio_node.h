#ifndef _BC_RADIO_NODE_H
#define _BC_RADIO_NODE_H

#include <bc_radio.h>
#include <bc_radio_pub.h>

//! @addtogroup bc_radio bc_radio
//! @brief Radio implementation send to node
//! @{

#define BC_RADIO_NODE_HEAD_SIZE (BC_RADIO_HEAD_SIZE + 1 + BC_RADIO_ID_SIZE)
#define BC_RADIO_NODE_MAX_BUFFER_SIZE (BC_RADIO_MAX_BUFFER_SIZE - BC_RADIO_ID_SIZE - 1)
#define BC_RADIO_NODE_MAX_COMPOUND_PART (BC_RADIO_NODE_MAX_BUFFER_SIZE / 5)
#define BC_RADIO_NODE_MAX_COMPOUND_BUFFER_SIZE (BC_RADIO_NODE_MAX_COMPOUND_PART * 5)

enum
{
    BC_RADIO_NODE_STATE_LED = BC_RADIO_PUB_STATE_LED,
    BC_RADIO_NODE_STATE_RELAY_MODULE_0 = BC_RADIO_PUB_STATE_RELAY_MODULE_0,
    BC_RADIO_NODE_STATE_RELAY_MODULE_1 = BC_RADIO_PUB_STATE_RELAY_MODULE_1,
    BC_RADIO_NODE_STATE_POWER_MODULE_RELAY = BC_RADIO_PUB_STATE_POWER_MODULE_RELAY,
};

typedef enum
{
    BC_RADIO_NODE_LED_STRIP_EFFECT_TEST = 0,
    BC_RADIO_NODE_LED_STRIP_EFFECT_RAINBOW = 1,
    BC_RADIO_NODE_LED_STRIP_EFFECT_RAINBOW_CYCLE = 2,
    BC_RADIO_NODE_LED_STRIP_EFFECT_THEATER_CHASE_RAINBOW = 3,
    BC_RADIO_NODE_LED_STRIP_EFFECT_COLOR_WIPE = 4,
    BC_RADIO_NODE_LED_STRIP_EFFECT_THEATER_CHASE = 5,
    BC_RADIO_NODE_LED_STRIP_EFFECT_STROBOSCOPE = 6,
    BC_RADIO_NODE_LED_STRIP_EFFECT_ICICLE = 7,
    BC_RADIO_NODE_LED_STRIP_EFFECT_PULSE_COLOR = 8

} bc_radio_node_led_strip_effect_t;

//! @brief Send request for set new state
//! @param[in] id Pointer to node id
//! @param[in] state_id State id from enum
//! @param[in] state Pointer to state which will be send
//! @return true On success
//! @return false On failure

bool bc_radio_node_state_set(uint64_t *id, uint8_t state_id, bool *state);

//! @brief Send request for get actual state
//! @param[in] id Pointer to node id
//! @param[in] state_id State id from enum
//! @return true On success
//! @return false On failure

bool bc_radio_node_state_get(uint64_t *id, uint8_t state_id);

//! @brief Send data to node
//! @param[in] id Pointer to node id
//! @param[in] buffer Pointer to buffer from which data will be send
//! @param[in] length Number of bytes to be send, max value is in BC_RADIO_NODE_MAX_BUFFER_SIZE
//! @return true On success
//! @return false On failure

bool bc_radio_node_buffer(uint64_t *id, void *buffer, size_t length);

//! @brief Send data to node
//! @param[in] id Pointer to node id
//! @param[in] color Color in RGBW format
//! @return true On success
//! @return false On failure

bool bc_radio_node_led_strip_color_set(uint64_t *id, uint32_t color);

//! @brief Send data to node
//! @param[in] id Pointer to node id
//! @param[in] brightness
//! @return true On success
//! @return false On failure

bool bc_radio_node_led_strip_brightness_set(uint64_t *id, uint8_t brightness);

//! @brief Send data to node
//! @param[in] id Pointer to node id
//! @param[in] compound Pointer to compound buffer
//! @param[in] length Number of bytes to be send, must be modulo 5, max value is in BC_RADIO_NODE_MAX_COMPOUND_BUFFER_SIZE
//! @return true On success
//! @return false On failure

bool bc_radio_node_led_strip_compound_set(uint64_t *id,  uint8_t *compound, size_t length);

//! @brief Send data to node
//! @param[in] id Pointer to node id
//! @param[in] type Effect type
//! @param[in] wait Interval between refresh
//! @param[in] color Color in RGBW format
//! @return true On success
//! @return false On failure

bool bc_radio_node_led_strip_effect_set(uint64_t *id, bc_radio_node_led_strip_effect_t type, uint16_t wait, uint32_t color);

//! @brief Send data to node
//! @param[in] id Pointer to node id
//! @param[in] temperature
//! @param[in] min Temperature on thermometer
//! @param[in] max Temperature on thermometer
//! @param[in] white_dots
//! @param[in] set_point Pointer to set point temperature, if NULL set_point and set_point_color not used
//! @param[in] set_point_color Color in RGBW format
//! @return true On success
//! @return false On failure

bool bc_radio_node_led_strip_thermometer_set(uint64_t *id, float temperature, int8_t min, int8_t max, uint8_t white_dots, float *set_point, uint32_t set_point_color);

//! @brief Internal decode function for bc_radio.c
//! @param[in] id Pointer on own id
//! @param[in] buffer Pointer to RX buffer
//! @param[in] length RX buffer length

void bc_radio_node_decode(uint64_t *id, uint8_t *buffer, size_t length);

//! @}

#endif // _BC_RADIO_NODE_H
