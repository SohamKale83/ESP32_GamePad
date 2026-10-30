#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HijelHID_BLEKeyboard.h>

// =====================================================
// OLED
// =====================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// =====================================================
// BLUETOOTH KEYBOARD
// =====================================================

HijelHID_BLEKeyboard keyboard("ESP32 Game Remote", "ESP32", 100);

// =====================================================
// BATTERY
// =====================================================

const int ANALOG_PIN = 36;

float battery_voltage = 0.0;
int battery_percent = 0;

// Example limits for a single-cell Li-ion/LiPo battery.
// Change these if your battery uses a different configuration.

const float BATTERY_EMPTY_V = 3.30;
const float BATTERY_FULL_V  = 4.20;

// Battery measurement timing
unsigned long lastBatteryRead = 0;
const unsigned long BATTERY_INTERVAL = 500;

// =====================================================
// BUTTON PINS
// =====================================================

const int lsw1 = 4;    // Q
const int lsw2 = 16;   // A
const int lsw3 = 17;
const int lsw4 = 5;

const int rsw1 = 13;
const int rsw2 = 26;
const int rsw3 = 27;
const int rsw4 = 14;

// Joystick click buttons
const int leftSW_pin  = 19;  // Enter
const int rightSW_pin = 18;  // Space

// =====================================================
// JOYSTICK PINS
// =====================================================

const int leftX_pin  = 32;
const int leftY_pin  = 33;

const int rightX_pin = 34;
const int rightY_pin = 35;

// =====================================================
// JOYSTICK SETTINGS
// =====================================================

const int LOW_THRESHOLD  = 1400;
const int HIGH_THRESHOLD = 2700;

const bool INVERT_LEFT_Y  = true;
const bool INVERT_RIGHT_X = false;

// =====================================================
// KEY STATES
// =====================================================

bool wasW = false;
bool wasS = false;
bool wasA = false;
bool wasD = false;

bool wasQ = false;
bool wasButtonA = false;
bool wasEnter = false;
bool wasSpace = false;

// =====================================================
// JOYSTICK READINGS
// =====================================================

int lJoyX = 0;
int lJoyY = 0;

int rJoyX = 0;
int rJoyY = 0;

// =====================================================
// KEYBOARD HANDLER
// =====================================================

void updateKey(uint8_t key, bool wanted, bool &previous) {

  if (wanted == previous) {
    return;
  }

  if (wanted) {
    keyboard.press(key);
  } else {
    keyboard.release(key);
  }

  previous = wanted;
}

// =====================================================
// BATTERY READING
// =====================================================

void readBattery() {

  uint32_t total = 0;

  for (int i = 0; i < 15; i++) {
    total += analogRead(ANALOG_PIN);
    delay(1);
  }

  float analog_value = total / 15.0;

  float pin_voltage = analog_value * (3.3 / 4095.0);

  // Your original voltage-divider multiplier
  battery_voltage = pin_voltage * 6.45;

  // Approximate voltage-based percentage
  float percent =
      ((battery_voltage - BATTERY_EMPTY_V) /
       (BATTERY_FULL_V - BATTERY_EMPTY_V)) * 100.0;

  percent = constrain(percent, 0, 100);

  battery_percent = (int)(percent + 0.5);
}

// =====================================================
// BATTERY ICON
// =====================================================

void drawBattery(int x, int y, int percent) {

  // Battery outline
  display.drawRoundRect(x, y, 19, 9, 2, SSD1306_WHITE);

  // Battery terminal
  display.fillRect(x + 19, y + 3, 2, 3, SSD1306_WHITE);

  // Fill level
  int fillWidth = map(percent, 0, 100, 0, 13);

  if (fillWidth > 0) {
    display.fillRect(
      x + 3,
      y + 3,
      fillWidth,
      3,
      SSD1306_WHITE
    );
  }
}

// =====================================================
// BLUETOOTH STATUS
// =====================================================

void drawBluetooth(bool connected) {

  display.setCursor(75, 1);
  display.print("BT:");

  if (connected) {
    display.print("ON");
  } else {
    display.print("WAIT");
  }
}

// =====================================================
// JOYSTICK GRAPHICS
// =====================================================

// directionX: -1 left, 0 center, +1 right
// directionY: -1 up,   0 center, +1 down

