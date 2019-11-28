#ifndef ARDUINO_GFX_H
#define ARDUINO_GFX_H

#include <Arduino_GFX.h>

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

static float sdeg, mdeg, hdeg;
static unsigned char osx = CENTER, osy = CENTER;   // Saved second coords
static unsigned char omx = CENTER, omy = CENTER;   // Saved minute coords
static unsigned char ohx = CENTER, ohy = CENTER;   // Saved hour coords
static unsigned char nsx, nsy, nmx, nmy, nhx, nhy; // H, M, S x & y coords
static unsigned char xMin, yMin, xMax, yMax;       // redraw range
static unsigned char tmp;
#if (CORE_DEBUG_LEVEL > 0)
static unsigned int loopCount = 0;
#endif

static unsigned char cached_points[(HOUR_LEN + 1 + MINUTE_LEN + 1 + SECOND_LEN + 1) * 2];
static int cached_points_idx = 0;
static unsigned char *last_cached_point;

void draw_round_clock_mark(Arduino_GFX *gfx, unsigned char innerR1, unsigned char outerR1, unsigned char innerR2, unsigned char outerR2, unsigned char innerR3, unsigned char outerR3);
void draw_square_clock_mark(Arduino_GFX *gfx, unsigned char innerR1, unsigned char outerR1, unsigned char innerR2, unsigned char outerR2, unsigned char innerR3, unsigned char outerR3);
void write_cache_pixel(Arduino_GFX *gfx, unsigned char x, unsigned char y, unsigned int color, bool cross_check_second, bool cross_check_hour);
void draw_and_erase_cached_line(Arduino_GFX *gfx, unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, unsigned int color, unsigned char *cache, unsigned int cache_len, bool cross_check_second, bool cross_check_hour);
void redraw_hands_cached_draw_and_erase(Arduino_GFX *gfx);
void init_watch_face(Arduino_GFX *gfx);
void update_watch_face(Arduino_GFX *gfx, unsigned char hh, unsigned char mm, unsigned char ss, unsigned long cur_millis);

#endif
