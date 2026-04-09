#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <WiFi.h>
#include <SpotifyEsp32.h>
#include <SPI.h>

#define TFT_CS   1
#define TFT_RST  2
#define TFT_DC   3
#define TFT_SCLK 4
#define TFT_MOSI 5

// Rotary encoder pins — adjust to your wiring
#define ENC_CLK 6
#define ENC_DT  7
#define ENC_SW  8   // Push button (mute toggle)

char* SSID          = "F.Gjonej 1";
const char* PASSWORD      = "04070925";
const char* CLIENT_ID     = "d683842bfd4647dca7020f537fbdf0b2";
const char* CLIENT_SECRET = "3ce334a6cfd84583bfc40eb5d715bf78";

String lastArtist;
String lastTrackname;
int    currentVolume  = 50;   // 0–100, seeded at 50
int    lastVolume     = 50;
bool   isMuted        = false;
int    volumeBeforeMute = 50;

// Encoder state (touched in ISR — must be volatile)
volatile int  encoderDelta   = 0;   // accumulated ticks from ISR
volatile bool buttonPressed  = false;

Spotify sp(CLIENT_ID, CLIENT_SECRET);
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// ── ISR: called on every CLK edge ──────────────────────────────────────────
void IRAM_ATTR encoderISR() {
    // If DT matches CLK at the moment of the rising edge → clockwise
    if (digitalRead(ENC_DT) == digitalRead(ENC_CLK)) {
        encoderDelta++;
    } else {
        encoderDelta--;
    }
}

void IRAM_ATTR buttonISR() {
    buttonPressed = true;
}

// ── Draw the volume bar on-screen ──────────────────────────────────────────
void drawVolumeBar(int vol) {
    const int BAR_X = 10, BAR_Y = 70, BAR_W = 140, BAR_H = 8;

    // Background track
    tft.fillRect(BAR_X, BAR_Y, BAR_W, BAR_H, ST77XX_WHITE);

    // Filled portion
    int filled = map(vol, 0, 100, 0, BAR_W);
    tft.fillRect(BAR_X, BAR_Y, filled, BAR_H, ST77XX_GREEN);

    // Volume number — clear old number first
    tft.fillRect(BAR_X, BAR_Y + 12, 60, 10, ST77XX_BLACK);
    tft.setCursor(BAR_X, BAR_Y + 12);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(1);

    if (isMuted) {
        tft.print("MUTED");
    } else {
        tft.print("VOL: ");
        tft.print(vol);
    }
}

void setup() {
    Serial.begin(115200);

    // Encoder pins
    pinMode(ENC_CLK, INPUT_PULLUP);
    pinMode(ENC_DT,  INPUT_PULLUP);
    pinMode(ENC_SW,  INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ENC_CLK), encoderISR,  CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_SW),  buttonISR,   FALLING);

    tft.initR(INITR_BLACKTAB);
    tft.setRotation(1);
    tft.fillScreen(ST77XX_BLACK);
    Serial.println("TFT Initialized!");

    WiFi.begin(SSID, PASSWORD);
    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nConnected!");

    tft.setCursor(0, 0);
    tft.write(WiFi.localIP().toString().c_str());

    sp.begin();
    while (!sp.is_auth()) {
        sp.handle_client();
    }
    Serial.println("Authenticated");

    drawVolumeBar(currentVolume);
}

void loop() {
    // ── 1. Handle encoder rotation ─────────────────────────────────────────
    if (encoderDelta != 0) {
        noInterrupts();
        int delta = encoderDelta;
        encoderDelta = 0;
        interrupts();

        if (!isMuted) {
            // Each detent = 2% volume; clamp to 0–100
            currentVolume = constrain(currentVolume + delta * 2, 0, 100);
        }
        Serial.println("Volume: " + String(currentVolume));
        drawVolumeBar(currentVolume);
    }

    // ── 2. Handle button press (mute toggle) ───────────────────────────────
    if (buttonPressed) {
        buttonPressed = false;
        delay(30); // simple debounce

        isMuted = !isMuted;
        if (isMuted) {
            volumeBeforeMute = currentVolume;
            sp.setVolume(0);
        } else {
            currentVolume = volumeBeforeMute;
            sp.setVolume(currentVolume);
        }
        drawVolumeBar(currentVolume);
        Serial.println(isMuted ? "Muted" : "Unmuted");
    }

    // ── 3. Push volume to Spotify if it changed ────────────────────────────
    if (currentVolume != lastVolume && !isMuted) {
        sp.setVolume(currentVolume);
        lastVolume = currentVolume;
    }

    // ── 4. Poll Spotify for track info (every 2s) ─────────────────────────
    String currentArtist    = sp.current_artist_names();
    String currentTrackname = sp.current_track_name();

    if (lastArtist != currentArtist &&
        currentArtist != "Something went wrong" &&
        !currentArtist.isEmpty())
    {
        tft.fillRect(0, 0, 160, 65, ST77XX_BLACK); // clear track area only
        lastArtist = currentArtist;
        Serial.println("Artist: " + lastArtist);
        tft.setCursor(10, 10);
        tft.setTextColor(ST77XX_WHITE);
        tft.write(lastArtist.c_str());
    }

    if (lastTrackname != currentTrackname &&
        currentTrackname != "Something went wrong" &&
        currentTrackname != "null")
    {
        lastTrackname = currentTrackname;
        Serial.println("Track: " + lastTrackname);
        tft.setCursor(10, 40);
        tft.setTextColor(ST77XX_WHITE);
        tft.write(lastTrackname.c_str());
    }

    delay(2000);
}