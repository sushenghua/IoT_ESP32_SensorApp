/*************************************************** 
  This is a library for the 0.96" 16-bit Color OLED with SSD1331 driver chip

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/684

  These displays use SPI to communicate, 4 or 5 pins are required to  
  interface
  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ****************************************************/

#include "OledSSD1331.h"
#include "AsciiFont5X7.h"

// boundary check decrease the performance
#define ENABLE_BOUNDARY_CHECK

// init
#define OLED_INIT_CMD_BY_CMD   0
#define OLED_INIT_CMD_BUF      1
#define OLED_INIT              OLED_INIT_CMD_BUF

// spi operation timeout
#ifndef SPI_TIMEOUT
#define SPI_TIMEOUT            100000
#endif

// screen dimension
#define SCREEN_WIDTH           96
#define SCREEN_HEIGHT          64

// time delay values
#define SSD1331_DELAYS_HWFILL  3
#define SSD1331_DELAYS_HWLINE  1

// delay definition
#define RESET_DELAY_TIME       5
#define delay(x)               HAL_Delay(x)
//#define delay(x)         {for(int i = 0; i < 6000; ++i);}

// GPIO
#define DC_CMD_MODE            GPIO_PIN_RESET
#define DC_DATA_MODE           GPIO_PIN_SET

// spi instance
SPI_HandleTypeDef *_oledSpiHandle;

// buf
#define OLED_BUF_COUNT 64
uint8_t _oledCmdBufCount;
uint8_t _oledDataBufCount;
uint8_t _oledCmdBuf[OLED_BUF_COUNT];
uint8_t _oledDataBuf[OLED_BUF_COUNT];

OledSSD1331::OledSSD1331(SPI_HandleTypeDef *spiHandle)
: DisplayGFX(SCREEN_WIDTH, SCREEN_HEIGHT)
{
	_oledSpiHandle = spiHandle;
}

// helper functions
inline void selectOled()
{
	HAL_GPIO_WritePin(OledSelect_GPIO_Port, OledSelect_Pin, GPIO_PIN_RESET);
}

inline void deselectOled()
{
	HAL_GPIO_WritePin(OledSelect_GPIO_Port, OledSelect_Pin, GPIO_PIN_SET);
}
	
void OledSSD1331::init()
{
	_spi.setHandle(_oledSpiHandle);
	//_spi.registerCallback();
	reset();
	
	selectOled();

	_init();
}

void OledSSD1331::reset()
{
	HAL_GPIO_WritePin(OledReset_GPIO_Port, OledReset_Pin, GPIO_PIN_SET);
	delay(RESET_DELAY_TIME);
	HAL_GPIO_WritePin(OledReset_GPIO_Port, OledReset_Pin, GPIO_PIN_RESET);
	delay(RESET_DELAY_TIME);
	HAL_GPIO_WritePin(OledReset_GPIO_Port, OledReset_Pin, GPIO_PIN_SET);
	delay(RESET_DELAY_TIME);
}

uint16_t color565(uint8_t r, uint8_t g, uint8_t b)
{
  uint16_t c;
  c = r >> 3;
  c <<= 6;
  c |= g >> 2;
  c <<= 5;
  c |= b >> 3;

  return c;
}

