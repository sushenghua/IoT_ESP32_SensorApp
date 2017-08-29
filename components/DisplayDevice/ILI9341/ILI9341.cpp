/*
 * ILI9341: ILI9341 display driver
 * based on https://github.com/adafruit/Adafruit_ILI9341/blob/master/Adafruit_ILI9341.cpp
 * Copyright (c) 2017 Shenghua Su
 *
 */
#include "ILI9341.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "LedController.h"

// delay definition
#define RESET_DELAY_TIME         5
#ifndef delay
#define delay(x)                 vTaskDelay((x)/portTICK_RATE_MS)
#endif

// GPIO
#define ILI9341_DC_CMD_MODE      0
#define ILI9341_DC_DATA_MODE     1

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
#define PIN_NUM_CLK  19
#define PIN_NUM_CS   22

#define PIN_NUM_DC   21
#define PIN_NUM_RST  18
#define PIN_NUM_BCKL 5

#define ILI9341_CHANNEL_CLK_SPEED   10000000
#define ILI9341_CHANNEL_QUEQUE_SIZE 5

// back light led controller
#define USING_LED_CONTROLLER
#ifdef USING_LED_CONTROLLER
LedController _backLed;
#endif

uint16_t ILI9341::color565(uint8_t r, uint8_t g, uint8_t b)
{
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);
}

ILI9341::ILI9341()
: DisplayGFX(ILI9341_TFTHEIGHT, ILI9341_TFTWIDTH)
, _backLedDuty(0)
{
}

#define ILI9341_SPI_TRANS_MAX 5
static spi_transaction_t _ili9341SpiTrans[ILI9341_SPI_TRANS_MAX];

void ILI9341::_initBus()
{
  // initialize non-SPI GPIOs
  gpio_set_direction((gpio_num_t)PIN_NUM_DC, GPIO_MODE_OUTPUT);
  gpio_set_direction((gpio_num_t)PIN_NUM_RST, GPIO_MODE_OUTPUT);

  // spi bus init
  SpiBus *bus = SpiBus::busForHost(HSPI_HOST);
  bus->init(PIN_NUM_MISO, PIN_NUM_MOSI, PIN_NUM_CLK);
  _spiChannel.setParams(0, PIN_NUM_CS, ILI9341_CHANNEL_QUEQUE_SIZE, ILI9341_CHANNEL_CLK_SPEED);
  _spiChannel.bindTransactionCache(_ili9341SpiTrans, ILI9341_SPI_TRANS_MAX);
  bus->addChannel(_spiChannel);
}

void ILI9341::_initBackLed()
{
#ifdef USING_LED_CONTROLLER
  _backLed.init(PIN_NUM_BCKL);
  _brightness = 100;
  _backLedDuty = 1023;
#else
  gpio_set_direction((gpio_num_t)PIN_NUM_BCKL, GPIO_MODE_OUTPUT);
  // turnOn(false);
#endif
}

void ILI9341::init()
{
  _initBus();

  _initBackLed();

  reset();
}

void ILI9341::reset()
{
  // turn off led
  turnOn(false);

  // reset
  _fireResetSignal();

  // init self
  _init();

  // turn on led
  turnOn(true);
}

void ILI9341::turnOn(bool on)
{
#ifdef USING_LED_CONTROLLER
  _backLed.setDuty(on ? _backLedDuty : 0);
#else
  gpio_set_level((gpio_num_t)PIN_NUM_BCKL, on ? 1 : 0);
#endif
}

void ILI9341::setBrightness(uint8_t b)
{
  if (_brightness != b) {
    _brightness = b;
    _backLedDuty = (uint32_t) (10.23f * _brightness);
    _backLed.setDuty(_backLedDuty);
  }
}

void ILI9341::fadeBrightness(uint8_t b, int duration)
{
  if (_brightness != b) {
    _brightness = b;
    _backLedDuty = (uint32_t) (10.23f * _brightness);
    _backLed.fadeToDuty(_backLedDuty, duration);
  }
}

