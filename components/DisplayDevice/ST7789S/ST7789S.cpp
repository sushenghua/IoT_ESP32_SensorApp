/*
 * ST7789S: ST7789S display driver
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "ST7789S.h"
#include "freertos/task.h"
#include "driver/gpio.h"

// delay definition
#define RESET_DELAY_TIME         120
#ifndef delay
#define delay(x)                 vTaskDelay((x)/portTICK_RATE_MS)
#endif

// GPIO
#define ST7789S_DC_CMD_MODE      0
#define ST7789S_DC_DATA_MODE     1

// used in rotation
#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH  0x04

// SPI pin definition
#define PIN_NUM_MISO 25
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  19
#define PIN_NUM_CS   22

#define PIN_NUM_DC   21
#define PIN_NUM_RST  18
#define PIN_NUM_BCKL 5

#define ST7789S_CHANNEL_CLK_SPEED   10000000
#define ST7789S_CHANNEL_QUEQUE_SIZE 5


uint16_t ST7789S::color565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);
}

ST7789S::ST7789S()
: DisplayGFX(ST7789S_TFTWIDTH, ST7789S_TFTHEIGHT)
{
}

#define ST7789S_SPI_TRANS_MAX 5
static spi_transaction_t _st7789vSpiTrans[ST7789S_SPI_TRANS_MAX];

void ST7789S::_initBus()
{
    // initialize non-SPI GPIOs
    gpio_set_direction((gpio_num_t)PIN_NUM_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)PIN_NUM_RST, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)PIN_NUM_BCKL, GPIO_MODE_OUTPUT);

    // spi bus init
    SpiBus *bus = SpiBus::busForHost(HSPI_HOST);
    bus->init(PIN_NUM_MISO, PIN_NUM_MOSI, PIN_NUM_CLK);
    _spiChannel.setParams(0, PIN_NUM_CS, ST7789S_CHANNEL_QUEQUE_SIZE, ST7789S_CHANNEL_CLK_SPEED);
    _spiChannel.bindTransactionCache(_st7789vSpiTrans, ST7789S_SPI_TRANS_MAX);
    bus->addChannel(_spiChannel);
}

void ST7789S::init()
{
    _initBus();

    reset();
}

void ST7789S::reset()
{
    // turn on led
    gpio_set_level((gpio_num_t)PIN_NUM_BCKL, 1);

    // reset
    _fireResetSignal();

    // init self
    _init();

    test();
}

void ST7789S::turnOn(bool on)
{
    gpio_set_level((gpio_num_t)PIN_NUM_BCKL, on ? 1 : 0);
}

void ST7789S::_fireResetSignal()
{
    gpio_set_level((gpio_num_t)PIN_NUM_RST, 0);
    delay(RESET_DELAY_TIME);
    gpio_set_level((gpio_num_t)PIN_NUM_RST, 1);
    delay(RESET_DELAY_TIME);
}

void ST7789S::writeCommand(uint8_t cmd)
{
    gpio_set_level((gpio_num_t)PIN_NUM_DC, ST7789S_DC_CMD_MODE);
    _spiChannel.tx(cmd);
    gpio_set_level((gpio_num_t)PIN_NUM_DC, ST7789S_DC_DATA_MODE);
}

void ST7789S::writeData(uint8_t data)
{
    gpio_set_level((gpio_num_t)PIN_NUM_DC, ST7789S_DC_DATA_MODE);
    _spiChannel.tx(data);
}

void ST7789S::writeData(const uint8_t *data, uint16_t count)
{
    gpio_set_level((gpio_num_t)PIN_NUM_DC, ST7789S_DC_DATA_MODE);
    _spiChannel.tx(data, count);
}

typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} ST7789SInitCmd;

// DRAM_ATTR static const ST7789SInitCmd st7789vInitCmd[]={
//     {0x11, {0}, 0x80},
//     {0x36, {0x00}, 1},                         // mem data access control
//     {0x3A, {0x05}, 1},                         // interface pixel format, MCU-18bit 06
//     {0x2A, {0x00, 0x00, 0x00, 0xEF}, 4},
//     {0x2B, {0x00, 0x00, 0x00, 0xEF}, 4},
//     {0xB2, {0x0C, 0x0C, 0x00, 0x33, 0x33}, 5}, // porch setting
//     {0xB7, {0x56}, 1},                         // gate control: VGH 14.06V, VGL=-11.38
//     {0xBB, {0x1E}, 1},
//     {0xC0, {0x2C}, 1},
//     {0xC2, {0x01}, 1},
//     {0xC3, {0x13}, 1},
//     {0xC4, {0x20}, 1},
//     {0xC6, {0x0F}, 1},
//     {0xD0, {0xA4, 0xA1}, 2},                   // pwer control: AVDD=6.8V, AVCL=-4.8V, VDS=2.3V
//     {0xE0, {0xD0, 0x03, 0x08, 0x0E, 0x11, 0x2B, 0x3B, 0x44, 0x4C, 0x2B, 0x16, 0x15, 0x1E, 0x21}, 14},
//     {0xE1, {0xD0, 0x03, 0x08, 0x0E, 0x11, 0x2B, 0x3B, 0x54, 0x4C, 0x2B, 0x16, 0x15, 0x1E, 0x21}, 14},
//     {0x51, {0x20}, 1},
//     {0xE7, {0x00}, 1},
//     {0x29, {0}, 0x80},
//     {0, {0}, 0xFF}
// };

DRAM_ATTR static const ST7789SInitCmd st7789vInitCmd[]={
    {0x11, {0}, 0x80},
    // --- display and color format
    {0x36, {0x00}, 1},                         // mem data access control
    {0x3A, {0x05}, 1},                         // interface pixel format, MCU-18bit 06
    {0x2A, {0x00, 0x00, 0x00, 0xEF}, 4},
    {0x2B, {0x00, 0x00, 0x00, 0xEF}, 4},
    // --- frame rate setting
    {0xB2, {0x0C, 0x0C, 0x00, 0x33, 0x33}, 5}, // porch setting
    {0xB7, {0x35}, 1},                         // gate control: VGH 14.06V, VGL=-11.38
    // --- power setting
    {0xBB, {0x2C}, 1},
    {0xC0, {0x2C}, 1},
    {0xC2, {0x01}, 1},
    {0xC3, {0x0B}, 1},
    {0xC4, {0x20}, 1},
    {0xC6, {0x0F}, 1},
    {0xD0, {0xA4, 0xA1}, 2},                   // pwer control: AVDD=6.8V, AVCL=-4.8V, VDS=2.3V
    // --- gamma setting
    {0xE0, {0xD0, 0x06, 0x01, 0x0E, 0x0E, 0x08, 0x32, 0x44, 0x40, 0x08, 0x10, 0x0F, 0x15, 0x19}, 14},
    {0xE1, {0xD0, 0x06, 0x01, 0x0F, 0x0E, 0x09, 0x2F, 0x54, 0x44, 0x0F, 0x1D, 0x1A, 0x16, 0x19}, 14},
    // {0x21, {0},    0},
    // {0x35, {0x00}, 1},
    // {0x44, {0x00, 0x00}, 2},
    // {0xE7, {0x10}, 1},
    {0x29, {0}, 0x80},
    // {0x2C, {0}, 0x00},
    {0, {0}, 0xFF}
};

// DRAM_ATTR static const ST7789SInitCmd st7789vInitCmd[]={
//     {0x11, {0x00}, 0x80},
//     {0x36, {0x00}, 1},                         // mem data access control
//     {0x3A, {0x05}, 1},                         // interface pixel format, MCU-18bit 06
//     {0x21, {0x00}, 0},
//     {0x2A, {0x00, 0x00, 0x00, 0xEF}, 4},
//     {0x2B, {0x00, 0x00, 0x00, 0xEF}, 4},
//     {0xB2, {0x0C, 0x0C, 0x00, 0x33, 0x33}, 5}, // porch setting
//     {0xB7, {0x35}, 1},                         // gate control: VGH 14.06V, VGL=-11.38
//     {0xBB, {0x1F}, 1},
//     {0xC0, {0x2C}, 1},
//     {0xC2, {0x01}, 1},
//     {0xC3, {0x12}, 1},
//     {0xC4, {0x20}, 1},
//     {0xC6, {0x0F}, 1},
//     {0xD0, {0xA4, 0xA1}, 2},                   // pwer control: AVDD=6.8V, AVCL=-4.8V, VDS=2.3V
//     {0xE0, {0xD0, 0x08, 0x11, 0x08, 0x0C, 0x15, 0x39, 0x33, 0x50, 0x36, 0x13, 0x14, 0x29, 0x2D}, 14},
//     {0xE1, {0xD0, 0x08, 0x10, 0x08, 0x06, 0x06, 0x39, 0x44, 0x51, 0x0B, 0x16, 0x14, 0x2F, 0x31}, 14},
//     // {0x51, {0x20}, 1},
//     // {0xE7, {0x10}, 1},
//     {0x29, {0}, 0x80},
//     // {0x2C, {0}, 0x00},
//     {0, {0}, 0xFF}
// };

void ST7789S::_init()
{
    int i = 0;
    // send all the commands and data
    while (st7789vInitCmd[i].databytes != 0xFF) {
        writeCommand(st7789vInitCmd[i].cmd);
        if ((st7789vInitCmd[i].databytes & 0x1F) > 0)
            writeData(st7789vInitCmd[i].data, st7789vInitCmd[i].databytes & 0x1F);
        if (st7789vInitCmd[i].databytes & 0x80) {
            delay(120);
        }
        ++i;
    }
}

void ST7789S::setRotation(uint8_t m)
{
    if (_rotation == m) return;
    _rotation = m;
    switch (_rotation) {
            case DISPLAY_ROTATION_CW_0:
                    m = (MADCTL_MX | MADCTL_BGR);
                    _width  = ST7789S_TFTWIDTH;
                    _height = ST7789S_TFTHEIGHT;
                    break;
            case DISPLAY_ROTATION_CW_90:
                    m = (MADCTL_MV | MADCTL_BGR);
                    _width  = ST7789S_TFTHEIGHT;
                    _height = ST7789S_TFTWIDTH;
                    break;
            case DISPLAY_ROTATION_CW_180:
                    m = (MADCTL_MY | MADCTL_BGR);
                    _width  = ST7789S_TFTWIDTH;
                    _height = ST7789S_TFTHEIGHT;
                    break;
            case DISPLAY_ROTATION_CW_270:
                    m = (MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR);
                    _width  = ST7789S_TFTHEIGHT;
                    _height = ST7789S_TFTWIDTH;
                    break;
    }
    writeCommand(ST7789S_MADCTL);
    _spiChannel.tx(m);
}

void ST7789S::invertDisplay(bool i)
{
    writeCommand(i ? ST7789S_INVON : ST7789S_INVOFF);
}

void ST7789S::scrollTo(uint16_t y)
{
    writeCommand(ST7789S_VSCRSADD);
    _spiChannel.tx16(y);
}

void ST7789S::setViewPort(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint32_t xa = ((uint32_t)x << 16) | (x+w-1);
    uint32_t ya = ((uint32_t)y << 16) | (y+h-1);
    writeCommand(ST7789S_CASET); // Column addr set
    _spiChannel.tx32(xa);
    writeCommand(ST7789S_PASET); // Row addr set
    _spiChannel.tx32(ya);
    writeCommand(ST7789S_RAMWR); // write to RAM
}

void ST7789S::writePixel(uint16_t color)
{
    _spiChannel.tx16(color);
}

#define SPI_MAX_PIXELS_AT_ONCE  128
static uint16_t _ili9431ColorTxBuf[SPI_MAX_PIXELS_AT_ONCE];

void ST7789S::writePixels(uint16_t *colors, uint32_t len)
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

void ST7789S::writeColor(uint16_t color, uint32_t len)
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

uint8_t ST7789S::readcommand8(uint8_t c, uint8_t index)
{
    writeCommand(0xD9);  // woo sekret command?
    _spiChannel.tx(0x10 + index);
    writeCommand(c);

    uint8_t r = 0;
    _spiChannel.rx(&r, 1);
    return r;
}

void ST7789S::drawPixel(int16_t x, int16_t y, uint16_t color)
{
    if((x < 0) ||(x >= _width) || (y < 0) || (y >= _height)) return;
    setViewPort(x,y,1,1);
    writePixel(color);
}

void ST7789S::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
    fillRect(x, y, 1, h, color);
}

void ST7789S::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
    fillRect(x, y, w, 1, color);
}

void ST7789S::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
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

// This code was ported/adapted from https://github.com/PaulStoffregen/ST7789S_t3
// by Marc MERLIN. See examples/pictureEmbed to use this.
void ST7789S::drawBitmap(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pcolors)
{
    int16_t x2, y2; // Lower-right coord
    if(( x             >= _width ) ||      // Off-edge right
         ( y             >= _height) ||      // " top
         ((x2 = (x+w-1)) <  0      ) ||      // " left
         ((y2 = (y+h-1)) <  0)     ) return; // " bottom

    int16_t bx1=0, by1=0, // Clipped top-left within bitmap
                    saveW=w;      // Save original bitmap width value
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

void ST7789S::test()
{
    this->fillScreen(RGB565_RED);
    // setViewPort(0, 40, 240, 240);
    // fillRect(0, 40, 240, 240, RGB565_GREEN);
    //this->scrollTo(100);
    this->setCursor(0, 100);
    this->setTextSize(2);
    this->write("oooooooppppp");
    // for (int i=0; i<100; ++i)
    //     this->writeColor(RGB565_GREEN, 240);
    drawCircle(120, 160, 50, RGB565_WHITE);
//     int         n, i, i2,
//                 cx = _width  / 2,
//                 cy = _height / 2;
//     n     = 240;//_width < _height? _width : _height;
// //  for(i=2; i<n; i+=6) {
// //    i2 = i / 2;
// //    fillRect(cx-i2, cy-i2, i, i, ST7789S_GREEN);
// //  }
//     i2 = n / 2;

// //    for(int j=0; j<10; ++j) {
// //    fillRect(cx-i2, cy-i2, n, n, ST7789S_RED);
// //    drawFastHLine(15, 50, 30, ST7789S_CYAN);
// //    drawCircle(120, 160, 50, ST7789S_YELLOW);

    // setCursor(0, 100);
    // setTextSize(2);
    // write("fffffff\n");
    //}
}
