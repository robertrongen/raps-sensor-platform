// Tower Kit documentation https://tower.hardwario.com/
// SDK API description https://sdk.hardwario.com/
// Forum https://forum.hardwario.com/

#include <application.h>

all_settings_t settings;
// LED instance
twr_led_t lcdLed_red;
twr_led_t lcdLed_blue;

// Button instance
twr_button_t button_left;
twr_button_t button_right;

int updateSchedule = 1000;

uint64_t _radio_id;

// Thermometer instance
twr_tmp112_t tmp112;
uint16_t button_click_count = 0;

bool update_done = false;
bool new_update = false;
bool first_update_done = false;

static const twr_radio_sub_t subs[] = {
    {"denurity/-/get/config", TWR_RADIO_SUB_PT_STRING, twr_get_config, NULL}
    //{"denurity/-/get/config", TWR_RADIO_SUB_PT_STRING, twr_get_config, NULL}
    };

void twr_get_config(uint64_t *id, const char *topic, void *value, void *param)
{
    (void)param;

    twr_log_debug("config recieved!");

    char *array[14]; // array with all settings
    int i = 0;
    char *p = strtok(value, ","); // change / to whatever you use to split the values

    while (p != NULL) // assign values to array
    {
        twr_log_debug(p);
        array[i++] = p;
        p = strtok(NULL, ",");
    }
    settings.SERVICE_INTERVAL_INTERVAL = array[0];
    settings.BATTERY_UPDATE_INTERVAL = array[1];
    settings.UPDATE_SERVICE_INTERVAL = array[2];
    settings.UPDATE_NORMAL_INTERVAL = array[3];
    settings.BAROMETER_UPDATE_SERVICE_INTERVAL = array[4];
    settings.BAROMETER_UPDATE_NORMAL_INTERVAL = array[5];
    settings.TEMPERATURE_TAG_PUB_NO_CHANGE_INTEVAL = array[6];
    settings.TEMPERATURE_TAG_PUB_VALUE_CHANGE = array[7];
    settings.HUMIDITY_TAG_PUB_NO_CHANGE_INTEVAL = array[8];
    settings.HUMIDITY_TAG_PUB_VALUE_CHANGE = array[9];
    settings.LUX_METER_TAG_PUB_NO_CHANGE_INTEVAL = array[10];
    settings.LUX_METER_TAG_PUB_VALUE_CHANGE = array[11];
    settings.BAROMETER_TAG_PUB_NO_CHANGE_INTEVAL = array[12];
    settings.BAROMETER_TAG_PUB_VALUE_CHANGE = array[13];

    update_done = true;
    new_update = false;

    twr_log_info("config loaded. executing main methods");
}

// Button event callback
void button_event_handler(twr_button_t *self, twr_button_event_t event, void *event_param)
{
    (void) self;
    (void) event_param;

    // Log button event
    twr_log_info("APP: Button event: %i", event);
 }

// Application initialization function which is called once after boot
void application_init(void)
{
    twr_log_init(TWR_LOG_LEVEL_DEBUG, TWR_LOG_TIMESTAMP_ABS);

    const twr_led_driver_t* driver = twr_module_lcd_get_led_driver();
    twr_led_init_virtual(&lcdLed_blue, TWR_MODULE_LCD_LED_GREEN, driver, 1);
    twr_led_init_virtual(&lcdLed_red, TWR_MODULE_LCD_LED_RED, driver, 1);

    const twr_button_driver_t* lcdButtonDriver =  twr_module_lcd_get_button_driver();
    twr_button_init_virtual(&button_left, 0, lcdButtonDriver, 0);
    twr_button_init_virtual(&button_right, 1, lcdButtonDriver, 0);

    twr_button_set_event_handler(&button_left, button_event_handler, (int*)0);
    twr_button_set_event_handler(&button_right, button_event_handler, (int*)1);

    twr_button_set_hold_time(&button_left, 500);
    twr_button_set_hold_time(&button_right, 500);

    twr_module_lcd_init();
    twr_module_lcd_set_font(&twr_font_ubuntu_15);

    // Initialize radio
    twr_radio_init(TWR_RADIO_MODE_NODE_LISTENING);
    twr_radio_set_rx_timeout_for_sleeping_node(500);
    twr_radio_set_subs((twr_radio_sub_t *)subs, sizeof(subs) / sizeof(twr_radio_sub_t));

    // Send radio pairing request
    twr_radio_pub_bool("sensor/start/get/config", true);
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
            twr_radio_pairing_request(id, 1);
        }
        twr_scheduler_plan_current_from_now(updateSchedule);
        return;
    }
    if (update_done && !new_update)
    {
        if (!first_update_done)
        {
            //set event handlers and settings
            twr_log_debug("doing once started");
            twr_tmp112_set_event_handler(&tmp112, TWR_I2C_I2C0, 0x49);
            first_update_done = true;
        }
        else
        {
            //reset updated settings
        }
        new_update = true;
    }
    twr_log_info("Started program");

    twr_module_lcd_clear();

    twr_module_lcd_draw_string(10, 5, "Ready", true);

    twr_module_lcd_update();

    twr_scheduler_plan_current_relative(500);
}