void OledSSD1331::_init()
{
#if OLED_INIT == OLED_INIT_CMD_BY_CMD
	// Initialization Sequence
	writeCommand(SSD1331_CMD_DISPLAYOFF);  	        // 0xAE
	writeCommand(SSD1331_CMD_SETREMAP);             // 0xA0
#if defined SSD1331_COLORORDER_RGB
	writeCommand(0x72);                             // RGB Color
#else
	writeCommand(0x76);                             // BGR Color
#endif
	writeCommand(SSD1331_CMD_STARTLINE);            // 0xA1
	writeCommand(0x0);
	writeCommand(SSD1331_CMD_DISPLAYOFFSET);        // 0xA2
	writeCommand(0x0);
	writeCommand(SSD1331_CMD_NORMALDISPLAY);        // 0xA4
	writeCommand(SSD1331_CMD_SETMULTIPLEX);         // 0xA8
	writeCommand(0x3F);                             // 0x3F 1/64 duty
	writeCommand(SSD1331_CMD_SETMASTER);            // 0xAD
	writeCommand(0x8E);
	writeCommand(SSD1331_CMD_POWERMODE);            // 0xB0
	writeCommand(0x0B);
	writeCommand(SSD1331_CMD_PRECHARGE);            // 0xB1
	writeCommand(0x31);
	writeCommand(SSD1331_CMD_CLOCKDIV);             // 0xB3
	writeCommand(0xF0);                             // 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
	writeCommand(SSD1331_CMD_PRECHARGEA);           // 0x8A
	writeCommand(0x64);
	writeCommand(SSD1331_CMD_PRECHARGEB);           // 0x8B
	writeCommand(0x78);
	writeCommand(SSD1331_CMD_PRECHARGEA);           // 0x8C
	writeCommand(0x64);
	writeCommand(SSD1331_CMD_PRECHARGELEVEL);       // 0xBB
	writeCommand(0x3A);
	writeCommand(SSD1331_CMD_VCOMH);                // 0xBE
	writeCommand(0x3E);
	writeCommand(SSD1331_CMD_MASTERCURRENT);        // 0x87
	writeCommand(0x06);
	writeCommand(SSD1331_CMD_CONTRASTA);            // 0x81
	writeCommand(0x91);
	writeCommand(SSD1331_CMD_CONTRASTB);            // 0x82
	writeCommand(0x50);
	writeCommand(SSD1331_CMD_CONTRASTC);            // 0x83
	writeCommand(0x7D);
	writeCommand(SSD1331_CMD_DISPLAYON);            // turn on oled panel

#elif OLED_INIT == OLED_INIT_CMD_BUF
	// Initialization Sequence
	_oledCmdBufCount = 0;
	_oledCmdBuf[_oledCmdBufCount++] = SSD1331_CMD_DISPLAYOFF;  	        // 0xAE
	_oledCmdBuf[_oledCmdBufCount++] = SSD1331_CMD_SETREMAP;             // 0xA0
#if defined SSD1331_COLORORDER_RGB
	_oledCmdBuf[_oledCmdBufCount++] = 0x72;                             // RGB Color
#else
	_oledCmdBuf[_oledCmdBufCount++] = 0x76;                             // BGR Color
#endif
	_oledCmdBuf[_oledCmdBufCount++] = SSD1331_CMD_STARTLINE;            // 0xA1
	_oledCmdBuf[_oledCmdBufCount++] = 0x0;
	_oledCmdBuf[_oledCmdBufCount++] = SSD1331_CMD_DISPLAYOFFSET;        // 0xA2
	_oledCmdBuf[_oledCmdBufCount++] = 0x0;
	_oledCmdBuf[_oledCmdBufCount++] = SSD1331_CMD_NORMALDISPLAY;        // 0xA4
	_oledCmdBuf[_oledCmdBufCount++] = SSD1331_CMD_SETMULTIPLEX;         // 0xA8
	_oledCmdBuf[_oledCmdBufCount++] = 0x3F;                             // 0x3F 1/64 duty
	_oledCmdBuf[_oledCmdBufCount++] = SSD1331_CMD_SETMASTER;            // 0xAD
	_oledCmdBuf[_oledCmdBufCount++] = 0x8E;
	_oledCmdBuf[_oledCmdBufCount++] = SSD1331_CMD_POWERMODE;            // 0xB0
	_oledCmdBuf[_oledCmdBufCount++] = 0x0B;
	_oledCmdBuf[_oledCmdBufCount++] = SSD1331_CMD_PRECHARGE;            // 0xB1
	_oledCmdBuf[_oledCmdBufCount++] = 0x31;
	_oledCmdBuf[_oledCmdBufCount++] = SSD1331_CMD_CLOCKDIV;             // 0xB3
	_oledCmdBuf[_oledCmdBufCount++] = 0xF0;                             // 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
	_oledCmdBuf[_oledCmdBufCount++] = SSD1331_CMD_PRECHARGEA;           // 0x8A
	_oledCmdBuf[_oledCmdBufCount++] = 0x64;
	_oledCmdBuf[_oledCmdBufCount++] = SSD1331_CMD_PRECHARGEB;           // 0x8B
	_oledCmdBuf[_oledCmdBufCount++] = 0x78;
	_oledCmdBuf[_oledCmdBufCount++] = SSD1331_CMD_PRECHARGEA;           // 0x8C
	_oledCmdBuf[_oledCmdBufCount++] = 0x64;
	_oledCmdBuf[_oledCmdBufCount++] = SSD1331_CMD_PRECHARGELEVEL;       // 0xBB
	_oledCmdBuf[_oledCmdBufCount++] = 0x3A;
	_oledCmdBuf[_oledCmdBufCount++] = SSD1331_CMD_VCOMH;                // 0xBE
	_oledCmdBuf[_oledCmdBufCount++] = 0x3E;
	_oledCmdBuf[_oledCmdBufCount++] = SSD1331_CMD_MASTERCURRENT;        // 0x87
	_oledCmdBuf[_oledCmdBufCount++] = 0x06;
	_oledCmdBuf[_oledCmdBufCount++] = SSD1331_CMD_CONTRASTA;            // 0x81
	_oledCmdBuf[_oledCmdBufCount++] = 0x91;
	_oledCmdBuf[_oledCmdBufCount++] = SSD1331_CMD_CONTRASTB;            // 0x82
	_oledCmdBuf[_oledCmdBufCount++] = 0x50;
	_oledCmdBuf[_oledCmdBufCount++] = SSD1331_CMD_CONTRASTC;            // 0x83
	_oledCmdBuf[_oledCmdBufCount++] = 0x7D;
	_oledCmdBuf[_oledCmdBufCount++] = SSD1331_CMD_DISPLAYON;            // turn on oled panel  

	// set as command mode
	HAL_GPIO_WritePin(OledDataCmd_GPIO_Port, OledDataCmd_Pin, DC_CMD_MODE);

	// send command	
	_spi.tx(_oledCmdBuf, _oledCmdBufCount);
#endif
}

