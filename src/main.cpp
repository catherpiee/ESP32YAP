// Custom text display for the LILYGO T-Display-S3 (ESP32-S3 with a 1.9" 320x170 screen).
// The board makes its own Wi-Fi network and serves a control page: join it on your phone,
// open http://192.168.4.1, type your text and pick a background. Settings live in include/config.h.
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

#include "config.h"
#include "display.h"
#include "screen.h"
#include "web_page.h"

static LGFX lcd;
static uint16_t* frame;
static TextSettings settings{DEFAULT_TEXT, DEFAULT_BACKGROUND, DEFAULT_DECORATION};
static Preferences prefs;
static WebServer server(80);
static uint32_t textChangedAt = 0;  // restarts the scroll when the text changes
static uint32_t saveAt = 0;         // saves a moment after the last change, to spare the flash

static void toJson(JsonDocument& doc) {
  doc["text"] = settings.text;
  doc["background"] = settings.background;
  doc["decoration"] = settings.decoration;
}

static void fromJson(JsonVariantConst in) {
  if (in["text"].is<const char*>()) {
    settings.text = in["text"].as<const char*>();
    textChangedAt = millis();
  }
  if (in["background"].is<const char*>()) {
    String bg = in["background"].as<const char*>();
    if (bg.startsWith("#") && bg.length() == 7) settings.background = bg;
  }
  if (in["decoration"].is<const char*>()) {
    String d = in["decoration"].as<const char*>();
    if (d == "stars" || d == "cat" || d == "none") settings.decoration = d;
  }
}

static void save() {
  JsonDocument doc;
  toJson(doc);
  String s;
  serializeJson(doc, s);
  prefs.putString("state", s);
}

static void sendState() {
  JsonDocument doc;
  toJson(doc);
  String out;
  serializeJson(doc, out);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", out);
}

void setup() {
  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, HIGH);  // keeps the screen powered when running on battery
  Serial.begin(115200);

  frame = (uint16_t*)ps_malloc(SCREEN_W * SCREEN_H * 2);
  if (!frame) frame = (uint16_t*)malloc(SCREEN_W * SCREEN_H * 2);
  lcd.init();
  lcd.setRotation(1);
  lcd.setBrightness(255);

  prefs.begin("customtext", false);
  JsonDocument saved;
  if (deserializeJson(saved, prefs.getString("state", "")) == DeserializationError::Ok) fromJson(saved.as<JsonVariantConst>());

  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_NAME, WIFI_PASSWORD);
  server.on("/", HTTP_GET, [] {
    server.sendHeader("Content-Encoding", "gzip");
    server.sendHeader("Cache-Control", "no-store");
    server.send_P(200, "text/html", (const char*)WEB_PAGE_GZ, WEB_PAGE_GZ_LEN);
  });
  server.on("/api/state", HTTP_GET, sendState);
  server.on("/api/state", HTTP_POST, [] {
    JsonDocument doc;
    if (deserializeJson(doc, server.arg("plain")) != DeserializationError::Ok) return server.send(400, "text/plain", "bad json");
    fromJson(doc.as<JsonVariantConst>());
    saveAt = millis() + 2000;
    sendState();
  });
  server.onNotFound([] {
    server.sendHeader("Location", "http://192.168.4.1/");
    server.send(302);
  });
  server.begin();

  // Show how to connect for a few seconds, as a scrolling line.
  TextSettings intro{String("Wi-Fi ") + WIFI_NAME + "  password " + WIFI_PASSWORD + "  open 192.168.4.1", "#000000", "none"};
  uint32_t start = millis();
  while (millis() - start < 9000) {
    server.handleClient();
    drawScreen(frame, intro, millis() - start);
    lcd.pushImage(0, 0, SCREEN_W, SCREEN_H, (lgfx::rgb565_t*)frame);
    delay(30);
  }
  textChangedAt = millis();
}

void loop() {
  server.handleClient();
  if (saveAt && millis() > saveAt) {
    saveAt = 0;
    save();
  }
  static uint32_t lastFrame = 0;
  if (millis() - lastFrame >= 33) {  // about 30 frames a second
    lastFrame = millis();
    drawScreen(frame, settings, millis() - textChangedAt);
    lcd.pushImage(0, 0, SCREEN_W, SCREEN_H, (lgfx::rgb565_t*)frame);
  }
  delay(1);
}
