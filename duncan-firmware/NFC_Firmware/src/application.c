// Tower Kit documentation https://tower.hardwario.com/
// SDK API description https://sdk.hardwario.com/
// Forum https://forum.hardwario.com/

#include <application.h>

all_settings_t settings;

static struct
{
    float_t temperature;
    float_t battery_voltage;
    float_t battery_pct;

} values;

// LED instance
twr_led_t led;

// Thermometer instance
twr_tmp112_t tmp112;

twr_tag_nfc_t tag_nfc;

uint64_t _radio_id;

bool update1_recieved = false;
bool update2_recieved = false;
bool new_update_configured = false;
bool first_update_done = false;

void twr_get_config1(uint64_t *id, const char *topic, void *value, void *param);
void twr_get_config2(uint64_t *id, const char *topic, void *value, void *param);
void twr_set_nfc(void *value, void *param);

static const twr_radio_sub_t subs[] = {
    {"raps/-/get/config1", TWR_RADIO_SUB_PT_STRING, twr_get_config1, NULL},
    {"raps/-/get/config2", TWR_RADIO_SUB_PT_STRING, twr_get_config2, NULL}
};

void twr_get_config1(uint64_t *id, const char *topic, void *value, void *param)
{
    (void)param;

    twr_log_debug("config1 recieved!");

    char *array[14]; // array with all settings
    int i = 0;
    char *p = strtok(value, ","); // change / to whatever you use to split the values

    while (p != NULL) // assign values to array
    {
        twr_log_debug(p);
        array[i++] = p;
        p = strtok(NULL, ",");
    }
    twr_log_info(array);

    settings.SERVICE_INTERVAL_INTERVAL = ((int)array[0] * 1000 * 60);
    settings.BATTERY_UPDATE_INTERVAL = ((int)array[1] * 1000 * 60);
    settings.UPDATE_SERVICE_INTERVAL = ((int)array[2] * 1000);
    settings.UPDATE_NORMAL_INTERVAL = ((int)array[3] * 1000);

    update1_recieved = true;

    twr_log_info("config loaded. executing main methods");
}

void twr_get_config2(uint64_t *id, const char *topic, void *value, void *param)
{
    (void)param;

    twr_log_debug("config2 recieved!");
    twr_set_nfc(value, NULL);
    update2_recieved = true;

    twr_log_info("config loaded. executing main methods");
}

void twr_set_nfc(void *value, void *param)
{
    (void) param;

    twr_log_debug(value);
    twr_log_debug("starting write");

    size_t value_size = strlen((char*)value);
    size_t block_size = 16;
    size_t max_buffer_size = TWR_TAG_NFC_BUFFER_SIZE;

    size_t aligned_length = (value_size + block_size - 1) / block_size * block_size;

    if (aligned_length > max_buffer_size)
    {
        twr_log_debug("Value size exceeds maximum buffer size");
        return;
    }

    if (twr_tag_nfc_memory_write(&tag_nfc, value, aligned_length))
    {
        twr_log_debug("nfc memory written");
    }
    else
    {
        twr_log_debug("failed");
    }
}

void tmp112_event_handler(twr_tmp112_t *self, twr_tmp112_event_t event, void *event_param)
{
    float value;
    event_param_t *param = (event_param_t *)event_param;

    if (event != TWR_TMP112_EVENT_UPDATE)
    {
        return;
    }

    if (twr_tmp112_get_temperature_celsius(self, &value))
    {
        if ((fabs(value - param->value) >= settings.TEMPERATURE_TAG_PUB_VALUE_CHANGE) || (param->next_pub < twr_scheduler_get_spin_tick()))
        {
            twr_radio_pub_temperature(param->channel, &value);
            param->value = value;
            param->next_pub = twr_scheduler_get_spin_tick() + settings.TEMPERATURE_TAG_PUB_NO_CHANGE_INTERVAL;

            values.temperature = value;
            twr_scheduler_plan_now(0);
        }
    }
}