void OledSSD1331::writeCommand(uint8_t cmd)
{
	HAL_GPIO_WritePin(OledDataCmd_GPIO_Port, OledDataCmd_Pin, DC_CMD_MODE);
	_spi.tx(&cmd, 1);
}

void OledSSD1331::writeData(uint8_t data)
{
	HAL_GPIO_WritePin(OledDataCmd_GPIO_Port, OledDataCmd_Pin, DC_DATA_MODE);
	_spi.tx(&data, 1);
}

void OledSSD1331::goTo(int x, int y)
{
#ifdef ENABLE_BOUNDARY_CHECK
  if ((x >= WIDTH) || (y >= HEIGHT)) return;
#endif

	// feed command buf and data buf
	_oledCmdBufCount = 0;
	_oledCmdBuf[_oledCmdBufCount++] = SSD1331_CMD_SETCOLUMN;
	_oledCmdBuf[_oledCmdBufCount++] = x;
	_oledCmdBuf[_oledCmdBufCount++] = WIDTH - 1;
	_oledCmdBuf[_oledCmdBufCount++] = SSD1331_CMD_SETROW;
	_oledCmdBuf[_oledCmdBufCount++] = y;
	_oledCmdBuf[_oledCmdBufCount++] = HEIGHT - 1;

	// set as command mode
	HAL_GPIO_WritePin(OledDataCmd_GPIO_Port, OledDataCmd_Pin, DC_CMD_MODE);

	// send command	
	_spi.tx(_oledCmdBuf, _oledCmdBufCount);
}

void OledSSD1331::goHome(void)
{
  goTo(0, 0);
}

