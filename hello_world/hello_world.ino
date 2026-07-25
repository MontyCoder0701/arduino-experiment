// Button + OLED test.
// Button: D2 to GND. OLED: SDA=A4, SCL=A5, VCC=5V, GND=GND.
// Press = built-in LED and OLED light up.

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

const uint8_t BUTTON_PIN = 2;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();
}

void loop() {
  // INPUT_PULLUP: pressed = LOW
  bool pressed = digitalRead(BUTTON_PIN) == LOW;
  digitalWrite(LED_BUILTIN, pressed ? HIGH : LOW);

  if (pressed) {
    display.fillScreen(WHITE);
  } else {
    display.clearDisplay();
  }
  display.display();
}
