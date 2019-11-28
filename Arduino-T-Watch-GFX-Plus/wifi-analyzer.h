#ifndef WIFI_ANALYZER_H
#define WIFI_ANALYZER_H

#include <Arduino_GFX.h>
#include <WiFi.h>

#define SCAN_INTERVAL 1 // milliseconds

// RSSI RANGE
#define RSSI_CEILING -40
#define RSSI_FLOOR -100

static int w, h, text_size, banner_height, graph_baseline, graph_height, channel_width;

// Channel color mapping from channel 1 to 14
static unsigned int channel_color[] = {
    RED, ORANGE, YELLOW, GREEN, CYAN, MAGENTA,
    RED, ORANGE, YELLOW, GREEN, CYAN, MAGENTA,
    RED, ORANGE};

static unsigned char scan_count = 0;

void init_wifi_analyzer(Arduino_GFX *gfx);
void update_wifi_analyzer(Arduino_GFX *gfx);

#endif
