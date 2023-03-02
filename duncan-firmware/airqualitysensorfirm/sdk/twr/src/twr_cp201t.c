#include <twr_adc.h>
#include <twr_cp201t.h>

typedef struct
{
    twr_module_sensor_channel_t channel;
    twr_adc_channel_t channel_adc;
    twr_scheduler_task_id_t task_id_interval;
    twr_scheduler_task_id_t task_id_measure;
    void (*event_handler)(twr_module_sensor_channel_t, twr_cp201t_event_t, void *);
    void *event_param;
    uint16_t thermistor_data;
    float temperature;
    twr_tick_t update_interval;
    bool initialized;
    bool measurement_active;

} twr_cp201t_t;

static twr_cp201t_t _twr_cp201t[2] =
{
    [0] =
    {
        .channel = TWR_MODULE_SENSOR_CHANNEL_A,
        .channel_adc = TWR_ADC_CHANNEL_A4,
        .initialized = false,
    },

    [1] =
    {
        .channel = TWR_MODULE_SENSOR_CHANNEL_B,
        .channel_adc = TWR_ADC_CHANNEL_A5,
        .initialized = false,
    }
};

static const uint16_t _twr_cp201t_lut[1024];

static void _twr_cp201t_adc_event_handler(twr_adc_channel_t channel, twr_adc_event_t event, void *param);

static void _twr_cp201t_task_interval(void *param);

static void _twr_cp201t_task_measure(void *param);

bool twr_cp201t_init(twr_module_sensor_channel_t channel)
{
    twr_cp201t_t *cp201t = &_twr_cp201t[channel];

    if (!cp201t->initialized)
    {
        if (!twr_module_sensor_init())
        {
            return false;
        }

        // Initialize ADC to measure voltage on CP-201T (temperature)
        twr_adc_init();
        twr_adc_set_event_handler(cp201t->channel_adc, _twr_cp201t_adc_event_handler, cp201t);

        cp201t->task_id_interval = twr_scheduler_register(_twr_cp201t_task_interval, cp201t, TWR_TICK_INFINITY);
        cp201t->task_id_measure = twr_scheduler_register(_twr_cp201t_task_measure, cp201t, TWR_TICK_INFINITY);

        cp201t->initialized = true;
    }

    return true;
}

void twr_cp201t_set_event_handler(twr_module_sensor_channel_t channel, void (*event_handler)(twr_module_sensor_channel_t, twr_cp201t_event_t, void *), void *event_param)
{
    twr_cp201t_t *cp201t = &_twr_cp201t[channel];

    cp201t->event_handler = event_handler;
    cp201t->event_param = event_param;
}

void twr_cp201t_set_update_interval(twr_module_sensor_channel_t channel, twr_tick_t interval)
{
    twr_cp201t_t *cp201t = &_twr_cp201t[channel];

    cp201t->update_interval = interval;

    if (cp201t->update_interval == TWR_TICK_INFINITY)
    {
        twr_scheduler_plan_absolute(cp201t->task_id_interval, TWR_TICK_INFINITY);
    }
    else
    {
        twr_scheduler_plan_relative(cp201t->task_id_interval, cp201t->update_interval);
    }
}

bool twr_cp201t_measure(twr_cp201t_t *self)
{
    if (self->measurement_active)
    {
        return false;
    }

    self->measurement_active = true;

    twr_scheduler_plan_now(self->task_id_measure);

    return true;
}

bool twr_cp201t_get_temperature_celsius(twr_module_sensor_channel_t channel, float *celsius)
{
    float vdda_voltage;
    int16_t temp_code;
    twr_cp201t_t *cp201t = &_twr_cp201t[channel];
    uint16_t data = cp201t->thermistor_data;

    // Get actual VDDA and accurate data
    twr_adc_get_vdda_voltage(&vdda_voltage);
    data *= 3.3f / vdda_voltage;

    // Software shuffle of pull-up and NTC with each other (So that the table can be used)
    data = 0xffff - data;

    // Find temperature in LUT
    temp_code = _twr_cp201t_lut[data >> 6];

    // If temperature is valid...
    if (temp_code != 0x7fff)
    {
        // Conversion from fixed point to float
        *celsius = temp_code / 10.f;

        return true;
    }
    else
    {
        return false;
    }
}

static void _twr_cp201t_task_interval(void *param)
{
    twr_cp201t_t *cp201t = param;

    twr_cp201t_measure(cp201t);

    twr_scheduler_plan_current_relative(cp201t->update_interval);
}

