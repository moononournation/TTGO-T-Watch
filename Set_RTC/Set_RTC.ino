#include <Wire.h>
#include "pcf8563.h"

PCF8563_Class rtc;

static uint8_t convMmm(const char* p) {
  uint8_t v = 0;
  if (*p == 'J') {
    if (*(p + 1) == 'a') {
      return 1;
    } else if (*(p + 2) == 'n') {
      return 6;
    } else {
      return 7;
    }
  } else if (*p == 'F') {
    return 2;
  } else if (*p == 'M') {
    if (*(p + 2) == 'r') {
      return 3;
    } else {
      return 5;
    }
  } else if (*p == 'A') {
    if (*(p + 1) == 'p') {
      return 3;
    } else {
      return 8;
    }
  } else if (*p == 'S') {
    return 9;
  } else if (*p == 'O') {
    return 10;
  } else if (*p == 'N') {
    return 11;
  } else {
    return 12;
  }
}

static uint8_t conv2d(const char* p) {
  uint8_t v = 0;
  return (10 * (*p - '0')) + (*++p - '0');
}

static uint16_t conv4d(const char* p) {
  return (1000 * (*p - '0')) + (100 * (*++p - '0')) + (10 * (*++p - '0')) + (*++p - '0');
}

// Get date and time from compile time
uint16_t vYYYY = conv4d(__DATE__ + 7);
uint8_t vMM = convMmm(__DATE__);
uint8_t vDD = conv2d(__DATE__ + 4);
uint8_t vH = conv2d(__TIME__);
uint8_t vM = conv2d(__TIME__ + 3);
uint8_t vS = conv2d(__TIME__ + 6);

void setup()
{
  Serial.begin(115200);
  Serial.println(__DATE__); // Jun 21 2019
  Serial.println(__TIME__); // 11:40:35
  Serial.print(vYYYY);
  Serial.print(',');
  Serial.print(vMM);
  Serial.print(',');
  Serial.print(vDD);
  Serial.print(',');
  Serial.print(vH);
  Serial.print(',');
  Serial.print(vM);
  Serial.print(',');
  Serial.println(vS);

  Wire.begin();
  rtc.begin();
  rtc.setDateTime(vYYYY, vMM, vDD, vH, vM, vS);
}

void loop()
{
  //Serial.println(rtc.formatDateTime(PCF_TIMEFORMAT_YYYY_MM_DD_H_M_S));

  RTC_Date rtc_date = rtc.getDateTime();
  Serial.print(rtc_date.year);
  Serial.print('-');
  Serial.print(rtc_date.month);
  Serial.print('-');
  Serial.print(rtc_date.day);
  Serial.print(' ');
  Serial.print(rtc_date.hour);
  Serial.print(':');
  Serial.print(rtc_date.minute);
  Serial.print(':');
  Serial.println(rtc_date.second);

  delay(1000);
}