void ILI9341::_fireResetSignal()
{
  gpio_set_level((gpio_num_t)PIN_NUM_RST, 0);
  delay(RESET_DELAY_TIME);
  gpio_set_level((gpio_num_t)PIN_NUM_RST, 1);
  delay(RESET_DELAY_TIME);
}

void ILI9341::writeCommand(uint8_t cmd)
{
  gpio_set_level((gpio_num_t)PIN_NUM_DC, ILI9341_DC_CMD_MODE);
  _spiChannel.tx(cmd);
  gpio_set_level((gpio_num_t)PIN_NUM_DC, ILI9341_DC_DATA_MODE);
}

void ILI9341::writeData(uint8_t data)
{
  gpio_set_level((gpio_num_t)PIN_NUM_DC, ILI9341_DC_DATA_MODE);
  _spiChannel.tx(data);
}

void ILI9341::writeData(const uint8_t *data, uint16_t count)
{
  gpio_set_level((gpio_num_t)PIN_NUM_DC, ILI9341_DC_DATA_MODE);
  _spiChannel.tx(data, count);
}

typedef struct {
  uint8_t cmd;
  uint8_t data[16];
  uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} ILI9341InitCmd;

// --- for TFT-LCD 2.2 Inch 240x320, the red pcb module
// DRAM_ATTR static const ILI9341InitCmd ili9341InitCmd[]={
//   {0xEF, {0x03, 0x80, 0x02}, 3},
//   {0xCF, {0x00, 0xC1, 0x30}, 3},
//   {0xED, {0x64, 0x03, 0x12, 0x81}, 4},
//   {0xE8, {0x85, 0x00, 0x78}, 3},
//   {0xCB, {0x39, 0x2C, 0x00, 0x34, 0x02}, 5},
//   {0xF7, {0x20}, 1},
//   {0xEA, {0x00, 0x00}, 2},
//   {0xC0, {0x23}, 1},
//   {0xC1, {0x10}, 1},
//   {0xC5, {0x3E, 0x28}, 2},
//   {0xC7, {0x86}, 1},
//   {0x36, {0x48}, 1},
//   {0x37, {0x00, 0x00}, 2},
//   {0x3A, {0x55}, 1},
//   {0xB1, {0x00, 0x18}, 2},
//   {0xB6, {0x08, 0x82, 0x27}, 3},
//   {0xF2, {0x00}, 1},
//   {0x26, {0x01}, 1},
//   {0xE0, {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00}, 15},
//   {0XE1, {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F}, 15},
//   {0x11, {0}, 0x80},
//   {0x29, {0}, 0x80},
//   {0, {0}, 0xFF}
// };

// --- for TFT-LCD 240x320 (Part NO. Z240IT008) from ZhanHengAn Tech
DRAM_ATTR static const ILI9341InitCmd ili9341InitCmd[] = {
  // {0x28, {0}, 0x80},                      // display off
  // {0xEF, {0x03, 0x80, 0x02}, 3},             // ?
  // --- power control A: 0x39, 0x2c, 0x00, 0x34(Vcore 1.6V), 0x02(DDVDH 5.6V)
  {0xCB, {0x39, 0x2C, 0x00, 0x34, 0x02}, 5},
  // --- power control B: 0x00, 0x, 0x30(ESD protection)
  {0xCF, {0x00, 0xC1, 0x30}, 3},
  // --- power on sequence control: 0x64(soft start), 0x03,0x12(power on sequence), 0x81(DDVDH enhance on)
  {0xED, {0x64, 0x03, 0x12, 0x81}, 4},
  // --- driver timing control A: 0x85(non-overlap), 0x00(EQ timing), 0x79(pre-charge timing)
  {0xE8, {0x85, 0x00, 0x79}, 3},
  // --- driver timing control B: 0x00(gate driver timing), 0x00
  {0xEA, {0x00, 0x00}, 2},
  // --- pump ratio control: DDVDH=2xVCI 
  {0xF7, {0x20}, 1},

  // --- power control
  {0xC0, {0x1D}, 1},  // VRH[5:0]: 4.30 V
  {0xC1, {0x12}, 1},  // SAP[2:0]; BT[3:0]
  // --- VCOM control
  {0xC5, {0x33, 0x3F}, 2},
  {0xC7, {0x92}, 1},
  // --- memory access control
  {0x36, {0xA8}, 1},  // (rotate to CW 90 initially) BGR color filter panel
  // --- pixel format
  {0x3A, {0x55}, 1},  // Pixel format(RGB Interface format, MCU Interface format): 16 bits / pixel
  // --- vertical scrolling star address
  // {0x37, {0x00, 0x00}, 2},
  // --- frame control
  {0xB1, {0x00, 0x12}, 2},
  // --- display function control
  {0xB6, {0x0A, 0xA2}, 2},
  // --- set tear scanline
  {0x44, {0x02}, 1},
  // --- 3g gamma control
  {0xF2, {0x00}, 1},
  // --- select gamma curve 
  {0x26, {0x01}, 1},
  // --- positive gamma correction
  {0xE0, {0x0F, 0x22, 0x1C, 0x1B, 0x08, 0x0F, 0x48, 0xB8, 0x34, 0x05, 0x0C, 0x09, 0x0F, 0x07, 0x00}, 15},
  // --- negative gamma correction
  {0XE1, {0x00, 0x23, 0x24, 0x07, 0x10, 0x07, 0x38, 0x47, 0x4B, 0x0A, 0x13, 0x06, 0x30, 0x38, 0x0F}, 15},
  // --- sleep out
  {0x11, {0}, 0x80},
  // --- display on
  {0x29, {0}, 0x80},
  // --- end of command/parameters sequence
  {0, {0}, 0xFF}
};

