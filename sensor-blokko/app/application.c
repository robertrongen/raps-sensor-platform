/// @dev change container number X in *container_id = "X" (line 28) and "blokko/order/qr/X (line 57)"
/// @dev variable [bc_scheduler_plan_current_relative(5000)] (msec) determines update frequency
// Hardwario support feedback: https://forum.bigclown.com/t/reconnecting-radio-dongle-results-disables-connections/553/11
// snprintf() function formats and stores a series of characters and values in the array buffer in order to send it to the LCD module or QR generator

#include "application.h"
#include "qrcodegen.h"

// Defaults
#define SERVICE_INTERVAL_INTERVAL   (60 * 60 * 1000)
#define BATTERY_UPDATE_INTERVAL     (60 * 60 * 1000)

#define APPLICATION_TASK_ID 0

// LED instance
bc_led_t led;
bc_led_t led_lcd_red;
bc_led_t led_lcd_blue;
bc_led_t led_lcd_green;

// Button instance
bc_button_t button;

// GFX instance
bc_gfx_t *gfx;

// QR code variables
char *container_id = "1";
char *orderIdUrl="http://app.smartys.eu/";
char qr_text[255];
char order_url[255];
int orderId;


void bc_change_qr_value(uint64_t *id, const char *topic, void *value, void *param);
void print_qr(const uint8_t qrcode[], char *qr_header_text);
void qrcode_project(char *text, char *header_text);

int getOrderID(const char *orderMsg)
{
    unsigned size = strlen(orderMsg);
    unsigned n = 3;

    return("%s\n", orderMsg + (size - n));
}

void create_qr_text(const char *container, const char *order)
{
    snprintf(qr_text, sizeof(qr_text), "Blokko CTR%s, PO: %s", container, order);
}

// subscribe table, format: topic, expect payload type, callback, user param
/// @dev Max subs topic length is 50 chars (https://forum.hardwario.com/t/example-of-how-to-subscribe-to-custom-mqtt-topic/360/15)
/// @dev Topic should be in structure a/b/c/d (so four levels)
/// @dev Subscription is initialized with bc_radio_set_subs
static const bc_radio_sub_t subs[] = {
    {"blokko/order/qr/1", BC_RADIO_SUB_PT_STRING, bc_change_qr_value, NULL}
};

void bc_change_qr_value(uint64_t *id, const char *topic, void *value, void *param)
{
    bc_led_pulse(&led_lcd_blue, 250);

    snprintf(order_url, sizeof(order_url), "%s", (char*)value);
     
    orderId = getOrderID(value);

    create_qr_text(container_id, (char*)orderId);
    qrcode_project(order_url, qr_text);
}

void print_qr(const uint8_t qrcode[], char *qr_header_text)
{
    bc_log_info("print_qr started");
    bc_gfx_clear(gfx);

    bc_gfx_set_font(gfx, &bc_font_ubuntu_13);
    bc_gfx_draw_string(gfx, 2, 0, qr_header_text, true);

    uint32_t offset_x = 9;
    uint32_t offset_y = 14;
    uint32_t box_size = 3;
	int size = qrcodegen_getSize(qrcode);
	int border = 1;
	for (int y = -border; y < size + border; y++) {
		for (int x = -border; x < size + border; x++) {
            uint32_t x1 = offset_x + x * box_size;
            uint32_t y1 = offset_y + y * box_size;
            uint32_t x2 = x1 + box_size;
            uint32_t y2 = y1 + box_size;

            bc_gfx_draw_fill_rectangle(gfx, x1, y1, x2, y2, qrcodegen_getModule(qrcode, x, y));
		}
	}
    bc_gfx_update(gfx);
}

// Make and print the QR Code symbol
void qrcode_project(char *text, char *header_text)
{
    bc_system_pll_enable();

	static uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
	static uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
	// bool ok = qrcodegen_encodeText(text, tempBuffer, qrcode, qrcodegen_Ecc_MEDIUM,	qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
	bool ok = qrcodegen_encodeText(text, tempBuffer, qrcode, qrcodegen_Ecc_HIGH, qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);

	if (ok)
    {
		print_qr(qrcode, header_text);
    }

    bc_system_pll_disable();
}

// Button instance
bc_button_t button;
uint16_t button_event_count = 0;

// Event handlers to publish battery voltage when either button is pressed or a battery events happens

void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    (void) self;
    (void) event_param;
    float voltage;

    if (event == BC_BUTTON_EVENT_PRESS)
    {
        if (bc_module_battery_get_voltage(&voltage))
        {
            bc_radio_pub_battery(&voltage);
            // bc_led_pulse(&led_lcd_green, 500);
        }
    }
}

void battery_event_handler(bc_module_battery_event_t event, void *event_param)
{
    (void) event_param;
    float voltage;

    if (event == BC_MODULE_BATTERY_EVENT_UPDATE)
    {
        if (bc_module_battery_get_voltage(&voltage))
        {
            bc_radio_pub_battery(&voltage);
        }
    }
}

void application_init(void)
{
    // Initialize logging
    bc_log_init(BC_LOG_LEVEL_DEBUG, BC_LOG_TIMESTAMP_ABS);
    
    // Create log
    bc_log_info("application_init started");

    // Initialize Radio
    //bc_radio_init(BC_RADIO_MODE_NODE_LISTENING); 
    bc_radio_init(BC_RADIO_MODE_NODE_SLEEPING); 
    bc_radio_set_rx_timeout_for_sleeping_node(500); // radio will be turned on for receiving a return message, time in milliseconds
    bc_radio_set_subs((bc_radio_sub_t *) subs, sizeof(subs)/sizeof(bc_radio_sub_t));
    bc_radio_pairing_request("bcf-qr-code", VERSION);

    // Initialize LCD
    bc_module_lcd_init();
    gfx = bc_module_lcd_get_gfx();

    // Initialize LED
    bc_led_init(&led, BC_GPIO_LED, false, false);
    bc_led_set_mode(&led, BC_LED_MODE_OFF);
    // bc_led_set_mode(&led, BC_LED_MODE_ON);

    // Initialize button
    bc_button_init(&button, BC_GPIO_BUTTON, BC_GPIO_PULL_DOWN, false);
    bc_button_set_event_handler(&button, button_event_handler, NULL);
  
    // Initialize LED on LCD module
    bc_led_init_virtual(&led_lcd_red, BC_MODULE_LCD_LED_RED, bc_module_lcd_get_led_driver(), true);
    bc_led_init_virtual(&led_lcd_blue, BC_MODULE_LCD_LED_BLUE, bc_module_lcd_get_led_driver(), true);
    bc_led_init_virtual(&led_lcd_green, BC_MODULE_LCD_LED_GREEN, bc_module_lcd_get_led_driver(), true);

    // Initialize battery
    bc_module_battery_init();
    bc_module_battery_set_event_handler(battery_event_handler, NULL);
    bc_module_battery_set_update_interval(BATTERY_UPDATE_INTERVAL);


    // Initialize project
    create_qr_text(container_id, "-");
    qrcode_project(orderIdUrl, qr_text);
    bc_led_pulse(&led_lcd_green, 1000);
}

void application_task()  // this task is called internallyn no need to call it
{
    bool parameter = true;
    bc_radio_pub_bool("update_request", &parameter); // send message "true" to MQTT to trigger return message

    // increase when more nodes will be connected! Test for 10 modules with 15-30 seconds
    bc_scheduler_plan_current_relative(5000); // wait time in milliseconds
}
