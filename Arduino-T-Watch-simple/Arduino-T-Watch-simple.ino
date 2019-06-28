/*
   TTGO T-Watch simple example (revise from TFT_eSPI TFT_Clock example)
   - read current time from RTC
   - draw analog clock face every loop
   - shutdown power after 60 seconds
   - press power button to restart
*/
#define SLEEP_TIME      (60 * 1000) /* auto shutdown time (in seconds) */
#define BACKGROUND      TFT_BLACK
#define MARK_COLOR      TFT_WHITE
#define HOUR_COLOR      TFT_WHITE
#define MINUTE_COLOR    TFT_LIGHTGREY
#define SECOND_COLOR    TFT_RED
#define SIXTIETH        0.016666667
#define SIXTIETH_RADIAN 0.10471976
#define TWELFTH_RADIAN  0.52359878
#define RIGHT_ANGLE     1.5707963
#define CENTER          120

// Uncomment to enable printing out nice debug messages.
//#define DEBUG_MODE

#ifdef DEBUG_MODE
// Define where debug output will be printed.
#define DEBUG_PRINTER Serial
#define DEBUG_BEGIN(...) { DEBUG_PRINTER.begin(__VA_ARGS__); }
#define DEBUG_PRINTM(...) { DEBUG_PRINTER.print(millis()); DEBUG_PRINTER.print(__VA_ARGS__); }
#define DEBUG_PRINT(...) { DEBUG_PRINTER.print(__VA_ARGS__); }
#define DEBUG_PRINTMLN(...) { DEBUG_PRINTER.print(millis()); DEBUG_PRINTER.println(__VA_ARGS__); }
#define DEBUG_PRINTLN(...) { DEBUG_PRINTER.println(__VA_ARGS__); }
#else
#define DEBUG_BEGIN(...) {}
#define DEBUG_PRINTM(...) {}
#define DEBUG_PRINT(...) {}
#define DEBUG_PRINTMLN(...) {}
#define DEBUG_PRINTLN(...) {}
#endif

#include "board_def.h"

#include <math.h>
#include <SPI.h>
#include <TFT_eSPI.h>

#include <Wire.h>
#include "pcf8563.h"
#include "axp20x.h"

TFT_eSPI tft = TFT_eSPI();
PCF8563_Class rtc;
AXP20X_Class axp;

float sdeg, mdeg, hdeg;
float sx, sy, mx, my, hx, hy; // Saved H, M, S x & y multipliers
uint8_t osx = CENTER, osy = CENTER, omx = CENTER, omy = CENTER, ohx = CENTER, ohy = CENTER; // Saved H, M, S x & y coords
uint8_t nsx, nsy, nmx, nmy, nhx, nhy; // H, M, S x & y coords
uint8_t xMin, yMin, xMax, yMax; // redraw range
uint32_t targetTime; // for next 1 second timeout

uint8_t hh, mm, ss;

void setup(void) {
  DEBUG_BEGIN(115200);
  DEBUG_PRINTMLN(": start");

  tft.init();
  DEBUG_PRINTMLN(": tft.init()");

  tft.setRotation(0);
  DEBUG_PRINTMLN(": tft.setRotation(0)");

  tft.fillScreen(BACKGROUND);
  DEBUG_PRINTMLN(": tft.fillScreen(BACKGROUND)");

  // start TFT back light
  ledcAttachPin(TFT_BL, 1); // assign TFT_BL pin to channel 1
  ledcSetup(1, 12000, 8); // 12 kHz PWM, 8-bit resolution
  ledcWrite(1, 192); // brightness 0 - 255
  DEBUG_PRINTMLN(": TFT back light");

  // Draw 60 clock marks
  draw_round_clock_mark(104, 120, 112, 120, 114, 114);
  //draw_square_clock_mark(104, 120, 112, 120, 114, 114);
  DEBUG_PRINTMLN(": Draw 60 clock marks");

  // read date and time from RTC
  Wire.begin(SEN_SDA, SEN_SCL);
  rtc.begin();
  RTC_Date rtc_date = rtc.getDateTime();
  hh = rtc_date.hour;
  mm = rtc_date.minute;
  ss = rtc_date.second;
  DEBUG_PRINTMLN(": read date and time from RTC");

  axp.begin();
  axp.setStartupTime(AXP202_STARTUP_TIME_128MS);
  axp.setlongPressTime(0);
  axp.setTimeOutShutdown(true);
  DEBUG_PRINTMLN(": AXP20X_Class init");

  targetTime = ((millis() / 1000) + 1) * 1000;
}

