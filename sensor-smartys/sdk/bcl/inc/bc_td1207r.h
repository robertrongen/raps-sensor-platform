#ifndef _BC_TD1207R_H
#define _BC_TD1207R_H

#include <bc_scheduler.h>
#include <bc_gpio.h>
#include <bc_uart.h>

//! @addtogroup bc_td1207r bc_td1207r
//! @brief Driver for TD1207R SigFox modem
//! @{

//! @cond

#define BC_TD1207R_TX_FIFO_BUFFER_SIZE 64
#define BC_TD1207R_RX_FIFO_BUFFER_SIZE 64

//! @endcond

//! @brief Callback events

typedef enum
{
    //! @brief Ready event
    BC_TD1207R_EVENT_READY = 0,

    //! @brief Error event
    BC_TD1207R_EVENT_ERROR = 1,

    //! @brief RF frame transmission started event
    BC_TD1207R_EVENT_SEND_RF_FRAME_START = 2,

    //! @brief RF frame transmission finished event
    BC_TD1207R_EVENT_SEND_RF_FRAME_DONE = 3

} bc_td1207r_event_t;

//! @brief TD1207R instance

typedef struct bc_td1207r_t bc_td1207r_t;

//! @cond

typedef enum
{
    BC_TD1207R_STATE_READY = 0,
    BC_TD1207R_STATE_ERROR = 1,
    BC_TD1207R_STATE_INITIALIZE = 2,
    BC_TD1207R_STATE_INITIALIZE_RESET_L = 3,
    BC_TD1207R_STATE_INITIALIZE_RESET_H = 4,
    BC_TD1207R_STATE_INITIALIZE_AT_COMMAND = 5,
    BC_TD1207R_STATE_INITIALIZE_AT_RESPONSE = 6,
    BC_TD1207R_STATE_SEND_RF_FRAME_COMMAND = 7,
    BC_TD1207R_STATE_SEND_RF_FRAME_RESPONSE = 8

} bc_td1207r_state_t;

struct bc_td1207r_t
{
    bc_scheduler_task_id_t _task_id;
    bc_gpio_channel_t _reset_signal;
    bc_uart_channel_t _uart_channel;
    bc_td1207r_state_t _state;
    bc_fifo_t _tx_fifo;
    bc_fifo_t _rx_fifo;
    uint8_t _tx_fifo_buffer[BC_TD1207R_TX_FIFO_BUFFER_SIZE];
    uint8_t _rx_fifo_buffer[BC_TD1207R_RX_FIFO_BUFFER_SIZE];
    void (*_event_handler)(bc_td1207r_t *, bc_td1207r_event_t, void *);
    void *_event_param;
    char _command[BC_TD1207R_TX_FIFO_BUFFER_SIZE];
    char _response[BC_TD1207R_RX_FIFO_BUFFER_SIZE];
    uint8_t _message_buffer[12];
    size_t _message_length;
};

//! @endcond

//! @brief Initialize TD1207R
//! @param[in] self Instance
//! @param[in] reset_signal GPIO channel where RST signal is connected
//! @param[in] uart_channel UART channel where TX and RX signals are connected

void bc_td1207r_init(bc_td1207r_t *self, bc_gpio_channel_t reset_signal, bc_uart_channel_t uart_channel);

//! @brief Set callback function
//! @param[in] self Instance
//! @param[in] event_handler Function address
//! @param[in] event_param Optional event parameter (can be NULL)

void bc_td1207r_set_event_handler(bc_td1207r_t *self, void (*event_handler)(bc_td1207r_t *, bc_td1207r_event_t, void *), void *event_param);

//! @brief Check if modem is ready for commands
//! @param[in] self Instance
//! @return true If ready
//! @return false If not ready

bool bc_td1207r_is_ready(bc_td1207r_t *self);

//! @brief Send RF frame command
//! @param[in] self Instance
//! @param[in] buffer Pointer to data to be transmitted
//! @param[in] length Length of data to be transmitted in bytes (must be from 1 to 12 bytes)
//! @return true If command was accepted for processing
//! @return false If command was denied for processing

bool bc_td1207r_send_rf_frame(bc_td1207r_t *self, const void *buffer, size_t length);

//! @}

#endif // _BC_TD1207R_H
