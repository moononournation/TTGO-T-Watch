/*
   TTGO T-Watch GFX example
   - depends on Arduino_GFX library
   - read current time from RTC
   - draw analog clock face every loop
   - shutdown power after 20 seconds
   - press power button to start again
*/
// auto sleep time (in milliseconds)
#define SLEEP_TIME 20000L
// uncomment below define for using power chip turnoff power instead of light sleep
#define AXP_POWER_OFF

#define BACKGROUND BLACK
#define MARK_COLOR WHITE
#define SUBMARK_COLOR DARKGREY // LIGHTGREY
#define HOUR_COLOR WHITE
#define MINUTE_COLOR BLUE // LIGHTGREY
#define SECOND_COLOR RED
#define HOUR_LEN 50
#define MINUTE_LEN 90
#define SECOND_LEN 100

#define SIXTIETH 0.016666667
#define TWELFTH 0.08333333
#define SIXTIETH_RADIAN 0.10471976
#define TWELFTH_RADIAN 0.52359878
#define RIGHT_ANGLE_RADIAN 1.5707963
#define CENTER 120

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

static float sdeg, mdeg, hdeg;
static uint8_t osx = CENTER, osy = CENTER, omx = CENTER, omy = CENTER, ohx = CENTER, ohy = CENTER; // Saved H, M, S x & y coords
static uint8_t nsx, nsy, nmx, nmy, nhx, nhy;                                                       // H, M, S x & y coords
static uint8_t xMin, yMin, xMax, yMax;                                                             // redraw range
static uint8_t hh, mm, ss;
static unsigned long nextSecond, sleepTime; // next action time
static bool ledTurnedOn = false;
static uint8_t tmp;
#if (CORE_DEBUG_LEVEL > 0)
static uint16_t loopCount = 0;
#endif

static uint8_t cached_points[(HOUR_LEN + 1 + MINUTE_LEN + 1 + SECOND_LEN + 1) * 2];
static int cached_points_idx = 0;
static uint8_t *last_cached_point;

void setup(void)
{
  log_i("start");

  tft->begin();
  log_i("tft->begin()");

  tft->fillScreen(BACKGROUND);
  log_i("tft->fillScreen(BACKGROUND)");
  pinMode(TFT_BL, OUTPUT);

  // Draw 60 clock marks
  //draw_round_clock_mark(104, 120, 112, 120, 114, 114);
  draw_square_clock_mark(102, 120, 108, 120, 114, 120);
  log_i("Draw 60 clock marks");

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

  // Pre-compute hand degrees, x & y coords for a fast screen update
  sdeg = SIXTIETH_RADIAN * ((0.001 * (cur_millis % 1000)) + ss); // 0-59 (includes millis)
  nsx = cos(sdeg - RIGHT_ANGLE_RADIAN) * SECOND_LEN + CENTER;
  nsy = sin(sdeg - RIGHT_ANGLE_RADIAN) * SECOND_LEN + CENTER;
  if ((nsx != osx) || (nsy != osy))
  {
    mdeg = (SIXTIETH * sdeg) + (SIXTIETH_RADIAN * mm); // 0-59 (includes seconds)
    hdeg = (TWELFTH * mdeg) + (TWELFTH_RADIAN * hh);   // 0-11 (includes minutes)
    mdeg -= RIGHT_ANGLE_RADIAN;
    hdeg -= RIGHT_ANGLE_RADIAN;
    nmx = cos(mdeg) * MINUTE_LEN + CENTER;
    nmy = sin(mdeg) * MINUTE_LEN + CENTER;
    nhx = cos(hdeg) * HOUR_LEN + CENTER;
    nhy = sin(hdeg) * HOUR_LEN + CENTER;

    // redraw hands
    redraw_hands_cached_draw_and_erase();

    ohx = nhx;
    ohy = nhy;
    omx = nmx;
    omy = nmy;
    osx = nsx;
    osy = nsy;

    if (!ledTurnedOn)
    {
      backlight(true);
    }

#if (CORE_DEBUG_LEVEL > 0)
    loopCount++;
#endif
  }

  if (cur_millis > sleepTime)
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
  delay(1);
}

