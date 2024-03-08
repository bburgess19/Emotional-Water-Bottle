#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3D
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
int LED_PIN = 1;

/*** STATE VARIABLES ***/
unsigned long outOfRangePingCount = 0;
unsigned long waterLevel = 0;
unsigned long happinessLevel = 0;

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
  if (Serial1.available()) {
    if (Serial1.read() == 255) {
      waterLevel = int(Serial1.read());
      happinessLevel = int(Serial1.read());
      sendAck();
      clearDiagnostics();
      printDiagnostics();
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
    }
  }
}

/* Sends an acknowledgement to the server */
void sendAck() {
  Serial1.write(255);
}

/* Clears the diagnostics */
void clearDiagnostics() {
  display.fillRect(60, 0, 40, 30, BLACK);
}

/* Prints diagnostics to the display */
void printDiagnostics() {
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(60, 0);
    display.print(happinessLevel);
    display.display();
}