void loop() {
  unsigned long cur_millis = millis();
  if (cur_millis > SLEEP_TIME) {
    // enter sleep
    DEBUG_PRINTMLN(": enter sleep");
    axp.shutdown();
  }

  if (cur_millis >= targetTime) {
    targetTime += 1000;
    ss++;              // Advance second
    if (ss == 60) {
      ss = 0;
      mm++;            // Advance minute
      if (mm > 59) {
        mm = 0;
        hh++;          // Advance hour
        if (hh > 23) {
          hh = 0;
        }
      }
    }
  }

  // Pre-compute hand degrees, x & y coords for a fast screen update
  sdeg = SIXTIETH_RADIAN * ((0.001 * (cur_millis % 1000)) + ss); // 0-59 (includes millis)
  mdeg = (SIXTIETH * sdeg) + (SIXTIETH_RADIAN * mm); // 0-59 (includes seconds)
  hdeg = (SIXTIETH * mdeg) + (TWELFTH_RADIAN * hh); // 0-11 (includes minutes)
  sdeg -= RIGHT_ANGLE;
  mdeg -= RIGHT_ANGLE;
  hdeg -= RIGHT_ANGLE;
  hx = cos(hdeg);
  hy = sin(hdeg);
  mx = cos(mdeg);
  my = sin(mdeg);
  sx = cos(sdeg);
  sy = sin(sdeg);

  // redraw hands
  /*
     method 1: erase and_draw
     not overwrite round_watch_mark
     notable blink
  */
  //redraw_hands_erase_and_draw(100, 90, 50);
  /*
     method 2: overwrite rectangle
     sometime notable second hand redraw
  */
  //redraw_hands_rect(overwrite_rect, 100, 90, 50); // square_watch_mark only
  //redraw_hands_rect(overwrite_rect, 85, 70, 50);
  /*
     method 3: overwrite 4 splitted rectangle
     smooth redraw
  */
  //redraw_hands_4_split(overwrite_rect, 100, 90, 50); // square_watch_mark only
  //redraw_hands_4_split(overwrite_rect, 85, 70, 40);
  /*
     method 4: draw and erase
     not overwrite round_watch_mark
     notable redraw
  */
  //redraw_hands_rect(draw_and_erase, 100, 90, 50);
  /*
     method 5: draw and erase with 4 splitted rectangle
     not overwrite round_watch_mark
     more smooth redraw
  */
  redraw_hands_4_split(draw_and_erase, 100, 90, 50);

  delay(10);
  DEBUG_PRINTMLN(": loop()");
}

void draw_round_clock_mark(uint8_t innerR1, uint8_t outerR1, uint8_t innerR2, uint8_t outerR2, uint8_t innerR3, uint8_t outerR3) {
  float x, y;
  uint8_t x0, x1, y0, y1, innerR, outerR;

  for (int i = 0; i < 60; i ++) {
    if ((i % 15) == 0) {
      innerR = innerR1;
      outerR = outerR1;
    } else if ((i % 5) == 0) {
      innerR = innerR2;
      outerR = outerR2;
    } else {
      innerR = innerR3;
      outerR = outerR3;
    }

    mdeg = (SIXTIETH_RADIAN * i) - RIGHT_ANGLE;
    x = cos(mdeg);
    y = sin(mdeg);
    x0 = x * outerR + CENTER;
    y0 = y * outerR + CENTER;
    x1 = x * innerR + CENTER;
    y1 = y * innerR + CENTER;

    tft.drawLine(x0, y0, x1, y1, MARK_COLOR);
    DEBUG_PRINTM(" i: "); DEBUG_PRINT(i);
    DEBUG_PRINT(", x0: "); DEBUG_PRINT(x0); DEBUG_PRINT(", y0: "); DEBUG_PRINT(y0);
    DEBUG_PRINT(", x1: "); DEBUG_PRINT(x1); DEBUG_PRINT(", y1: "); DEBUG_PRINTLN(y1);
  }
}

