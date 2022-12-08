/**
 * Ported to BigClown from HX711 library for Arduino
 * https://github.com/bogde/HX711
 * 
 * MIT License
 * (c) 2018 Bogdan Necula
 * (c) 2020 Petr Matejicek
**/


#ifndef _HX711_H
#define _HX711_H

// #include <bc_gpio.h>
// #include <bc_tick.h>
// #include <bc_timer.h>
// #include <bc_scheduler.h>
// #include <bc_usb_cdc.h>
// #include <bc_eeprom.h>
#include <bcl.h>

#define HX711_INVALID_VALUE -99999999
#define _HX711_TIMES 5
#define _HX711_MEM_ADDRESS 0


// HX711 scale modlule configuration
typedef struct hx711_t hx711_t;


// channels of HX711 decoder
// the number corresponds to gain on selected channel. 
// Channel B is fix set to 32, Chnnel A can be set to 128 and 64 
typedef enum {
    HX711_CHANNEL_A = 128,
    HX711_CHANNEL_A64 = 64,
    HX711_CHANNEL_B = 32
} hx711_channel_t;


// state of HX711 decoder
typedef enum {
    // module is not detected
    HX711_STATE_ERROR = -1,
    // not initialized yet
    HX711_STATE_INITIALIZE = 0,
    // ready to read
    HX711_STATE_READY = 1,
    // reading data
    HX711_STATE_READING = 2,
    // sleep - based on the function call
    HX711_STATE_SLEEP = 3
} hx711_state_t;


// event types
typedef enum {
    // error event
    HX711_EVENT_ERROR = 0,
    // timer update
    HX711_EVENT_UPDATE = 1,
    // measure
    HX711_EVENT_MEASURE = 2
} hx711_event_t;


// instance of the scale
struct hx711_t
{
    bc_gpio_channel_t _PD_SCK;  // Power Down and Serial Clock Input Pin
    bc_gpio_channel_t _DOUT;    // Serial Data Output Pin
    uint8_t _gain;              // amplification factor
    long _offset;               // used for tare weight
    float _scale;               // used to return weight in grams, kg, ounces, whatever
    hx711_state_t _state;

    uint8_t _times;             // number of measurements to return value

    bc_scheduler_task_id_t _task_id_interval;
    bc_tick_t _update_interval;
    void (*_event_handler)(hx711_t *, hx711_event_t, double, void *);
    void *_event_param;

    bool _hybernate;
};


// Initialize library with data output pin, clock input pin and gain factor.
// Channel selection is made by passing the appropriate gain:
// - With a gain factor of 64 or 128, channel A is selected
// - With a gain factor of 32, channel B is selected
// DOUT - data output GPIO port
// PD_SCK - power down and Serial Clock GPIO port
void hx711_init(hx711_t *self, bc_gpio_channel_t dout, bc_gpio_channel_t pd_sck, hx711_channel_t channel);

// register eventhandler to be executed on update timer
void hx711_set_event_handler(hx711_t *self, void (*event_handler)(hx711_t *, hx711_event_t, double, void *), void *event_param);

// set measuring frequency
void hx711_set_update_interval(hx711_t *self, bc_tick_t interval);

// Check if HX711 is ready
// from the datasheet: When output data is not ready for retrieval, digital output pin DOUT is high. Serial clock
// input PD_SCK should be low. When DOUT goes to low, it indicates data is ready for retrieval.
bool hx711_is_ready(hx711_t *self);

// Wait for the HX711 to become ready
void hx711_wait_ready(hx711_t *self, unsigned long delay_ms);
bool hx711_wait_ready_retry(hx711_t *self, int retries, unsigned long delay_ms);
bool hx711_wait_ready_timeout(hx711_t *self, unsigned long timeout, unsigned long delay_ms);


// waits for the chip to be ready and returns a reading
long hx711_read_raw(hx711_t *self);

// returns an average reading; times = how many times to read
long hx711_read_raw_average(hx711_t *self, uint8_t times );

// returns (read_average() - OFFSET), that is the current value without the tare weight; 
double hx711_get_value(hx711_t *self);

// returns get_value() divided by SCALE, that is the raw value divided by a value obtained via calibration
float hx711_get_units(hx711_t *self);

// measure the value and call the registered event handler
bool hx711_measure(hx711_t *self);

// tare thr scale - set the current weiht as the OFFSET; 
bool hx711_tare(hx711_t *self);

// calibrate scale - set the SCALE value from current weight; 
bool hx711_calibrate(hx711_t *self, float weight);

// set the SCALE value; this value is used to convert the raw data to "human readable" data (measure units)
bool hx711_set_scale(hx711_t *self, float scale);

// get the current SCALE
float hx711_get_scale(hx711_t *self);

// set OFFSET, the value that's subtracted from the actual reading (tare weight)
bool hx711_set_offset(hx711_t *self, long offset);

// get the current OFFSET
long hx711_get_offset(hx711_t *self);

// set times = how many times to read raw data to get weight
bool hx711_set_reads(hx711_t *self, uint8_t times);

// get the current number of reads to get weight
uint8_t hx711_get_reads(hx711_t *self);

// puts the chip into power down mode
void hx711_power_down(hx711_t *self);

// wakes up the chip after power down mode
void hx711_power_up(hx711_t *self);

// save configuration to the EEPROM memory
bool hx711_save(hx711_t *self);

// reload configuration frm EEPROM memory
bool hx711_load(hx711_t *self);

#endif // _HX711_H