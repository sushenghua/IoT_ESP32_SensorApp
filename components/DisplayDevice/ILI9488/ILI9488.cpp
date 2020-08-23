/*
 * ILI9488: ILI9488 display driver
 * based on https://github.com/adafruit/Adafruit_ILI9341/blob/master/Adafruit_ILI9341.cpp
 * Copyright (c) 2017 Shenghua Su
 *
 */
#include "ILI9488.h"
#include "freertos/task.h"
#include "driver/gpio.h"
// #include "esp_heap_alloc_caps.h"
#include "LedController.h"

// delay definition
#define RESET_DELAY_TIME         5
#ifndef delay
#define delay(x)                 vTaskDelay((x)/portTICK_RATE_MS)
#endif

// GPIO
#define ILI9488_DC_CMD_MODE      0
#define ILI9488_DC_DATA_MODE     1

// used in rotation
#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH  0x04

// SPI pin definition
#define PIN_NOT_USED -1
// #define PIN_NUM_MISO 25
#define PIN_NUM_MISO PIN_NOT_USED
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  21
#define PIN_NUM_CS   19

#define USE_CS_CONTROL
#ifdef  USE_CS_CONTROL
#define ILI9488_PIN_CS   PIN_NUM_CS
#else
#define ILI9488_PIN_CS   PIN_NOT_USED
#endif

#define PIN_NUM_DC   18
#define PIN_NUM_RST  22
#define PIN_NUM_BCKL 5

// #define ILI9488_CHANNEL_WAIT_TICKS  portMAX_DELAY
#define ILI9488_CHANNEL_WAIT_TICKS  (6000/portTICK_RATE_MS)
#define ILI9488_CHANNEL_CLK_SPEED   20000000
#define ILI9488_CHANNEL_QUEQUE_SIZE 1
#define ILI9488_SPI_TRANS_MAX       1
static spi_transaction_t _ili9488SpiTrans[ILI9488_SPI_TRANS_MAX];

// back light led controller
#define USING_LED_CONTROLLER
#ifdef USING_LED_CONTROLLER
LedController _backLed;
#endif

uint16_t ILI9488::color565(uint8_t r, uint8_t g, uint8_t b)
{
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);
}

ILI9488::ILI9488()
: DisplayGFX(ILI9488_TFTHEIGHT, ILI9488_TFTWIDTH)
, _on(true)
, _backLedDuty(0)
{}

void ILI9488::_initBus()
{
  // initialize non-SPI GPIOs
  gpio_set_direction((gpio_num_t)PIN_NUM_DC, GPIO_MODE_OUTPUT);
  gpio_set_direction((gpio_num_t)PIN_NUM_RST, GPIO_MODE_OUTPUT);
#ifndef USE_CS_CONTROL
  gpio_set_direction((gpio_num_t)PIN_NUM_CS, GPIO_MODE_OUTPUT);
#endif

  // spi bus init
  SpiBus *bus = SpiBus::busForHost(VSPI_HOST);
  bus->init(PIN_NUM_MISO, PIN_NUM_MOSI, PIN_NUM_CLK);
  _spiChannel.setParams(0, ILI9488_PIN_CS, ILI9488_CHANNEL_QUEQUE_SIZE, ILI9488_CHANNEL_CLK_SPEED);
  _spiChannel.bindTransactionCache(_ili9488SpiTrans, ILI9488_SPI_TRANS_MAX);
  bus->addChannel(_spiChannel);
}

void ILI9488::_initBackLed()
{
#ifdef USING_LED_CONTROLLER
  _backLed.init(PIN_NUM_BCKL);
  _brightness = 100;
  _backLedDuty = 1023;
#else
  gpio_set_direction((gpio_num_t)PIN_NUM_BCKL, GPIO_MODE_OUTPUT);
#endif
}

void ILI9488::_initIli9488()
{
  turnOn(false);          // turn off led
  _fireResetSignal();     // hard wire reset
  _initIli9488WithCmd();  // send init cmd sequence to device
  fillScreen(RGB565_BLACK);
  turnOn(true);           // turn on led
}

void ILI9488::init(int mode)
{
  switch (mode) {
    case DISPLAY_INIT_ALL:
      _initBus();
      _initBackLed();
      _initIli9488();
      break;

    case DISPLAY_INIT_ONLY_BUS:
      _initBus();
      _initBackLed();
      break;

    case DISPLAY_INIT_ONLY_DEVICE:
      _initBackLed();
      _initIli9488();
      break;

    default:
      break;
  }
}

