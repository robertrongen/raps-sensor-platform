#include <application.h>

#define SCALE_FREQUENCY 3000
#define RADIO_INTERVAL 3000

// LED instance
bc_led_t _led;

// scale instance
hx711_t scale;
float _scale_value;
bool _scale_on = true;

bc_scheduler_task_id_t _task_id_getradioid;

static int _rmenu_level=0;
static int _rmenu_pos=-1;
#define MENULEFT 10
#define MENURIGHT 90

void _turn_on();
void _turn_off();
void _lcd_rewrite();
void _lcd_write_menu();
void _lcd_navigate(bool rbutton);


void lcd_button_left_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    (void) event_param;
    if (event == BC_BUTTON_EVENT_CLICK)
	{
        if (_rmenu_level!=0)
        {
            // accept menu selection    
            _lcd_navigate(false);
        }
        else if (_scale_on)
        {
            // measure
            hx711_measure(&scale);
        }
    }
    else if (event == BC_BUTTON_EVENT_HOLD)
    {
        if (_scale_on)
        {
            _turn_off();
        }
        else
        {
            _turn_on();
        }
    }
    
}

void lcd_button_right_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    (void) event_param;

	if (event == BC_BUTTON_EVENT_CLICK)
	{
        _lcd_navigate(true);
    }
}

void hx711_event_handler(hx711_t *self, hx711_event_t event, double value, void *event_param)
{
    // twr_log_debug("hx711_event_handler start");

    (void) event_param;

    _scale_value = value;;
    _lcd_rewrite();

    long val = hx711_read_raw(&scale);

    if(_radio_id!=0) {
        bc_radio_pub_float("blokko/value", &_scale_value); 
    }

    // twr_log_debug("hx711_event_handler end");
}

// switch ON scale and LCD
void _turn_on(){
    bc_module_lcd_on();
    bc_module_lcd_init();
    bc_module_lcd_clear();

    bc_module_lcd_draw_circle(64,64,25,false);
    bc_module_lcd_update();

    hx711_set_update_interval(&scale, SCALE_FREQUENCY);
    hx711_power_up(&scale);
    _scale_on = true;

    hx711_measure(&scale);

    twr_log_debug("ON\r\n",4);
}

// switch OFF scale and LCD
void _turn_off(){
    bc_module_lcd_off();
    hx711_set_update_interval(&scale, BC_TICK_INFINITY);
    hx711_power_down(&scale);
    _scale_on = false;

    twr_log_debug("OFF\r\n",5);
}
// write measured weight
// value - measured units will be rounded to 2 decimal digits
void _lcd_rewrite()
{
    bc_module_lcd_clear();

    if (scale._state==HX711_STATE_INITIALIZE)
    {
        bc_module_lcd_set_font(&bc_font_ubuntu_15);
        bc_module_lcd_draw_string(10, 40, "NOT calibrated", true);       
    }
    else
    {
        char buffer[10];
        sprintf(buffer, "%.2f", 0.01*round(_scale_value*100));
        bc_module_lcd_set_font(&bc_font_ubuntu_28);
        bc_module_lcd_draw_string(40, 40, buffer, true);
    }

    _lcd_write_menu();

    bc_module_lcd_update();
}

// write menu depends on state and level
void _lcd_write_menu()
{
    bc_module_lcd_set_font(&bc_font_ubuntu_11);

    if (_rmenu_level==0)
    {
        bc_module_lcd_draw_string(MENULEFT, 115, "Get / Off", true);
        bc_module_lcd_draw_string(MENURIGHT, 115, "Settings", true);
    }
    else if (_rmenu_level==1)
    {
        bc_module_lcd_draw_string(MENULEFT, 115, "Select", true);

        bc_module_lcd_draw_rectangle(MENURIGHT-5, 69+_rmenu_pos*15,127, 84+_rmenu_pos*15, true);
        bc_module_lcd_draw_string(MENURIGHT, 70, "<back>", true);
        bc_module_lcd_draw_string(MENURIGHT, 85, "Tare", true);
        bc_module_lcd_draw_string(MENURIGHT, 100, "Calibrate", true);
        bc_module_lcd_draw_string(MENURIGHT, 115, "Save", true);
    }
    
    float volt;
    char vtxt[10];
    bc_module_battery_measure();
    bc_module_battery_get_voltage(&volt);
    sprintf(vtxt, "%.2f V", 0.01*round(volt*100));
    bc_module_lcd_draw_string(MENULEFT, 1, vtxt, true);
}