void OledSSD1331::drawRect1(int16_t x, int16_t y, int16_t w, int16_t h,
	                          uint16_t lineColor, uint16_t fillColor, bool fill) 
{
  // check rotation, move rect around if necessary
  switch (_rotation) {
		case 1:
			SWAP_INT16(x, y);
			SWAP_INT16(w, h);
			x = WIDTH - x - 1;
			break;
		case 2:
			x = WIDTH - x - 1;
			y = HEIGHT - y - 1;
			break;
		case 3:
			SWAP_INT16(x, y);
			SWAP_INT16(w, h);
			y = HEIGHT - y - 1;
			break;
  }

#ifdef ENABLE_BOUNDARY_CHECK
  // Bounds check
  if ((x >= SCREEN_WIDTH) || (y >= SCREEN_HEIGHT)) return;

  // Y bounds check
  if (y + h > SCREEN_HEIGHT) {
    h = SCREEN_HEIGHT - y;
  }

  // X bounds check
  if (x + w > SCREEN_WIDTH) {
    w = SCREEN_WIDTH - x;
  }
#endif

	// rectangle outline and fill command buffer
	_oledCmdBufCount = 0;

	// fill command
	if (fill) {
	_oledCmdBuf[_oledCmdBufCount++] = SSD1331_CMD_FILL;
	_oledCmdBuf[_oledCmdBufCount++] = 0x01;
	}

	// draw command
	_oledCmdBuf[_oledCmdBufCount++] = SSD1331_CMD_DRAWRECT;

	// start, end point
	_oledCmdBuf[_oledCmdBufCount++] = x & 0xFF;            // Starting column
	_oledCmdBuf[_oledCmdBufCount++] = y & 0xFF;            // Starting row
	_oledCmdBuf[_oledCmdBufCount++] = (x + w - 1) & 0xFF;  // End column
	_oledCmdBuf[_oledCmdBufCount++] = (y + h - 1) & 0xFF;  // End row

	// outline color
	_oledCmdBuf[_oledCmdBufCount++] = (lineColor >> 11) << 1;
	_oledCmdBuf[_oledCmdBufCount++] = (lineColor >> 5) & 0x3F;
	_oledCmdBuf[_oledCmdBufCount++] = (lineColor << 1) & 0x3F;

	// fill color
	_oledCmdBuf[_oledCmdBufCount++] = (fillColor >> 11) << 1;
	_oledCmdBuf[_oledCmdBufCount++] = (fillColor >> 5) & 0x3F;
	_oledCmdBuf[_oledCmdBufCount++] = (fillColor << 1) & 0x3F;

	// set as command mode
	HAL_GPIO_WritePin(OledDataCmd_GPIO_Port, OledDataCmd_Pin, DC_CMD_MODE);

	// send command	
	_spi.tx(_oledCmdBuf, _oledCmdBufCount);

	// give some delay
	delay(SSD1331_DELAYS_HWFILL);
}

void OledSSD1331::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t fillColor)
{
	drawRect1(x, y, w, h, fillColor, fillColor, true);
}

void OledSSD1331::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {	
  // check rotation, move pixel around if necessary
  switch (_rotation) {
		case 1:
			SWAP_INT16(x0, y0);
			SWAP_INT16(x1, y1);
			x0 = WIDTH - x0 - 1;
			x1 = WIDTH - x1 - 1;
			break;
		case 2:
			x0 = WIDTH - x0 - 1;
			y0 = HEIGHT - y0 - 1;
			x1 = WIDTH - x1 - 1;
			y1 = HEIGHT - y1 - 1;
			break;
		case 3:
			SWAP_INT16(x0, y0);
			SWAP_INT16(x1, y1);
			y0 = HEIGHT - y0 - 1;
			y1 = HEIGHT - y1 - 1;
			break;
  }

#ifdef ENABLE_BOUNDARY_CHECK
  // Boundary check
	if (x0 < 0 || x0 >= SCREEN_WIDTH || x1 < 0 || x1 >= SCREEN_WIDTH ||
		  y0 < 0 || y0 >= SCREEN_HEIGHT || y1 < 0 || y1 >= SCREEN_HEIGHT)
		return;
//	if ((x0 >= SCREEN_WIDTH) && (x1 >= SCREEN_WIDTH)) return;
//  if ((y0 >= SCREEN_HEIGHT) && (y1 >= SCREEN_HEIGHT)) return;

//  if (x0 >= SCREEN_WIDTH) x0 = SCREEN_WIDTH - 1;
//  if (y0 >= SCREEN_HEIGHT) y0 = SCREEN_HEIGHT - 1;
//  if (x1 >= SCREEN_WIDTH) x1 = SCREEN_WIDTH - 1;
//  if (y1 >= SCREEN_HEIGHT) y1 = SCREEN_HEIGHT - 1;
#endif

	// feed command buf
	_oledCmdBufCount = 0;
	_oledCmdBuf[_oledCmdBufCount++] = SSD1331_CMD_DRAWLINE;
	_oledCmdBuf[_oledCmdBufCount++] = x0;
	_oledCmdBuf[_oledCmdBufCount++] = y0;
	_oledCmdBuf[_oledCmdBufCount++] = x1;
	_oledCmdBuf[_oledCmdBufCount++] = y1;

	_oledCmdBuf[_oledCmdBufCount++] = (color >> 11) << 1;
	_oledCmdBuf[_oledCmdBufCount++] = (color >> 5) & 0x3F;
	_oledCmdBuf[_oledCmdBufCount++] = (color << 1) & 0x3F;

	// set as command mode
	HAL_GPIO_WritePin(OledDataCmd_GPIO_Port, OledDataCmd_Pin, DC_CMD_MODE);

	// send command	
	_spi.tx(_oledCmdBuf, _oledCmdBufCount);

	// give short delay
	delay(SSD1331_DELAYS_HWLINE);
}

