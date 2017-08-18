/*
 * TouchPad: Wrap the esp touch_pad interface
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _TOUCH_PAD_H
#define _TOUCH_PAD_H

#include <string.h>
#include "driver/touch_pad.h"

// --- enum type touch_pad_t
// TOUCH_PAD_NUM0 = 0, /*!< Touch pad channel 0 is GPIO4 */
// TOUCH_PAD_NUM1,     /*!< Touch pad channel 0 is GPIO0 */
// TOUCH_PAD_NUM2,     /*!< Touch pad channel 0 is GPIO2 */
// TOUCH_PAD_NUM3,     /*!< Touch pad channel 0 is GPIO15 */
// TOUCH_PAD_NUM4,     /*!< Touch pad channel 0 is GPIO13 */
// TOUCH_PAD_NUM5,     /*!< Touch pad channel 0 is GPIO12 */
// TOUCH_PAD_NUM6,     /*!< Touch pad channel 0 is GPIO14 */
// TOUCH_PAD_NUM7,     /*!< Touch pad channel 0 is GPIO27*/
// TOUCH_PAD_NUM8,     /*!< Touch pad channel 0 is GPIO33*/
// TOUCH_PAD_NUM9,     /*!< Touch pad channel 0 is GPIO32*/

class TouchPad
{
public:
  // constructor
  TouchPad(touch_pad_t padNum);

  // init and deinit
  void init();
  void deinit();

  // communication
  uint16_t readValue();
  bool     touched();

protected:
  const touch_pad_t  _padNum;
  bool               _inited;
  uint16_t           _value;
};

#endif // _TOUCH_PAD_H
