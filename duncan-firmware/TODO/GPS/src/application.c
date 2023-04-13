// Tower Kit documentation https://tower.hardwario.com/
// SDK API description https://sdk.hardwario.com/
// Forum https://forum.hardwario.com/

#include <application.h>

#define MAX_PAGE_INDEX 3

#define PAGE_INDEX_MENU -1

all_settings_t settings;

static struct
{
    float_t temperature;
    float_t humidity;
    float_t tvoc;
    float_t pressure;
    float_t altitude;
    float_t co2_concentation;
    float_t battery_voltage;
    float_t battery_pct;

} values;

static const struct
{
    char *name0;
    char *format0;
    float_t *value0;
    char *unit0;
    char *name1;
    char *format1;
    float_t *value1;
    char *unit1;

} pages[] = {
    {"Temperature   ", "%.1f", &values.temperature, "\xb0" "C",
     "Humidity      ", "%.1f", &values.humidity, "%"},
    {"CO2           ", "%.0f", &values.co2_concentation, "ppm",
     "TVOC          ", "%.1f", &values.tvoc, "ppb"},
    {"Pressure      ", "%.0f", &values.pressure, "hPa",
     "Altitude      ", "%.1f", &values.altitude, "m"},
    {"Battery       ", "%.2f", &values.battery_voltage, "V",
     "Battery       ", "%.0f", &values.battery_pct, "%"},
};

static int page_index = 0;
static int menu_item = 0;

static struct
{
    twr_tick_t next_update;
    bool mqtt;

} lcd;

twr_led_t led_lcd_green;

// LED instance
twr_led_t led;

// Thermometer instance
twr_tmp112_t tmp112;

twr_tag_barometer_t tag_barometer;
twr_tag_humidity_t tag_humidity;
twr_tag_voc_lp_t tag_voc_lp;
twr_tag_nfc_t tag_nfc;

uint64_t _radio_id;

bool update1_recieved = false;
bool update2_recieved = false;
bool new_update_configured = false;
bool first_update_done = false;

void twr_get_config1(uint64_t *id, const char *topic, void *value, void *param);
void twr_get_config2(uint64_t *id, const char *topic, void *value, void *param);
void twr_set_nfc(uint64_t *id, const char *topic, void *value, void *param);

static void lcd_page_render();

void lcd_event_handler(twr_module_lcd_event_t event, void *event_param);
void tmp112_event_handler(twr_tmp112_t *self, twr_tmp112_event_t event, void *event_param);
void humidity_tag_event_handler(twr_tag_humidity_t *self, twr_tag_humidity_event_t event, void *event_param);
void barometer_tag_event_handler(twr_tag_barometer_t *self, twr_tag_barometer_event_t event, void *event_param);
void co2_event_handler(twr_module_co2_event_t event, void *event_param);
void voc_lp_tag_event_handler(twr_tag_voc_lp_t *self, twr_tag_voc_lp_event_t event, void *event_param);

static const twr_radio_sub_t subs[] = {
    {"raps/-/get/config1", TWR_RADIO_SUB_PT_STRING, twr_get_config1, NULL},
    {"raps/-/get/config2", TWR_RADIO_SUB_PT_STRING, twr_get_config2, NULL},
    {"raps/-/set/nfc", TWR_RADIO_SUB_PT_STRING, }
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
    settings.CO2_UPDATE_SERVICE_INTERVAL = ((int)array[0] * 1000 * 60);
    settings.CO2_UPDATE_NORMAL_INTERVAL = ((int)array[1] * 1000 * 60);
    settings.VOC_LP_UPDATE_SERVICE_INTERVAL = ((int)array[2] * 1000 * 60);
    settings.VOC_LP_UPDATE_NORMAL_INTERVAL = ((int)array[3] * 1000 * 60);
    settings.TEMPERATURE_TAG_PUB_NO_CHANGE_INTERVAL = ((int)array[4] * 1000 * 60);
    settings.TEMPERATURE_TAG_PUB_VALUE_CHANGE = ((int)array[5]);
    settings.HUMIDITY_TAG_PUB_NO_CHANGE_INTERVAL = ((int)array[6] * 1000 * 60);
    settings.HUMIDITY_TAG_PUB_VALUE_CHANGE = ((int)array[7]);
    settings.BAROMETER_TAG_PUB_NO_CHANGE_INTERVAL = ((int)array[8] * 1000 * 60);
    settings.BAROMETER_TAG_PUB_VALUE_CHANGE = ((int)array[9]);

    update2_recieved = true;

    twr_log_info("config loaded. executing main methods");
}

