/*
 * ============================================================================
 *  Shorts / TikTok Bluetooth Remote  -  M5StickC Plus2
 *  Tuned for: Samsung Galaxy S22 Ultra (Android / One UI)
 * ============================================================================
 *  Works as a Bluetooth MOUSE and reproduces the native touch gestures that
 *  TikTok and YouTube Shorts already understand. No phone settings needed on
 *  Android beyond pairing -- the cursor appears automatically.
 *
 *    Button A (big front button)
 *        tap   -> SKIP   (swipe up = next video)
 *        hold  -> toggle AUTOSCROLL on/off
 *
 *    Button B (side button)
 *        single press -> PAUSE / PLAY   (single tap)
 *        double press -> LIKE           (double tap = native like gesture)
 *        hold         -> SPEED UP        (press-and-hold; release = normal)
 *
 *  Libraries required (Library Manager):  M5Unified  and  ESP32-BLE-Mouse (by T-vK)
 *
 *  If a tap ever lands on the wrong spot, nudge TAP_RIGHT_STEPS / TAP_DOWN_STEPS
 *  below. Everything else should just work on the S22 Ultra.
 * ============================================================================
 */

// #define USE_NIMBLE      // <- uncomment ONLY if ESP32-BLE-Mouse won't compile
                           //    (then also install the "NimBLE-Arduino" library)

#include <M5Unified.h>
#include <BleMouse.h>

// ----------------------------- CONFIG ---------------------------------------
#define DEVICE_NAME      "Shorts Remote"

// --- Cursor homing (relative-move counts). Tuned for S22 Ultra. ---
const int SLAM_MOVES       = 30;   // pins the cursor into the top-left corner
const int STEP_PX          = 60;   // counts per homing step
const int TAP_RIGHT_STEPS  = 1;    // -> tap point sits center-left of the video
const int TAP_DOWN_STEPS   = 1;    // -> tap point sits in the upper-middle area
const int SWIPE_START_DOWN = 2;    // how far down the skip-swipe begins

// --- Skip (swipe-up) gesture ---
const bool USE_WHEEL_SKIP  = false; // set true to try scroll-wheel skip instead
const int  WHEEL_AMOUNT    = -3;    // wheel notches per skip (flip sign if wrong way)
const int  SWIPE_STEPS     = 26;    // length of the upward drag
const int  SWIPE_PX        = 48;    // upward counts per drag step
const int  SWIPE_DELAY     = 10;    // ms between drag steps (slow = clean single skip)

// --- Button timing ---
const uint16_t A_HOLD_MS   = 700;   // hold A -> skip previous
const uint16_t B_HOLD_MS   = 400;   // hold B -> speed up

const bool BEEP = true;

// ----------------------------- STATE ----------------------------------------
BleMouse bleMouse(DEVICE_NAME, "M5Stack", 100);
bool      speedHold  = false;
bool      lastConn   = false;
String    lastAction = "ready";
unsigned long lastInputTime = 0;
bool      screenDimmed = false;

// ----------------------------- HELPERS --------------------------------------
void beep(int f = 2000, int ms = 40) { if (BEEP) M5.Speaker.tone(f, ms); }

void slamCorner() {                       // pin cursor to the top-left corner
  for (int i = 0; i < SLAM_MOVES; i++) { bleMouse.move(-127, -127); delay(3); }
}

void homeTap() {                          // move to a safe neutral spot on the video
  slamCorner();
  for (int i = 0; i < TAP_RIGHT_STEPS; i++) { bleMouse.move(STEP_PX, 0); delay(4); }
  for (int i = 0; i < TAP_DOWN_STEPS;  i++) { bleMouse.move(0, STEP_PX); delay(4); }
}

void skip() {
  if (USE_WHEEL_SKIP) {
    for (int i = 0; i < 3; i++) { bleMouse.move(0, 0, WHEEL_AMOUNT); delay(20); }
    return;
  }
  slamCorner();                           // start lower-center, then drag up
  for (int i = 0; i < TAP_RIGHT_STEPS;  i++) { bleMouse.move(STEP_PX, 0); delay(4); }
  for (int i = 0; i < SWIPE_START_DOWN; i++) { bleMouse.move(0, STEP_PX); delay(4); }
  bleMouse.press(MOUSE_LEFT);
  for (int i = 0; i < SWIPE_STEPS; i++) { bleMouse.move(0, -SWIPE_PX); delay(SWIPE_DELAY); }
  bleMouse.release(MOUSE_LEFT);
}