void ILI9341::_init()
{
  int i = 0;
  // send all the commands and data
  while (ili9341InitCmd[i].databytes != 0xFF) {
    writeCommand(ili9341InitCmd[i].cmd);
    if ((ili9341InitCmd[i].databytes & 0x1F) > 0)
      writeData(ili9341InitCmd[i].data, ili9341InitCmd[i].databytes & 0x1F);
    if (ili9341InitCmd[i].databytes & 0x80) {
      delay(100);
    }
    ++i;
  }
}

void ILI9341::setRotation(uint8_t m)
{
  if (_rotation == m) return;
  _rotation = m;
  switch (_rotation) {
    case DISPLAY_ROTATION_CW_0:
      m = (MADCTL_MY | MADCTL_MV | MADCTL_BGR);
      _width  = ILI9341_TFTHEIGHT;
      _height = ILI9341_TFTWIDTH;
      break;
    case DISPLAY_ROTATION_CW_90:
      m = (MADCTL_ML | MADCTL_MH | MADCTL_BGR);
      _width  = ILI9341_TFTWIDTH;
      _height = ILI9341_TFTHEIGHT;
      break;
    case DISPLAY_ROTATION_CW_180:
      m = (MADCTL_MX | MADCTL_MH | MADCTL_MV | MADCTL_BGR);
      _width  = ILI9341_TFTHEIGHT;
      _height = ILI9341_TFTWIDTH;
      break;
    case DISPLAY_ROTATION_CW_270:
      m = (MADCTL_MX | MADCTL_MY | MADCTL_ML | MADCTL_MH | MADCTL_BGR);
      _width  = ILI9341_TFTWIDTH;
      _height = ILI9341_TFTHEIGHT;
      break;
  }
  writeCommand(ILI9341_MADCTL);
  _spiChannel.tx(m);
}

void ILI9341::invertDisplay(bool i)
{
  writeCommand(i ? ILI9341_INVON : ILI9341_INVOFF);
}

void ILI9341::scrollTo(uint16_t y)
{
  writeCommand(ILI9341_VSCRSADD);
  _spiChannel.tx16(y);
}

void ILI9341::setViewPort(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  uint32_t xa = ((uint32_t)x << 16) | (x+w-1);
  uint32_t ya = ((uint32_t)y << 16) | (y+h-1);
  writeCommand(ILI9341_CASET); // Column addr set
  _spiChannel.tx32(xa);
  writeCommand(ILI9341_PASET); // Row addr set
  _spiChannel.tx32(ya);
  writeCommand(ILI9341_RAMWR); // write to RAM
}

