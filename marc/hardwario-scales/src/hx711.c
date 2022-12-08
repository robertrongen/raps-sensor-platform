/**
 * Ported to BigClown from HX711 library for Arduino
 * https://github.com/bogde/HX711
 * 
 * MIT License
 * (c) 2018 Bogdan Necula
 * (c) 2020 Matej
**/

#include "hx711.h"

#define _HX711_CLOCK 0

//#define LIB_DEBUG
//#define COREv1

// delay 
// ms = ticks
void _hx711_delay(long ms)
{
  if (ms<=0)
    return;
  twr_tick_t t = twr_tick_get()+ms;
  while (twr_tick_get()<t)
  {
    continue;
  }
}

// void _hx711_log(char* message)
// {
//   #ifdef LIB_DEBUG
//   #ifdef COREv1
//   for testing prposes
//   char buffer[100];
//   sprintf(buffer, "log:> %s\r\n", message);
//   twr_usb_cdc_write(buffer, strlen(buffer));
//   #else
//   twr_log_info("log:> %s\r\n", message);
//   #endif
//   #endif
// }

void _hx711_check(hx711_t *self)
{
  // #ifdef LIB_DEBUG 
  // // for testing purposes
  // int sck = twr_gpio_get_output(self->_PD_SCK);
  // int dout = twr_gpio_get_input(self->_DOUT);
  // char msg[50];
  // sprintf(msg, "check CLOCK=%d; INPUT=%d", sck, dout);
  // twr_log_debug(msg);
  // #endif
}


// Make shiftIn() be aware of clockspeed for
// faster CPUs like ESP32, Teensy 3.x and friends.
// See also:
// - https://github.com/bogde/HX711/issues/75
// - https://github.com/arduino/Arduino/issues/6561
// - https://community.hiveeyes.org/t/using-bogdans-canonical-hx711-library-on-the-esp32/539
uint8_t _hx711_shiftInSlow(twr_gpio_channel_t dataPin, twr_gpio_channel_t clockPin) 
{
    uint8_t value = 0;
    uint8_t i;

    for(i = 0; i < 8; ++i) 
    {
        twr_gpio_set_output(clockPin, 1);
        _hx711_delay(_HX711_CLOCK);
        value |= twr_gpio_get_input(dataPin) << (7 - i);
        twr_gpio_set_output(clockPin, 0);
        _hx711_delay(_HX711_CLOCK);
    }
    return value;
}

long _hx711_read_raw_internal(hx711_t *self) 
{
  // setup clock to 0 when is changed
  if (twr_gpio_get_output(self->_PD_SCK)==1)
    twr_gpio_set_output(self->_PD_SCK, 0);

  // Wait for the chip to become ready.
  if (!hx711_wait_ready_retry(self, 15, 1)) {
    twr_log_debug("hx711 not ready");
    return HX711_INVALID_VALUE;
  }

  // Define structures for reading data into.
  unsigned long value = 0;
  uint8_t data[3] = { 0 };
  uint8_t filler = 0x00;

  // Protect the read sequence from system interrupts.  If an interrupt occurs during
  // the time the PD_SCK signal is high it will stretch the length of the clock pulse.
  // If the total pulse time exceeds 60 uSec this will cause the HX711 to enter
  // power down mode during the middle of the read sequence.  While the device will
  // wake up when PD_SCK goes low again, the reset starts a new conversion cycle which
  // forces DOUT high until that cycle is completed.
  //
  // The result is that all subsequent bits read by shiftIn() will read back as 1,
  // corrupting the value returned by read().  The ATOMIC_BLOCK macro disables
  // interrupts during the sequence and then restores the interrupt mask to its previous
  // state after the sequence completes, insuring that the entire read-and-gain-set
  // sequence is not interrupted.  The macro has a few minor advantages over bracketing
  // the sequence between `noInterrupts()` and `interrupts()` calls.
 
  // Disable interrupts.
  // noInterrupts();
  self->_state = HX711_STATE_READING;
 
  // Pulse the clock pin 24 times to read the data.
  data[2] = _hx711_shiftInSlow(self->_DOUT, self->_PD_SCK);
  data[1] = _hx711_shiftInSlow(self->_DOUT, self->_PD_SCK);
  data[0] = _hx711_shiftInSlow(self->_DOUT, self->_PD_SCK);

  // Set the channel and the gain factor for the next reading using the clock pin.
  for (unsigned int i = 0; i < self->_gain; i++) 
  {
    twr_gpio_set_output(self->_PD_SCK, 1);
    _hx711_delay(_HX711_CLOCK);
    twr_gpio_set_output(self->_PD_SCK, 0);
    _hx711_delay(_HX711_CLOCK);
  }

  // Enable interrupts again.
  //interrupts();
  self->_state = HX711_STATE_READY;


  // Replicate the most significant bit to pad out a 32-bit signed integer
  if (data[2] & 0x80) 
  {
    filler = 0xFF;
  } 
  else 
  {
    filler = 0x00;
  }

  // Construct a 32-bit signed integer
  value = ( ((unsigned long)filler) << 24
      | ((unsigned long)data[2]) << 16
      | ((unsigned long)data[1]) << 8
      | ((unsigned long)data[0]) );

  return (long)value;
}

