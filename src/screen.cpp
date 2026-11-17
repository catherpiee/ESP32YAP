#include "screen.h"

#include "assets.h"
#include "config.h"

static uint16_t* fb;
static const uint16_t WHITE = 0xFFFF, BLACK = 0x0000;

static constexpr uint16_t rgb565(uint32_t rgb) {
  return ((rgb >> 8) & 0xF800) | ((rgb >> 5) & 0x07E0) | ((rgb >> 3) & 0x001F);
}

static inline uint16_t blend(uint16_t fg, uint16_t bg, uint8_t a) {
  if (a == 255) return fg;
  if (a == 0) return bg;
  uint32_t r = (((fg >> 11) * a) + ((bg >> 11) * (255 - a))) / 255;
  uint32_t g = ((((fg >> 5) & 0x3F) * a) + (((bg >> 5) & 0x3F) * (255 - a))) / 255;
  uint32_t b = (((fg & 0x1F) * a) + ((bg & 0x1F) * (255 - a))) / 255;
  return (r << 11) | (g << 5) | b;
}

static void fill(uint16_t c) {
  for (int i = 0; i < SCREEN_W * SCREEN_H; i++) fb[i] = c;
}

// An alpha mask drawn in one colour at (x, y), clipped to the screen.
static void drawMask(const uint8_t* m, int w, int h, int x, int y, uint16_t ink) {
  int x0 = max(0, -x), x1 = min(w, SCREEN_W - x);
  int y0 = max(0, -y), y1 = min(h, SCREEN_H - y);
  for (int my = y0; my < y1; my++) {
    const uint8_t* row = m + my * w;
    uint16_t* out = fb + (y + my) * SCREEN_W + x;
    for (int mx = x0; mx < x1; mx++)
      if (row[mx]) out[mx] = blend(ink, out[mx], row[mx]);
  }
}

// ---------- text ----------
static const uint32_t HEART_CP = 0x2665;

// UTF-8 to code points, with "<3" and the heart emoji turned into the heart.
static int toCodepoints(const String& s, uint32_t* out, int maxLen) {
  int n = 0;
  const uint8_t* p = (const uint8_t*)s.c_str();
  while (*p && n < maxLen) {
    uint32_t cp;
    if (*p < 0x80) cp = *p++;
    else if ((*p & 0xE0) == 0xC0 && p[1]) { cp = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F); p += 2; }
    else if ((*p & 0xF0) == 0xE0 && p[1] && p[2]) { cp = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F); p += 3; }
    else { p++; continue; }
    if (cp == '<' && *p == '3') { cp = HEART_CP; p++; }
    if (cp == 0x2764) cp = HEART_CP;
    if (cp == 0xFE0F) continue;
    if (cp == 0x2018) cp = '\'';
    out[n++] = cp;
  }
  return n;
}

static const Glyph* findGlyph(uint32_t cp) {
  for (int i = 0; i < TEXT_FONT.count; i++)
    if (TEXT_FONT.glyphs[i].cp == cp) return &TEXT_FONT.glyphs[i];
  return nullptr;
}

static int heartAdvance() { return HEART.w + 8; }

static int textWidth(const uint32_t* cps, int n) {
  int w = 0;
  for (int i = 0; i < n; i++) {
    if (cps[i] == HEART_CP) { w += heartAdvance(); continue; }
    const Glyph* g = findGlyph(cps[i]);
    if (g) w += g->adv;
  }
  return w;
}

static void drawText(const uint32_t* cps, int n, int x, int lineTop, uint16_t ink) {
  int capMid = lineTop + TEXT_FONT.capTop + TEXT_FONT.capH / 2;
  for (int i = 0; i < n && x < SCREEN_W; i++) {
    if (cps[i] == HEART_CP) {
      drawMask(HEART.alpha, HEART.w, HEART.h, x + 4, capMid - HEART.h / 2, ink);
      x += heartAdvance();
      continue;
    }
    const Glyph* g = findGlyph(cps[i]);
    if (!g) continue;
    if (g->w && x + g->adv > 0) drawMask(TEXT_FONT.data + g->off, g->w, g->h, x + g->xoff, lineTop + g->yoff, ink);
    x += g->adv;
  }
}

// Centred if it fits, otherwise scrolls left and loops.
static void drawCentredOrScrolling(const String& text, uint16_t ink, uint32_t ms) {
  static uint32_t cps[160];
  int n = toCodepoints(text, cps, 160);
  int w = textWidth(cps, n);
  int top = (SCREEN_H + TEXT_FONT.capH) / 2 - TEXT_FONT.ascent;
  if (w <= SCREEN_W - 32) {
    drawText(cps, n, (SCREEN_W - w) / 2, top, ink);
    return;
  }
  int period = w + 56;
  uint32_t offset = (uint64_t)ms * SCROLL_SPEED / 1000;
  for (int x = 16 - (int)(offset % period) - period; x < SCREEN_W; x += period) drawText(cps, n, x, top, ink);
}

// ---------- the screen ----------
// Where the nine stars sit, around the edges and clear of the text.
static const int STAR_POS[9][2] = {{14, 12}, {92, 6}, {196, 14}, {252, 8}, {300, 62}, {280, 132}, {168, 148}, {60, 140}, {8, 96}};

void drawScreen(uint16_t* buf, const TextSettings& s, uint32_t ms) {
  fb = buf;
  uint32_t rgb = 0;
  if (s.background.startsWith("#") && s.background.length() == 7) rgb = strtoul(s.background.c_str() + 1, nullptr, 16);
  fill(rgb565(rgb));
  // White text and decorations, or black on very pale backgrounds so it stays readable.
  uint16_t ink = ((rgb >> 16) & 255) * 3 + ((rgb >> 8) & 255) * 6 + (rgb & 255) > 2000 ? BLACK : WHITE;

  if (s.decoration == "stars")
    for (int i = 0; i < 9; i++) drawMask(STARS[i].alpha, STARS[i].w, STARS[i].h, STAR_POS[i][0], STAR_POS[i][1], ink);

  drawCentredOrScrolling(s.text, ink, ms);

  if (s.decoration == "cat")  // small grumpy cat logo in the bottom-right corner
    drawMask(CAT.alpha, CAT.w, CAT.h, SCREEN_W - CAT.w - 8, SCREEN_H - CAT.h - 6, ink);
}
