/*
 * SpiBus: Wrap the spi_master
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _SPI_BUS_H
#define _SPI_BUS_H

#include <string.h>
#include "driver/spi_master.h"

class SpiChannel;

class SpiBus
{
public:
    // static class methods
    static SpiBus* busForHost(spi_host_device_t host);

public:
    // constructor
    SpiBus(spi_host_device_t host);

    // config, init and deinit
    void setPins(int pinMiso, int pinMosi, int pinClk);
    void init(int pinMiso, int pinMosi, int pinClk);
    void init();
    void deinit();

    // channel methods
    void addChannel(SpiChannel &channel);
    void removeChannel(SpiChannel &channel);

protected:
    const spi_host_device_t  _host;
    bool                     _initialized;
    spi_bus_config_t         _config;
};


#define DEFAULT_SPI_CLK_SPEED   10000000   // 10 MHz

class SpiChannel
{
public:
    SpiChannel();
    SpiChannel(uint8_t mode, int pinCs, int queueSize, int clkSpeed = DEFAULT_SPI_CLK_SPEED);

    spi_device_interface_config_t * config() { return &_config; }

    // set parameters
    void setParams(uint8_t mode, int pinCs, int queueSize, int clkSpeed = DEFAULT_SPI_CLK_SPEED);

    // handle
    void setHandle(spi_device_handle_t handle) { _handle = handle; }
    spi_device_handle_t handle() { return _handle; }

    // transaction cache
    void bindTransactionCache(spi_transaction_t *trans, int count) {
        _trans = trans;
        _transCount = count;
    }

    // callback
    void enableCallback(bool preCbEnabled = true, bool postCbEnabled = true);

    // tx and rx
    bool tx(const uint8_t *data, uint16_t size) {
        _clearTrans(&_trans[0]);
        _trans[0].length = size * 8;
        _trans[0].tx_buffer = data;
        _trans[0].rx_buffer = NULL;
        // return spi_device_queue_trans(_handle, &_trans[0], portMAX_DELAY) == ESP_OK;
        return spi_device_transmit(_handle, &_trans[0]);
    }

    bool rx(uint8_t *data, uint16_t size) {
        _clearTrans(&_trans[0]);
        _trans[0].length = size;
        _trans[0].tx_buffer = NULL;
        _trans[0].rx_buffer = data;
        // return spi_device_queue_trans(_handle, &_trans[0], portMAX_DELAY) == ESP_OK;
        return spi_device_transmit(_handle, &_trans[0]);
    }

    bool tx(uint8_t byte) {
        _clearTrans(&_trans[0]);
        _trans[0].length = 1 * 8;
        _trans[0].tx_data[0] = byte;
        _trans[0].flags = SPI_TRANS_USE_TXDATA;
        _trans[0].rx_buffer = NULL;
        // return spi_device_queue_trans(_handle, &_trans[0], portMAX_DELAY) == ESP_OK;
        return spi_device_transmit(_handle, &_trans[0]);
    }

    bool tx16(uint16_t halfword) {
        _clearTrans(&_trans[0]);
        _trans[0].length = 2 * 8;
        _trans[0].tx_data[0] = (halfword >> 8) & 0xFF;
        _trans[0].tx_data[1] = halfword & 0xFF;
        _trans[0].flags = SPI_TRANS_USE_TXDATA;
        _trans[0].rx_buffer = NULL;
        // return spi_device_queue_trans(_handle, &_trans[0], portMAX_DELAY) == ESP_OK;
        return spi_device_transmit(_handle, &_trans[0]);
    }

    bool tx32(uint32_t word) {
        _clearTrans(&_trans[0]);
        _trans[0].length = 4 * 8;
        _trans[0].tx_data[0] = (word >> 24) & 0xFF;
        _trans[0].tx_data[1] = (word >> 16) & 0xFF;
        _trans[0].tx_data[2] = (word >> 8) & 0xFF;
        _trans[0].tx_data[3] = word & 0xFF;
        _trans[0].flags = SPI_TRANS_USE_TXDATA;
        _trans[0].rx_buffer = NULL;
        // return spi_device_queue_trans(_handle, &_trans[0], portMAX_DELAY) == ESP_OK;
        return spi_device_transmit(_handle, &_trans[0]);
    }

public:
    // virtual callback
    virtual void preTxCb() {}
    virtual void postTxCb() {}

protected:
    // helper
    void _clearTrans(spi_transaction_t *trans) {
        //memset(trans, 0, sizeof(spi_transaction_t)); // also clear the trans->user -> may cause callback crash
        trans->flags = 0;
        trans->tx_buffer = NULL;
        trans->rx_buffer = NULL;
    }

protected:
    // spi handle of esp-idf framework
    spi_device_handle_t           _handle;
    // spi config
    spi_device_interface_config_t _config;
    // following two variable must be initialized in subclass or by calling bindTransactionCache    
    spi_transaction_t            *_trans;
    int                           _transCount;
};

#endif // _SPI_BUS_H
