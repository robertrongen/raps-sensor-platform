// Tower Kit documentation https://tower.hardwario.com/
// SDK API description https://sdk.hardwario.com/
// Forum https://forum.hardwario.com/

#include <application.h>

all_settings_t settings;

struct
{
    event_param_t temperature;
    event_param_t humidity;
    event_param_t illuminance;
    event_param_t pressure;
} params;

// LED instance
twr_led_t led;

// Thermometer instance
twr_tmp112_t tmp112;

uint64_t _radio_id;

int updateSchedule = 1000;

bool update1_recieved = false;
bool update2_recieved = false;
bool new_update_configured = false;
bool first_update_done = false;

void twr_get_config1(uint64_t *id, const char *topic, void *value, void *param);
void twr_get_config2(uint64_t *id, const char *topic, void *value, void *param);
void twr_update(uint64_t *id, const char *topic, void *value, void *param);

static const twr_radio_sub_t subs[] = {
    {"raps/-/get/config1", TWR_RADIO_SUB_PT_STRING, twr_get_config1, NULL},
    {"raps/-/get/config2", TWR_RADIO_SUB_PT_STRING, twr_get_config2, NULL},
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
    settings.BAROMETER_UPDATE_SERVICE_INTERVAL = ((int)array[4] * 1000 * 60);
    settings.BAROMETER_UPDATE_NORMAL_INTERVAL = ((int)array[5] * 1000 * 60);
    settings.TEMPERATURE_UPDATE_SERVICE_INTERVAL = ((int)array[6] * 1000 * 60);
    settings.TEMPERATURE_UPDATE_NORMAL_INTERVAL = ((int)array[7] * 1000 * 60);
    settings.HUMIDITY_UPDATE_SERVICE_INTERVAL = ((int)array[8] * 1000 * 60);
    settings.HUMIDITY_UPDATE_NORMAL_INTERVAL = ((int)array[9] * 1000 * 60);

    update1_recieved = true;

    twr_log_info("config loaded. executing main methods");
}

void twr_get_config2(uint64_t *id, const char *topic, void *value, void *param)
{
    (void)param;

    twr_log_debug("config2 recieved!");

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

    settings.LUX_METER_UPDATE_SERVICE_INTERVAL = ((int)array[0] * 1000 * 60);
    settings.LUX_METER_UPDATE_NORMAL_INTERVAL = ((int)array[1] * 1000 * 60);
    settings.TEMPERATURE_TAG_PUB_NO_CHANGE_INTERVAL = ((int)array[2] * 1000 * 60);
    settings.TEMPERATURE_TAG_PUB_VALUE_CHANGE = ((int)array[3]);
    settings.HUMIDITY_TAG_PUB_NO_CHANGE_INTERVAL = ((int)array[4] * 1000 * 60);
    settings.HUMIDITY_TAG_PUB_VALUE_CHANGE = ((int)array[5]);
    settings.LUX_METER_TAG_PUB_NO_CHANGE_INTERVAL = ((int)array[6] * 1000 * 60);
    settings.LUX_METER_TAG_PUB_VALUE_CHANGE = ((int)array[7]);
    settings.BAROMETER_TAG_PUB_NO_CHANGE_INTERVAL = ((int)array[8] * 1000 * 60);
    settings.BAROMETER_TAG_PUB_VALUE_CHANGE = ((int)array[9]);

    update2_recieved = true;

    twr_log_info("config loaded. executing main methods");
}

void battery_event_handler(twr_module_battery_event_t event, void *event_param)
{
    (void)event_param;

    float voltage;

    if (event == TWR_MODULE_BATTERY_EVENT_UPDATE)
    {
        if (twr_module_battery_get_voltage(&voltage))
        {
            twr_radio_pub_battery(&voltage);
        }
    }
}

