/*
 * InputMonitor: touch pad and button press monitor
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "InputMonitor.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "System.h"
#include "AppLog.h"

#define PWR_BUTTON_PIN             25
#define USER_BUTTON_PIN            35
#define PWR_INT_PIN                34
#define GPIO_INPUT_PIN_SEL         (((uint64_t)1 << PWR_BUTTON_PIN) | ((uint64_t)1 << USER_BUTTON_PIN) | ((uint64_t)1 << PWR_INT_PIN))
#define ESP_INTR_FLAG_DEFAULT      0

/////////////////////////////////////////////////////////////////////////////////////////
// Interrupt service routine handler, gpio check task
/////////////////////////////////////////////////////////////////////////////////////////
#define GPIO_IT_QUEQUE_LENGTH      20

static xQueueHandle _gpioEventQueue = NULL;

static void IRAM_ATTR gpioIsrHandler(void* arg)
{
  uint32_t gpioNum = (uint32_t) arg;
  xQueueSendFromISR(_gpioEventQueue, &gpioNum, NULL);
}

#define GPIO_CHECK_TASK_PRIORITY          100
#define GPIO_CHECK_TASK_DELAY_UNIT        10

#define PWR_TOGGLE_OFF_LOW_COUNT          200  // 2 second
#define PWR_TOGGLE_DISPLAY_LOW_COUNT      50   // 0.5 second

#define USER_TOGGLE_DISPLAY_LOW_COUNT      50   // 0.5 second
#define USER_TOGGLE_WIFI_ON_LOW_COUNT     200  // 2 second
#define USER_TOGGLE_WIFI_MODE_LOW_COUNT   400  // 4 sencod

int      _pwrGpioLvl = 1;  // pull up externally
int      _usrGpioLvl = 1;

uint16_t _pwrLowDurationCount = 0;
uint16_t _usrLowDurationCount = 0;

static void gpio_check_task(void* args)
{
  uint32_t ioNum;
  int lvl;
  while (true) {
    if(xQueueReceive(_gpioEventQueue, &ioNum, GPIO_CHECK_TASK_DELAY_UNIT)) {
      lvl = gpio_get_level((gpio_num_t)ioNum);
      switch(ioNum) {

        case PWR_BUTTON_PIN:
          if (_pwrGpioLvl != lvl) {
            _pwrGpioLvl = lvl;
            if (lvl == 1) { // released
              // APP_LOGC("[BTN]", "pwr released, duration cnt: %d, time: %.2f",
              //          _pwrLowDurationCount, GPIO_CHECK_TASK_DELAY_UNIT*_pwrLowDurationCount/1000.0f);
              if (_pwrLowDurationCount < PWR_TOGGLE_DISPLAY_LOW_COUNT) {
                System::instance()->toggleDisplay();
              }
            }
            else {          // pressed
              // APP_LOGC("[BTN]", "pwr pressed");
              _pwrLowDurationCount = 0;
            }
          }
          break;

        case USER_BUTTON_PIN:
          if (_usrGpioLvl != lvl) {
            _usrGpioLvl = lvl;
            if (lvl == 1) { // released
              // APP_LOGC("[BTN]", "usr released, duration cnt: %d, time: %.2f",
              //          _usrLowDurationCount, GPIO_CHECK_TASK_DELAY_UNIT*_usrLowDurationCount/1000.0f);
              if (_usrLowDurationCount > 0 &&
                  _usrLowDurationCount < USER_TOGGLE_DISPLAY_LOW_COUNT) {
                System::instance()->toggleDisplay();
              }
              else if (_usrLowDurationCount > USER_TOGGLE_WIFI_ON_LOW_COUNT &&
                       _usrLowDurationCount < USER_TOGGLE_WIFI_MODE_LOW_COUNT) {
                System::instance()->toggleWifi();
              }
            }
            else {          // pressed
              // APP_LOGC("[BTN]", "usr pressed");
              _usrLowDurationCount = 0;
            }
          }
          break;

        case PWR_INT_PIN:
          // APP_LOGC("[PWR_INT]", "power int");
          System::instance()->markPowerEvent();
          break;

        default:
          break;
      }
    }
    else {
      if (_usrGpioLvl == 0) {
        ++_usrLowDurationCount;
        if (_usrLowDurationCount >= USER_TOGGLE_WIFI_MODE_LOW_COUNT)
          System::instance()->toggleDeployMode();
      }
      if (_pwrGpioLvl == 0) {
      	++_pwrLowDurationCount;
      	if (_pwrLowDurationCount >= PWR_TOGGLE_OFF_LOW_COUNT)
          System::instance()->powerOff();
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// InputMonitor class
/////////////////////////////////////////////////////////////////////////////////////////
static InputMonitor _inputMonitor;

InputMonitor * InputMonitor::instance()
{
  return &_inputMonitor;
}

InputMonitor::InputMonitor()
{}

void InputMonitor::init()
{
  if (_inited) return;

  _gpioConfig.intr_type = GPIO_INTR_ANYEDGE;       // interrupt of rising edge
  
  _gpioConfig.pin_bit_mask = GPIO_INPUT_PIN_SEL;   // bit mask of the pins, use GPIO4/5 here
  
  _gpioConfig.mode = GPIO_MODE_INPUT;              // set as input mode    
  
  _gpioConfig.pull_up_en = GPIO_PULLUP_DISABLE;    // pull-up externally

  gpio_config(&_gpioConfig);

  gpio_set_intr_type((gpio_num_t)PWR_INT_PIN, GPIO_INTR_NEGEDGE); // power int falling edge

  gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT); //install gpio isr service

  // hook isr handler for specific PWR and USER pin
  gpio_isr_handler_add((gpio_num_t)PWR_BUTTON_PIN, gpioIsrHandler, (void*) PWR_BUTTON_PIN);
  gpio_isr_handler_add((gpio_num_t)USER_BUTTON_PIN, gpioIsrHandler, (void*) USER_BUTTON_PIN);
  gpio_isr_handler_add((gpio_num_t)PWR_INT_PIN, gpioIsrHandler, (void*) PWR_INT_PIN);
  
  //create a queue to handle gpio event from isr
  _gpioEventQueue = xQueueCreate(GPIO_IT_QUEQUE_LENGTH, sizeof(uint32_t));

  xTaskCreate(gpio_check_task, "gpio_check_task", 2048, NULL, GPIO_CHECK_TASK_PRIORITY, NULL);

  _inited = true;
}

void InputMonitor::deinit()
{
  if (!_inited) return;
  gpio_isr_handler_remove((gpio_num_t)PWR_BUTTON_PIN);
  gpio_isr_handler_remove((gpio_num_t)USER_BUTTON_PIN);
  gpio_isr_handler_remove((gpio_num_t)PWR_INT_PIN);
  gpio_uninstall_isr_service();
  vQueueDelete(_gpioEventQueue);
  _inited = false;
}