void ILI9341::writePixel(uint16_t color)
{
  _spiChannel.tx16(color);
}

#define SPI_MAX_PIXELS_AT_ONCE  128
static uint16_t _ili9431ColorTxBuf[SPI_MAX_PIXELS_AT_ONCE];

void ILI9341::writePixels(uint16_t *colors, uint32_t len)
{
  uint32_t colorIndex = 0;
  uint32_t txLen = 0;
  uint32_t bufLen = (len > SPI_MAX_PIXELS_AT_ONCE)? SPI_MAX_PIXELS_AT_ONCE : len;

  while (len > 0) {
    txLen = (len > bufLen) ? bufLen : len;
    // cover 16-bit color to bytes array
    for (uint32_t i = 0;  i < txLen; ++i) {
      _ili9431ColorTxBuf[i] = (colors[colorIndex] << 8) | (colors[colorIndex] >> 8);
      ++colorIndex;
    }
    _spiChannel.tx((uint8_t*)_ili9431ColorTxBuf , txLen * 2);
    len -= txLen;
  }
}

void ILI9341::writeColor(uint16_t color, uint32_t len)
{
  uint32_t blen = (len > SPI_MAX_PIXELS_AT_ONCE)? SPI_MAX_PIXELS_AT_ONCE : len;
  uint32_t tlen = 0;

  for (uint16_t t = 0;  t < blen; ++t)
    _ili9431ColorTxBuf[t] = (color << 8) | (color >> 8);

  while(len) {
    tlen = (len > blen)? blen : len;
    _spiChannel.tx((uint8_t*)_ili9431ColorTxBuf, tlen * 2);
    len -= tlen;
  }
}

uint8_t ILI9341::readcommand8(uint8_t c, uint8_t index)
{
  writeCommand(0xD9);  // woo sekret command?
  _spiChannel.tx(0x10 + index);
  writeCommand(c);

  uint8_t r = 0;
  _spiChannel.rx(&r, 1);
  return r;
}

void ILI9341::drawPixel(int16_t x, int16_t y, uint16_t color)
{
  if((x < 0) ||(x >= _width) || (y < 0) || (y >= _height)) return;
  setViewPort(x,y,1,1);
  writePixel(color);
}

void ILI9341::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
  fillRect(x, y, 1, h, color);
}

void ILI9341::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
  fillRect(x, y, w, 1, color);
}

void ILI9341::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
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

// This code was ported/adapted from https://github.com/PaulStoffregen/ILI9341_t3
// by Marc MERLIN. See examples/pictureEmbed to use this.
void ILI9341::drawBitmap(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pcolors)
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
    _spiChannel.tx((uint8_t*)pcolors, w * 2); // Push one (clipped) row
    pcolors += saveW; // Advance pointer by one full (unclipped) line
  }
}

void ILI9341::test()
{
  this->fillScreen(RGB565_RED);
  // setViewPort(0, 40, 240, 240);
  // fillRect(0, 40, 240, 240, RGB565_GREEN);
  //this->scrollTo(100);
  this->setCursor(0, 100);
  this->setTextSize(2);
  this->write("oooooooppppp");
  // for (int i=0; i<100; ++i)
  //   this->writeColor(RGB565_GREEN, 240);
  drawCircle(120, 160, 50, RGB565_WHITE);
//   int     n, i, i2,
//         cx = _width  / 2,
//         cy = _height / 2;
//   n   = 240;//_width < _height? _width : _height;
// //  for(i=2; i<n; i+=6) {
// //  i2 = i / 2;
// //  fillRect(cx-i2, cy-i2, i, i, ILI9341_GREEN);
// //  }
//   i2 = n / 2;

// //  for(int j=0; j<10; ++j) {
// //  fillRect(cx-i2, cy-i2, n, n, ILI9341_RED);
// //  drawFastHLine(15, 50, 30, ILI9341_CYAN);
// //  drawCircle(120, 160, 50, ILI9341_YELLOW);

  // setCursor(0, 100);
  // setTextSize(2);
  // write("fffffff\n");
  //}
}
