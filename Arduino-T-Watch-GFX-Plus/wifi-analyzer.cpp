#include "wifi-analyzer.h"

void init_wifi_analyzer(Arduino_GFX *gfx)
{
  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // init value
  w = gfx->width();
  h = gfx->height();
  text_size = (w <= 240) ? 1 : 2;
  banner_height = text_size * 8;
  graph_baseline = h - 20;                            // minus 2 text lines
  graph_height = graph_baseline - banner_height - 30; // minus 3 text lines
  channel_width = w / 16;

  // init banner
  gfx->setTextSize(text_size);
  gfx->fillScreen(BLUE);
  gfx->setTextColor(WHITE, RED);
  gfx->setCursor(0, 0);
  gfx->print(" TTGO ");
  gfx->setTextColor(WHITE, ORANGE);
  gfx->print(" T-Watch ");
  gfx->setTextColor(WHITE, GREEN);
  gfx->print(" WiFi ");
  gfx->setTextColor(WHITE, BLUE);
  gfx->print(" Analyzer");
}

void update_wifi_analyzer(Arduino_GFX *gfx)
{
  uint8_t ap_count_list[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  int32_t noise_list[] = {RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR};

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();

  // clear old graph
  gfx->fillRect(0, banner_height, w, h - banner_height, BLACK);
  gfx->setTextSize(1);

  if (n == 0)
  {
    gfx->setTextColor(WHITE);
    gfx->setCursor(0, banner_height);
    gfx->println("no networks found");
  }
  else
  {
    // plot found WiFi info
    for (int i = 0; i < n; i++)
    {
      int32_t channel = WiFi.channel(i);
      int32_t rssi = WiFi.RSSI(i);
      uint16_t color = channel_color[channel - 1];
      int height = constrain(map(rssi, RSSI_FLOOR, RSSI_CEILING, 1, graph_height), 1, graph_height);

      // trim rssi with RSSI_FLOOR
      if (rssi < RSSI_FLOOR)
      {
        rssi = RSSI_FLOOR;
      }

      // channel stat
      ap_count_list[channel - 1]++;

      // noise stat
      int32_t noise = rssi - RSSI_FLOOR;
      if (channel > 2)
      {
        noise_list[channel - 3] += noise / 4;
      }
      if (channel > 1)
      {
        noise_list[channel - 2] += noise / 2;
      }
      noise_list[channel - 1] += noise / 2;
      if (channel < 14)
      {
        noise_list[channel] += noise / 2;
      }
      if (channel < 13)
      {
        noise_list[channel + 1] += noise / 4;
      }

      // plot chart
      gfx->drawLine(channel * channel_width, graph_baseline - height, (channel - 1) * channel_width, graph_baseline + 1, color);
      gfx->drawLine(channel * channel_width, graph_baseline - height, (channel + 1) * channel_width, graph_baseline + 1, color);

      // Print SSID, signal strengh and if not encrypted
      gfx->setTextColor(color);
      gfx->setCursor((channel - 1) * (channel_width - 4), graph_baseline - 10 - height);
      gfx->print(WiFi.SSID(i));
      gfx->print('(');
      gfx->print(rssi);
      gfx->print(')');
#if defined(ESP32)
      if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN)
#else
      if (WiFi.encryptionType(i) == ENC_TYPE_NONE)
#endif
      {
        gfx->print('*');
      }

      // rest for WiFi routine?
      delay(10);
    }
  }

  // print WiFi stat
  gfx->setTextColor(WHITE);
  gfx->setCursor(0, banner_height);
  gfx->print(n);
  gfx->print(" networks found, free channels: ");
  bool listed_first_channel = false;
  int32_t min_noise = 0;
  for (int i = 1; i <= 11; i++) // channels 12-14 may not available
  {
    if (noise_list[i - 1] < min_noise)
    {
      min_noise = noise_list[i - 1];
    }
  }

  for (int i = 1; i <= 11; i++) // channels 12-14 may not available
  {
    // check channel with min noise
    if (noise_list[i - 1] == min_noise)
    {
      if (!listed_first_channel)
      {
        listed_first_channel = true;
      }
      else
      {
        gfx->print(", ");
      }
      gfx->print(i);
    }
  }

  // draw graph base axle
  gfx->drawFastHLine(0, graph_baseline, 320, WHITE);
  for (int i = 1; i <= 14; i++)
  {
    gfx->setTextColor(channel_color[i - 1]);
    gfx->setCursor((i * channel_width) - ((i < 10) ? 3 : 6), graph_baseline + 2);
    gfx->print(i);
    if (ap_count_list[i - 1] > 0)
    {
      gfx->setCursor((i * channel_width) - ((ap_count_list[i - 1] < 10) ? 9 : 12), graph_baseline + 8 + 2);
      gfx->print('(');
      gfx->print(ap_count_list[i - 1]);
      gfx->print(')');
    }
  }

  // Wait a bit before scanning again
  delay(SCAN_INTERVAL);
}
