#include <bc_tag_humidity.h>

static void _bc_tag_humidity_event_handler_hts221(bc_hts221_t *child, bc_hts221_event_t event, void *event_param);

static void _bc_tag_humidity_event_handler_hdc2080(bc_hdc2080_t *child, bc_hdc2080_event_t event, void *event_param);

static void _bc_tag_humidity_event_handler_sht20(bc_sht20_t *child, bc_sht20_event_t event, void *event_param);

static void _bc_tag_humidity_event_handler_sht30(bc_sht30_t *child, bc_sht30_event_t event, void *event_param);

void bc_tag_humidity_init(bc_tag_humidity_t *self, bc_tag_humidity_revision_t revision, bc_i2c_channel_t i2c_channel, bc_tag_humidity_i2c_address_t i2c_address)
{
    memset(self, 0, sizeof(*self));

    self->_revision = revision;

    if (self->_revision == BC_TAG_HUMIDITY_REVISION_R1)
    {
        bc_hts221_init(&self->_sensor.hts221, i2c_channel, 0x5f);

        bc_hts221_set_event_handler(&self->_sensor.hts221, _bc_tag_humidity_event_handler_hts221, self);
    }
    else if (self->_revision == BC_TAG_HUMIDITY_REVISION_R2)
    {
        bc_hdc2080_init(&self->_sensor.hdc2080, i2c_channel, i2c_address == BC_TAG_HUMIDITY_I2C_ADDRESS_DEFAULT ? 0x40 : 0x41);

        bc_hdc2080_set_event_handler(&self->_sensor.hdc2080, _bc_tag_humidity_event_handler_hdc2080, self);
    }
    else if (self->_revision == BC_TAG_HUMIDITY_REVISION_R3)
    {
        bc_sht20_init(&self->_sensor.sht20, i2c_channel, 0x40);

        bc_sht20_set_event_handler(&self->_sensor.sht20, _bc_tag_humidity_event_handler_sht20, self);
    }
    else
    {
        bc_sht30_init(&self->_sensor.sht30, i2c_channel, i2c_address == BC_TAG_HUMIDITY_I2C_ADDRESS_DEFAULT ? 0x44 : 0x45);

        bc_sht30_set_event_handler(&self->_sensor.sht30, _bc_tag_humidity_event_handler_sht30, self);
    }
}

void bc_tag_humidity_set_event_handler(bc_tag_humidity_t *self, void (*event_handler)(bc_tag_humidity_t *, bc_tag_humidity_event_t, void *), void *event_param)
{
    self->_event_handler = event_handler;
    self->_event_param = event_param;
}

void bc_tag_humidity_set_update_interval(bc_tag_humidity_t *self, bc_tick_t interval)
{
    if (self->_revision == BC_TAG_HUMIDITY_REVISION_R1)
    {
        bc_hts221_set_update_interval(&self->_sensor.hts221, interval);
    }
    else if (self->_revision == BC_TAG_HUMIDITY_REVISION_R2)
    {
        bc_hdc2080_set_update_interval(&self->_sensor.hdc2080, interval);
    }
    else if (self->_revision == BC_TAG_HUMIDITY_REVISION_R3)
    {
        bc_sht20_set_update_interval(&self->_sensor.sht20, interval);
    }
    else
    {
        bc_sht30_set_update_interval(&self->_sensor.sht30, interval);
    }
}

bool bc_tag_humidity_measure(bc_tag_humidity_t *self)
{
    if (self->_revision == BC_TAG_HUMIDITY_REVISION_R1)
    {
        return bc_hts221_measure(&self->_sensor.hts221);
    }
    else if (self->_revision == BC_TAG_HUMIDITY_REVISION_R2)
    {
        return bc_hdc2080_measure(&self->_sensor.hdc2080);
    }
    else if (self->_revision == BC_TAG_HUMIDITY_REVISION_R3)
    {
        return bc_sht20_measure(&self->_sensor.sht20);
    }
    else
    {
        return bc_sht30_measure(&self->_sensor.sht30);
    }
}

bool bc_tag_humidity_get_temperature_raw(bc_tag_humidity_t *self, uint16_t *raw)
{
    if (self->_revision == BC_TAG_HUMIDITY_REVISION_R1)
    {
        return false;
    }
    else if (self->_revision == BC_TAG_HUMIDITY_REVISION_R2)
    {
        return bc_hdc2080_get_temperature_raw(&self->_sensor.hdc2080, raw);
    }
    else if (self->_revision == BC_TAG_HUMIDITY_REVISION_R3)
    {
        return bc_sht20_get_temperature_raw(&self->_sensor.sht20, raw);
    }
    else
    {
        return bc_sht30_get_temperature_raw(&self->_sensor.sht30, raw);
    }
}

