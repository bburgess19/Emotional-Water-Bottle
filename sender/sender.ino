#include "math.h"
#include "HX711.h"
#define LOADCELL_DOUT_PIN  0
#define LOADCELL_SCK_PIN  8

enum ConnectionState { CONNECTED, DISCONNECTED };

/*** GLOBAL CONSTANTS ***/
const int PING_TIMEOUT = 200;
const int PING_MAX = 5;
const int HAPPINESS_COOLDOWN = 1000;
const float SIP_THRESHOLD = 0.1;

/*** STATE VARIABLES ***/
ConnectionState state = DISCONNECTED;
unsigned long lastPingTime = 0;
unsigned long lastHappinessCooldownTime = 0;
unsigned long outOfRangePingCount = 0;
int waterLevel = 0;
int happinessLevel = 0;
int bottleMin = 0;
int bottleMax = 0;
bool isCalibrationMode = false;
bool isBottleLifted = false;

/*** LOAD CELL VARIABLES ***/
HX711 scale;
float calibration_factor = -110000;

void setup() 
{
  Serial1.begin(115200);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale();
  scale.tare();
}

void loop() 
{
  unsigned long currentTime = millis();
  // Every 200ms, always send [255, x, y]

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
  if (isCalibrationMode) return;

  unsigned long currentTime = millis();
  if (!isCalibrationMode) {
    int weight = scale.get_units(10);
    // Serial.print("Read weight: ");
    // Serial.print(weight);
    // Serial.print(" Water level: ");
    // Serial.println(waterLevel);
    if (weight < waterLevel - SIP_THRESHOLD) {
      happinessLevel += 20; // Increase happiness if the user drinks water
      lastHappinessCooldownTime = currentTime;
    } else if (currentTime - lastHappinessCooldownTime > HAPPINESS_COOLDOWN) {
      happinessLevel--; // Decrease happiness if the user hasn't drank water in a while
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
  checkForAck();
}

/*
 * Checks for an acknowledgement from the wristband
 */
void checkForAck()
{
  if (Serial1.available()) {
    if (Serial1.read() == 255) {
      outOfRangePingCount = 0;
      state = CONNECTED;
      Serial.println("ack");
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