void draw_square_clock_mark(uint8_t innerR1, uint8_t outerR1, uint8_t innerR2, uint8_t outerR2, uint8_t innerR3, uint8_t outerR3) {
  float x, y;
  uint8_t x0, x1, y0, y1, innerR, outerR;

  for (int i = 0; i < 60; i ++) {
    if ((i % 15) == 0) {
      innerR = innerR1;
      outerR = outerR1;
    } else if ((i % 5) == 0) {
      innerR = innerR2;
      outerR = outerR2;
    } else {
      innerR = innerR3;
      outerR = outerR3;
    }

    if ((i >= 53) || (i < 8)) {
      x = tan(SIXTIETH_RADIAN * i);
      x0 = x * outerR + CENTER;
      y0 = 1 - outerR + CENTER;
      x1 = x * innerR + CENTER;
      y1 = 1 - innerR + CENTER;
    } else if (i < 23) {
      y = tan((SIXTIETH_RADIAN * i) - RIGHT_ANGLE);
      x0 = outerR + CENTER;
      y0 = y * outerR + CENTER;
      x1 = innerR + CENTER;
      y1 = y * innerR + CENTER;
    } else if (i < 38) {
      x = tan(SIXTIETH_RADIAN * i);
      x0 = x * outerR + CENTER;
      y0 = outerR + CENTER;
      x1 = x * innerR + CENTER;
      y1 = innerR + CENTER;
    } else if (i < 53) {
      y = tan((SIXTIETH_RADIAN * i) - RIGHT_ANGLE);
      x0 = 1 - outerR + CENTER;
      y0 = y * outerR + CENTER;
      x1 = 1 - innerR + CENTER;
      y1 = y * innerR + CENTER;
    }
    tft.drawLine(x0, y0, x1, y1, MARK_COLOR);
    DEBUG_PRINTM(" i: "); DEBUG_PRINT(i);
    DEBUG_PRINT(", x0: "); DEBUG_PRINT(x0); DEBUG_PRINT(", y0: "); DEBUG_PRINT(y0);
    DEBUG_PRINT(", x1: "); DEBUG_PRINT(x1); DEBUG_PRINT(", y1: "); DEBUG_PRINTLN(y1);
  }
}

void redraw_hands_erase_and_draw(uint8_t secondLen, uint8_t minuteLen, uint8_t hourLen) {
  // Erase 3 handw positions
  tft.drawLine(ohx, ohy, CENTER, CENTER, BACKGROUND);
  tft.drawLine(omx, omy, CENTER, CENTER, BACKGROUND);
  tft.drawLine(osx, osy, CENTER, CENTER, BACKGROUND);
  ohx = hx * hourLen + CENTER;
  ohy = hy * hourLen + CENTER;
  omx = mx * minuteLen + CENTER;
  omy = my * minuteLen + CENTER;
  osx = sx * secondLen + CENTER;
  osy = sy * secondLen + CENTER;
  tft.drawLine(omx, omy, CENTER, CENTER, MINUTE_COLOR);
  tft.drawLine(ohx, ohy, CENTER, CENTER, HOUR_COLOR);
  tft.drawLine(osx, osy, CENTER, CENTER, SECOND_COLOR);

  tft.fillCircle(CENTER, CENTER, 3, SECOND_COLOR);
}