static void _twr_cp201t_task_measure(void *param)
{
    twr_cp201t_t *cp201t = param;

    // Connect pull-up
    twr_module_sensor_set_pull(cp201t->channel, TWR_MODULE_SENSOR_PULL_UP_4K7);

    // Start another reading
    twr_adc_async_measure(cp201t->channel_adc);
}

static void _twr_cp201t_adc_event_handler(twr_adc_channel_t channel, twr_adc_event_t event, void *param)
{
    (void) channel;

    twr_cp201t_t *cp201t = param;

    cp201t->measurement_active = false;

    if (event == TWR_ADC_EVENT_DONE)
    {
        // Disconnect pull-up
        twr_module_sensor_set_pull(cp201t->channel, TWR_MODULE_SENSOR_PULL_NONE);

        twr_adc_async_get_value(cp201t->channel_adc, &cp201t->thermistor_data);

        cp201t->event_handler(cp201t->channel, TWR_CP201T_EVENT_UPDATE, cp201t->event_param);
    }
    else
    {
        cp201t->event_handler(cp201t->channel, TWR_CP201T_EVENT_ERROR, cp201t->event_param);
    }
}

// A value of 0x7FFF indicates out of range signal
static const uint16_t _twr_cp201t_lut[] =
{
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0xfe70,
    0xfe7a, 0xfe7e, 0xfe84, 0xfe8e, 0xfe93, 0xfe98, 0xfea2, 0xfea8,
    0xfeac, 0xfeb1, 0xfeb6, 0xfeba, 0xfec0, 0xfeca, 0xfed0, 0xfed4,
    0xfed9, 0xfede, 0xfee2, 0xfee5, 0xfee8, 0xfeed, 0xfef2, 0xfef7,
    0xfefc, 0xfeff, 0xff02, 0xff06, 0xff0b, 0xff10, 0xff14, 0xff16,
    0xff1a, 0xff1f, 0xff24, 0xff28, 0xff2b, 0xff2e, 0xff31, 0xff35,
    0xff38, 0xff3b, 0xff3f, 0xff42, 0xff45, 0xff49, 0xff4c, 0xff50,
    0xff53, 0xff56, 0xff59, 0xff5b, 0xff5d, 0xff60, 0xff63, 0xff67,
    0xff6a, 0xff6d, 0xff6f, 0xff71, 0xff74, 0xff77, 0xff7b, 0xff7e,
    0xff81, 0xff83, 0xff86, 0xff88, 0xff8a, 0xff8d, 0xff8f, 0xff92,
    0xff95, 0xff97, 0xff9a, 0xff9c, 0xff9e, 0xffa1, 0xffa3, 0xffa6,
    0xffa9, 0xffab, 0xffae, 0xffb0, 0xffb2, 0xffb4, 0xffb6, 0xffb8,
    0xffba, 0xffbc, 0xffbf, 0xffc2, 0xffc4, 0xffc6, 0xffc8, 0xffca,
    0xffcc, 0xffce, 0xffd0, 0xffd3, 0xffd6, 0xffd8, 0xffda, 0xffdc,
    0xffde, 0xffe0, 0xffe2, 0xffe4, 0xffe6, 0xffe8, 0xffea, 0xffec,
    0xffee, 0xfff0, 0xfff2, 0xfff4, 0xfff6, 0xfff8, 0xfffa, 0xfffc,
    0xfffe, 0x0000, 0x0002, 0x0004, 0x0006, 0x0008, 0x000a, 0x000c,
    0x000d, 0x000f, 0x0011, 0x0012, 0x0014, 0x0016, 0x0018, 0x001a,
    0x001c, 0x001e, 0x0020, 0x0022, 0x0023, 0x0025, 0x0026, 0x0028,
    0x002a, 0x002b, 0x002d, 0x002e, 0x0030, 0x0032, 0x0034, 0x0036,
    0x0038, 0x003a, 0x003c, 0x003e, 0x0040, 0x0041, 0x0043, 0x0044,
    0x0046, 0x0048, 0x0049, 0x004b, 0x004d, 0x004e, 0x0050, 0x0052,
    0x0053, 0x0055, 0x0057, 0x0058, 0x005a, 0x005b, 0x005d, 0x005e,
    0x0060, 0x0061, 0x0063, 0x0064, 0x0066, 0x0067, 0x0069, 0x006b,
    0x006c, 0x006e, 0x0070, 0x0071, 0x0073, 0x0075, 0x0076, 0x0078,
    0x007a, 0x007b, 0x007c, 0x007e, 0x007f, 0x0081, 0x0082, 0x0083,
    0x0085, 0x0086, 0x0087, 0x0089, 0x008a, 0x008c, 0x008e, 0x008f,
    0x0091, 0x0093, 0x0094, 0x0096, 0x0098, 0x0099, 0x009a, 0x009c,
    0x009d, 0x009f, 0x00a0, 0x00a1, 0x00a3, 0x00a4, 0x00a6, 0x00a7,
    0x00a9, 0x00aa, 0x00ab, 0x00ad, 0x00ae, 0x00b0, 0x00b1, 0x00b3,
    0x00b4, 0x00b5, 0x00b7, 0x00b8, 0x00ba, 0x00bb, 0x00bd, 0x00be,
    0x00bf, 0x00c1, 0x00c2, 0x00c4, 0x00c5, 0x00c7, 0x00c8, 0x00c9,
    0x00cb, 0x00cc, 0x00ce, 0x00cf, 0x00d1, 0x00d2, 0x00d3, 0x00d5,
    0x00d6, 0x00d8, 0x00d9, 0x00db, 0x00dc, 0x00dd, 0x00df, 0x00e0,
    0x00e2, 0x00e3, 0x00e5, 0x00e6, 0x00e7, 0x00e9, 0x00ea, 0x00ec,
    0x00ed, 0x00ef, 0x00f0, 0x00f1, 0x00f3, 0x00f4, 0x00f5, 0x00f6,
    0x00f7, 0x00f9, 0x00fa, 0x00fb, 0x00fd, 0x00fe, 0x0100, 0x0101,
    0x0103, 0x0104, 0x0105, 0x0107, 0x0108, 0x010a, 0x010b, 0x010d,
    0x010e, 0x010f, 0x0111, 0x0112, 0x0113, 0x0114, 0x0115, 0x0117,
    0x0118, 0x0119, 0x011b, 0x011c, 0x011e, 0x011f, 0x0121, 0x0122,
    0x0123, 0x0125, 0x0126, 0x0128, 0x0129, 0x012b, 0x012c, 0x012d,
    0x012f, 0x0130, 0x0131, 0x0132, 0x0133, 0x0135, 0x0136, 0x0137,
    0x0139, 0x013a, 0x013c, 0x013d, 0x013f, 0x0140, 0x0141, 0x0143,
    0x0144, 0x0145, 0x0146, 0x0147, 0x0149, 0x014a, 0x014b, 0x014d,
    0x014e, 0x0150, 0x0151, 0x0153, 0x0154, 0x0155, 0x0157, 0x0158,
    0x0159, 0x015a, 0x015b, 0x015d, 0x015e, 0x015f, 0x0161, 0x0162,
    0x0164, 0x0165, 0x0167, 0x0168, 0x0169, 0x016b, 0x016c, 0x016e,
    0x016f, 0x0171, 0x0172, 0x0173, 0x0175, 0x0176, 0x0177, 0x0178,
    0x0179, 0x017b, 0x017c, 0x017d, 0x017f, 0x0180, 0x0182, 0x0183,
    0x0185, 0x0186, 0x0187, 0x0189, 0x018a, 0x018c, 0x018d, 0x018f,
    0x0190, 0x0191, 0x0193, 0x0194, 0x0195, 0x0196, 0x0197, 0x0199,
    0x019a, 0x019b, 0x019d, 0x019e, 0x01a0, 0x01a1, 0x01a3, 0x01a4,
    0x01a5, 0x01a7, 0x01a8, 0x01aa, 0x01ab, 0x01ad, 0x01ae, 0x01af,
    0x01b1, 0x01b2, 0x01b4, 0x01b5, 0x01b7, 0x01b8, 0x01b9, 0x01bb,
    0x01bc, 0x01be, 0x01bf, 0x01c1, 0x01c2, 0x01c3, 0x01c5, 0x01c6,
    0x01c8, 0x01c9, 0x01cb, 0x01cc, 0x01cd, 0x01cf, 0x01d0, 0x01d2,
    0x01d3, 0x01d5, 0x01d6, 0x01d7, 0x01d9, 0x01da, 0x01dc, 0x01dd,
    0x01df, 0x01e0, 0x01e1, 0x01e3, 0x01e4, 0x01e6, 0x01e7, 0x01e9,
    0x01ea, 0x01eb, 0x01ed, 0x01ee, 0x01f0, 0x01f1, 0x01f3, 0x01f4,
    0x01f5, 0x01f7, 0x01f8, 0x01fa, 0x01fb, 0x01fd, 0x01fe, 0x01ff,
    0x0201, 0x0202, 0x0204, 0x0205, 0x0206, 0x0208, 0x020a, 0x020b,
    0x020d, 0x020f, 0x0210, 0x0212, 0x0213, 0x0215, 0x0216, 0x0218,
    0x0219, 0x021a, 0x021c, 0x021e, 0x021f, 0x0221, 0x0223, 0x0224,
    0x0226, 0x0228, 0x0229, 0x022a, 0x022c, 0x022d, 0x022f, 0x0230,
    0x0232, 0x0233, 0x0235, 0x0237, 0x0238, 0x023a, 0x023c, 0x023d,
    0x023f, 0x0241, 0x0242, 0x0244, 0x0246, 0x0247, 0x0249, 0x024b,
    0x024c, 0x024e, 0x0250, 0x0251, 0x0253, 0x0255, 0x0256, 0x0258,
    0x025a, 0x025b, 0x025d, 0x025f, 0x0260, 0x0262, 0x0264, 0x0265,
    0x0267, 0x0269, 0x026a, 0x026c, 0x026e, 0x026f, 0x0271, 0x0272,
    0x0274, 0x0276, 0x0278, 0x027a, 0x027c, 0x027e, 0x0280, 0x0282,
    0x0283, 0x0285, 0x0287, 0x0288, 0x028a, 0x028c, 0x028e, 0x0290,
    0x0292, 0x0294, 0x0296, 0x0297, 0x0299, 0x029b, 0x029c, 0x029e,
    0x02a0, 0x02a2, 0x02a4, 0x02a6, 0x02a8, 0x02aa, 0x02ac, 0x02ae,
    0x02b0, 0x02b2, 0x02b4, 0x02b6, 0x02b8, 0x02ba, 0x02bc, 0x02be,
    0x02c0, 0x02c2, 0x02c4, 0x02c6, 0x02c8, 0x02ca, 0x02cc, 0x02ce,
    0x02d0, 0x02d2, 0x02d4, 0x02d6, 0x02d8, 0x02da, 0x02dc, 0x02de,
    0x02e0, 0x02e2, 0x02e4, 0x02e6, 0x02e9, 0x02ec, 0x02ee, 0x02f0,
    0x02f2, 0x02f4, 0x02f6, 0x02f8, 0x02fa, 0x02fd, 0x0300, 0x0302,
    0x0304, 0x0306, 0x0308, 0x030a, 0x030c, 0x030e, 0x0311, 0x0313,
    0x0316, 0x0319, 0x031b, 0x031e, 0x0320, 0x0322, 0x0325, 0x0327,
    0x032a, 0x032d, 0x032f, 0x0332, 0x0334, 0x0337, 0x0339, 0x033c,
    0x033e, 0x0340, 0x0343, 0x0345, 0x0348, 0x034b, 0x034d, 0x0350,
    0x0352, 0x0354, 0x0357, 0x0359, 0x035c, 0x035f, 0x0363, 0x0366,
    0x0369, 0x036b, 0x036d, 0x0370, 0x0373, 0x0377, 0x037a, 0x037d,
    0x037f, 0x0381, 0x0384, 0x0387, 0x038a, 0x038e, 0x0392, 0x0395,
    0x0398, 0x039b, 0x039d, 0x039f, 0x03a2, 0x03a5, 0x03a9, 0x03ac,
    0x03af, 0x03b3, 0x03b6, 0x03b9, 0x03bd, 0x03c0, 0x03c3, 0x03c7,
    0x03ca, 0x03cd, 0x03d0, 0x03d4, 0x03d9, 0x03de, 0x03e2, 0x03e5,
    0x03e8, 0x03eb, 0x03ee, 0x03f2, 0x03f7, 0x03fc, 0x03ff, 0x0402,
    0x0406, 0x040b, 0x0410, 0x0414, 0x0416, 0x041a, 0x041f, 0x0424,
    0x0428, 0x042b, 0x042e, 0x0433, 0x0438, 0x043d, 0x0442, 0x0447,
    0x044c, 0x0451, 0x0456, 0x045b, 0x0460, 0x0465, 0x046a, 0x046f,
    0x0474, 0x0479, 0x047e, 0x0483, 0x0488, 0x048d, 0x0492, 0x0497,
    0x049c, 0x04a0, 0x04a6, 0x04b0, 0x04b7, 0x04ba, 0x04bd, 0x04c4,
    0x04ce, 0x04d4, 0x04d8, 0x04e2, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff
};
