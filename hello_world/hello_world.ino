// Minimal board test: wipe EEPROM, print Hello World on Serial.
// Open Serial Monitor at 9600 baud.
//
// If you see Hello World, the board + USB serial are fine.
// Built-in LED also blinks on pin 13 (Nano).

#include <EEPROM.h>

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  // Wipe all EEPROM so old pet data is gone
  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.update(i, 0xFF);
  }

  Serial.begin(9600);
  while (!Serial && millis() < 3000) {
    // wait briefly for Serial Monitor (USB boards); Nano continues anyway
  }

  Serial.println();
  Serial.println("Hello World");
  Serial.println("EEPROM cleared");
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("Hello World");
  delay(1000);

  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
}
