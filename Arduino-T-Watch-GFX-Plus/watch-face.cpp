#include "watch-face.h"

void draw_round_clock_mark(Arduino_GFX *gfx, unsigned char innerR1, unsigned char outerR1, unsigned char innerR2, unsigned char outerR2, unsigned char innerR3, unsigned char outerR3)
{
  float x, y;
  unsigned char x0, x1, y0, y1, innerR, outerR;
  unsigned int c;

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

    gfx->drawLine(x0, y0, x1, y1, c);
    log_d(" i: %d, x0: %d, y0: %d, x1: %d, y1: %d", i, x0, y0, x1, y1);
  }
}

void draw_square_clock_mark(Arduino_GFX *gfx, unsigned char innerR1, unsigned char outerR1, unsigned char innerR2, unsigned char outerR2, unsigned char innerR3, unsigned char outerR3)
{
  float x, y;
  unsigned char x0, x1, y0, y1, innerR, outerR;
  unsigned int c;

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
    gfx->drawLine(x0, y0, x1, y1, c);
    log_d(" i: %d, x0: %d, y0: %d, x1: %d, y1: %d", i, x0, y0, x1, y1);
  }
}

void write_cache_pixel(Arduino_GFX *gfx, unsigned char x, unsigned char y, unsigned int color, bool cross_check_second, bool cross_check_hour)
{
  unsigned char *cache = cached_points;
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
  gfx->writePixel(x, y, color);
}

void draw_and_erase_cached_line(Arduino_GFX *gfx, unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, unsigned int color, unsigned char *cache, unsigned int cache_len, bool cross_check_second, bool cross_check_hour)
{
  bool steep = _diff(y1, y0) > _diff(x1, x0);
  if (steep)
  {
    _swap_uint8_t(x0, y0);
    _swap_uint8_t(x1, y1);
  }

  unsigned char dx, dy;
  dx = _diff(x1, x0);
  dy = _diff(y1, y0);

  unsigned char err = dx / 2;
  int8_t xstep = (x0 < x1) ? 1 : -1;
  int8_t ystep = (y0 < y1) ? 1 : -1;
  x1 += xstep;
  unsigned char x, y, ox, oy;
  for (unsigned int i = 0; i <= dx; i++)
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
        write_cache_pixel(gfx, x, y, color, cross_check_second, cross_check_hour);
      }
    }
    else
    {
      write_cache_pixel(gfx, x, y, color, cross_check_second, cross_check_hour);
      if ((ox > 0) || (oy > 0))
      {
        write_cache_pixel(gfx, ox, oy, BACKGROUND, cross_check_second, cross_check_hour);
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
  for (unsigned int i = dx + 1; i < cache_len; i++)
  {
    ox = *(cache + (i * 2));
    oy = *(cache + (i * 2) + 1);
    if ((ox > 0) || (oy > 0))
    {
      write_cache_pixel(gfx, ox, oy, BACKGROUND, cross_check_second, cross_check_hour);
    }
    *(cache + (i * 2)) = 0;
    *(cache + (i * 2) + 1) = 0;
  }
}

void redraw_hands_cached_draw_and_erase(Arduino_GFX *gfx)
{
  gfx->startWrite();
  draw_and_erase_cached_line(gfx, CENTER, CENTER, nsx, nsy, SECOND_COLOR, cached_points, SECOND_LEN + 1, false, false);
  draw_and_erase_cached_line(gfx, CENTER, CENTER, nhx, nhy, HOUR_COLOR, cached_points + ((SECOND_LEN + 1) * 2), HOUR_LEN + 1, true, false);
  draw_and_erase_cached_line(gfx, CENTER, CENTER, nmx, nmy, MINUTE_COLOR, cached_points + ((SECOND_LEN + 1 + HOUR_LEN + 1) * 2), MINUTE_LEN + 1, true, true);
  gfx->endWrite();
}

void init_watch_face(Arduino_GFX *gfx)
{
    // Draw 60 clock marks
    //draw_round_clock_mark(gfx, 104, 120, 112, 120, 114, 114);
    draw_square_clock_mark(gfx, 102, 120, 108, 120, 114, 120);
    log_i("Draw 60 clock marks");
}

void update_watch_face(Arduino_GFX *gfx, unsigned char hh, unsigned char mm, unsigned char ss, unsigned long cur_millis)
{
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
        redraw_hands_cached_draw_and_erase(gfx);

        ohx = nhx;
        ohy = nhy;
        omx = nmx;
        omy = nmy;
        osx = nsx;
        osy = nsy;

#if (CORE_DEBUG_LEVEL > 0)
        loopCount++;
#endif
    }

  delay(1);
}