void OledSSD1331::drawPixel(int16_t x, int16_t y, uint16_t color)
{
#ifdef ENABLE_BOUNDARY_CHECK
	// boundary check
  if (x < 0 || x >= _width || y < 0 || y >= _height) return;
#endif

  // check rotation, move pixel around if necessary
  switch (_rotation) {
		case 1:
			SWAP_INT16(x, y);
			x = WIDTH - x - 1;
			break;
		case 2:
			x = WIDTH - x - 1;
			y = HEIGHT - y - 1;
			break;
		case 3:
			SWAP_INT16(x, y);
			y = HEIGHT - y - 1;
			break;
  }

	// feed command buf and data buf
	_oledCmdBufCount = 0;
	_oledCmdBuf[_oledCmdBufCount++] = SSD1331_CMD_SETCOLUMN;
	_oledCmdBuf[_oledCmdBufCount++] = x;
	_oledCmdBuf[_oledCmdBufCount++] = WIDTH - 1;
	_oledCmdBuf[_oledCmdBufCount++] = SSD1331_CMD_SETROW;
	_oledCmdBuf[_oledCmdBufCount++] = y;
	_oledCmdBuf[_oledCmdBufCount++] = HEIGHT - 1;

	_oledDataBufCount = 0;
	_oledDataBuf[_oledDataBufCount++] = (color >> 8) & 0xFF;
	_oledDataBuf[_oledDataBufCount++] = color & 0xFF;

	// set as command mode
	HAL_GPIO_WritePin(OledDataCmd_GPIO_Port, OledDataCmd_Pin, DC_CMD_MODE);

	// send command
	_spi.tx(_oledCmdBuf, _oledCmdBufCount);

	// set as data mode
	HAL_GPIO_WritePin(OledDataCmd_GPIO_Port, OledDataCmd_Pin, DC_DATA_MODE);

	// send data
	_spi.tx(_oledDataBuf, _oledDataBufCount);
}

void OledSSD1331::updateColor(uint16_t color)
{
	// set as data mode
	HAL_GPIO_WritePin(OledDataCmd_GPIO_Port, OledDataCmd_Pin, DC_DATA_MODE);

	// feed data
	_oledDataBufCount = 0;
	_oledDataBuf[_oledDataBufCount++] = (color >> 8) & 0xFF;
	_oledDataBuf[_oledDataBufCount++] = color & 0xFF;

	// send data
	_spi.tx(_oledDataBuf, _oledDataBufCount);
}


