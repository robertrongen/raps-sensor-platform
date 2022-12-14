#include <bc_onewire_gpio.h>
#include <bc_system.h>
#include <bc_timer.h>
#include <bc_irq.h>
#include <stm32l0xx.h>

static bool _bc_onewire_gpio_init(void *ctx);
static bool _bc_onewire_gpio_enable(void *ctx);
static bool _bc_onewire_gpio_disable(void *ctx);
static bool _bc_onewire_gpio_reset(void *ctx);
static void _bc_onewire_gpio_write_bit(void *ctx, uint8_t bit);
static uint8_t _bc_onewire_gpio_read_bit(void *ctx);
static void _bc_onewire_gpio_write_byte(void *ctx, uint8_t byte);
static uint8_t _bc_onewire_gpio_read_byte(void *ctx);
static bool _bc_onewire_gpio_search_next(void *ctx, bc_onewire_t *onewire, uint64_t *device_number);

static const bc_onewire_driver_t _bc_onewire_gpio_driver =
{
    .init = _bc_onewire_gpio_init,
    .enable = _bc_onewire_gpio_enable,
    .disable = _bc_onewire_gpio_disable,
    .reset = _bc_onewire_gpio_reset,
    .write_bit = _bc_onewire_gpio_write_bit,
    .read_bit = _bc_onewire_gpio_read_bit,
    .write_byte = _bc_onewire_gpio_write_byte,
    .read_byte = _bc_onewire_gpio_read_byte,
    .search_next = _bc_onewire_gpio_search_next
};

void bc_onewire_gpio_init(bc_onewire_t *onewire, bc_gpio_channel_t channel)
{
    bc_onewire_init(onewire, &_bc_onewire_gpio_driver, (void *) channel);
}

const bc_onewire_driver_t *bc_onewire_gpio_det_driver(void)
{
    return &_bc_onewire_gpio_driver;
}

static bool _bc_onewire_gpio_init(void *ctx)
{
    bc_gpio_channel_t channel = (bc_gpio_channel_t) ctx;

    bc_gpio_init(channel);

    bc_gpio_set_pull(channel, BC_GPIO_PULL_NONE);

    bc_gpio_set_mode(channel, BC_GPIO_MODE_INPUT);

    return true;
}

static bool _bc_onewire_gpio_enable(void *ctx)
{
    (void) ctx;
    bc_system_pll_enable();
    bc_timer_init();
    bc_timer_start();
    return true;
}

static bool _bc_onewire_gpio_disable(void *ctx)
{
    (void) ctx;
    bc_timer_stop();
    bc_system_pll_disable();
    return true;
}

static bool _bc_onewire_gpio_reset(void *ctx)
{
    bc_gpio_channel_t channel = (bc_gpio_channel_t) ctx;

    int i;
    uint8_t retries = 125;

    bc_gpio_set_mode(channel, BC_GPIO_MODE_INPUT);

    do
    {
        if (retries-- == 0)
        {
            return false;
        }
        bc_timer_delay(2);
    }
    while (bc_gpio_get_input(channel) == 0);

    bc_gpio_set_output(channel, 0);
    bc_gpio_set_mode(channel, BC_GPIO_MODE_OUTPUT);

    bc_timer_delay(480);

    bc_gpio_set_mode(channel, BC_GPIO_MODE_INPUT);

    // Some devices responds little bit later than expected 70us, be less strict in timing...
    // Low state of data line (presence detect) should be definitely low between 60us and 75us from now
    // According to datasheet: t_PDHIGH=15..60us ; t_PDLOW=60..240us
    //
    //     t_PDHIGH    t_PDLOW
    //      /----\ ... \      / ... /-----
    //  ___/      \ ... \____/ ... /
    //     ^     ^     ^      ^     ^
    //     now   15    60     75    300  us
    //
    bc_timer_delay(60);
    retries = 4;
    do {
        i = bc_gpio_get_input(channel);
        bc_timer_delay(4);
    }
    while (i && --retries);
    bc_timer_delay(retries * 4);
    //bc_log_debug("retries=%d",retries);

    bc_timer_delay(410);

    return i == 0;
}

static void _bc_onewire_gpio_write_bit(void *ctx, uint8_t bit)
{
    bc_gpio_channel_t channel = (bc_gpio_channel_t) ctx;

    bc_gpio_set_output(channel, 0);
    bc_gpio_set_mode(channel, BC_GPIO_MODE_OUTPUT);

    if (bit)
    {
        bc_irq_disable();

        bc_timer_delay(3);

        bc_irq_enable();

        bc_gpio_set_mode(channel, BC_GPIO_MODE_INPUT);

        bc_timer_delay(60);
    }
    else
    {
        bc_timer_delay(55);

        bc_gpio_set_mode(channel, BC_GPIO_MODE_INPUT);

        bc_timer_delay(8);
    }
}

