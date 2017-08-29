/*
This is the core graphics library for all our displays, providing a common
set of graphics primitives (points, lines, circles, etc.).  It needs to be
paired with a hardware-specific library for each display device we carry
(to handle the lower-level functions).

Adafruit invests time and resources providing this open source code, please
support Adafruit & open-source hardware by purchasing products from Adafruit!

Copyright (c) 2013 Adafruit Industries.  All rights reserved.
Modified 2017 Shenghua, Su

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef _OLED_GFX_H
#define _OLED_GFX_H

#include "GfxFont.h"

#ifndef NULL
#define NULL 0
#endif

// Color definitions
#define RGB565_BLACK         0x0000      /*   0,   0,   0 */
#define RGB565_NAVY          0x000F      /*   0,   0, 128 */
#define RGB565_DARKGREEN     0x03E0      /*   0, 128,   0 */
#define RGB565_DARKCYAN      0x03EF      /*   0, 128, 128 */
#define RGB565_MAROON        0x7800      /* 128,   0,   0 */
#define RGB565_PURPLE        0x780F      /* 128,   0, 128 */
#define RGB565_OLIVE         0x7BE0      /* 128, 128,   0 */
#define RGB565_LIGHTGREY     0xC618      /* 192, 192, 192 */
#define RGB565_DARKGREY      0x7BEF      /* 128, 128, 128 */
#define RGB565_BLUE          0x001F      /*   0,   0, 255 */
#define RGB565_GREEN         0x07E0      /*   0, 255,   0 */
#define RGB565_CYAN          0x07FF      /*   0, 255, 255 */
#define RGB565_RED           0xF800      /* 255,   0,   0 */
#define RGB565_MAGENTA       0xF81F      /* 255,   0, 255 */
#define RGB565_YELLOW        0xFFE0      /* 255, 255,   0 */
#define RGB565_WHITE         0xFFFF      /* 255, 255, 255 */
#define RGB565_ORANGE        0xFD20      /* 255, 165,   0 */
#define RGB565_GREENYELLOW   0xAFE5      /* 173, 255,  47 */
#define RGB565_PINK          0xF81F

#define RGB565_WEAKWHITE     0xCE79

// rotation definitions
#define DISPLAY_ROTATION_CW_0    0
#define DISPLAY_ROTATION_CW_90   1
#define DISPLAY_ROTATION_CW_180  2
#define DISPLAY_ROTATION_CW_270  3

/////////////////////////////////////////////////////////////////////////////////////////
// DisplayGFX class
/////////////////////////////////////////////////////////////////////////////////////////
class DisplayGFX
{
public:
  DisplayGFX(int16_t w, int16_t h);

public:
  // all subclass should implement these common method
  virtual void init() = 0;
  virtual void reset() = 0;
  virtual void turnOn(bool on = true) = 0;
  virtual void setBrightness(uint8_t b) = 0;
  virtual void fadeBrightness(uint8_t b, int duration = 500) = 0;

  // device-specific method
  virtual void drawPixel(int16_t x, int16_t y, uint16_t color) = 0;

  // line, shape draw and fill methods
  virtual void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
  virtual void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  virtual void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  virtual void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  virtual void fillScreen(uint16_t color);

  // byte render method
  virtual uint32_t write(const uint8_t *buf, uint32_t size);
  virtual uint32_t write(uint8_t);

  // display control
  virtual void invertDisplay(bool i);

  virtual uint8_t rotation() { return _rotation; }
  virtual void setRotation(uint8_t r);
  virtual void setViewPort(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {};

public:
	uint32_t write(const char *str);
	uint32_t write(const char *buf, uint32_t size);
	uint32_t writeln();
	uint32_t write(char c);
	uint32_t write(long n, int base = 10);
	uint32_t write(unsigned long n, int base = 10);
	uint32_t write(float f, int digits);

public:
  void drawCircle(int16_t x, int16_t y, int16_t r, uint16_t color);
  void drawCircleHelper(int16_t x, int16_t y, int16_t r, uint8_t corner, uint16_t color);
  void fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color);
  void fillCircleHelper(int16_t x, int16_t y, int16_t r, uint8_t corner, int16_t delta, uint16_t color);

