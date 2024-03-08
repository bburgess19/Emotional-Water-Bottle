#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3D
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define LED1_PIN 1
#define LED2_PIN 2

bool pinOn = false


void setup() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  int program_time = millis();
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  display.display()
}

void loop() {

if (millis() % 500 == 0) {
      if (pinOn) {
        digitalWrite(LED1_PIN, HIGH);
        digitalWrite(LED2_PIN, LOW);
        pinOn = false
      } else {
        digitalWrite(LED1_PIN, LOW);
        digitalWrite(LED2_PIN, HIGH);
        pinOn = true
      }
    }

  if (millis() - program_time >= 15000) {
    program_time = millis()
  } else if (millis() - program_time >= 10000) {

    drawWorriedFace()

  } else if (millis() - program_time >= 5000) {

    drawSadFace()

  } else if (millis() - program_time >= 0) {

    drawHappyFace()
  }
}


void drawHappyFace() {
  display.clearDisplay();

  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,20);
  display.print("(^ w ^)");
  display.display();
}

void drawSadFace() {
  display.clearDisplay();

  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,20);
  display.print("(T o T)");
  display.display();
}

void drawWorriedFace() {
  display.clearDisplay();

  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,20);
  display.print("(. ~ .)");
  display.display();
}




