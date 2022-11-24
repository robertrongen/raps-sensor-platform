#include <bc_hdc2080.h>

// TODO Clarify timing with TI
#define _BC_HDC2080_DELAY_RUN 50
#define _BC_HDC2080_DELAY_INITIALIZATION 50
#define _BC_HDC2080_DELAY_MEASUREMENT 50

static void _bc_hdc2080_task_interval(void *param);

static void _bc_hdc2080_task_measure(void *param);

void bc_hdc2080_init(bc_hdc2080_t *self, bc_i2c_channel_t i2c_channel, uint8_t i2c_address)
{
    memset(self, 0, sizeof(*self));

    self->_i2c_channel = i2c_channel;
    self->_i2c_address = i2c_address;

    self->_task_id_interval = bc_scheduler_register(_bc_hdc2080_task_interval, self, BC_TICK_INFINITY);
    self->_task_id_measure = bc_scheduler_register(_bc_hdc2080_task_measure, self, _BC_HDC2080_DELAY_RUN);

    self->_tick_ready = _BC_HDC2080_DELAY_RUN;

    bc_i2c_init(self->_i2c_channel, BC_I2C_SPEED_400_KHZ);
}

void bc_hdc2080_set_event_handler(bc_hdc2080_t *self, void (*event_handler)(bc_hdc2080_t *, bc_hdc2080_event_t, void *), void *event_param)
{
    self->_event_handler = event_handler;
    self->_event_param = event_param;
}

void bc_hdc2080_set_update_interval(bc_hdc2080_t *self, bc_tick_t interval)
{
    self->_update_interval = interval;

    if (self->_update_interval == BC_TICK_INFINITY)
    {
        bc_scheduler_plan_absolute(self->_task_id_interval, BC_TICK_INFINITY);
    }
    else
    {
        bc_scheduler_plan_relative(self->_task_id_interval, self->_update_interval);

        bc_hdc2080_measure(self);
    }
}

bool bc_hdc2080_measure(bc_hdc2080_t *self)
{
    if (self->_measurement_active)
    {
        return false;
    }

    self->_measurement_active = true;

    bc_scheduler_plan_absolute(self->_task_id_measure, self->_tick_ready);

    return true;
}

bool bc_hdc2080_get_humidity_raw(bc_hdc2080_t *self, uint16_t *raw)
{
    if (!self->_humidity_valid)
    {
        return false;
    }

    *raw = self->_reg_humidity;

    return true;
}

bool bc_hdc2080_get_humidity_percentage(bc_hdc2080_t *self, float *percentage)
{
    uint16_t raw;

    if (!bc_hdc2080_get_humidity_raw(self, &raw))
    {
        return false;
    }

    *percentage = (float) raw / 65536.f * 100.f;

    if (*percentage >= 100.f)
    {
        *percentage = 100.f;
    }

    return true;
}

bool bc_hdc2080_get_temperature_raw(bc_hdc2080_t *self, uint16_t *raw)
{
    if (!self->_temperature_valid)
    {
        return false;
    }

    *raw = self->_reg_temperature;

    return true;
}

bool bc_hdc2080_get_temperature_celsius(bc_hdc2080_t *self, float *celsius)
{
    uint16_t raw;

    if (!bc_hdc2080_get_temperature_raw(self, &raw))
    {
        return false;
    }

    *celsius = (float) raw / 65536.f * 165.f - 40.f;

    return true;
}

static void _bc_hdc2080_task_interval(void *param)
{
    bc_hdc2080_t *self = param;

    bc_hdc2080_measure(self);

    bc_scheduler_plan_current_relative(self->_update_interval);
}

static void _bc_hdc2080_task_measure(void *param)
{
    bc_hdc2080_t *self = param;

start:

    switch (self->_state)
    {
        case BC_HDC2080_STATE_ERROR:
        {
            self->_humidity_valid = false;
            self->_temperature_valid = false;

            self->_measurement_active = false;

            if (self->_event_handler != NULL)
            {
                self->_event_handler(self, BC_HDC2080_EVENT_ERROR, self->_event_param);
            }

            self->_state = BC_HDC2080_STATE_INITIALIZE;

            return;
        }
        case BC_HDC2080_STATE_INITIALIZE:
        {
            self->_state = BC_HDC2080_STATE_ERROR;

            if (!bc_i2c_memory_write_8b(self->_i2c_channel, self->_i2c_address, 0x0e, 0x80))
            {
                goto start;
            }

            self->_state = BC_HDC2080_STATE_MEASURE;

            self->_tick_ready = bc_tick_get() + _BC_HDC2080_DELAY_INITIALIZATION;

            if (self->_measurement_active)
            {
                bc_scheduler_plan_current_absolute(self->_tick_ready);
            }

            return;
        }
        case BC_HDC2080_STATE_MEASURE:
        {
            self->_state = BC_HDC2080_STATE_ERROR;

            if (!bc_i2c_memory_write_8b(self->_i2c_channel, self->_i2c_address, 0x0f, 0x07))
            {
                goto start;
            }

            self->_state = BC_HDC2080_STATE_READ;

            bc_scheduler_plan_current_from_now(_BC_HDC2080_DELAY_MEASUREMENT);

            return;
        }
        case BC_HDC2080_STATE_READ:
        {
            self->_state = BC_HDC2080_STATE_ERROR;

            uint8_t reg_interrupt;

            if (!bc_i2c_memory_read_8b(self->_i2c_channel, self->_i2c_address, 0x04, &reg_interrupt))
            {
                goto start;
            }

            if ((reg_interrupt & 0x80) == 0)
            {
                goto start;
            }

            if (!bc_i2c_memory_read_16b(self->_i2c_channel, self->_i2c_address, 0x02, &self->_reg_humidity))
            {
                goto start;
            }

            if (!bc_i2c_memory_read_16b(self->_i2c_channel, self->_i2c_address, 0x00, &self->_reg_temperature))
            {
                goto start;
            }

            self->_reg_temperature = self->_reg_temperature << 8 | self->_reg_temperature >> 8;
            self->_reg_humidity = self->_reg_humidity << 8 | self->_reg_humidity >> 8;

            self->_temperature_valid = true;
            self->_humidity_valid = true;

            self->_state = BC_HDC2080_STATE_UPDATE;

            goto start;
        }
        case BC_HDC2080_STATE_UPDATE:
        {
            self->_measurement_active = false;

            if (self->_event_handler != NULL)
            {
                self->_event_handler(self, BC_HDC2080_EVENT_UPDATE, self->_event_param);
            }

            self->_state = BC_HDC2080_STATE_MEASURE;

            return;
        }
        default:
        {
            self->_state = BC_HDC2080_STATE_ERROR;

            goto start;
        }
    }
}