void climate_module_event_handler(twr_module_climate_event_t event, void *event_param)
{
    (void)event_param;

    float value;

    if (event == TWR_MODULE_CLIMATE_EVENT_UPDATE_THERMOMETER)
    {
        if (twr_module_climate_get_temperature_celsius(&value))
        {
            if ((fabs(value - params.temperature.value) >= settings.TEMPERATURE_TAG_PUB_VALUE_CHANGE) || (params.temperature.next_pub < twr_scheduler_get_spin_tick()))
            {
                twr_radio_pub_temperature(TWR_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_DEFAULT, &value);
                params.temperature.value = value;
                params.temperature.next_pub = twr_scheduler_get_spin_tick() + settings.TEMPERATURE_TAG_PUB_NO_CHANGE_INTERVAL;
            }
        }
    }
    else if (event == TWR_MODULE_CLIMATE_EVENT_UPDATE_HYGROMETER)
    {
        if (twr_module_climate_get_humidity_percentage(&value))
        {
            if ((fabs(value - params.humidity.value) >= settings.HUMIDITY_TAG_PUB_VALUE_CHANGE) || (params.humidity.next_pub < twr_scheduler_get_spin_tick()))
            {
                twr_radio_pub_humidity(TWR_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_DEFAULT, &value);
                params.humidity.value = value;
                params.humidity.next_pub = twr_scheduler_get_spin_tick() + settings.HUMIDITY_TAG_PUB_NO_CHANGE_INTERVAL;
            }
        }
    }
    else if (event == TWR_MODULE_CLIMATE_EVENT_UPDATE_LUX_METER)
    {
        if (twr_module_climate_get_illuminance_lux(&value))
        {
            if (value < 1)
            {
                value = 0;
            }
            if ((fabs(value - params.illuminance.value) >= settings.LUX_METER_TAG_PUB_VALUE_CHANGE) || (params.illuminance.next_pub < twr_scheduler_get_spin_tick()) ||
                ((value == 0) && (params.illuminance.value != 0)) || ((value > 1) && (params.illuminance.value == 0)))
            {
                twr_radio_pub_luminosity(TWR_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_DEFAULT, &value);
                params.illuminance.value = value;
                params.illuminance.next_pub = twr_scheduler_get_spin_tick() + settings.LUX_METER_TAG_PUB_NO_CHANGE_INTERVAL;
            }
        }
    }
    else if (event == TWR_MODULE_CLIMATE_EVENT_UPDATE_BAROMETER)
    {
        if (twr_module_climate_get_pressure_pascal(&value))
        {
            if ((fabs(value - params.pressure.value) >= settings.BAROMETER_TAG_PUB_VALUE_CHANGE) || (params.pressure.next_pub < twr_scheduler_get_spin_tick()))
            {
                float meter;

                if (!twr_module_climate_get_altitude_meter(&meter))
                {
                    return;
                }

                twr_radio_pub_barometer(TWR_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_DEFAULT, &value, &meter);
                params.pressure.value = value;
                params.pressure.next_pub = twr_scheduler_get_spin_tick() + settings.BAROMETER_TAG_PUB_NO_CHANGE_INTERVAL;
            }
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
    settings.BAROMETER_UPDATE_SERVICE_INTERVAL = (1 * 60 * 1000);
    settings.BAROMETER_UPDATE_NORMAL_INTERVAL = (5 * 60 * 1000);
    settings.TEMPERATURE_UPDATE_SERVICE_INTERVAL = (1 * 60 * 1000);
    settings.TEMPERATURE_UPDATE_NORMAL_INTERVAL = (5 * 60 * 1000);
    settings.HUMIDITY_UPDATE_SERVICE_INTERVAL = (1 * 60 * 1000);
    settings.HUMIDITY_UPDATE_NORMAL_INTERVAL = (5 * 60 * 1000);
    settings.LUX_METER_UPDATE_SERVICE_INTERVAL = (1 * 60 * 1000);
    settings.LUX_METER_UPDATE_NORMAL_INTERVAL = (5 * 60 * 1000);
    settings.TEMPERATURE_TAG_PUB_NO_CHANGE_INTERVAL = (15 * 60 * 1000);
    settings.TEMPERATURE_TAG_PUB_VALUE_CHANGE = 0.2f;
    settings.HUMIDITY_TAG_PUB_NO_CHANGE_INTERVAL = (15 * 60 * 1000);
    settings.HUMIDITY_TAG_PUB_VALUE_CHANGE = 5.0f;
    settings.LUX_METER_TAG_PUB_NO_CHANGE_INTERVAL = (15 * 60 * 1000);
    settings.LUX_METER_TAG_PUB_VALUE_CHANGE = 25.0f;
    settings.BAROMETER_TAG_PUB_NO_CHANGE_INTERVAL = (15 * 60 * 1000);
    settings.BAROMETER_TAG_PUB_VALUE_CHANGE = 20.0f;

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

    // Initialize climate module
    twr_module_climate_init();
    twr_module_climate_set_event_handler(climate_module_event_handler, NULL);
    twr_module_climate_set_update_interval_thermometer(settings.TEMPERATURE_UPDATE_SERVICE_INTERVAL);
    twr_module_climate_set_update_interval_hygrometer(settings.HUMIDITY_UPDATE_SERVICE_INTERVAL);
    twr_module_climate_set_update_interval_lux_meter(settings.LUX_METER_UPDATE_SERVICE_INTERVAL);
    twr_module_climate_set_update_interval_barometer(settings.BAROMETER_UPDATE_SERVICE_INTERVAL);
    twr_module_climate_measure_all_sensors();

    size_t eeprom = twr_eeprom_get_size();
    twr_log_debug("EEPROM size: %d", eeprom);
    char testWrite[] = "test";
    //twr_eeprom_write(0, testWrite, sizeof(testWrite));
    twr_log_debug("size is %d", sizeof(testWrite));
    char read[7];
    twr_eeprom_read(0, read, 4);
    read[4] = '\0';
    twr_log_debug("Data: %s", read);
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
            char *id = malloc(32);
            sprintf(id, "%llX", _radio_id);
            twr_log_debug("register with ID %s", id);
            twr_radio_pairing_request(id, "1");
        }
        twr_scheduler_plan_current_from_now(1000);
        return;
    }
    if (update1_recieved && update2_recieved || new_update_configured)
    {
        twr_log_info("UPDATE 1 AND 2 RECIEVED AND WILL BE APPLIED");
        twr_module_climate_set_update_interval_thermometer(settings.TEMPERATURE_UPDATE_SERVICE_INTERVAL);
        twr_module_climate_set_update_interval_hygrometer(settings.HUMIDITY_UPDATE_SERVICE_INTERVAL);
        twr_module_climate_set_update_interval_lux_meter(settings.LUX_METER_UPDATE_SERVICE_INTERVAL);
        twr_module_climate_set_update_interval_barometer(settings.BAROMETER_UPDATE_SERVICE_INTERVAL);
        twr_module_climate_measure_all_sensors();
        new_update_configured = false;
        update1_recieved = false;
        update2_recieved = false;

        twr_radio_pub_bool("settings/are/applied", true)
    }
    static int counter = 0;

    // Log task run and increment counter
    twr_log_debug("APP: Task run (count: %d)", ++counter);

    // Plan next run of this task in 1000 ms
    //twr_scheduler_plan_current_from_now(settings.SERVICE_INTERVAL_INTERVAL);
    twr_scheduler_plan_current_from_now(settings.SERVICE_INTERVAL_INTERVAL);
}