void redraw_hands_rect(void (*redrawFunc)(), uint8_t secondLen, uint8_t minuteLen, uint8_t hourLen) {
  nhx = hx * hourLen + CENTER;
  nhy = hy * hourLen + CENTER;
  nmx = mx * minuteLen + CENTER;
  nmy = my * minuteLen + CENTER;
  nsx = sx * secondLen + CENTER;
  nsy = sy * secondLen + CENTER;

  xMin = min(min(min((uint8_t)CENTER, ohx), min(omx, osx)), min(min(nhx, nmx), nsx));
  xMax = max(max(max((uint8_t)CENTER, ohx), max(omx, osx)), max(max(nhx, nmx), nsx));
  yMin = min(min(min((uint8_t)CENTER, ohy), min(omy, osy)), min(min(nhy, nmy), nsy));
  yMax = max(max(max((uint8_t)CENTER, ohy), max(omy, osy)), max(max(nhy, nmy), nsy));

  redrawFunc();

  ohx = nhx;
  ohy = nhy;
  omx = nmx;
  omy = nmy;
  osx = nsx;
  osy = nsy;
}

void redraw_hands_4_split(void (*redrawFunc)(), uint8_t secondLen, uint8_t minuteLen, uint8_t hourLen) {
  nhx = hx * hourLen + CENTER;
  nhy = hy * hourLen + CENTER;
  nmx = mx * minuteLen + CENTER;
  nmy = my * minuteLen + CENTER;
  nsx = sx * secondLen + CENTER;
  nsy = sy * secondLen + CENTER;

  if (in_split_range(1, 1, CENTER, CENTER)) {
    redrawFunc();
  }

  if (in_split_range(CENTER, 1, CENTER * 2, CENTER)) {
    redrawFunc();
  }

  if (in_split_range(1, CENTER, CENTER, CENTER * 2)) {
    redrawFunc();
  }

  if (in_split_range(CENTER, CENTER, CENTER * 2, CENTER * 2)) {
    redrawFunc();
  }

  ohx = nhx;
  ohy = nhy;
  omx = nmx;
  omy = nmy;
  osx = nsx;
  osy = nsy;
}

void overwrite_rect() {
  tft.setAddrWindow(xMin, yMin, xMax, yMax);
  for (uint8_t y = yMin; y <= yMax; y ++) {
    for (uint8_t x = xMin; x <= xMax; x ++) {
      if (inLine(x, y, nsx, nsy, CENTER, CENTER)) {
        tft.pushColor(SECOND_COLOR); // draw second arm
      } else if (inLine(x, y, nhx, nhy, CENTER, CENTER)) {
        tft.pushColor(HOUR_COLOR); // draw hour arm
      } else if (inLine(x, y, nmx, nmy, CENTER, CENTER)) {
        tft.pushColor(MINUTE_COLOR); // draw minute arm
      } else {
        tft.pushColor(BACKGROUND); // over write background color
      }
    }
  }
}

void draw_and_erase() {
  for (uint8_t y = yMin; y <= yMax; y ++) {
    for (uint8_t x = xMin; x <= xMax; x ++) {
      if (inLine(x, y, nsx, nsy, CENTER, CENTER)) {
        tft.drawPixel(x, y, SECOND_COLOR); // draw second arm
      } else if (inLine(x, y, nhx, nhy, CENTER, CENTER)) {
        tft.drawPixel(x, y, HOUR_COLOR); // draw hour arm
      } else if (inLine(x, y, nmx, nmy, CENTER, CENTER)) {
        tft.drawPixel(x, y, MINUTE_COLOR); // draw minute arm
      } else if (
        (inLine(x, y, osx, osy, CENTER, CENTER))
        || (inLine(x, y, ohx, ohy, CENTER, CENTER))
        || (inLine(x, y, omx, omy, CENTER, CENTER))
      ) {
        tft.drawPixel(x, y, BACKGROUND); // erase with background color
      }
    }
  }
}