void twr_set_nfc(uint64_t *id, const char *topic, void *value, void *param)
{
    (void) param;

    twr_log_debug(value);

    if(twr_tag_nfc_memory_write(&tag_nfc, value, sizeof(value)))
    {
        twr_log_debug("nfc memory written");
    }
}

static void lcd_page_render()
{
    int w;
    char str[32];

    twr_system_pll_enable();

    twr_module_lcd_clear();

    if ((page_index <= MAX_PAGE_INDEX) && (page_index != PAGE_INDEX_MENU))
    {
        twr_module_lcd_set_font(&twr_font_ubuntu_15);
        twr_module_lcd_draw_string(10, 5, pages[page_index].name0, true);

        twr_module_lcd_set_font(&twr_font_ubuntu_28);
        snprintf(str, sizeof(str), pages[page_index].format0, *pages[page_index].value0);
        w = twr_module_lcd_draw_string(25, 25, str, true);
        twr_module_lcd_set_font(&twr_font_ubuntu_15);
        w = twr_module_lcd_draw_string(w, 35, pages[page_index].unit0, true);

        twr_module_lcd_set_font(&twr_font_ubuntu_15);
        twr_module_lcd_draw_string(10, 55, pages[page_index].name1, true);

        twr_module_lcd_set_font(&twr_font_ubuntu_28);
        snprintf(str, sizeof(str), pages[page_index].format1, *pages[page_index].value1);
        w = twr_module_lcd_draw_string(25, 75, str, true);
        twr_module_lcd_set_font(&twr_font_ubuntu_15);
        twr_module_lcd_draw_string(w, 85, pages[page_index].unit1, true);
    }

    snprintf(str, sizeof(str), "%d/%d", page_index + 1, MAX_PAGE_INDEX + 1);
    twr_module_lcd_set_font(&twr_font_ubuntu_13);
    twr_module_lcd_draw_string(55, 115, str, true);

    twr_system_pll_disable();
}

void lcd_event_handler(twr_module_lcd_event_t event, void *event_param)
{
    (void) event_param;

    if (event == TWR_MODULE_LCD_EVENT_LEFT_CLICK)
    {
        if ((page_index != PAGE_INDEX_MENU))
        {
            // Key previous page
            page_index--;
            if (page_index < 0)
            {
                page_index = MAX_PAGE_INDEX;
                menu_item = 0;
            }
        }
        else
        {
            // Key menu down
            menu_item++;
            if (menu_item > 4)
            {
                menu_item = 0;
            }
        }

        static uint16_t left_event_count = 0;
        left_event_count++;
        //twr_radio_pub_event_count(TWR_RADIO_PUB_EVENT_LCD_BUTTON_LEFT, &left_event_count);
    }
    else if(event == TWR_MODULE_LCD_EVENT_RIGHT_CLICK)
    {
        if ((page_index != PAGE_INDEX_MENU) || (menu_item == 0))
        {
            // Key next page
            page_index++;
            if (page_index > MAX_PAGE_INDEX)
            {
                page_index = 0;
            }
            if (page_index == PAGE_INDEX_MENU)
            {
                menu_item = 0;
            }
        }

        static uint16_t right_event_count = 0;
        right_event_count++;
        //twr_radio_pub_event_count(TWR_RADIO_PUB_EVENT_LCD_BUTTON_RIGHT, &right_event_count);
    }
    else if(event == TWR_MODULE_LCD_EVENT_LEFT_HOLD)
    {
        static int left_hold_event_count = 0;
        left_hold_event_count++;
        twr_radio_pub_int("push-button/lcd:left-hold/event-count", &left_hold_event_count);

        twr_led_pulse(&led_lcd_green, 100);
    }
    else if(event == TWR_MODULE_LCD_EVENT_RIGHT_HOLD)
    {
        static int right_hold_event_count = 0;
        right_hold_event_count++;
        twr_radio_pub_int("push-button/lcd:right-hold/event-count", &right_hold_event_count);

        twr_led_pulse(&led_lcd_green, 100);

    }
    else if(event == TWR_MODULE_LCD_EVENT_BOTH_HOLD)
    {
        static int both_hold_event_count = 0;
        both_hold_event_count++;
        twr_radio_pub_int("push-button/lcd:both-hold/event-count", &both_hold_event_count);

        twr_led_pulse(&led_lcd_green, 100);
    }

    twr_scheduler_plan_now(0);
}