void battery_event_handler(twr_module_battery_event_t event, void *event_param)
{
    (void) event;
    (void) event_param;

    float voltage;
    int percentage;

    if(event == TWR_MODULE_BATTERY_EVENT_UPDATE)
    {
        if (twr_module_battery_get_voltage(&voltage))
        {

            values.battery_voltage = voltage;
            twr_radio_pub_battery(&voltage);
        }

        if (twr_module_battery_get_charge_level(&percentage))
        {
            values.battery_pct = percentage;
        }
    }
}

void write_eeprom(void)
{

}

// Application initialization function which is called once after boot
void application_init(void)
{
    // Initialize logging
    twr_log_init(TWR_LOG_LEVEL_DUMP, TWR_LOG_TIMESTAMP_ABS);

    settings.SERVICE_INTERVAL_INTERVAL = (1 * 60 * 1000);
    settings.BATTERY_UPDATE_INTERVAL = (60 * 60 * 1000);
    settings.UPDATE_SERVICE_INTERVAL = (5 * 1000);
    settings.UPDATE_NORMAL_INTERVAL = (10 * 1000);
    settings.TEMPERATURE_UPDATE_NORMAL_INTERVAL = (60 * 60 * 1000);

    // Initialize LED
    twr_led_init(&led, TWR_GPIO_LED, false, false);
    twr_led_set_mode(&led, TWR_LED_MODE_OFF);

    // Initialize thermometer on core module
    twr_tmp112_init(&tmp112, TWR_I2C_I2C0, 0x49);

    // Initialize radio
    twr_radio_init(TWR_RADIO_MODE_NODE_LISTENING);
    twr_radio_set_rx_timeout_for_sleeping_node(500);
    twr_radio_set_subs((twr_radio_sub_t *)subs, sizeof(subs) / sizeof(twr_radio_sub_t));

    // Initialize battery
    twr_module_battery_init();
    twr_module_battery_set_event_handler(battery_event_handler, NULL);
    twr_module_battery_set_update_interval(settings.BATTERY_UPDATE_INTERVAL);

    // Initialize all components
    twr_tag_nfc_init(&tag_nfc, TWR_I2C_I2C0, TWR_TAG_NFC_I2C_ADDRESS_DEFAULT);

    static twr_tmp112_t temperature;
    static event_param_t temperature_event_param = { .next_pub = 0, .channel = TWR_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_ALTERNATE };
    twr_tmp112_init(&temperature, TWR_I2C_I2C0, 0x49);
    twr_tmp112_set_event_handler(&temperature, tmp112_event_handler, &temperature_event_param);
    twr_tmp112_set_update_interval(&temperature, settings.TEMPERATURE_UPDATE_NORMAL_INTERVAL);

    static event_param_t voc_lp_event_param = { .next_pub = 0 };

    twr_led_pulse(&led, 2000);
}

// Application task function (optional) which is called peridically if scheduled
void application_task(void)
{
    if (_radio_id == 0)
    {
        _radio_id = twr_radio_get_my_id();
        twr_log_debug("get Radio ID: %llX", _radio_id);
        if (_radio_id != 0)
        {
            // id is now known: initialize subscriptions
            static char id[32];
            sprintf(id, "%llX", _radio_id);
            twr_log_debug("register with ID %s", id);
            twr_radio_pairing_request(id, "1");
        }
        twr_scheduler_plan_current_from_now(2000);
        return;
    }
    if (update1_recieved && update2_recieved || new_update_configured)
    {
        new_update_configured = false;
        update1_recieved = false;
        update2_recieved = false;

        twr_radio_pub_bool("settings/are/applied", true);
    }
    static int counter = 0;

    // Log task run and increment counter
    twr_log_debug("APP: Task run (count: %d)", ++counter);

    // Plan next run of this task in 1000 ms
    //twr_scheduler_plan_current_from_now(settings.SERVICE_INTERVAL_INTERVAL);
    twr_scheduler_plan_current_from_now(1000);
}