bool bc_tag_humidity_get_temperature_celsius(bc_tag_humidity_t *self, float *celsius)
{
    if (self->_revision == BC_TAG_HUMIDITY_REVISION_R1)
    {
        return false;
    }
    else if (self->_revision == BC_TAG_HUMIDITY_REVISION_R2)
    {
        return bc_hdc2080_get_temperature_celsius(&self->_sensor.hdc2080, celsius);
    }
    else if (self->_revision == BC_TAG_HUMIDITY_REVISION_R3)
    {
        return bc_sht20_get_temperature_celsius(&self->_sensor.sht20, celsius);
    }
    else
    {
        return bc_sht30_get_temperature_celsius(&self->_sensor.sht30, celsius);
    }
}

bool bc_tag_humidity_get_humidity_raw(bc_tag_humidity_t *self, uint16_t *raw)
{
    if (self->_revision == BC_TAG_HUMIDITY_REVISION_R1)
    {
        return false;
    }
    else if (self->_revision == BC_TAG_HUMIDITY_REVISION_R2)
    {
        return bc_hdc2080_get_humidity_raw(&self->_sensor.hdc2080, raw);
    }
    else if (self->_revision == BC_TAG_HUMIDITY_REVISION_R3)
    {
        return bc_sht20_get_humidity_raw(&self->_sensor.sht20, raw);
    }
    else
    {
        return bc_sht30_get_humidity_raw(&self->_sensor.sht30, raw);
    }
}

bool bc_tag_humidity_get_humidity_percentage(bc_tag_humidity_t *self, float *percentage)
{
    if (self->_revision == BC_TAG_HUMIDITY_REVISION_R1)
    {
        return bc_hts221_get_humidity_percentage(&self->_sensor.hts221, percentage);
    }
    else  if (self->_revision == BC_TAG_HUMIDITY_REVISION_R2)
    {
        return bc_hdc2080_get_humidity_percentage(&self->_sensor.hdc2080, percentage);
    }
    else if (self->_revision == BC_TAG_HUMIDITY_REVISION_R3)
    {
        return bc_sht20_get_humidity_percentage(&self->_sensor.sht20, percentage);
    }
    else
    {
        return bc_sht30_get_humidity_percentage(&self->_sensor.sht30, percentage);
    }
}

static void _bc_tag_humidity_event_handler_hts221(bc_hts221_t *child, bc_hts221_event_t event, void *event_param)
{
    (void) child;

    bc_tag_humidity_t *self = event_param;

    if (self->_event_handler != NULL)
    {
        if (event == BC_HTS221_EVENT_UPDATE)
        {
            self->_event_handler(self, BC_TAG_HUMIDITY_EVENT_UPDATE, self->_event_param);
        }
        else if (event == BC_HTS221_EVENT_ERROR)
        {
            self->_event_handler(self, BC_TAG_HUMIDITY_EVENT_ERROR, self->_event_param);
        }
    }
}

static void _bc_tag_humidity_event_handler_hdc2080(bc_hdc2080_t *child, bc_hdc2080_event_t event, void *event_param)
{
    (void) child;

    bc_tag_humidity_t *self = event_param;

    if (self->_event_handler != NULL)
    {
        if (event == BC_HDC2080_EVENT_UPDATE)
        {
            self->_event_handler(self, BC_TAG_HUMIDITY_EVENT_UPDATE, self->_event_param);
        }
        else if (event == BC_HDC2080_EVENT_ERROR)
        {
            self->_event_handler(self, BC_TAG_HUMIDITY_EVENT_ERROR, self->_event_param);
        }
    }
}

static void _bc_tag_humidity_event_handler_sht20(bc_sht20_t *child, bc_sht20_event_t event, void *event_param)
{
    (void) child;

    bc_tag_humidity_t *self = event_param;

    if (self->_event_handler != NULL)
    {
        if (event == BC_SHT20_EVENT_UPDATE)
        {
            self->_event_handler(self, BC_TAG_HUMIDITY_EVENT_UPDATE, self->_event_param);
        }
        else if (event == BC_SHT20_EVENT_ERROR)
        {
            self->_event_handler(self, BC_TAG_HUMIDITY_EVENT_ERROR, self->_event_param);
        }
    }
}

static void _bc_tag_humidity_event_handler_sht30(bc_sht30_t *child, bc_sht30_event_t event, void *event_param)
{
    (void) child;

    bc_tag_humidity_t *self = event_param;

    if (self->_event_handler != NULL)
    {
        if (event == BC_SHT30_EVENT_UPDATE)
        {
            self->_event_handler(self, BC_TAG_HUMIDITY_EVENT_UPDATE, self->_event_param);
        }
        else if (event == BC_SHT30_EVENT_ERROR)
        {
            self->_event_handler(self, BC_TAG_HUMIDITY_EVENT_ERROR, self->_event_param);
        }
    }
}
