/*
 * Uart: Wrap the esp uart
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _UART_H
#define _UART_H

#include <string.h>
#include "driver/uart.h"

// default uart buf, queue size
#define UART_DEFAULT_TX_SIZE         0
#define UART_DEFAULT_RX_SIZE         (UART_FIFO_LEN * 4)
#define UART_DEFAULT_QUEUE_SIZE      0

// uart config params
#define UART_DEFAULT_BAUDRATE        115200
#define UART_DEFAULT_WORD_LEN        UART_DATA_8_BITS 
#define UART_DEFAULT_PARITY          UART_PARITY_DISABLE
#define UART_DEFAULT_STOP_BITS       UART_STOP_BITS_1
#define UART_DEFAULT_HW_FLOWCTRL     UART_HW_FLOWCTRL_DISABLE
#define UART_DEFAULT_RX_CTRL_THRESH  122

// default pin mapping
// +----------+----------+----------+---------+---------+
// |  UART    |  RX      |  TX      |  CTS    |  RTS    |
// |----------+----------+----------+---------+---------+
// |  UART0   |  GPIO3   |  GPIO1   |  N/A    |  N/A    |
// |----------+----------+----------+---------+---------+
// |  UART1   |  GPIO9   |  GPIO10  |  GPIO6  |  GPIO11 |
// |----------+----------+----------+---------+---------+
// |  UART2   |  GPIO16  |  GPIO17  |  GPIO8  |  GPIO7  |
// +----------+----------+----------+---------+---------+
#define UART_DEFAULT_PIN             UART_PIN_NO_CHANGE

// tx
#define UART_DEFAULT_RX_WAIT_TICKS   100
#define UART_MAX_RX_WAIT_TICKS       portMAX_DELAY

class Uart
{
public:
    // constructor
    Uart(uart_port_t port, int rxBufSize = UART_DEFAULT_RX_SIZE,
                           int txBufSize = UART_DEFAULT_TX_SIZE,
                           int queueSize = UART_DEFAULT_QUEUE_SIZE);

    // config, init and deinit
    void setPins(int pinTx, int pinRx, int pinRts = UART_DEFAULT_PIN, int pinCts = UART_DEFAULT_PIN);
    void setParams(int                   baudRate = UART_DEFAULT_BAUDRATE,
                   uart_word_length_t    dataBits = UART_DEFAULT_WORD_LEN,
                   uart_parity_t         parity   = UART_DEFAULT_PARITY,
                   uart_stop_bits_t      stopBits = UART_DEFAULT_STOP_BITS,
                   uart_hw_flowcontrol_t flowCtrl = UART_DEFAULT_HW_FLOWCTRL,
                   uint8_t               rxFlowCtrlThresh = UART_DEFAULT_RX_CTRL_THRESH);

    void init(int pinTx, int pinRx, int pinRts = UART_DEFAULT_PIN, int pinCts = UART_DEFAULT_PIN);
    void init();
    void deinit();

    // tx and rx
    int tx(const uint8_t *data, size_t size) {
        return uart_write_bytes(_port, (const char *)data, size);
    }

    int rx(uint8_t *data, size_t size, TickType_t waitTicks = UART_DEFAULT_RX_WAIT_TICKS) {
        return uart_read_bytes(_port, data, size, waitTicks);
    }

protected:
    // uart port
    const uart_port_t    _port;
    // indicates if uart resource allocated
    bool                 _initialized;
    // pins
    int                  _pinTx;
    int                  _pinRx;
    int                  _pinRts;
    int                  _pinCts;
    // uart buf size, queue size
    int                  _rxBufSize;
    int                  _txBufSize;
    int                  _queueSize;
    uart_config_t        _config;
};

#endif // _UART_H