bool _measure_internal(hx711_t *self, hx711_event_t event_type)
{
  if (self->_hybernate)
    hx711_power_up(self);

  _hx711_check(self);
  double value = hx711_get_units(self);
  self->_event_handler(self, event_type, value, self->_event_param);  
  _hx711_check(self);

  if (self->_hybernate)
    hx711_power_down(self);  

  return true;
}

// executed on interval event 
void _hx711_task_interval(void *param)
{
  hx711_t *self = param;
  // twr_log_debug("interval task");
  _measure_internal(self, HX711_EVENT_UPDATE);

  twr_scheduler_plan_current_relative(self->_update_interval);
}

// set the gain factor; takes effect only after a call to read()
// channel A can be set for a 128 or 64 gain; channel B has a fixed 32 gain
// depending on the parameter, the channel is also set to either A or B
void _hx711_set_gain(hx711_t *self, hx711_channel_t channel) 
{
  switch (channel) 
  {
    case HX711_CHANNEL_A64:    // channel A, gain factor 64
      self->_gain = 3;
      break;
    case HX711_CHANNEL_B:    // channel B, gain factor 32
      self->_gain = 2;
      break;
    case HX711_CHANNEL_A:   // channel A, gain factor 128
    default:
      self->_gain = 1;
      break;
  }
}



void hx711_init(hx711_t *self, twr_gpio_channel_t dout, twr_gpio_channel_t pd_sck, hx711_channel_t channel) 
{
  twr_log_debug("hx711_init - entry");

  // #ifdef LIB_DEBUG
  // #ifdef COREv1
  // twr_usb_cdc_init();
  // #endif
  // #endif


  self->_state = HX711_STATE_INITIALIZE;
  self->_PD_SCK = pd_sck;
  self->_DOUT = dout;

  twr_gpio_init(self->_PD_SCK);
  twr_gpio_set_mode(self->_PD_SCK, TWR_GPIO_MODE_OUTPUT);

  twr_gpio_init(self->_DOUT);
  twr_gpio_set_mode(self->_DOUT, TWR_GPIO_MODE_INPUT);
  twr_gpio_set_pull(self->_DOUT, TWR_GPIO_PULL_UP);

  // _hx711_delay(1000);

  _hx711_set_gain(self, channel);

  // _hx711_delay(1000);

  self->_task_id_interval = twr_scheduler_register(_hx711_task_interval, self, TWR_TICK_INFINITY);
  self->_times = _HX711_TIMES;
  self->_scale = 1;
  self->_hybernate = false;

  twr_log_debug("initialized");
}

void hx711_set_event_handler(hx711_t *self, void (*event_handler)(hx711_t *, hx711_event_t, double, void *), void *event_param)
{
  self->_event_handler = event_handler;
  self->_event_param = event_param;

  twr_log_debug("event handler set");
}

void hx711_set_update_interval(hx711_t *self, twr_tick_t interval)
{
    self->_update_interval = interval;

    if (self->_update_interval == TWR_TICK_INFINITY)
    {
        twr_scheduler_plan_absolute(self->_task_id_interval, TWR_TICK_INFINITY);
    }
    else
    {
        twr_scheduler_plan_relative(self->_task_id_interval, self->_update_interval);
        // self->_hybernate = (interval>1000)?true:false;
        self->_hybernate = false; 
    }

    twr_log_debug("interval set");
}

bool hx711_is_ready(hx711_t *self) 
{
   return twr_gpio_get_input(self->_DOUT) == 0;
}


void hx711_wait_ready(hx711_t *self, unsigned long delay_ms) 
{
  // Wait for the chip to become ready.
  // This is a blocking implementation and will
  // halt the sketch until a load cell is connected.
  while (!hx711_is_ready(self)) 
  {
    // Probably will do no harm on AVR but will feed the Watchdog Timer (WDT) on ESP.
    // https://github.com/bogde/HX711/issues/73
    _hx711_delay(delay_ms);
  }
}

bool hx711_wait_ready_retry(hx711_t *self, int retries, unsigned long delay_ms) {
  // Wait for the chip to become ready by
  // retrying for a specified amount of attempts.
  // https://github.com/bogde/HX711/issues/76
  int count = 0;
  while (count < retries) 
  {
    if (hx711_is_ready(self)) 
      return true;
    
    _hx711_delay(delay_ms);
    count++;
  }
  return false;
}