static uint8_t _bc_onewire_gpio_read_bit(void *ctx)
{
    bc_gpio_channel_t channel = (bc_gpio_channel_t) ctx;

    uint8_t bit = 0;

    bc_gpio_set_output(channel, 0);

    bc_gpio_set_mode(channel, BC_GPIO_MODE_OUTPUT);

    bc_irq_disable();

    bc_timer_delay(3);

    bc_irq_enable();

    bc_gpio_set_mode(channel, BC_GPIO_MODE_INPUT);

    bc_irq_disable();

    bc_timer_delay(8);

    bc_irq_enable();

    bit = bc_gpio_get_input(channel);

    bc_timer_delay(50);

    return bit;
}

static void _bc_onewire_gpio_write_byte(void *ctx, uint8_t byte)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        _bc_onewire_gpio_write_bit(ctx, byte & 0x01);
        byte >>= 1;
    }
}

static uint8_t _bc_onewire_gpio_read_byte(void *ctx)
{
    uint8_t byte = 0;
    for (uint8_t i = 0; i < 8; i++)
    {
        byte |= (_bc_onewire_gpio_read_bit(ctx) << i);
    }
    return byte;
}

static bool _bc_onewire_gpio_search_next(void *ctx, bc_onewire_t *onewire, uint64_t *device_number)
{
    bool search_result = false;
    uint8_t id_bit_number;
    uint8_t last_zero, rom_byte_number;
    uint8_t id_bit, cmp_id_bit;
    uint8_t rom_byte_mask, search_direction;

    /* Initialize for search */
    id_bit_number = 1;
    last_zero = 0;
    rom_byte_number = 0;
    rom_byte_mask = 1;

    if (!_bc_onewire_gpio_reset(ctx))
    {
        return false;
    }

    // issue the search command
    _bc_onewire_gpio_write_byte(ctx, 0xf0);

    // loop to do the search
    do
    {
        id_bit = _bc_onewire_gpio_read_bit(ctx);
        cmp_id_bit = _bc_onewire_gpio_read_bit(ctx);

        // check for no devices on 1-wire
        if ((id_bit == 1) && (cmp_id_bit == 1))
        {
            break;
        }
        else
        {
            /* All devices coupled have 0 or 1 */
            if (id_bit != cmp_id_bit)
            {
                /* Bit write value for search */
                search_direction = id_bit;
            }
            else
            {
                /* If this discrepancy is before the Last Discrepancy on a previous next then pick the same as last time */
                if (id_bit_number < onewire->_last_discrepancy)
                {
                    search_direction = ((onewire->_last_rom_no[rom_byte_number] & rom_byte_mask) > 0);
                }
                else
                {
                    /* If equal to last pick 1, if not then pick 0 */
                    search_direction = (id_bit_number == onewire->_last_discrepancy);
                }

                /* If 0 was picked then record its position in LastZero */
                if (search_direction == 0)
                {
                    last_zero = id_bit_number;

                    /* Check for Last discrepancy in family */
                    if (last_zero < 9)
                    {
                        onewire->_last_family_discrepancy = last_zero;
                    }
                }
            }

            /* Set or clear the bit in the ROM byte rom_byte_number with mask rom_byte_mask */
            if (search_direction == 1)
            {
                onewire->_last_rom_no[rom_byte_number] |= rom_byte_mask;
            }
            else
            {
                onewire->_last_rom_no[rom_byte_number] &= ~rom_byte_mask;
            }

            /* Serial number search direction write bit */
            _bc_onewire_gpio_write_bit(ctx, search_direction);

            /* Increment the byte counter id_bit_number and shift the mask rom_byte_mask */
            id_bit_number++;
            rom_byte_mask <<= 1;

            /* If the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask */
            if (rom_byte_mask == 0)
            {
                rom_byte_number++;
                rom_byte_mask = 1;
            }
        }
    }
    while (rom_byte_number < 8);

    /* If the search was successful then */
    if (!(id_bit_number < 65))
    {
        /* Search successful so set LastDiscrepancy, LastDeviceFlag, search_result */
        onewire->_last_discrepancy = last_zero;

        /* Check for last device */
        if (onewire->_last_discrepancy == 0)
        {
            onewire->_last_device_flag = true;
        }

        search_result = !onewire->_last_rom_no[0] ? false : true;
    }

    if (search_result && bc_onewire_crc8(onewire->_last_rom_no, sizeof(onewire->_last_rom_no), 0x00) != 0)
    {
        search_result = false;
    }

    if (search_result)
    {
        memcpy(device_number, onewire->_last_rom_no, sizeof(onewire->_last_rom_no));
    }
    else
    {
        _bc_onewire_gpio_reset(ctx);
    }

    return search_result;
}
