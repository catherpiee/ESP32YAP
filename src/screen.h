#pragma once
#include <Arduino.h>

#define SCREEN_W 320
#define SCREEN_H 170

struct TextSettings {
  String text;        // what to show; "<3" or the heart emoji draws a heart
  String background;  // "#rrggbb"
  String decoration;  // "stars", "cat" or "none"
};

// Draws one frame into fb (RGB565, SCREEN_W x SCREEN_H). ms = time since the text last changed, for scrolling.
void drawScreen(uint16_t* fb, const TextSettings& s, uint32_t ms);
