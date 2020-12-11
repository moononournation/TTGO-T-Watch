/*
   TTGO T-Watch GFX Plus
   - depends on Arduino_GFX library
   - read current time from RTC
   - draw analog clock face every loop
   - shutdown power after 20 seconds
   - press power button to start again
*/
// auto sleep time (in milliseconds)
#define SLEEP_TIME 60000L
// uncomment below define for using power chip turnoff power instead of light sleep
#define AXP_POWER_OFF

#define TFT_CS 5
#define TFT_DC 27
#define TFT_RST -1
#define TFT_BL 12
#define LED_LEVEL 240

#include <math.h>

#include <Arduino_GFX_Library.h>
Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, 18 /* SCK */, 19 /* MOSI */);
Arduino_ST7789 *tft = new Arduino_ST7789(bus, TFT_RST, 2 /* rotation */, true /* IPS */, 240 /* width */, 240 /* height */, 0 /* col offset 1 */, 80 /* row offset 1 */);

#include <Wire.h>
#include "pcf8563.h"
PCF8563_Class rtc;

#if defined(AXP_POWER_OFF)
#include "axp20x.h"
AXP20X_Class axp;
#else
#include <esp_sleep.h>
#endif

static unsigned char hh, mm, ss;
static unsigned long nextSecond, sleepTime; // next action time
static bool btn_clicked = true;
static char mode = -1;

#include "watch-face.h"
#include "wifi-analyzer.h"

void IRAM_ATTR ISR()
{
  btn_clicked = true;
  log_i("clicked");
}

void setup(void)
{
  log_i("start");

  tft->begin();
  log_i("tft->begin()");

  tft->fillScreen(BACKGROUND);
  log_i("tft->fillScreen(BACKGROUND)");
  pinMode(TFT_BL, OUTPUT);

  // read date and time from RTC
  read_rtc();
  log_i("read date and time from RTC");

#if defined(AXP_POWER_OFF)
  axp.begin();
  axp.setStartupTime(AXP202_STARTUP_TIME_128MS);
  axp.setlongPressTime(0);
  axp.setTimeOutShutdown(true);
  axp.enableChargeing(true);
#endif

  nextSecond = ((millis() / 1000) + 1) * 1000;
  sleepTime = millis() + SLEEP_TIME;

  pinMode(KEY_BUILTIN, INPUT_PULLUP);
  attachInterrupt(KEY_BUILTIN, ISR, FALLING);

  backlight(true);
}

void loop()
{
  unsigned long cur_millis = millis();
  if (cur_millis >= nextSecond)
  {
    nextSecond += 1000;
    ss++; // Advance second
    if (ss == 60)
    {
      ss = 0;
      mm++; // Advance minute
      if (mm > 59)
      {
        mm = 0;
        hh++; // Advance hour
        if (hh > 23)
        {
          hh = 0;
        }
      }
    }
  }

  if (btn_clicked)
  {
    btn_clicked = false;
    sleepTime = millis() + SLEEP_TIME;

    mode = (mode + 1) % 2; // currently 2 modes
    log_i("mode: %d", mode);

    tft->fillScreen(BACKGROUND);
    switch (mode)
    {
    case 0: // watch face
      init_watch_face(tft);
      break;
    case 1: // WiFi Analyzer
      init_wifi_analyzer(tft);
      break;
    }
  }

  switch (mode)
  {
  case 0: // watch face
    update_watch_face(tft, hh, mm, ss, cur_millis);
    break;
  case 1: // WiFi Analyzer
    update_wifi_analyzer(tft);
    break;
  }

  if (millis() > sleepTime)
  {
    // enter sleep
#if (CORE_DEBUG_LEVEL > 0)
    log_i("enter sleep, loopCount: %d", loopCount);
    delay(10);
#endif

    enterSleep();

#if (CORE_DEBUG_LEVEL > 0)
    loopCount = 0;
#endif
  }
}

void backlight(bool enable)
{
  if (enable)
  {
    ledcAttachPin(TFT_BL, 1); // assign TFT_BL pin to channel 1
    ledcSetup(1, 12000, 8);   // 12 kHz PWM, 8-bit resolution
    ledcWrite(1, LED_LEVEL);  // brightness 0 - 255
  }
  else
  {
    ledcDetachPin(TFT_BL);
  }
}

void read_rtc()
{
  Wire.begin();
  rtc.begin();
  RTC_Date rtc_date = rtc.getDateTime();
  hh = rtc_date.hour;
  mm = rtc_date.minute;
  ss = rtc_date.second;
  log_i(": read date and time from RTC: %02d:%02d:%02d", hh, mm, ss);
}

void enterSleep()
{
#if defined(AXP_POWER_OFF)
  axp.shutdown();
#else
  backlight(false);
  tft->displayOff();

  esp_sleep_enable_ext0_wakeup((gpio_num_t)TP_INT, 0);
  esp_light_sleep_start();

  tft->displayOn();

  // read date and time from RTC
  read_rtc();

  nextSecond = ((millis() / 1000) + 1) * 1000;
  sleepTime = millis() + SLEEP_TIME;
#endif
}