void previous() {
  // Same start position as skip, but swipe DOWN (and start 30px higher)
  slamCorner();
  for (int i = 0; i < TAP_RIGHT_STEPS;  i++) { bleMouse.move(STEP_PX, 0); delay(4); }
  for (int i = 0; i < SWIPE_START_DOWN; i++) { bleMouse.move(0, STEP_PX); delay(4); }
  // Move up 30px (0.5 steps) to raise starting position
  bleMouse.move(0, -30); delay(4);
  bleMouse.press(MOUSE_LEFT);
  for (int i = 0; i < SWIPE_STEPS; i++) { bleMouse.move(0, SWIPE_PX); delay(SWIPE_DELAY); }
  bleMouse.release(MOUSE_LEFT);
}

void tapCenter()       { homeTap(); bleMouse.click(MOUSE_LEFT); }
void doubleTapCenter() { homeTap(); bleMouse.click(MOUSE_LEFT); delay(90); bleMouse.click(MOUSE_LEFT); }
void speedStart()      { homeTap(); bleMouse.press(MOUSE_LEFT); }
void speedEnd()        { bleMouse.release(MOUSE_LEFT); }

// ----------------------------- DISPLAY --------------------------------------

// 16-bit colour helpers (RGB565)
#define COL_STATUSBAR  0x1082   // very dark grey strip
#define COL_FOOTERBAR  0x2104   // dark grey footer strip
#define COL_DIVIDER    0x2104   // subtle separator line
#define COL_BOX_BG     0x1082   // action box background
#define COL_AUTO_ON_BG 0x0320   // dark green for AUTO ON badge
#define COL_AUTO_OFF_BG 0x2104  // dark grey for AUTO off badge
#define COL_AUTO_OFF_TXT 0x4208 // dim text for AUTO off label

void drawUI() {
  auto& d = M5.Display;
  bool c = bleMouse.isConnected();
  int  bat = M5.Power.getBatteryLevel();

  d.fillScreen(TFT_BLACK);
  d.setTextDatum(TL_DATUM);

  // ── status bar ──────────────────────────────────────────────────────────
  d.fillRect(0, 0, d.width(), 18, COL_STATUSBAR);

  // BT indicator dot
  d.fillCircle(9, 9, 4, c ? TFT_GREEN : TFT_ORANGE);

  d.setTextSize(1);
  d.setTextColor(c ? TFT_GREEN : TFT_ORANGE);
  d.drawString(c ? "linked" : "pairing", 18, 5);

  // Version number in top right
  d.setTextColor(TFT_WHITE);
  d.drawString("v1.1.0", d.width() - 36, 5);

  // ── title ────────────────────────────────────────────────────────────────
  d.setTextSize(1);
  d.setTextColor(0x4208);  // very dim
  d.drawString("SHORTS REMOTE", 6, 24);

  // ── divider ──────────────────────────────────────────────────────────────
  d.drawFastHLine(0, 34, d.width(), COL_DIVIDER);

  // ── footer bar with battery indicator ────────────────────────────────────

  // ── last action box ──────────────────────────────────────────────────────
  d.setTextSize(1);
  d.setTextColor(TFT_DARKGREY);
  d.drawString("LAST ACTION", 6, 80);

  d.fillRoundRect(6, 90, d.width() - 12, 30, 4, COL_BOX_BG);

  // Action text centred in the box
  d.setTextSize(2);
  d.setTextColor(TFT_WHITE);
  d.setTextDatum(MC_DATUM);
  d.drawString(lastAction, d.width() / 2, 105);
  d.setTextDatum(TL_DATUM);

  // ── divider ──────────────────────────────────────────────────────────────
  d.drawFastHLine(0, 127, d.width(), COL_DIVIDER);

  // ── speed-hold indicator (only visible when active) ──────────────────────
  if (speedHold) {
    d.fillRoundRect(6, 132, d.width() - 12, 14, 3, 0x6200);  // dark orange
    d.setTextSize(1);
    d.setTextColor(TFT_ORANGE);
    d.setTextDatum(MC_DATUM);
    d.drawString("HOLDING SPEED+", d.width() / 2, 139);
    d.setTextDatum(TL_DATUM);
  }

  // ── button hints ─────────────────────────────────────────────────────────
  int hintY = speedHold ? 152 : 135;
  d.setTextSize(1);
  d.setTextColor(TFT_WHITE);  // Bright white for readability
  d.drawString("A: tap=skip  hold=prev", 6, hintY);
  d.drawString("B: tap=pause  double=like", 6, hintY + 14);
  d.drawString("    hold=speed+", 6, hintY + 28);

  // ── battery indicator at bottom of screen ──────────────────────────────────
  int batY = d.height() - 11;
  String batStr = String(bat) + "%";
  
  // Calculate battery width based on screen width (full bar = ~100px)
  int fullBarWidth = 100;
  int barWidth = (bat * fullBarWidth) / 100;
  if (barWidth < 2) barWidth = 2;  // minimum width
  
  d.setTextColor(bat > 20 ? TFT_GREEN : TFT_RED);
  d.fillRect(6, batY - 3, fullBarWidth + 10, 10, TFT_DARKGREY);  // battery background
  d.fillRect(7, batY - 2, 8, 8, TFT_DARKGREY);  // battery container
  d.fillRect(8, batY - 1, barWidth, 6, TFT_GREEN);  // charge level (green)
  d.fillRect(8 + barWidth, batY - 1, fullBarWidth - barWidth, 6, TFT_BLACK);  // empty part
  d.fillRect(d.width() - 15, batY - 3, 4, 4, TFT_DARKGREY);  // battery terminal
  d.setTextSize(1);
  d.setTextColor(TFT_WHITE);
  d.drawString(batStr, 20, batY);
}

