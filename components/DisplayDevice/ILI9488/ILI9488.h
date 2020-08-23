/******************************************************************
  This class is based on the Adafruit ILI9341

  Adafruit invests time and resources providing this open source
  code, please support Adafruit and open-source hardware by
  purchasing products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 *******************************************************************/

#ifndef _ILI9488_H
#define _ILI9488_H

#include "DisplayGFX.h"
#include "Spi.h"

#define ILI9488_TFTWIDTH   320
#define ILI9488_TFTHEIGHT  480

#define ILI9488_NOP        0x00
#define ILI9488_SWRESET    0x01
#define ILI9488_RDDID      0x04
#define ILI9488_RDDST      0x09

#define ILI9488_SLPIN      0x10
#define ILI9488_SLPOUT     0x11
#define ILI9488_PTLON      0x12
#define ILI9488_NORON      0x13

#define ILI9488_RDMODE     0x0A
#define ILI9488_RDMADCTL   0x0B
#define ILI9488_RDPIXFMT   0x0C
#define ILI9488_RDIMGFMT   0x0D
#define ILI9488_RDSELFDIAG 0x0F

#define ILI9488_INVOFF     0x20
#define ILI9488_INVON      0x21
#define ILI9488_GAMMASET   0x26
#define ILI9488_DISPOFF    0x28
#define ILI9488_DISPON     0x29

#define ILI9488_CASET      0x2A
#define ILI9488_PASET      0x2B
#define ILI9488_RAMWR      0x2C
#define ILI9488_RAMRD      0x2E

#define ILI9488_PTLAR      0x30
#define ILI9488_MADCTL     0x36
#define ILI9488_VSCRSADD   0x37
#define ILI9488_PIXFMT     0x3A

#define ILI9488_FRMCTR1    0xB1
#define ILI9488_FRMCTR2    0xB2
#define ILI9488_FRMCTR3    0xB3
#define ILI9488_INVCTR     0xB4
#define ILI9488_DFUNCTR    0xB6

#define ILI9488_PWCTR1     0xC0
#define ILI9488_PWCTR2     0xC1
#define ILI9488_PWCTR3     0xC2
#define ILI9488_PWCTR4     0xC3
#define ILI9488_PWCTR5     0xC4
#define ILI9488_VMCTR1     0xC5
#define ILI9488_VMCTR2     0xC7

#define ILI9488_RDID1      0xDA
#define ILI9488_RDID2      0xDB
#define ILI9488_RDID3      0xDC
#define ILI9488_RDID4      0xDD

#define ILI9488_GMCTRP1    0xE0
#define ILI9488_GMCTRN1    0xE1

//#define ILI9488_PWCTR6     0xFC

class ILI9488 : public DisplayGFX
{
public:
  ILI9488();

public:
  // virtual methods
  virtual void init(int mode = DISPLAY_INIT_ALL);
  virtual void reset();
  virtual bool on() { return _on; }
  virtual void turnOn(bool on = true);
  virtual void setBrightness(uint8_t b);
  virtual void fadeBrightness(uint8_t b, int duration = 500);

  virtual void drawPixel(int16_t x, int16_t y, uint16_t color);
  virtual void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  virtual void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  virtual  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

  virtual void invertDisplay(bool i);

  virtual void setRotation(uint8_t r);
  virtual void setViewPort(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

public:
  void scrollTo(uint16_t y);

  // Transaction API not used by GFX
  void writePixel(uint16_t color);
  void writePixels(uint16_t *colors, uint32_t len);

  void writeColor(uint16_t color, uint32_t len);

  void drawBitmap(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pcolors);

  uint8_t readcommand8(uint8_t reg, uint8_t index = 0);

  uint16_t color565(uint8_t r, uint8_t g, uint8_t b);

protected:
  void _initBus();
  void _initBackLed();
  void _initIli9488WithCmd();
  void _fireResetSignal();
  void _initIli9488();

  void writeCommand(uint8_t cmd);
  void writeData(uint8_t data);
  void writeData(const uint8_t *data, uint16_t count);

protected:
  bool        _on;
  uint32_t    _backLedDuty;
  SpiChannel  _spiChannel;

public:
  void test();
};

#endif