void ILI9488::reset()
{
  _spiChannel.setDisabled(true);
  _spiChannel.reset();
  // _spiChannel.setDisabled(false);

  // // remove channel device and free bus
  // SpiBus *bus = SpiBus::busForHost(HSPI_HOST);
  // bus->removeChannel(_spiChannel);
  // bus->deinit();

  // // spi bus init and add channel device
  // bus->init(PIN_NUM_MISO, PIN_NUM_MOSI, PIN_NUM_CLK);
  // bus->addChannel(_spiChannel);

  // re-enable spi channel device
  _spiChannel.setDisabled(false);

  // init ili9488 device
  // _fireResetSignal();
  // _initIli9488WithCmd();

  // soft reset ili9488
  // writeCommand(ILI9488_SWRESET);
  // delay(RESET_DELAY_TIME);
}

void ILI9488::turnOn(bool on)
{
  if (_on != on) {
    _on = on;
#ifdef USING_LED_CONTROLLER
    _backLed.setDuty(on ? _backLedDuty : 0);
#else
    gpio_set_level((gpio_num_t)PIN_NUM_BCKL, on ? 1 : 0);
#endif
  }
}

void ILI9488::setBrightness(uint8_t b)
{
  if (_brightness != b && _on) {
    _brightness = b;
    _backLedDuty = (uint32_t) (10.23f * _brightness);
    _backLed.setDuty(_backLedDuty);
  }
}

void ILI9488::fadeBrightness(uint8_t b, int duration)
{
  if (_brightness != b && _on) {
    _brightness = b;
    _backLedDuty = (uint32_t) (10.23f * _brightness);
    _backLed.fadeToDuty(_backLedDuty, duration);
  }
}

void ILI9488::_fireResetSignal()
{
#ifndef USE_CS_CONTROL
  gpio_set_level((gpio_num_t)PIN_NUM_CS, 0);
#endif
  gpio_set_level((gpio_num_t)PIN_NUM_RST, 0);
  delay(RESET_DELAY_TIME);
  gpio_set_level((gpio_num_t)PIN_NUM_RST, 1);
  delay(RESET_DELAY_TIME);
}

void ILI9488::writeCommand(uint8_t cmd)
{
  gpio_set_level((gpio_num_t)PIN_NUM_DC, ILI9488_DC_CMD_MODE);
  _spiChannel.tx(cmd, ILI9488_CHANNEL_WAIT_TICKS);
  gpio_set_level((gpio_num_t)PIN_NUM_DC, ILI9488_DC_DATA_MODE);
}

void ILI9488::writeData(uint8_t data)
{
  gpio_set_level((gpio_num_t)PIN_NUM_DC, ILI9488_DC_DATA_MODE);
  _spiChannel.tx(data, ILI9488_CHANNEL_WAIT_TICKS);
}

void ILI9488::writeData(const uint8_t *data, uint16_t count)
{
  gpio_set_level((gpio_num_t)PIN_NUM_DC, ILI9488_DC_DATA_MODE);
  _spiChannel.tx(data, count, ILI9488_CHANNEL_WAIT_TICKS);
}

typedef struct {
  uint8_t cmd;
  uint8_t data[16];
  uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} ILI9488InitCmd;

// --- for TFT-LCD 320x480 (Part NO. Z350IT008) from ZhanHengAn Tech
DRAM_ATTR static const ILI9488InitCmd ili9488InitCmd[] = {

  // --- display off
  // {0x28, {0}, 0x80},

  // --- adjust control 3
  {0xF7, {0xA9, 0x51, 0x2C, 0x82}, 4},

  // --- power control 1
  {0xC0, {0x11, 0x09}, 2},
  // --- power control 2
  {0xC1, {0x41}, 1},
  // --- VCOM control
  {0xC5, {0x00, 0x2A, 0x80}, 3},

  // --- frame rate control (In Normal Mode/Full Colors)
  {0xB1, {0xB0, 0x11}, 2},
  // --- display inversion control
  {0xB4, {0x02}, 1},
  // --- display function control
  {0xB6, {0x02, 0x22}, 2},
  // --- entry mode set
  {0xB7, {0xC6}, 1},

  // --- HS Lanes Control
  {0xBE, {0x00, 0x04}, 2},

  // --- Set Image Function
  {0xE9, {0x00}, 1},

  // --- memory access control
  {0x36, {0xA8}, 1},  // (rotate to CW 0 initially) BGR color filter panel
  // --- pixel format
  {0x3A, {0x66}, 1},  // Pixel format(RGB Interface format, MCU Interface format): 24 bits / pixel

  // --- positive gamma correction
  {0xE0, {0x00, 0x27, 0x12, 0x0B, 0x18, 0x0B, 0x3F, 0x9B, 0x4B, 0x0B, 0x0F, 0x0B, 0x15, 0x17, 0x0F}, 15},
  // --- negative gamma correction
  {0XE1, {0x00, 0x16, 0x1B, 0x02, 0x0F, 0x06, 0x34, 0x46, 0x48, 0x04, 0x0D, 0x0D, 0x35, 0x36, 0x0F}, 15},

  // --- sleep out
  {0x11, {0}, 0x80},
  // --- display on
  {0x29, {0}, 0x80},

  // --- end of command/parameters sequence
  {0, {0}, 0xFF}
};