bool hx711_wait_ready_timeout(hx711_t *self, unsigned long timeout, unsigned long delay_ms) 
{
  // Wait for the chip to become ready until timeout.
  // https://github.com/bogde/HX711/pull/96
  twr_tick_t millisStarted = twr_tick_get();
  while (twr_tick_get() - millisStarted < timeout) 
  {
    if (hx711_is_ready(self)) 
    {
      return true;
    }
    _hx711_delay(delay_ms);
  }
  return false;
}

long hx711_read_raw(hx711_t *self) 
{
  return _hx711_read_raw_internal(self);
}

long hx711_read_raw_average(hx711_t *self, uint8_t times) 
{
  long sum = 0;
  for (uint8_t i = 0; i < times; i++) 
  {
    sum += _hx711_read_raw_internal(self);
    // Probably will do no harm on AVR but will feed the Watchdog Timer (WDT) on ESP.
    // https://github.com/bogde/HX711/issues/73
    _hx711_delay(0);
  }
  return sum / times;
}

// return measured value decreased by offset (zero)
double hx711_get_value(hx711_t *self) 
{
  return hx711_read_raw_average(self, self->_times) - self->_offset;
}

// return weigth in units (kgs/lbs)
float hx711_get_units(hx711_t *self)
{
  return hx711_get_value(self) / self->_scale;
}

bool hx711_measure(hx711_t *self)
{
  return _measure_internal(self, HX711_EVENT_MEASURE);
}

// get current raw number and set it as zero
bool hx711_tare(hx711_t *self) 
{
  double raw = hx711_read_raw_average(self, self->_times);
  if (raw==0)
    return false;
  // set the offset - zero
  hx711_set_offset(self, raw);
  
  return true;
}

// get current raw number and set scale coeficient for known weight
bool hx711_calibrate(hx711_t *self, float weight)
{
  if (weight==0)
    return false;

  long raw = hx711_read_raw_average(self, self->_times);
  if (raw==0)
    return false;

  double coef = (raw - self->_offset)/weight;
  hx711_set_scale(self, coef);

  return true;
}


// set scale coefficient - measured value vs weight unit
bool hx711_set_scale(hx711_t *self, float scale) 
{
  if (scale==0)
    return false;

  self->_scale = scale;
  if (self->_state == HX711_STATE_INITIALIZE)
    self->_state = HX711_STATE_READY;
  
  return true;
}

// get scale coefficient
float hx711_get_scale(hx711_t *self) 
{
  return self->_scale;
}


// set offset - measured value vs zero
bool hx711_set_offset(hx711_t *self, long offset) 
{
  self->_offset = offset;

  return true;
}

// get offset
long hx711_get_offset(hx711_t *self) 
{
  return self->_offset;
}


// how many reads from scale is required when measuring
bool hx711_set_reads(hx711_t *self, uint8_t times)
{
  if (times<=0)
    return false;
  
  self->_times = times;
  return true;
}

uint8_t hx711_get_reads(hx711_t *self)
{
  return self->_times;
}


// turn of the scales
void hx711_power_down(hx711_t *self) 
{
    twr_gpio_set_output(self->_PD_SCK, 0);
    _hx711_delay(0);
    twr_gpio_set_output(self->_PD_SCK, 1);
    _hx711_delay(5);

    self->_state = HX711_STATE_SLEEP;
    twr_log_debug("power down");
}


// power up scales
void hx711_power_up(hx711_t *self) 
{
  twr_gpio_set_output(self->_PD_SCK, 0);
  self->_state = HX711_STATE_READY;
  twr_log_debug("power up");
}

typedef struct hx711_save_t hx711_save_t;
struct hx711_save_t
{
  long offset;
  float scale;
  uint8_t times;
};


bool hx711_save(hx711_t *self)
{
  twr_log_debug("save config - start");
  hx711_save_t tmp;
  tmp.offset = self->_offset;
  tmp.scale = self->_scale;
  tmp.times = self->_times;
  return twr_eeprom_write(_HX711_MEM_ADDRESS, &(tmp), sizeof(tmp));
}

bool hx711_load(hx711_t *self)
{
  twr_log_debug("Load config - start");
  hx711_save_t tmp;
  if (!twr_eeprom_read(_HX711_MEM_ADDRESS, &(tmp), sizeof(tmp)))
    return false;
  if (tmp.scale==0 || tmp.times<=0)
    return false;
  
  hx711_set_offset(self, tmp.offset);
  hx711_set_scale(self, tmp.scale);
  hx711_set_reads(self, tmp.times);

  self->_state = HX711_STATE_READY;

  twr_log_debug("Load config - success");
  return true;
}