void OledSSD1331::test1()
{
	//int _testOffset = 0;
	//if (_updateFrame) {

		fillScreen(RGB565_BLACK);

//		drawRoundRect(10, 20, 35, 25, 10, OLED_BLUE);
//		drawRoundRect(50, 20, 35, 25, 10, OLED_BLUE);

//		fillRoundRect(18 + _testOffset, 28, 20, 10, 5, OLED_WHITE);
//		fillRoundRect(58 + _testOffset, 28, 20, 10, 5, OLED_WHITE);

//		if (_testOffset != 0) _testOffset = 0;
//		else _testOffset = 3;
		
//		drawChar(30, 30, '1', OLED_BLUE, OLED_BLACK, 1);
//		drawChar(34, 30, '.', OLED_BLUE, OLED_BLACK, 1);
//		drawChar(36, 30, '0'+(cc++%10), OLED_BLUE, OLED_BLACK, 1);

		//int cc = 0;
		const char *str = "56.43";
		setCursor(0, 0);
		setTextColor(RGB565_CYAN, RGB565_BLACK);
		write((const uint8_t*)str, 5);
	//}
}

void OledSSD1331::test2()
{
	fillScreen(RGB565_BLACK);
	//setCursor(0, 0);
	drawPixel(_width/2, _height/2, RGB565_RED);
//	drawPixel(width()/2+1, height()/2, OLED_RED);
//	drawPixel(width()/2, height()/2+1, OLED_RED);
	//drawLine(0, 0, 20, 20, OLED_MAGENTA);
//	drawRect(0, 0, 48, 48, OLED_RED);
//  fillRect(40, 40, 55, 20, OLED_BLUE);
	
	drawChar(30, 30, '1', RGB565_BLUE, RGB565_BLACK, 1);
	drawChar(34, 30, '3', RGB565_BLUE, RGB565_BLACK, 1);

//	int s = 3;
//	for (int i = 0; i < 10; ++i) {

//		fillScreen(OLED_BLACK);

//		drawRoundRect(10, 20, 35, 25, 10, OLED_BLUE);
//		drawRoundRect(50, 20, 35, 25, 10, OLED_BLUE);

//		//drawCircle(47, 55, 5, OLED_BLUE);

//		fillRoundRect(18+s, 28, 20, 10, 5, OLED_WHITE);
//		fillRoundRect(58+s, 28, 20, 10, 5, OLED_WHITE);

//		if (i % 2 == 0) s = 3;
//		else s = 0;
////		delay(400);
//	}


//	for (int16_t x=0; x < _width; x+=6) {
//		drawLine(0, 0, x, _height-1, OLED_RED);
//	}
//	for (int16_t x=0; x < _width; x+=6) {
//		drawLine(_width-1, 0, x, _height-1, OLED_BLUE);
//	}
	
	//drawLine(10, 10, 35, 55, OLED_YELLOW);
	
	
//	uint32_t i,j;
//  goTo(0, 0);
//  
//  for(i=0;i<64;i++)
//  {
//    for(j=0;j<96;j++)
//    {
//      if(i>55){writeData(OLED_WHITE>>8);writeData(OLED_WHITE);}
//      else if(i>47){writeData(OLED_BLUE>>8);writeData(OLED_BLUE);}
//      else if(i>39){writeData(OLED_GREEN>>8);writeData(OLED_GREEN);}
//      else if(i>31){writeData(OLED_CYAN>>8);writeData(OLED_CYAN);}
//      else if(i>23){writeData(OLED_RED>>8);writeData(OLED_RED);}
//      else if(i>15){writeData(OLED_MAGENTA>>8);writeData(OLED_MAGENTA);}
//      else if(i>7){writeData(OLED_YELLOW>>8);writeData(OLED_YELLOW);}
//      else {writeData(OLED_BLACK>>8);writeData(OLED_BLACK);}
//    }
//  }
//	
	
//	goTo(48, 32);
//	writeData(OLED_RED>>8);
//	writeData(OLED_RED);
//	
//	goTo(58, 32);
//	writeData(OLED_RED>>8);
//	writeData(OLED_RED);
	
	
//	//fillRect(10, 10, 55, 40, OLED_BLUE);
//	drawRoundRect(10, 20, 35, 25, 10, OLED_BLUE);
//	drawRoundRect(50, 20, 35, 25, 10, OLED_BLUE);

//	drawCircle(47, 55, 5, OLED_BLUE);
//	
//	fillRoundRect(18, 28, 20, 10, 5, OLED_WHITE);
//	fillRoundRect(58, 28, 20, 10, 5, OLED_WHITE);

}