void ILI9488::_initIli9488WithCmd()
{
  int i = 0;
  // send all the commands and data
  while (ili9488InitCmd[i].databytes != 0xFF) {
    writeCommand(ili9488InitCmd[i].cmd);
    if ((ili9488InitCmd[i].databytes & 0x1F) > 0)
      writeData(ili9488InitCmd[i].data, ili9488InitCmd[i].databytes & 0x1F);
    if (ili9488InitCmd[i].databytes & 0x80) {
      delay(100);
    }
    ++i;
  }
}

void ILI9488::setRotation(uint8_t m)
{
  if (_rotation == m) return;
  _rotation = m;
  switch (_rotation) {
    case DISPLAY_ROTATION_CW_0:
      m = (MADCTL_MY | MADCTL_MV | MADCTL_BGR);
      _width  = ILI9488_TFTHEIGHT;
      _height = ILI9488_TFTWIDTH;
      break;
    case DISPLAY_ROTATION_CW_90:
      m = (MADCTL_ML | MADCTL_MH | MADCTL_BGR);
      _width  = ILI9488_TFTWIDTH;
      _height = ILI9488_TFTHEIGHT;
      break;
    case DISPLAY_ROTATION_CW_180:
      m = (MADCTL_MX | MADCTL_MH | MADCTL_MV | MADCTL_BGR);
      _width  = ILI9488_TFTHEIGHT;
      _height = ILI9488_TFTWIDTH;
      break;
    case DISPLAY_ROTATION_CW_270:
      m = (MADCTL_MX | MADCTL_MY | MADCTL_ML | MADCTL_MH | MADCTL_BGR);
      _width  = ILI9488_TFTWIDTH;
      _height = ILI9488_TFTHEIGHT;
      break;
  }
  writeCommand(ILI9488_MADCTL);
  _spiChannel.tx(m, ILI9488_CHANNEL_WAIT_TICKS);
}

void ILI9488::invertDisplay(bool i)
{
  writeCommand(i ? ILI9488_INVON : ILI9488_INVOFF);
}

void ILI9488::scrollTo(uint16_t y)
{
  writeCommand(ILI9488_VSCRSADD);
  _spiChannel.tx16(y);
}

void ILI9488::setViewPort(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  uint32_t xa = ((uint32_t)x << 16) | (x+w-1);
  uint32_t ya = ((uint32_t)y << 16) | (y+h-1);
  writeCommand(ILI9488_CASET); // Column addr set
  _spiChannel.tx32(xa);
  writeCommand(ILI9488_PASET); // Row addr set
  _spiChannel.tx32(ya);
  writeCommand(ILI9488_RAMWR); // write to RAM
}


#ifdef ILI9488_SUPPORT_SPI_RGB565

  #define COLOR_BYTE_COUNT  2

  inline void _color_to_esp32SpiBytes(uint8_t *target, const uint16_t &color)
  {
    *(target  ) = color & 0xFF;
    *(target+1) = (color >> 8) & 0xFF;
  }

#else // seems ILI9488 only support RGB666 format in SPI 4-line interface

  #define COLOR_BYTE_COUNT  3

  inline void _color_to_esp32SpiBytes(uint8_t *target, const uint16_t &color)
  {
    *(target  ) = (color & 0xF800) >> 8;
    *(target+1) = (color & 0x07E0) >> 3;
    *(target+2) = (color & 0x001F) << 3;
  }

#endif

#define SPI_MAX_PIXELS_AT_ONCE  128
static uint8_t _ili9488ColorTxBuf[SPI_MAX_PIXELS_AT_ONCE * COLOR_BYTE_COUNT];

void ILI9488::writePixel(uint16_t color)
{
  _color_to_esp32SpiBytes(_ili9488ColorTxBuf, color);
  _spiChannel.tx(_ili9488ColorTxBuf, COLOR_BYTE_COUNT, ILI9488_CHANNEL_WAIT_TICKS);
}

void ILI9488::writePixels(uint16_t *colors, uint32_t len)
{
  uint32_t colorIndex = 0;
  uint32_t txLen = 0;
  uint32_t bufLen = (len > SPI_MAX_PIXELS_AT_ONCE)? SPI_MAX_PIXELS_AT_ONCE : len;

  while (len > 0) {
    txLen = (len > bufLen) ? bufLen : len;
    // cover colors to bytes array
    uint8_t *p = _ili9488ColorTxBuf;
    for (uint32_t i = 0;  i < txLen; ++i) {
      _color_to_esp32SpiBytes(p, colors[colorIndex]);
      p += COLOR_BYTE_COUNT;
      ++colorIndex;
    }
    _spiChannel.tx(_ili9488ColorTxBuf, txLen * COLOR_BYTE_COUNT, ILI9488_CHANNEL_WAIT_TICKS);
    len -= txLen;
  }
}