void voc_lp_tag_event_handler(twr_tag_voc_lp_t *self, twr_tag_voc_lp_event_t event, void *event_param)
{
    event_param_t *param = (event_param_t *)event_param;

    if (event == TWR_TAG_VOC_LP_EVENT_UPDATE)
    {
        uint16_t value;

        if (twr_tag_voc_lp_get_tvoc_ppb(self, &value))
        {
            if ((fabsf(value - param->value) >= 5.0f) || (param->next_pub < twr_scheduler_get_spin_tick()))
            {
                param->value = value;
                param->next_pub = twr_scheduler_get_spin_tick() + (15 * 60 * 1000);

                int radio_tvoc = value;

                values.tvoc = radio_tvoc;

                twr_radio_pub_int("voc-lp-sensor/0:0/tvoc", &radio_tvoc);
                twr_scheduler_plan_now(0);

            }
        }
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

void humidity_tag_event_handler(twr_tag_humidity_t *self, twr_tag_humidity_event_t event, void *event_param)
{
    float value;
    event_param_t *param = (event_param_t *)event_param;
    twr_log_debug("running hum");
    if (twr_tag_humidity_get_humidity_percentage(self, &value))
    {
        if ((fabs(value - param->value) >= settings.HUMIDITY_TAG_PUB_VALUE_CHANGE) || (param->next_pub < twr_scheduler_get_spin_tick()))
        {
            twr_radio_pub_humidity(param->channel, &value);
            param->value = value;
            param->next_pub = twr_scheduler_get_spin_tick() + settings.HUMIDITY_TAG_PUB_NO_CHANGE_INTERVAL;

            values.humidity = value;
            twr_scheduler_plan_now(0);
        }
    }
}

void barometer_tag_event_handler(twr_tag_barometer_t *self, twr_tag_barometer_event_t event, void *event_param)
{
    float pascal;
    float meter;
    event_param_t *param = (event_param_t *)event_param;

    if (event != TWR_TAG_BAROMETER_EVENT_UPDATE)
    {
        return;
    }

    if (!twr_tag_barometer_get_pressure_pascal(self, &pascal))
    {
        return;
    }

    if ((fabs(pascal - param->value) >= settings.BAROMETER_TAG_PUB_VALUE_CHANGE) || (param->next_pub < twr_scheduler_get_spin_tick()))
    {

        if (!twr_tag_barometer_get_altitude_meter(self, &meter))
        {
            return;
        }

        twr_radio_pub_barometer(param->channel, &pascal, &meter);
        param->value = pascal;
        param->next_pub = twr_scheduler_get_spin_tick() + settings.BAROMETER_TAG_PUB_NO_CHANGE_INTERVAL;

        values.pressure = pascal / 100.0;
        values.altitude = meter;
        twr_scheduler_plan_now(0);
    }
}

void co2_event_handler(twr_module_co2_event_t event, void *event_param)
{
    event_param_t *param = (event_param_t *) event_param;
    float value;

    if (event == TWR_MODULE_CO2_EVENT_UPDATE)
    {
        if (twr_module_co2_get_concentration_ppm(&value))
        {
            if ((fabs(value - param->value) >= 50.f) || (param->next_pub < twr_scheduler_get_spin_tick()))
            {
                twr_radio_pub_co2(&value);
                param->value = value;
                param->next_pub = twr_scheduler_get_spin_tick() + settings.CO2_UPDATE_SERVICE_INTERVAL;

                values.co2_concentation = value;
                twr_scheduler_plan_now(0);
            }
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
    settings.BAROMETER_UPDATE_SERVICE_INTERVAL = (1 * 60 * 1000);
    settings.BAROMETER_UPDATE_NORMAL_INTERVAL = (5 * 60 * 1000);
    settings.TEMPERATURE_UPDATE_SERVICE_INTERVAL = (1 * 60 * 1000);
    settings.TEMPERATURE_UPDATE_NORMAL_INTERVAL = (5 * 60 * 1000);
    settings.HUMIDITY_UPDATE_SERVICE_INTERVAL = (1 * 60 * 1000);
    settings.HUMIDITY_UPDATE_NORMAL_INTERVAL = (5 * 60 * 1000);
    settings.CO2_UPDATE_NORMAL_INTERVAL = (1 * 60 * 1000);
    settings.CO2_UPDATE_SERVICE_INTERVAL = (5 * 60 * 1000);
    settings.VOC_LP_UPDATE_NORMAL_INTERVAL = (1 * 60 * 1000);
    settings.VOC_LP_UPDATE_SERVICE_INTERVAL = (5 * 60 * 1000);
    settings.TEMPERATURE_TAG_PUB_NO_CHANGE_INTERVAL = (15 * 60 * 1000);
    settings.TEMPERATURE_TAG_PUB_VALUE_CHANGE = 0.2f;
    settings.HUMIDITY_TAG_PUB_NO_CHANGE_INTERVAL = (15 * 60 * 1000);
    settings.HUMIDITY_TAG_PUB_VALUE_CHANGE = 5.0f;
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

    // Initialize all components
    twr_tag_barometer_init(&tag_barometer, TWR_I2C_I2C0);
    twr_tag_humidity_init(&tag_humidity, TWR_TAG_HUMIDITY_REVISION_R2, TWR_I2C_I2C0, TWR_TAG_HUMIDITY_I2C_ADDRESS_DEFAULT);
    twr_tag_voc_lp_init(&tag_voc_lp, TWR_I2C_I2C0);
    twr_tag_nfc_init(&tag_nfc, TWR_I2C_I2C0, TWR_TAG_NFC_I2C_ADDRESS_DEFAULT);
    twr_module_co2_init();

    static twr_tmp112_t temperature;
    static event_param_t temperature_event_param = { .next_pub = 0, .channel = TWR_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_ALTERNATE };
    twr_tmp112_init(&temperature, TWR_I2C_I2C0, 0x49);
    twr_tmp112_set_event_handler(&temperature, tmp112_event_handler, &temperature_event_param);
    twr_tmp112_set_update_interval(&temperature, settings.TEMPERATURE_UPDATE_NORMAL_INTERVAL);

    static event_param_t voc_lp_event_param = { .next_pub = 0 };
    twr_tag_barometer_set_event_handler(&tag_barometer, barometer_tag_event_handler, NULL);
    twr_tag_humidity_set_event_handler(&tag_humidity, humidity_tag_event_handler, &voc_lp_event_param);
    twr_tag_voc_lp_set_event_handler(&tag_voc_lp, voc_lp_tag_event_handler, &voc_lp_event_param);
    twr_module_co2_set_event_handler(co2_event_handler, NULL);

    memset(&values, 0xff, sizeof(values));
    twr_module_lcd_init();
    twr_module_lcd_set_event_handler(lcd_event_handler, NULL);
    twr_module_lcd_set_button_hold_time(1000);
    const twr_led_driver_t* driver = twr_module_lcd_get_led_driver();
    twr_led_init_virtual(&led_lcd_green, 1, driver, 1);

    //TODO FILL THIS WITH SETTINGS
    twr_tag_barometer_set_update_interval(&tag_barometer, settings.BAROMETER_UPDATE_NORMAL_INTERVAL);
    twr_tag_humidity_set_update_interval(&tag_humidity, settings.HUMIDITY_UPDATE_NORMAL_INTERVAL);
    twr_tag_voc_lp_set_update_interval(&tag_voc_lp, settings.VOC_LP_UPDATE_NORMAL_INTERVAL);
    twr_module_co2_set_update_interval(settings.CO2_UPDATE_NORMAL_INTERVAL);

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
        twr_tag_barometer_set_update_interval(&tag_barometer, settings.BAROMETER_UPDATE_SERVICE_INTERVAL);
        twr_tag_humidity_set_update_interval(&tag_humidity, settings.BAROMETER_UPDATE_SERVICE_INTERVAL);
        twr_tag_voc_lp_set_update_interval(&tag_voc_lp, settings.BAROMETER_UPDATE_SERVICE_INTERVAL);
        twr_module_co2_set_update_interval(settings.BAROMETER_UPDATE_SERVICE_INTERVAL);

        new_update_configured = false;
        update1_recieved = false;
        update2_recieved = false;
    }

    if (!twr_module_lcd_is_ready())
    {
        return;
    }

    if (!lcd.mqtt)
    {
        lcd_page_render();
    }
    twr_module_lcd_update();
    static int counter = 0;

    // Log task run and increment counter
    twr_log_debug("APP: Task run (count: %d)", ++counter);

    // Plan next run of this task in 1000 ms
    //twr_scheduler_plan_current_from_now(settings.SERVICE_INTERVAL_INTERVAL);
    twr_scheduler_plan_current_from_now(settings.SERVICE_INTERVAL_INTERVAL);
}
