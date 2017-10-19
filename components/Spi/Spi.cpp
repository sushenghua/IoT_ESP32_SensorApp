/*
 * Spi: Wrap the stm32 HAL SPI
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "Spi.h"
#include "AppLog.h"

/////////////////////////////////////////////////////////////////////////////////////////
// SpiBus
/////////////////////////////////////////////////////////////////////////////////////////
static SpiBus  _spiBus[] = { SpiBus(SPI_HOST), SpiBus(HSPI_HOST), SpiBus(VSPI_HOST) };

inline int mapSpiHost(spi_host_device_t host)
{
    return (int)host;
}

SpiBus* SpiBus::busForHost(spi_host_device_t host)
{
    return &_spiBus[mapSpiHost(host)];
}

SpiBus::SpiBus(spi_host_device_t host)
: _host(host)
, _initialized(false)
{
    _config.miso_io_num = -1;
    _config.mosi_io_num = -1;
    _config.sclk_io_num = -1;
    _config.quadwp_io_num = -1;
    _config.quadhd_io_num = -1;
}

void SpiBus::setPins(int pinMiso, int pinMosi, int pinClk)
{
    if (!_initialized) {
        _config.miso_io_num = pinMiso;
        _config.mosi_io_num = pinMosi;
        _config.sclk_io_num = pinClk;
    }
}

void SpiBus::init(int pinMiso, int pinMosi, int pinClk)
{
    if (!_initialized) {
        _config.miso_io_num = pinMiso;
        _config.mosi_io_num = pinMosi;
        _config.sclk_io_num = pinClk;
        init();
    }
}

void SpiBus::init()
{
    if (!_initialized) {
        // initialize the SPI bus
        APP_LOGI("[SpiBus]", "init spi bus with host %d", _host);

        esp_err_t ret;
        ret = spi_bus_initialize(_host, &_config, 1);
        assert(ret == ESP_OK);
        _initialized = true;
    }
}

void SpiBus::deinit()
{
    if (_initialized) {
        APP_LOGI("[SpiBus]", "deinit spi bus with host %d", _host);
        esp_err_t ret;
        ret = spi_bus_free(_host);
        assert(ret == ESP_OK);
        _initialized = false;
    }
}

void SpiBus::addChannel(SpiChannel &channel)
{
    APP_LOGI("[SpiBus]", "add spi slave with cs pin %d to bus with host %d", 
             channel.config()->spics_io_num, _host);

    spi_device_handle_t spiHandle;
    esp_err_t ret;
    ret = spi_bus_add_device(_host, channel.config(), &spiHandle);
    assert(ret == ESP_OK);
    channel.setHandle(spiHandle);
}

void SpiBus::removeChannel(SpiChannel &channel)
{
    APP_LOGI("[SpiBus]", "remove spi slave with cs pin %d from bus with host %d",
             channel.config()->spics_io_num, _host);

    esp_err_t ret;
    ret = spi_bus_remove_device(channel.handle());
    assert(ret == ESP_OK);
}

/////////////////////////////////////////////////////////////////////////////////////////
// SpiChannel
/////////////////////////////////////////////////////////////////////////////////////////
void spi_pre_transfer_cb(spi_transaction_t *t) 
{
    SpiChannel *spiChannel = static_cast<SpiChannel*>(t->user);
    spiChannel->preTxCb();
}

void spi_post_transfer_cb(spi_transaction_t *t) 
{
    SpiChannel *spiChannel = static_cast<SpiChannel*>(t->user);
    spiChannel->postTxCb();
}

SpiChannel::SpiChannel()
: _disabled(false)
, _trans(NULL)
, _transCount(0)
{
    _config.pre_cb = NULL;
    _config.post_cb = NULL;
}

SpiChannel::SpiChannel(uint8_t mode, int pinCs, int queueSize, int clkSpeed)
: _disabled(false)
, _trans(NULL)
, _transCount(0)
{
    _config.clock_speed_hz = clkSpeed;
    _config.mode = mode;
    _config.spics_io_num = pinCs;
    _config.queue_size = queueSize;
    _config.pre_cb = NULL;
    _config.post_cb = NULL;
}

void SpiChannel::setParams(uint8_t mode, int pinCs, int queueSize, int clkSpeed)
{
    _config.clock_speed_hz = clkSpeed;
    _config.mode = mode;
    _config.spics_io_num = pinCs;
    _config.queue_size = queueSize;
}

void SpiChannel::reset(TickType_t waitTicks)
{
    spi_device_reset(_handle, &_rtrans, waitTicks);
}

#ifdef DEBUG_FLAG_ENABLED
void SpiChannel::spi_bug()
{
    spi_device_try_to_block(_handle, &_rtrans, 0);
}
#endif

void SpiChannel::enableCallback(bool preCbEnabled, bool postCbEnabled)
{
    _config.pre_cb = preCbEnabled? spi_pre_transfer_cb : NULL;
    _config.post_cb = postCbEnabled? spi_post_transfer_cb : NULL;

    void* userData = (preCbEnabled || postCbEnabled)? static_cast<void*>(this) : NULL;
    for (int i = 0; i < _transCount; ++i) {
        _trans[i].user = userData;
    }
}