// --------------------------- SETUP / LOOP -----------------------------------
void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(0);

  M5.BtnA.setHoldThresh(A_HOLD_MS);
  M5.BtnB.setHoldThresh(B_HOLD_MS);

  // Set screen brightness to max initially
  M5.Lcd.setBrightness(255);
  
  bleMouse.begin();
  drawUI();
}

void loop() {
  M5.update();

  // Track input time (button presses or screen updates)
  unsigned long now = millis();
  if (M5.BtnA.wasClicked() || M5.BtnA.wasHold() || M5.BtnB.wasHold() || M5.BtnB.wasReleased() || M5.BtnB.wasDoubleClicked() || M5.BtnB.wasSingleClicked()) {
    lastInputTime = now;
    if (screenDimmed) {
      screenDimmed = false;
      M5.Lcd.setBrightness(255);
    }
  }

  // Dim screen after 10 seconds of no input (10000ms)
  if (!screenDimmed && now - lastInputTime >= 10000) {
    screenDimmed = true;
    M5.Lcd.setBrightness(25);  // 10% brightness
  }

  bool c = bleMouse.isConnected();
  if (c != lastConn) { lastConn = c; lastInputTime = now; drawUI(); }
  if (!c) { delay(20); return; }          // idle until the phone connects

  // ---- Button A : tap = skip, hold = previous (skip backward) ----
  if (M5.BtnA.wasClicked()) { skip(); lastAction = "Skip"; beep(2200); lastInputTime = now; drawUI(); }
  if (M5.BtnA.wasHold())    { previous(); delay(500); lastAction = "Previous"; beep(800); lastInputTime = now; drawUI(); }

  // ---- Button B : hold = speed up, release = normal ----
  if (M5.BtnB.wasHold())                  { speedStart(); speedHold = true;  lastAction = "Speed+";    beep(1500); lastInputTime = now; drawUI(); }
  if (speedHold && M5.BtnB.wasReleased()) { speedEnd();   speedHold = false; lastAction = "Speed off"; beep(1200); lastInputTime = now; drawUI(); }

  // ---- Button B : single = pause, double = like ----
  if (!speedHold) {
    if (M5.BtnB.wasDoubleClicked())      { doubleTapCenter(); lastAction = "Like";  beep(2800); lastInputTime = now; drawUI(); }
    else if (M5.BtnB.wasSingleClicked()) { tapCenter();       lastAction = "Pause"; beep(1800); lastInputTime = now; drawUI(); }
  }

  // Update battery indicator every 5 seconds for real-time updates
  static unsigned long lastBatUpdate = 0;
  if (now - lastBatUpdate >= 5000) {
    lastBatUpdate = now;
    lastInputTime = now;
    drawUI();
  }

  delay(5);
}
