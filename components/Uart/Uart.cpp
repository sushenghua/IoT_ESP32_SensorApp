/*
 * Uart: Wrap the esp uart
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "Uart.h"
#include "esp_log.h"

Uart::Uart(uart_port_t port, int rxBufSize, int txBufSize, int queueSize)
: _port(port)
, _allocated(false)
, _rxBufSize(rxBufSize)
, _txBufSize(txBufSize)
, _queueSize(queueSize)
{
	setParams(); // use default settings
}

void Uart::setParams(int                   baudRate,
                     uart_word_length_t    dataBits,
                     uart_parity_t         parity,
                     uart_stop_bits_t      stopBits,
                     uart_hw_flowcontrol_t flowCtrl,
                     uint8_t               rxFlowCtrlThresh)
{
    _config.baud_rate           = baudRate;
    _config.data_bits           = dataBits;
    _config.parity              = parity;
    _config.stop_bits           = stopBits;
    _config.flow_ctrl           = flowCtrl;
    _config.rx_flow_ctrl_thresh = rxFlowCtrlThresh;
}

void Uart::setPins(int pinTx, int pinRx, int pinRts, int pinCts)
{
    if (!_allocated) {
        _pinTx = pinTx;
        _pinRx = pinRx;
        _pinRts = pinRts;
        _pinCts = pinCts;
    }
}

void Uart::alloc(int pinTx, int pinRx, int pinRts, int pinCts)
{
    if (!_allocated) {
        _pinTx = pinTx;
        _pinRx = pinRx;
        _pinRts = pinRts;
        _pinCts = pinCts;
        alloc();
    }
}

void Uart::alloc()
{
    if (!_allocated) {

        ESP_LOGI("[Uart]", "alloc uart with port %d", _port);

        esp_err_t ret;

        ret = uart_param_config(_port, &_config);
        assert(ret == ESP_OK);

        ret = uart_set_pin(_port, _pinTx, _pinRx, _pinRts, _pinCts);
        assert(ret == ESP_OK);

        ret = uart_driver_install(_port, _rxBufSize, _txBufSize, _queueSize, NULL, 0);
        assert(ret == ESP_OK);

        _allocated = true;
    }
}

void Uart::free()
{
    if (_allocated) {

        ESP_LOGI("[Uart]", "free uart with port %d", _port);

        esp_err_t ret;

        ret = uart_driver_delete(_port);
        assert(ret == ESP_OK);

        _allocated = false;
    }
}