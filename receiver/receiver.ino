#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
int LED_PIN = 1;
int BUTTON_PIN = 2;
enum CalibrationState { Initial, CalibrateLow, CalibrateHigh };

/*** GLOBAL CONSTANTS ***/
const int GESTURE_DURATION = 1000;
const int GESTURE_COOLDOWN = 200;
const int DIAG_DURATION = 3000;
const int MAX_PING_INTERVAL = 1000;

/*** STATE VARIABLES ***/
unsigned long lastPingTime = 0;
unsigned long gestureStartTime = 0;
unsigned long gestureCooldownStartTime = 0;
unsigned long diagStartTime = 0;
int waterLevel = 0;
int happinessLevel = 0;
bool isOn = false;
bool isCalibrating = false;
bool isDiagShowing = false;
bool didStartGesture = false;
bool didReleaseDuringGesture = false;
bool didNotifyDistanceError = false;
CalibrationState calibrationState = Initial;

void setup() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  pinMode(LED_PIN, OUTPUT);
  Serial1.begin(115200);
  display.display();
}

void loop() {
  unsigned long currentTime = millis();

  if (Serial1.available()) {
    if (isOn) {
      checkForDisconnection();

      if (Serial1.read() == 255) {
        byte code = Serial1.read();

        switch (code) {
          case 'a':
            break; // empty to allow KEEPALIVE messages
          case 'p':
            waterLevel = int(Serial1.read());
            happinessLevel = int(Serial1.read());
            sendAck();
            break;
          
          case 'v':
            // Vibrate
            break;

          lastPingTime = currentTime;
          didNotifyDistanceIssue = false;
        }
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
      }
    } else {
      // Empty the buffer, no acknowledgments
      while (Serial1.available()) { Serial1.read(); }
    }
  }

  if (isDiagShowing && currentTime - diagStartTime > DIAG_DURATION) {
    hideDiagnostics();
  }

  handleButtonGesture();
}

/* Sends an acknowledgement to the server */
void sendAck() {
  Serial1.write(255);
  Serial1.write('a');
}

/* Hides diagnostics data, resetting any state variables */
void hideDiagnostics() {
  isDiagShowing = false;
  clearDiagnostics();
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
    return;
  }

  // Exit early if this is not a gesture
  if (!didStartGesture) return;

  if (isHeld) {
    // End the gesture after the timeout
    if (currentTime - gestureStartTime > GESTURE_DURATION) {
      if (isCalibrating) {
        sendCalibrationPing();
      }

      // We short circuit the double-press gesture, so we only have to check long presses
      // or just a regular press here
      if (!didReleaseDuringGesture && isHeld) {
        togglePower();
      } else {
        printDiagnostics();
      }

      gestureCooldownStartTime = currentTime;
    }

    if (didReleaseDuringGesture) {
      if (isOn && !isCalibrating) {
        // Start calibration
        beginCalibration();
        didStartGesture = false;
      }
    }
  } else {
      didReleaseDuringGesture = true;
  }
}

/* Initializes calibration mode */
void beginCalibration() {
  isCalibrating = true;
  calibrationState = Initial;
  drawCalibrationState();
}

/* Sends the calibration ping to the bottle */ 
void sendCalibrationPing() {
  Serial1.write(255);
  Serial1.write('c');

  switch (calibrationState) {
    case Initial:
      calibrationState = CalibrateLow;
      break;
    case CalibrateLow:
      calibrationState = CalibrateHigh;
      break;
    case CalibrateHigh:
      isCalibrating = false;
      break;
  }
  drawCalibrationState();
}

/* Toggles the wristbands power state */
void togglePower() {
  if (isOn) {
    isOn = false;
  } else {
    isCalibrating = false;
    isOn = true;
  }
}

/* Vibrates wristband */
void vibrate()
{
  // TODO
}

/* Checks if too much time has passed since receiving a message */
void checkForDisconnection() {
  unsigned long currentTime = millis();
  if (MAX_PING_INTERVAL - currentTime > MAX_PING_INTERVAL && !didNotifyDistanceError) {
    notifyDistanceIssue();
    vibrate();
  }
}

/* Notifies wearer that they are out of range */
void notifyDistanceIssue() {
  display.clear();
  display.setTextSize(2);
  display.setCursor((0, 0);
  display.print("Don't forget me!");
  display.display();
  vibrate();
  didNotifyDistanceError = false;
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
