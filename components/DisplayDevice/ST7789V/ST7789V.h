/*
 * ST7789V: ST7789V display driver
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _ST7789V_H
#define _ST7789V_H

#include "DisplayGFX.h"
#include "Spi.h"

#define ST7789V_TFTWIDTH   240
#define ST7789V_TFTHEIGHT  240

#define ST7789V_NOP        0x00
#define ST7789V_SWRESET    0x01
#define ST7789V_RDDID      0x04
#define ST7789V_RDDST      0x09

#define ST7789V_SLPIN      0x10
#define ST7789V_SLPOUT     0x11
#define ST7789V_PTLON      0x12
#define ST7789V_NORON      0x13

#define ST7789V_RDMODE     0x0A
#define ST7789V_RDMADCTL   0x0B
#define ST7789V_RDPIXFMT   0x0C
#define ST7789V_RDIMGFMT   0x0D
#define ST7789V_RDSELFDIAG 0x0F

#define ST7789V_INVOFF     0x20
#define ST7789V_INVON      0x21
#define ST7789V_GAMMASET   0x26
#define ST7789V_DISPOFF    0x28
#define ST7789V_DISPON     0x29

#define ST7789V_CASET      0x2A
#define ST7789V_PASET      0x2B
#define ST7789V_RAMWR      0x2C
#define ST7789V_RAMRD      0x2E

#define ST7789V_PTLAR      0x30
#define ST7789V_MADCTL     0x36
#define ST7789V_VSCRSADD   0x37
#define ST7789V_PIXFMT     0x3A

#define ST7789V_FRMCTR1    0xB1
#define ST7789V_FRMCTR2    0xB2
#define ST7789V_FRMCTR3    0xB3
#define ST7789V_INVCTR     0xB4
#define ST7789V_DFUNCTR    0xB6

#define ST7789V_PWCTR1     0xC0
#define ST7789V_PWCTR2     0xC1
#define ST7789V_PWCTR3     0xC2
#define ST7789V_PWCTR4     0xC3
#define ST7789V_PWCTR5     0xC4
#define ST7789V_VMCTR1     0xC5
#define ST7789V_VMCTR2     0xC7

#define ST7789V_RDID1      0xDA
#define ST7789V_RDID2      0xDB
#define ST7789V_RDID3      0xDC
#define ST7789V_RDID4      0xDD

#define ST7789V_GMCTRP1    0xE0
#define ST7789V_GMCTRN1    0xE1


class ST7789V : public DisplayGFX
{
public:
    ST7789V();

public:
	// virtual methods
	virtual void init();
	virtual void reset();
	virtual void turnOn(bool on = true);

	virtual void drawPixel(int16_t x, int16_t y, uint16_t color);
	virtual void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
	virtual void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
	virtual	void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

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
	void _init();
	void _fireResetSignal();

	void writeCommand(uint8_t cmd);
	void writeData(uint8_t data);
	void writeData(const uint8_t *data, uint16_t count);

protected:
	SpiChannel  _spiChannel;

public:
	void test();
};

#endif