void backlight(bool enable)
{
  if (enable)
  {
    ledcAttachPin(TFT_BL, 1); // assign TFT_BL pin to channel 1
    ledcSetup(1, 12000, 8);   // 12 kHz PWM, 8-bit resolution
    ledcWrite(1, LED_LEVEL);  // brightness 0 - 255
    ledTurnedOn = true;
  }
  else
  {
    ledcDetachPin(TFT_BL);
    ledTurnedOn = false;
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
  log_i(": read date and time from RTC: %02d:%02d:%02d", hh, mm , ss);
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

void draw_round_clock_mark(uint8_t innerR1, uint8_t outerR1, uint8_t innerR2, uint8_t outerR2, uint8_t innerR3, uint8_t outerR3)
{
  float x, y;
  uint8_t x0, x1, y0, y1, innerR, outerR;
  uint16_t c;

  for (int i = 0; i < 60; i++)
  {
    if ((i % 15) == 0)
    {
      innerR = innerR1;
      outerR = outerR1;
      c = MARK_COLOR;
    }
    else if ((i % 5) == 0)
    {
      innerR = innerR2;
      outerR = outerR2;
      c = MARK_COLOR;
    }
    else
    {
      innerR = innerR3;
      outerR = outerR3;
      c = SUBMARK_COLOR;
    }

    mdeg = (SIXTIETH_RADIAN * i) - RIGHT_ANGLE_RADIAN;
    x = cos(mdeg);
    y = sin(mdeg);
    x0 = x * outerR + CENTER;
    y0 = y * outerR + CENTER;
    x1 = x * innerR + CENTER;
    y1 = y * innerR + CENTER;

    tft->drawLine(x0, y0, x1, y1, c);
    log_d(" i: %d, x0: %d, y0: %d, x1: %d, y1: %d", i, x0, y0, x1, y1);
  }
}

void draw_square_clock_mark(uint8_t innerR1, uint8_t outerR1, uint8_t innerR2, uint8_t outerR2, uint8_t innerR3, uint8_t outerR3)
{
  float x, y;
  uint8_t x0, x1, y0, y1, innerR, outerR;
  uint16_t c;

  for (int i = 0; i < 60; i++)
  {
    if ((i % 15) == 0)
    {
      innerR = innerR1;
      outerR = outerR1;
      c = MARK_COLOR;
    }
    else if ((i % 5) == 0)
    {
      innerR = innerR2;
      outerR = outerR2;
      c = MARK_COLOR;
    }
    else
    {
      innerR = innerR3;
      outerR = outerR3;
      c = SUBMARK_COLOR;
    }

    if ((i >= 53) || (i < 8))
    {
      x = tan(SIXTIETH_RADIAN * i);
      x0 = CENTER + (x * outerR);
      y0 = CENTER + (1 - outerR);
      x1 = CENTER + (x * innerR);
      y1 = CENTER + (1 - innerR);
    }
    else if (i < 23)
    {
      y = tan((SIXTIETH_RADIAN * i) - RIGHT_ANGLE_RADIAN);
      x0 = CENTER + (outerR);
      y0 = CENTER + (y * outerR);
      x1 = CENTER + (innerR);
      y1 = CENTER + (y * innerR);
    }
    else if (i < 38)
    {
      x = tan(SIXTIETH_RADIAN * i);
      x0 = CENTER - (x * outerR);
      y0 = CENTER + (outerR);
      x1 = CENTER - (x * innerR);
      y1 = CENTER + (innerR);
    }
    else if (i < 53)
    {
      y = tan((SIXTIETH_RADIAN * i) - RIGHT_ANGLE_RADIAN);
      x0 = CENTER + (1 - outerR);
      y0 = CENTER - (y * outerR);
      x1 = CENTER + (1 - innerR);
      y1 = CENTER - (y * innerR);
    }
    tft->drawLine(x0, y0, x1, y1, c);
    log_d(" i: %d, x0: %d, y0: %d, x1: %d, y1: %d", i, x0, y0, x1, y1);
  }
}

void redraw_hands_cached_draw_and_erase()
{
  tft->startWrite();
  draw_and_erase_cached_line(CENTER, CENTER, nsx, nsy, SECOND_COLOR, cached_points, SECOND_LEN + 1, false, false);
  draw_and_erase_cached_line(CENTER, CENTER, nhx, nhy, HOUR_COLOR, cached_points + ((SECOND_LEN + 1) * 2), HOUR_LEN + 1, true, false);
  draw_and_erase_cached_line(CENTER, CENTER, nmx, nmy, MINUTE_COLOR, cached_points + ((SECOND_LEN + 1 + HOUR_LEN + 1) * 2), MINUTE_LEN + 1, true, true);
  tft->endWrite();
}

void draw_and_erase_cached_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color, uint8_t *cache, uint16_t cache_len, bool cross_check_second, bool cross_check_hour)
{
  bool steep = _diff(y1, y0) > _diff(x1, x0);
  if (steep)
  {
    _swap_uint8_t(x0, y0);
    _swap_uint8_t(x1, y1);
  }

  uint8_t dx, dy;
  dx = _diff(x1, x0);
  dy = _diff(y1, y0);

  uint8_t err = dx / 2;
  int8_t xstep = (x0 < x1) ? 1 : -1;
  int8_t ystep = (y0 < y1) ? 1 : -1;
  x1 += xstep;
  uint8_t x, y, ox, oy;
  for (uint16_t i = 0; i <= dx; i++)
  {
    if (steep)
    {
      x = y0;
      y = x0;
    }
    else
    {
      x = x0;
      y = y0;
    }
    ox = *(cache + (i * 2));
    oy = *(cache + (i * 2) + 1);
    if ((x == ox) && (y == oy))
    {
      if (cross_check_second || cross_check_hour)
      {
        write_cache_pixel(x, y, color, cross_check_second, cross_check_hour);
      }
    }
    else
    {
      write_cache_pixel(x, y, color, cross_check_second, cross_check_hour);
      if ((ox > 0) || (oy > 0))
      {
        write_cache_pixel(ox, oy, BACKGROUND, cross_check_second, cross_check_hour);
      }
      *(cache + (i * 2)) = x;
      *(cache + (i * 2) + 1) = y;
    }
    if (err < dy)
    {
      y0 += ystep;
      err += dx;
    }
    err -= dy;
    x0 += xstep;
  }
  for (uint16_t i = dx + 1; i < cache_len; i++)
  {
    ox = *(cache + (i * 2));
    oy = *(cache + (i * 2) + 1);
    if ((ox > 0) || (oy > 0))
    {
      write_cache_pixel(ox, oy, BACKGROUND, cross_check_second, cross_check_hour);
    }
    *(cache + (i * 2)) = 0;
    *(cache + (i * 2) + 1) = 0;
  }
}

void write_cache_pixel(uint8_t x, uint8_t y, uint16_t color, bool cross_check_second, bool cross_check_hour)
{
  uint8_t *cache = cached_points;
  if (cross_check_second)
  {
    for (int i = 0; i <= SECOND_LEN; i++)
    {
      if ((x == *(cache++)) && (y == *(cache)))
      {
        return;
      }
      cache++;
    }
  }
  if (cross_check_hour)
  {
    cache = cached_points + ((SECOND_LEN + 1) * 2);
    for (int i = 0; i <= HOUR_LEN; i++)
    {
      if ((x == *(cache++)) && (y == *(cache)))
      {
        return;
      }
      cache++;
    }
  }
  log_v("writePixel(%d, %d, %d)", x, y, color);
  tft->writePixel(x, y, color);
}
