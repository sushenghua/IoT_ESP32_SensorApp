/*
 * ST7789S: ST7789S display driver
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _ST7789S_H
#define _ST7789S_H

#include "DisplayGFX.h"
#include "Spi.h"

#define ST7789S_TFTWIDTH   240
#define ST7789S_TFTHEIGHT  240

#define ST7789S_NOP        0x00
#define ST7789S_SWRESET    0x01
#define ST7789S_RDDID      0x04
#define ST7789S_RDDST      0x09

#define ST7789S_SLPIN      0x10
#define ST7789S_SLPOUT     0x11
#define ST7789S_PTLON      0x12
#define ST7789S_NORON      0x13

#define ST7789S_RDMODE     0x0A
#define ST7789S_RDMADCTL   0x0B
#define ST7789S_RDPIXFMT   0x0C
#define ST7789S_RDIMGFMT   0x0D
#define ST7789S_RDSELFDIAG 0x0F

#define ST7789S_INVOFF     0x20
#define ST7789S_INVON      0x21
#define ST7789S_GAMMASET   0x26
#define ST7789S_DISPOFF    0x28
#define ST7789S_DISPON     0x29

#define ST7789S_CASET      0x2A
#define ST7789S_PASET      0x2B
#define ST7789S_RAMWR      0x2C
#define ST7789S_RAMRD      0x2E

#define ST7789S_PTLAR      0x30
#define ST7789S_MADCTL     0x36
#define ST7789S_VSCRSADD   0x37
#define ST7789S_PIXFMT     0x3A

#define ST7789S_FRMCTR1    0xB1
#define ST7789S_FRMCTR2    0xB2
#define ST7789S_FRMCTR3    0xB3
#define ST7789S_INVCTR     0xB4
#define ST7789S_DFUNCTR    0xB6

#define ST7789S_PWCTR1     0xC0
#define ST7789S_PWCTR2     0xC1
#define ST7789S_PWCTR3     0xC2
#define ST7789S_PWCTR4     0xC3
#define ST7789S_PWCTR5     0xC4
#define ST7789S_VMCTR1     0xC5
#define ST7789S_VMCTR2     0xC7

#define ST7789S_RDID1      0xDA
#define ST7789S_RDID2      0xDB
#define ST7789S_RDID3      0xDC
#define ST7789S_RDID4      0xDD

#define ST7789S_GMCTRP1    0xE0
#define ST7789S_GMCTRN1    0xE1


class ST7789S : public DisplayGFX
{
public:
    ST7789S();

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