void drawJoystick(int cx, int cy, int directionX, int directionY) {

  // Outer circle
  display.drawCircle(cx, cy, 7, SSD1306_WHITE);

  // Direction cross
  display.drawFastHLine(cx - 4, cy, 9, SSD1306_WHITE);
  display.drawFastVLine(cx, cy - 4, 9, SSD1306_WHITE);

  // Position indicator
  int dotX = cx + directionX * 3;
  int dotY = cy + directionY * 3;

  display.fillCircle(dotX, dotY, 2, SSD1306_WHITE);
}

// =====================================================
// BUTTON STATUS
// =====================================================

void drawButton(int x, int y, const char *label, bool pressed) {

  display.setCursor(x, y);

  display.print(label);
  display.print(':');

  if (pressed) {
    display.print("ON");
  } else {
    display.print("--");
  }
}

// =====================================================
// SINGLE FIXED OLED DASHBOARD
// =====================================================

void updateOLED(
  bool connected,
  bool forward,
  bool backward,
  bool steerLeft,
  bool steerRight,
  bool qButton,
  bool aButton,
  bool enterButton,
  bool spaceButton
) {

  // Draw everything into the display buffer.
  // Only ONE display.display() call is made.

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  // ---------------------------------------------------
  // ROW 1: TITLE AND BLUETOOTH
  // ---------------------------------------------------

  display.setCursor(0, 1);
  display.print("GAME REMOTE");

  drawBluetooth(connected);

  display.drawFastHLine(0, 10, 128, SSD1306_WHITE);

  // ---------------------------------------------------
  // ROW 2: BATTERY
  // ---------------------------------------------------

  drawBattery(2, 12, battery_percent);

  display.setCursor(26, 13);
  display.print(battery_percent);
  display.print('%');

  display.setCursor(69, 13);
  display.print(battery_voltage, 2);
  display.print('V');

  display.drawFastHLine(0, 23, 128, SSD1306_WHITE);

  // ---------------------------------------------------
  // ROW 3: BOTH JOYSTICKS
  // ---------------------------------------------------

  display.setCursor(9, 25);
  display.print("LEFT");

  display.setCursor(78, 25);
  display.print("RIGHT");

  // Left joystick: forward/backward
  int leftDirectionY = 0;

  if (forward) {
    leftDirectionY = -1;
  } else if (backward) {
    leftDirectionY = 1;
  }

  // Right joystick: left/right steering
  int rightDirectionX = 0;

  if (steerLeft) {
    rightDirectionX = -1;
  } else if (steerRight) {
    rightDirectionX = 1;
  }

  drawJoystick(22, 38, 0, leftDirectionY);
  drawJoystick(90, 38, rightDirectionX, 0);

  // Left joystick text
  display.setCursor(34, 35);

  if (forward) {
    display.print("UP");
  } else if (backward) {
    display.print("DOWN");
  } else {
    display.print("STOP");
  }

  // Right joystick text
  display.setCursor(101, 35);

  if (steerLeft) {
    display.print("LEFT");
  } else if (steerRight) {
    display.print("RIGHT");
  } else {
    display.print("STOP");
  }

  // ---------------------------------------------------
  // ROW 4: MOVEMENT SUMMARY
  // ---------------------------------------------------

  display.drawFastHLine(0, 47, 128, SSD1306_WHITE);

  display.setCursor(0, 49);
  display.print("MOVE:");

  bool anyMovement = false;

  if (forward) {
    display.print(" UP");
    anyMovement = true;
  }

  if (backward) {
    display.print(" DOWN");
    anyMovement = true;
  }

  if (steerLeft) {
    display.print(" LEFT");
    anyMovement = true;
  }

  if (steerRight) {
    display.print(" RIGHT");
    anyMovement = true;
  }

  if (!anyMovement) {
    display.print(" STOP");
  }

  // ---------------------------------------------------
  // ROW 5: ALL FOUR BUTTONS — FIXED POSITION
  // ---------------------------------------------------

  display.drawFastHLine(0, 56, 128, SSD1306_WHITE);

  // Move text up so all characters fit within 64 pixels
  drawButton(1, 57, "Q", qButton);
  drawButton(29, 57, "A", aButton);
  drawButton(54, 57, "ENT", enterButton);
  drawButton(94, 57, "SP", spaceButton);

  // Display the complete dashboard
  display.display();
}

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  // Initialize OLED using GPIO 21 (SDA) and GPIO 22 (SCL)
  Wire.begin(21, 22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {

    Serial.println("OLED initialization failed");

    while (true) {
      delay(100);
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(15, 8);
  display.println("ESP32 GAME REMOTE");

  display.setCursor(15, 27);
  display.println("Starting Bluetooth");

  display.setCursor(15, 43);
  display.println("Please wait...");

  display.display();

  // Configure pushbuttons
  pinMode(lsw1, INPUT_PULLUP);
  pinMode(lsw2, INPUT_PULLUP);
  pinMode(lsw3, INPUT_PULLUP);
  pinMode(lsw4, INPUT_PULLUP);

  pinMode(rsw1, INPUT_PULLUP);
  pinMode(rsw2, INPUT_PULLUP);
  pinMode(rsw3, INPUT_PULLUP);
  pinMode(rsw4, INPUT_PULLUP);

  // Joystick click buttons
  pinMode(leftSW_pin, INPUT_PULLUP);
  pinMode(rightSW_pin, INPUT_PULLUP);

  // ADC attenuation
  analogSetAttenuation(ADC_11db);

  // Initialize battery reading
  readBattery();
  lastBatteryRead = millis();

  // Start Bluetooth keyboard
  keyboard.begin();

  Serial.println("ESP32 game remote started");
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop() {

  // 1. Read joystick axes
  lJoyX = analogRead(leftX_pin);
  lJoyY = analogRead(leftY_pin);

  rJoyX = analogRead(rightX_pin);
  rJoyY = analogRead(rightY_pin);

  // 2. Read buttons
  bool qButton = !digitalRead(lsw1);
  bool aButton = !digitalRead(lsw2);

  bool enterButton = !digitalRead(leftSW_pin);
  bool spaceButton = !digitalRead(rightSW_pin);

  // 3. Left joystick movement
  bool forward;
  bool backward;

  if (INVERT_LEFT_Y) {
    forward  = lJoyY > HIGH_THRESHOLD;
    backward = lJoyY < LOW_THRESHOLD;
  } else {
    forward  = lJoyY < LOW_THRESHOLD;
    backward = lJoyY > HIGH_THRESHOLD;
  }

  // 4. Right joystick movement
  bool steerLeft;
  bool steerRight;

  if (INVERT_RIGHT_X) {
    steerLeft  = rJoyX > HIGH_THRESHOLD;
    steerRight = rJoyX < LOW_THRESHOLD;
  } else {
    steerLeft  = rJoyX < LOW_THRESHOLD;
    steerRight = rJoyX > HIGH_THRESHOLD;
  }

  // 5. Update battery every 500 milliseconds
  if (millis() - lastBatteryRead >= BATTERY_INTERVAL) {

    readBattery();
    lastBatteryRead = millis();
  }

  // 6. Bluetooth keyboard controls
  bool connected = keyboard.isConnected();

  if (connected) {

    // Left joystick: Up and Down
    updateKey(KEY_UP, forward, wasW);
    updateKey(KEY_DOWN, backward, wasS);

    // Right joystick: Left and Right
    updateKey(KEY_LEFT, steerLeft, wasA);
    updateKey(KEY_RIGHT, steerRight, wasD);

    // Q and A buttons
    updateKey(KEY_Q, qButton, wasQ);
    updateKey(KEY_A, aButton, wasButtonA);

    // Joystick clicks
    updateKey(KEY_RETURN, enterButton, wasEnter);
    updateKey(KEY_SPACE, spaceButton, wasSpace);

  } else {

    // Reset software key states on disconnect
    wasW = false;
    wasS = false;
    wasA = false;
    wasD = false;

    wasQ = false;
    wasButtonA = false;

    wasEnter = false;
    wasSpace = false;
  }

  // 7. Update the single dashboard
  updateOLED(
    connected,
    forward,
    backward,
    steerLeft,
    steerRight,
    qButton,
    aButton,
    enterButton,
    spaceButton
  );

  // Short delay; no screen switching
  delay(30);
}