void ILI9488::writeColor(uint16_t color, uint32_t len)
{
  uint32_t blen = (len > SPI_MAX_PIXELS_AT_ONCE)? SPI_MAX_PIXELS_AT_ONCE : len;
  uint32_t txLen = 0;

  uint8_t *p = _ili9488ColorTxBuf;
  for (uint32_t i = 0;  i < blen; ++i) {
    _color_to_esp32SpiBytes(p, color);
    p += COLOR_BYTE_COUNT;
  }

  while(len) {
    txLen = (len > blen)? blen : len;
    _spiChannel.tx(_ili9488ColorTxBuf, txLen * COLOR_BYTE_COUNT, ILI9488_CHANNEL_WAIT_TICKS);
    len -= txLen;
  }
}

uint8_t ILI9488::readcommand8(uint8_t c, uint8_t index)
{
  writeCommand(0xD9);  // woo sekret command?
  _spiChannel.tx(0x10 + index, ILI9488_CHANNEL_WAIT_TICKS);
  writeCommand(c);

  uint8_t r = 0;
  _spiChannel.rx(&r, 1);
  return r;
}

void ILI9488::drawPixel(int16_t x, int16_t y, uint16_t color)
{
  if((x < 0) ||(x >= _width) || (y < 0) || (y >= _height)) return;
  setViewPort(x,y,1,1);
  writePixel(color);
}

void ILI9488::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
  fillRect(x, y, 1, h, color);
}

void ILI9488::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
  fillRect(x, y, w, 1, color);
}

void ILI9488::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  if((x >= _width) || (y >= _height)) return;
  int16_t x2 = x + w - 1, y2 = y + h - 1;
  if((x2 < 0) || (y2 < 0)) return;

  // Clip left/top
  if(x < 0) {
    x = 0;
    w = x2 + 1;
  }
  if(y < 0) {
    y = 0;
    h = y2 + 1;
  }

  // Clip right/bottom
  if(x2 >= _width)  w = _width  - x;
  if(y2 >= _height) h = _height - y;

  int32_t len = (int32_t)w * h;
  setViewPort(x, y, w, h);
  writeColor(color, len);
}

// This code was ported/adapted from https://github.com/PaulStoffregen/ILI9488_t3
// by Marc MERLIN. See examples/pictureEmbed to use this.
void ILI9488::drawBitmap(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pcolors)
{
  int16_t x2, y2; // Lower-right coord
  if ( (x              >= _width ) ||      // Off-edge right
       (y              >= _height) ||      // " top
       ((x2 = (x+w-1)) <  0     )  ||      // " left
       ((y2 = (y+h-1)) <  0)    ) return;  // " bottom

  int16_t bx1=0, by1 = 0; // Clipped top-left within bitmap
  int16_t saveW = w;      // Save original bitmap width value
  if(x < 0) { // Clip left
    w  +=  x;
    bx1 = -x;
    x   =  0;
  }
  if(y < 0) { // Clip top
    h  +=  y;
    by1 = -y;
    y   =  0;
  }
  if(x2 >= _width ) w = _width  - x; // Clip right
  if(y2 >= _height) h = _height - y; // Clip bottom

  pcolors += by1 * saveW + bx1; // Offset bitmap ptr to clipped top-left

  setViewPort(x, y, w, h); // Clipped area
  while(h--) { // For each (clipped) scanline...

#ifdef ILI9488_SUPPORT_SPI_RGB565

    _spiChannel.tx((uint8_t*)pcolors, w * COLOR_BYTE_COUNT, ILI9488_CHANNEL_WAIT_TICKS); // Push one (clipped) row

#else

    uint8_t *p = _ili9488ColorTxBuf;
    for (uint16_t i = 0;  i < w; ++i) {
      _color_to_esp32SpiBytes(p, pcolors[i]);
      p += COLOR_BYTE_COUNT;
    }
    _spiChannel.tx(_ili9488ColorTxBuf, w * COLOR_BYTE_COUNT, ILI9488_CHANNEL_WAIT_TICKS);

#endif

    pcolors += saveW; // Advance pointer by one full (unclipped) line
  }
}

void ILI9488::test()
{
  this->fillScreen(RGB565_RED);
  setViewPort(0, 40, 240, 240);
  fillRect(0, 40, 240, 240, RGB565_GREEN);
  //this->scrollTo(100);
  this->setCursor(100, 100);
  this->setTextSize(2);
  this->write("hello");
  // for (int i=0; i<100; ++i)
  //   this->writeColor(RGB565_GREEN, 240);
  drawCircle(120, 160, 50, RGB565_WHITE);
}