// navigate in the right menu
// rbutton - right LCD module button pressed 
void _lcd_navigate(bool rbutton)
{
    if (rbutton)
    {
        if (_rmenu_level==0){
            _rmenu_pos=-1;
            _rmenu_level++;
        }
        _rmenu_pos++;
        if (_rmenu_pos>3)
            _rmenu_pos=0;

        _lcd_rewrite();
    }
    else
    {
        if (_rmenu_level==0)
            return;

        switch (_rmenu_pos)
        {
            case 1:
                hx711_tare(&scale);
                break;
            case 2:
                hx711_calibrate(&scale,1);
                break;
            case 3:
                hx711_save(&scale);
                break;
            default:
                break;
        }

        _rmenu_level = 0;
        _rmenu_pos = -1;

        hx711_measure(&scale);
    }
}
// initialize display
void lcd_init(){
    // Initialize LCD
    bc_module_lcd_init();

    bc_module_lcd_clear();
    bc_module_lcd_set_font(&bc_font_ubuntu_24);
    bc_module_lcd_draw_string(35, 25, "Clown", true);
    bc_module_lcd_draw_string(40, 50, "Scale", true);
    bc_module_lcd_set_font(&bc_font_ubuntu_13);
    bc_module_lcd_draw_string(30, 100, "(C) 2020 Matej", true);    
    bc_module_lcd_update();

    // Initialize LCD buttons
    static bc_button_t lcd_left;
    bc_button_init_virtual(&lcd_left, BC_MODULE_LCD_BUTTON_LEFT, bc_module_lcd_get_button_driver(), false);
    bc_button_set_event_handler(&lcd_left, lcd_button_left_event_handler, NULL);
    static bc_button_t lcd_right;
    bc_button_init_virtual(&lcd_right, BC_MODULE_LCD_BUTTON_RIGHT, bc_module_lcd_get_button_driver(), false);
    bc_button_set_event_handler(&lcd_right, lcd_button_right_event_handler, NULL);
}

// executed on interval event 
void _get_radio_id_task_interval(void *param)
{
    if(_radio_id==0) {
        _radio_id = twr_radio_get_my_id();
        twr_log_debug("get Radio ID: %llX", _radio_id);         
        if(_radio_id!=0) {
            // id is now known: initialize subscriptions
            char * id = malloc(32);
            sprintf(id, "%llX", _radio_id);
            twr_log_debug("register with ID %s", id);
            twr_radio_pairing_request(id, VERSION);
        } else {
            // still 0? try again in a while
            twr_scheduler_plan_current_relative(RADIO_INTERVAL);
        }
    }
}

// initialize application
void application_init(void){
    // init USB
    // bc_usb_cdc_init();

    // log module
    twr_log_init(TWR_LOG_LEVEL_DUMP, TWR_LOG_TIMESTAMP_ABS);    
    twr_log_debug("init application complete");

    // battery module
    bc_module_battery_init();

    // Initialize LED
    bc_led_init(&_led, BC_GPIO_LED, false, false);
    bc_led_set_mode(&_led, BC_LED_MODE_ON);

    // initialize LCD module
    lcd_init();
    twr_log_debug("init lcd complete");

    // init scale
    hx711_init(&scale, DTPIN, CLKPIN, HX711_CHANNEL_A); // HX711_CHANNEL_A64
    twr_log_debug("init hx711 complete");
    // hx711_set_reads(&scale, 3);
    // hx711_tare(&scale);
    hx711_set_scale(&scale, 23800);

    hx711_load(&scale);
    hx711_set_update_interval(&scale, SCALE_FREQUENCY);
    hx711_set_event_handler(&scale, hx711_event_handler, NULL);

    bc_led_set_mode(&_led, BC_LED_MODE_OFF);
    twr_log_debug("init application complete");

    // init radio
    twr_radio_init(BC_RADIO_MODE_NODE_SLEEPING); 
    twr_radio_set_rx_timeout_for_sleeping_node(500); // radio will be turned on for receiving a return message, time in milliseconds

    // fetch radio id
    _task_id_getradioid = twr_scheduler_register(_get_radio_id_task_interval, NULL, TWR_TICK_INFINITY);
    twr_scheduler_plan_relative(_task_id_getradioid, RADIO_INTERVAL);
}