bool in_split_range(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
  bool in_range = false;

  xMin = CENTER;
  yMin = CENTER;
  xMax = CENTER;
  yMax = CENTER;

  if ((x0 <= ohx) && (ohx <= x1) && (y0 <= ohy) && (ohy <= y1)) {
    in_range = true;
    xMin = min(xMin, ohx);
    xMax = max(xMax, ohx);
    yMin = min(yMin, ohy);
    yMax = max(yMax, ohy);
  }

  if ((x0 <= omx) && (omx <= x1) && (y0 <= omy) && (omy <= y1)) {
    in_range = true;
    xMin = min(xMin, omx);
    xMax = max(xMax, omx);
    yMin = min(yMin, omy);
    yMax = max(yMax, omy);
  }

  if ((x0 <= osx) && (osx <= x1) && (y0 <= osy) && (osy <= y1)) {
    in_range = true;
    xMin = min(xMin, osx);
    xMax = max(xMax, osx);
    yMin = min(yMin, osy);
    yMax = max(yMax, osy);
  }

  if ((x0 <= nhx) && (nhx <= x1) && (y0 <= nhy) && (nhy <= y1)) {
    in_range = true;
    xMin = min(xMin, nhx);
    xMax = max(xMax, nhx);
    yMin = min(yMin, nhy);
    yMax = max(yMax, nhy);
  }

  if ((x0 <= nmx) && (nmx <= x1) && (y0 <= nmy) && (nmy <= y1)) {
    in_range = true;
    xMin = min(xMin, nmx);
    xMax = max(xMax, nmx);
    yMin = min(yMin, nmy);
    yMax = max(yMax, nmy);
  }

  if ((x0 <= nsx) && (nsx <= x1) && (y0 <= nsy) && (nsy <= y1)) {
    in_range = true;
    xMin = min(xMin, nsx);
    xMax = max(xMax, nsx);
    yMin = min(yMin, nsy);
    yMax = max(yMax, nsy);
  }

  return in_range;
}

bool inLine(uint8_t x, uint8_t y, uint8_t lx0, uint8_t ly0, uint8_t lx1, uint8_t ly1) {
  if ((lx0 == lx1) && (lx0 == x)) { // vertical line checking
    if (ly0 < ly1) {
      return ((ly0 <= y) && (y <= ly1));
    } else {
      return ((ly1 <= y) && (y <= ly0));
    }
  } else if ((ly0 == ly1) && (ly0 == y)) { // horizontal line checking
    if (lx0 < lx1) {
      return ((lx0 <= x) && (x <= lx1));
    } else {
      return ((lx1 <= x) && (x <= lx0));
    }
  } else { // general line checking
    if (lx0 > lx1) {
      // swap points if lx0 larger
      uint8_t tmp = lx0;
      lx0 = lx1;
      lx1 = tmp;
      tmp = ly0;
      ly0 = ly1;
      ly1 = tmp;
    }
    if ((x < lx0) || (lx1 < x)) { // x range checking
      return false;
    }
    float xLen = lx1 - lx0;
    float yLen;
    if (ly0 < ly1) {
      if ((y < ly0) || (ly1 < y)) { // y range checking
        return false;
      }
      yLen = ly1 - ly0;
    } else {
      if ((y < ly1) || (ly0 < y)) { // y range checking
        return false;
      }
      yLen = ly0 - ly1;
    }
    if (xLen > yLen) {
      if ((lx0 <= x) && (x <= lx1)) {
        if (ly0 < ly1) {
          if ((ly0 <= y) && (y <= ly1)) {
            return (y == (ly0 + round((x - lx0) * yLen / xLen)));
          }
        } else { // ly0 >= ly1
          if ((ly1 <= y) && (y <= ly0)) {
            return (y == (ly0 - round((x - lx0) * yLen / xLen)));
          }
        }
      }
    } else { // xLen <= yLen
      if ((lx0 <= x) && (x <= lx1)) {
        if (ly0 < ly1) {
          if ((ly0 <= y) && (y <= ly1)) {
            return (x == (lx0 + round((y - ly0) * xLen / yLen)));
          }
        } else { // ly0 >= ly1
          if ((ly1 <= y) && (y <= ly0)) {
            return (x == (lx1 - round((y - ly1) * xLen / yLen)));
          }
        }
      }
    }
  }
  return false;
}