  void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
  void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);

  void drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);
  void fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);

  void drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color);
  void drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg);
  void drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h, uint16_t color);
  void drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg);
  void drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color);

  void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size);

  void setCursor(int16_t x, int16_t y);
  void setTextColor(uint16_t c);
  void setTextColor(uint16_t c, uint16_t bg);
  void setTextSize(uint8_t s);
  void setTextWrap(bool w);
  void cp437(bool x = true);
  void setFont(const GFXfont *f = NULL);
  void getTextBounds(char *string, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h);
  //void getTextBounds(const __FlashStringHelper *s, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h);

 int16_t height(void) const { return _height; }
 int16_t width(void) const { return _width; }
//  uint8_t rotation(void) const;

  // get current cursor position (get rotation safe maximum values, using: width() for x, height() for y)
  int16_t getCursorX(void) const;
  int16_t getCursorY(void) const;

protected:
  const int16_t   WIDTH, HEIGHT;             // This is the 'raw' display w/h - never changes
  uint8_t        _brightness;                // screen (lcd back led) brightness
  int16_t        _width, _height;            // Display w/h as modified by current rotation
  int16_t        _cursorX, _cursorY;
  uint16_t       _textColor, _textBgColor;
  uint8_t        _textSize, _rotation;
  bool           _wrapTextAtRightEdge;       // If set, 'wrap' text at right edge of display
  bool           _enableCodePage437Charset;  // If set, use correct CP437 charset (default is off)
  GFXfont       *_gfxFont;
};

/////////////////////////////////////////////////////////////////////////////////////////
// OledButton
/////////////////////////////////////////////////////////////////////////////////////////
class OledButton
{
public:
  OledButton(void);

  // "Classic" initButton() uses center & size
  void initButton(DisplayGFX *gfx, int16_t x, int16_t y,
                  uint16_t w, uint16_t h, uint16_t outline, uint16_t fill,
                  uint16_t textcolor, char *label, uint8_t textsize);
  // New/alt initButton() uses upper-left corner & size
  void initButtonUL(DisplayGFX *gfx, int16_t x1, int16_t y1,
                    uint16_t w, uint16_t h, uint16_t outline, uint16_t fill,
                    uint16_t textcolor, char *label, uint8_t textsize);

  void drawButton(bool inverted = false);

  bool contains(int16_t x, int16_t y);

  void press(bool p);
  bool isPressed();
  bool justPressed();
  bool justReleased();

 private:
  DisplayGFX   *_gfx;
  int16_t       _x1, _y1; // Coordinates of top-left corner
  uint16_t      _w, _h;
  uint8_t       _textsize;
  uint16_t      _outlinecolor, _fillcolor, _textcolor;
  char          _label[10];

  bool          _currentState, _lastState;
};

/////////////////////////////////////////////////////////////////////////////////////////
// OledCanvas8Bit
/////////////////////////////////////////////////////////////////////////////////////////
class OledCanvas8Bit : public DisplayGFX
{
public:
  OledCanvas8Bit(uint16_t w, uint16_t h);
  ~OledCanvas8Bit(void);

  virtual void drawPixel(int16_t x, int16_t y, uint16_t color);
  virtual void fillScreen(uint16_t color);

  uint8_t *getBuffer(void);

private:
  uint8_t *buffer;
};

/////////////////////////////////////////////////////////////////////////////////////////
// OledCanvas16Bit
/////////////////////////////////////////////////////////////////////////////////////////
class OledCanvas16Bit : public DisplayGFX
{
public:
  OledCanvas16Bit(uint16_t w, uint16_t h);
  ~OledCanvas16Bit(void);

  virtual void drawPixel(int16_t x, int16_t y, uint16_t color);
  virtual void fillScreen(uint16_t color);

  uint16_t *getBuffer(void);

private:
  uint16_t *buffer;
};

#endif // _OLED_GFX_H
