# ESP32 Text

Show any text you like on a tiny ESP32 screen, and change it from your phone. No app and no internet needed: the board makes its own Wi-Fi network and serves the control page itself.

- Type any text. Short text sits centred, and long text scrolls across.
- Pick a background colour: seven presets or any colour you like.
- Add stars, a little grumpy cat logo, or nothing.
- Type `<3` or ❤️ for a heart.
- It remembers your last screen when it's switched off.

## What you need

- A **LILYGO T-Display-S3**: an ESP32-S3 board with a built-in 1.9" colour screen (320 × 170). Get the non-touch version.
- A **USB-C cable that carries data**.
- Optional: a **3.7 V LiPo battery** with a JST 1.25 mm 2-pin plug, to run it without a cable. Check the red wire lines up with **+** before plugging it in.

## Put it on your board

1. Install **[PlatformIO](https://platformio.org/install/cli)**, or VS Code with the PlatformIO extension.
2. Download this repository and open a terminal in its folder.
3. Plug the board in with USB-C and run:
   ```
   pio run -t upload
   ```
4. If the upload doesn't connect, hold the **BOOT** button while plugging the board in, then let go and try again.

## Use it

1. The screen scrolls its Wi-Fi details when it starts.
2. On your phone, join the Wi-Fi network **CUSTOM-TEXT** (password **customtext**). "No Internet Connection" is expected, so stay connected.
3. Open **http://192.168.4.1** in Safari or Chrome.
4. Type your text, pick a colour, and tap **Show**.

Tip: in Safari, tap **Share → Add to Home Screen** to open it like an app.

## Make it yours

Everything you're likely to change is in **`include/config.h`**:

```c
#define WIFI_NAME "CUSTOM-TEXT"
#define WIFI_PASSWORD "customtext"  // at least 8 characters
#define DEFAULT_TEXT "hello <3"
#define DEFAULT_BACKGROUND "#000000"
#define DEFAULT_DECORATION "stars"   // "stars", "cat" or "none"
#define SCROLL_SPEED 60
```

Change them and upload again. If more than one person in a room uses this, give each board a different Wi-Fi name.

**Colour presets** are the buttons in `web/index.html` (search for `data-bg`).

**Different font:** put a `.ttf` or `.otf` file in `fonts/`, change `FONT_FILE` and `FONT_SIZE` in `tools/make_assets.py`, then run:

```
pip install pillow
python3 tools/make_assets.py
```

The phone preview uses `fonts/Archivo-Black-latin.ttf`. If you change the font, change the file name in `web/index.html` and `tools/make_web.py` too.

## How it works

| File | What it does |
|---|---|
| `src/main.cpp` | Wi-Fi network, web server, saving settings |
| `src/screen.cpp` | Draws the background, stars, text and cat |
| `src/display.h` | Screen pins and driver setup for the T-Display-S3 |
| `include/config.h` | Your settings |
| `include/assets.h` | The font, stars, heart and cat as bitmaps (made by `tools/make_assets.py`) |
| `web/index.html` | The phone control page, with a live preview |
| `tools/make_web.py` | Packs the control page into the firmware when you build |

The control page is served by the board over its own Wi-Fi, so it works anywhere, even with no phone signal. Opened from anywhere else, the page runs as a demo that doesn't control anything.

## Credits

- Font: [Archivo Black](https://fonts.google.com/specimen/Archivo+Black), SIL Open Font License (`fonts/OFL-Archivo.txt`).
- Screen library: [LovyanGFX](https://github.com/lovyan03/LovyanGFX). JSON: [ArduinoJson](https://arduinojson.org).
