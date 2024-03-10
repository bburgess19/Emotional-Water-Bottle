#include "math.h"
#include "HX711.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define LOADCELL_DOUT_PIN  0
#define LOADCELL_SCK_PIN  8
#define VIBROMOTOR_PIN 1
#define BUTTON_PIN 2
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define USB  // USB or BT

#ifdef USB
#define MySerial Serial  // use this for USB
#endif

#ifdef BT
#define MySerial Serial1  // use this for Bluetooth: connect HC-05's RX to Xiao's TX (6), HC-05's TX to Xiao's RX (7)
#endif

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

enum ConnectionState { CONNECTED, DISCONNECTED };
enum CalibrationState { Initial, CalibrateLow, CalibrateHigh };

/*** GLOBAL CONSTANTS ***/
const int PING_TIMEOUT = 200;
const int PING_MAX = 5;
const int HAPPINESS_COOLDOWN = 1000;// * 60;
const float SIP_THRESHOLD = 0.1;
const int GESTURE_DURATION = 1000;
const int GESTURE_COOLDOWN = 200;
const int DIAG_DURATION = 3000;
const int MAX_PING_INTERVAL = 1000;

/*** STATE VARIABLES ***/
ConnectionState state = DISCONNECTED;
unsigned long lastPingTime = 0;
unsigned long lastHappinessCooldownTime = 0;
unsigned long outOfRangePingCount = 0;
unsigned long gestureStartTime = 0;
unsigned long gestureCooldownStartTime = 0;
int waterLevel = 0;
int happinessLevel = 0;
float bottleMin = 0;
float bottleMax = 0;
bool isCalibrating = false;
bool isBottleLifted = false;
bool isDiagShowing = false;
bool didStartGesture = false;
bool didReleaseDuringGesture = false;
bool didNotifyDistanceError = false;
CalibrationState calibrationState = Initial;

/*** LOAD CELL VARIABLES ***/
HX711 scale;
float calibration_factor = -110000;

void setup() 
{
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  MySerial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale();
  scale.tare();
  display.display();
}

void loop() 
{
  updateStatus();
  handleButtonGesture();
}

/*
 * Updates the status of the wristband
 */
void updateStatus()
{
  unsigned long currentTime = millis();
  float weight = scale.get_units(10);
  waterLevel = (weight - bottleMin) / (bottleMax - bottleMin) * 100;
  if (weight < waterLevel - SIP_THRESHOLD) {
    happinessLevel = min(happinessLevel + 20, 100); // Increase happiness if the user drinks water
    lastHappinessCooldownTime = currentTime;
  } else if (currentTime - lastHappinessCooldownTime > HAPPINESS_COOLDOWN) {
    happinessLevel = max(happinessLevel - 1, 0); // Decrease happiness if the user hasn't drank water in a while
    lastHappinessCooldownTime = currentTime;
  }
}

/*
 * Sends a ping to the wristband
 */
void pingWristband(unsigned long currentTime)
{
  lastPingTime = currentTime;
  sendStatus();
  checkForMessage();
}

/*
 * Checks for messages from the wristband
 */
void checkForMessage()
{
  if (MySerial.available()) {
    if (MySerial.read() == 255) {
      byte code = MySerial.read();

      switch (code) {
        case 'a':
          handleAck();
          break;
        case 'c':
          if (!isCalibrating) {
            beginCalibration();
          } else {
            calibrationHandler();
          }
          break;
      }
    } else { 
      outOfRangePingCount++;
    }
  }
}

/* 
 * Reports the current status to the wristband
 */
void sendStatus()
{
  MySerial.write(255);
  MySerial.write(waterLevel);
  MySerial.write(happinessLevel);
}

/* Handles the acknowledgement from the wristband */
void handleAck() {
  outOfRangePingCount = 0;
  state = CONNECTED;
  Serial.println("ack");
}

/* Handles a calibration message */
void calibrationHandler() {
  int weight = scale.get_units(10);
  switch (calibrationState) {
    case Initial:
      bottleMin = weight;
      calibrationState = CalibrateLow;
      drawCalibrationState();
      break;
    case CalibrateLow:
      bottleMax = weight;
      calibrationState = CalibrateHigh;
      drawCalibrationState();
      break;
    default:
      isCalibrating = false;
      calibrationState = Initial;
      drawFace();
  }
}

/* Initializes calibration mode */
void beginCalibration() {
  isCalibrating = true;
  calibrationState = Initial;
  drawCalibrationState();
}

/* Draws the instructions for the calibration states */
void drawCalibrationState() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);

  switch (calibrationState) {
    case Initial:
      display.print("Place empty bottle");
      display.display();
      break;
    case CalibrateLow:
      display.print("Place full bottle");
      display.display();
      break;
    default:
      display.clearDisplay();
      break;
  }
}

/* Handles all button gestures.
 *
 * The functionality is twofold:
 * 1. Show diagnostics with a single button press
 * 2. Start calibration with a double button press
 * 3. Turn off pairing with a long press
 */
void handleButtonGesture() {
  unsigned long currentTime = millis();
  int isHeld = digitalRead(BUTTON_PIN);
  
  // Exit early for a cooldown
  if (currentTime - gestureCooldownStartTime < GESTURE_COOLDOWN)
    return;

  // Begin a gesture if it hasn't started yet
  if (isHeld && !didStartGesture) {
    didStartGesture = true;
    gestureStartTime = currentTime;
    digitalWrite(LED_BUILTIN, HIGH);
    vibrate();
    return;
  }

  // Exit early if this is not a gesture
  if (!didStartGesture) return;

  if (isHeld) {
    // End the gesture after the timeout
    if (currentTime - gestureStartTime > GESTURE_DURATION) {
      if (isCalibrating) {
        calibrationHandler();
      }

      printDiagnostics();
      
      didStartGesture = false;
      gestureCooldownStartTime = currentTime;
      digitalWrite(LED_BUILTIN, LOW);
      stopVibrating();
    }

    if (didReleaseDuringGesture) {
      if (!isCalibrating) {
        // Start calibration
        beginCalibration();
        didStartGesture = false;
        digitalWrite(LED_BUILTIN, LOW);
        stopVibrating();
      }
    }
  } else {
      didReleaseDuringGesture = true;
  }
}

/* Vibrates the vibromotor */
void vibrate() {
  digitalWrite(VIBROMOTOR_PIN, HIGH);
}

/* Stops the vibration */
void stopVibrating() {
  digitalWrite(VIBROMOTOR_PIN, LOW);
}

/* Clears the diagnostics */
void clearDiagnostics() {
  display.fillRect(60, 0, 40, 30, BLACK);
}

/* Prints diagnostics to the display */
void printDiagnostics() {
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 10);
    display.print("Water %: ");
    display.print(waterLevel);
    display.print(0, 20);
    display.print("Happiness: ");
    display.print(happinessLevel);
    display.display();
    isDiagShowing = true;
}

/* Draws the face for the bottle */
void drawFace() {
  display.clearDisplay();
  if (happinessLevel > 66) {
    drawHappyFace();
  } else if (happinessLevel > 33) {
    drawWorriedFace();
  } else {
    drawSadFace();
  }
}

/* Draws the happy face */
void drawHappyFace() {
  display.clearDisplay();

  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,20);
  display.print("(^ w ^)");
  display.display();
}

/* Draws the sad face */
void drawSadFace() {
  display.clearDisplay();

  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,20);
  display.print("(T o T)");
  display.display();
}

/* Draws the worried face */
void drawWorriedFace() {
  display.clearDisplay();

  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,20);
  display.print("(. ~ .)");
  display.display();
}
