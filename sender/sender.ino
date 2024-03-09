#include "math.h"
#include "HX711.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define LOADCELL_DOUT_PIN  0
#define LOADCELL_SCK_PIN  8
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

enum ConnectionState { CONNECTED, DISCONNECTED };
enum CalibrationState { Initial, CalibrateLow, CalibrateHigh };

/*** GLOBAL CONSTANTS ***/
const int PING_TIMEOUT = 200;
const int PING_MAX = 5;
const int HAPPINESS_COOLDOWN = 1000;// * 60;
const float SIP_THRESHOLD = 0.1;

/*** STATE VARIABLES ***/
ConnectionState state = DISCONNECTED;
unsigned long lastPingTime = 0;
unsigned long lastHappinessCooldownTime = 0;
unsigned long outOfRangePingCount = 0;
int waterLevel = 0;
int happinessLevel = 0;
float bottleMin = 0;
float bottleMax = 0;
bool isCalibrating = false;
CalibrationState calibrationState = Initial;
bool isBottleLifted = false;

/*** LOAD CELL VARIABLES ***/
HX711 scale;
float calibration_factor = -110000;

void setup() 
{
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  Serial1.begin(115200);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale();
  scale.tare();
  display.display();
}

void loop() 
{
  unsigned long currentTime = millis();
  // Change to disconnected state if we haven't received a ping in a while
  if (outOfRangePingCount > PING_MAX) {
    state = DISCONNECTED;
    outOfRangePingCount = 0;
  }

  if (currentTime - lastPingTime > PING_TIMEOUT) {
    pingWristband(currentTime);
  }

  updateStatus();
}

/*
 * Updates the status of the wristband
 */
void updateStatus()
{
  if (isCalibrating) return;

  unsigned long currentTime = millis();
  if (!isCalibrating) {
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
  if (Serial1.available()) {
    if (Serial1.read() == 255) {
      byte code = Serial1.read();

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
  Serial1.write(255);
  Serial1.write(waterLevel);
  Serial1.write(happinessLevel);
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
      drawCalibrationState();
      break;
    case CalibrateLow:
      bottleMax = weight;
      drawCalibrationState();
      break;
    default:
      isCalibrating = false;
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
