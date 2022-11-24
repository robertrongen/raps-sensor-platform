#include <bc_usb_cdc.h>
#include <bc_scheduler.h>
#include <bc_fifo.h>
#include <bc_system.h>

#include <usbd_core.h>
#include <usbd_cdc.h>
#include <usbd_cdc_if.h>
#include <usbd_desc.h>

#include <stm32l0xx.h>

static struct
{
    bc_fifo_t receive_fifo;
    uint8_t receive_buffer[1024];
    uint8_t transmit_buffer[512];
    size_t transmit_length;
    bc_scheduler_task_id_t task_id;

} _bc_usb_cdc;

USBD_HandleTypeDef hUsbDeviceFS;

static void _bc_usb_cdc_task_start(void *param);
static void _bc_usb_cdc_task(void *param);
static void _bc_usb_cdc_init_hsi48();

void bc_usb_cdc_init(void)
{
    memset(&_bc_usb_cdc, 0, sizeof(_bc_usb_cdc));

    _bc_usb_cdc_init_hsi48();

    bc_fifo_init(&_bc_usb_cdc.receive_fifo, _bc_usb_cdc.receive_buffer, sizeof(_bc_usb_cdc.receive_buffer));

    __HAL_RCC_GPIOA_CLK_ENABLE();

    USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS);
    USBD_RegisterClass(&hUsbDeviceFS, &USBD_CDC);
    USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS);

    _bc_usb_cdc.task_id = bc_scheduler_register(_bc_usb_cdc_task_start, NULL, 0);
}

bool bc_usb_cdc_write(const void *buffer, size_t length)
{
    if (length > (sizeof(_bc_usb_cdc.transmit_buffer) - _bc_usb_cdc.transmit_length))
    {
        return false;
    }

    memcpy(&_bc_usb_cdc.transmit_buffer[_bc_usb_cdc.transmit_length], buffer, length);

    _bc_usb_cdc.transmit_length += length;

    bc_scheduler_plan_now(_bc_usb_cdc.task_id);

    return true;
}

size_t bc_usb_cdc_read(void *buffer, size_t length)
{
    size_t bytes_read = 0;

    while (length != 0)
    {
        uint8_t value;

        if (bc_fifo_read(&_bc_usb_cdc.receive_fifo, &value, 1) == 1)
        {
            *(uint8_t *) buffer = value;

            buffer = (uint8_t *) buffer + 1;

            bytes_read++;
        }
        else
        {
            break;
        }

        length--;
    }

    return bytes_read;
}

void bc_usb_cdc_received_data(const void *buffer, size_t length)
{
    bc_fifo_irq_write(&_bc_usb_cdc.receive_fifo, (uint8_t *) buffer, length);
}

static void _bc_usb_cdc_task_start(void *param)
{
    (void) param;

    bc_scheduler_unregister(_bc_usb_cdc.task_id);

    _bc_usb_cdc.task_id = bc_scheduler_register(_bc_usb_cdc_task, NULL, 0);

    USBD_Start(&hUsbDeviceFS);
}

static void _bc_usb_cdc_task(void *param)
{
    (void) param;

    if (_bc_usb_cdc.transmit_length == 0)
    {
        return;
    }

    HAL_NVIC_DisableIRQ(USB_IRQn);

    if (CDC_Transmit_FS(_bc_usb_cdc.transmit_buffer, _bc_usb_cdc.transmit_length) == USBD_OK)
    {
        _bc_usb_cdc.transmit_length = 0;
    }

    HAL_NVIC_EnableIRQ(USB_IRQn);

    bc_scheduler_plan_current_now();
}

static void _bc_usb_cdc_init_hsi48()
{
    bc_system_pll_enable();

    RCC->CRRCR |= RCC_CRRCR_HSI48ON;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    SYSCFG->CFGR3 |= SYSCFG_CFGR3_ENREF_HSI48;

    while((RCC->CRRCR & RCC_CRRCR_HSI48ON) == 0)
    {
        continue;
    }

    RCC->CCIPR |= RCC_USBCLKSOURCE_HSI48;
    RCC->CFGR &= ~RCC_CFGR_STOPWUCK_Msk;
}
