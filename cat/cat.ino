// ADHDBuddy.ino
//
// ADHD companion pet: two reminders that drain over ~20 minutes.
//   MOVE (fun)  -> stretch / walk break
//   H2O  (food) -> drink water
// Wiring: one push button between D2 and GND (uses the internal pull-up).
//   single press  -> logged a movement break
//   double press  -> logged a water sip
//   triple press  -> show MOVE / H2O / care gauges
//   hold 5 seconds -> turn OFF (hold 5s again to turn back ON)
//   hold 10 seconds -> wipe memory and start fresh
#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const uint8_t BUTTON_PIN = 2;

// ==========================================
//   THEMED PETS (drawn procedurally — flash-friendly)
//   Sport  = fox athlete
//   Chonk  = round loaf blob
//   Balanced = classic dog
// ==========================================
const uint8_t SPRITE_W = 32;
const uint8_t SPRITE_H = 20;

enum Pose {
  WALK_R, WALK_L, SIT, SLEEP, GROOM, STRETCH, HAPPY,
  PLAYING, EATING, BORED, HUNGRY, MISERABLE, REJECT
};
Pose pose = SIT;
Pose idlePose = SIT;

const Pose IDLE_CHOICES[] = { WALK_R, WALK_L, SIT, GROOM, STRETCH, HAPPY };
const uint8_t IDLE_CHOICE_COUNT = sizeof(IDLE_CHOICES) / sizeof(IDLE_CHOICES[0]);

int dogX = 48;
const int dogY = 40; // locks pet to the bottom grassline
unsigned long lastIdleChange = 0;
unsigned long lastFrameUpdate = 0;
unsigned long lastFunDrain = 0;
unsigned long lastFoodDrain = 0;
unsigned long lastInteractAt = 0;
unsigned long actionUntil = 0;
Pose actionPose = PLAYING;
uint8_t animPhase = 0; // 0..3 for smoother looping motions
bool animFrame = false; // phase & 1 — kept for blink/gauge helpers
bool rejectWasFood = false; // true = stuffed, false = already too playful
bool showingBonk = false;
unsigned long bonkUntil = 0;
uint8_t goofThought = 0;

// --- NEEDS ENGINE (ADHD reminders) ---
// fun  = movement / break energy
// food = hydration
int fun = 70;
int food = 70;
const int LOW_LEVEL = 25;                 // start nagging
const int CRITICAL_LEVEL = 12;            // loud nag
const int FULL_LEVEL = 90;                // already topped up
// ~20 min from topped (~100) down to LOW (25) ≈ 75 points
const unsigned long FUN_DRAIN_MS = (20UL * 60UL * 1000UL) / 75UL;   // move reminder
const unsigned long FOOD_DRAIN_MS = (20UL * 60UL * 1000UL) / 75UL;  // water reminder
const unsigned long ACTION_MS = 2200;      // celebration after logging move/sip
const unsigned long IDLE_CHANGE_MS = 2400;
const unsigned long NAP_AFTER_MS = 50000;
const unsigned long FRAME_MS = 130;
const unsigned long DAY_MS = 86400000UL;
const unsigned long BATTERY_MS = 2000;

// Board VCC as battery % (uses internal 1.1V bandgap; no extra wiring).
// 5.0V = 100%, 3.3V = 0%.
unsigned long lastBatteryAt = 0;
uint8_t batteryPct = 100;

// --- BUTTON ENGINE ---
// single tap = move break, double tap = drink water, triple tap = gauges
// hold 5s = power toggle (off / on)
// hold 10s = full reset
const unsigned long DEBOUNCE_MS = 40;
const unsigned long MULTI_GAP_MS = 600;     // window between taps
const unsigned long STATS_SHOW_MS = 3500;   // how long gauges stay after triple tap
const unsigned long POWER_HINT_MS = 2000;   // start showing power-off progress
const unsigned long POWER_HOLD_MS = 5000;   // turn off / on
const unsigned long LONG_PRESS_MS = 10000;  // factory reset
bool rawButton = HIGH;
bool stableButton = HIGH;
unsigned long lastButtonEdge = 0;
uint8_t clickCount = 0;
unsigned long lastClickAt = 0;
unsigned long holdStartedAt = 0;
unsigned long holdMs = 0;
bool longPressDone = false;
bool showingReset = false;
bool showingLevelUp = false;
unsigned long levelUpUntil = 0;
unsigned long statsUntil = 0;    // gauges visible until this time
bool powerOn = true;             // false = screen off / sleeping deeply

// --- EEPROM (survives power-off) ---
const uint8_t EEPROM_MAGIC = 0xC9; // bumped for care-style fields
const int EEPROM_ADDR_MAGIC = 0;
const int EEPROM_ADDR_FUN = 1;
const int EEPROM_ADDR_FOOD = 2;
const int EEPROM_ADDR_CATX = 3;
const int EEPROM_ADDR_NAME = 4;
const int EEPROM_ADDR_AGE_LOW = 5;
const int EEPROM_ADDR_AGE_HIGH = 6;
const int EEPROM_ADDR_LEVEL = 7;
const int EEPROM_ADDR_PETS = 8;
const int EEPROM_ADDR_FEEDS = 9;
const int EEPROM_ADDR_TOTAL_PETS = 10;
const int EEPROM_ADDR_TOTAL_FEEDS = 11;
const int EEPROM_ADDR_STYLE = 12;
const unsigned long SAVE_MS = 2000; // write at most every 2s when dirty
bool stateDirty = false;
unsigned long lastSave = 0;
unsigned long lastAgeDayAt = 0;

// --- LEVEL / GROWTH ---
const uint8_t MAX_DOG_LEVEL = 5;
// Base care needed at Lv1->2; each next level adds CARE_LEVEL_STEP more pets AND feeds.
const uint8_t CARE_BASE = 6;
const uint8_t CARE_LEVEL_STEP = 3;
enum CareStyle : uint8_t { STYLE_BALANCED = 0, STYLE_PLAYFUL = 1, STYLE_CHONKY = 2 };
uint8_t dogLevel = 1;
uint8_t petsSinceLevel = 0;
uint8_t feedsSinceLevel = 0;
uint8_t totalPets = 0;
uint8_t totalFeeds = 0;
uint8_t careStyle = STYLE_BALANCED;

const char NAME_0[] PROGMEM = "Tomato";
const char NAME_1[] PROGMEM = "Potato";
const char NAME_2[] PROGMEM = "Bean";
const char *const DOG_NAMES[] PROGMEM = {
  NAME_0, NAME_1, NAME_2
};
const uint8_t DOG_NAME_COUNT = 3;
uint8_t nameIndex = 0;
uint16_t ageDays = 1;
char statusBuf[22]; // reusable RAM for captions copied from flash
char nameBuf[12];

// Random awake chatter (flash-light set).
const char C_0[] PROGMEM = "u got this";
const char C_1[] PROGMEM = "breathe..";
const char C_2[] PROGMEM = "tiny win!";
const char C_3[] PROGMEM = "still here";
const char C_4[] PROGMEM = "body check?";
const char C_5[] PROGMEM = "no rush";
const char C_6[] PROGMEM = "it's alright";
const char C_7[] PROGMEM = "be kind to yourself";
const char *const CHATTER[] PROGMEM = {
  C_0, C_1, C_2, C_3, C_4, C_5, C_6, C_7
};
const uint8_t CHATTER_COUNT = 8;

const char M_0[] PROGMEM = "stretch!";
const char M_1[] PROGMEM = "walk bit?";
const char M_2[] PROGMEM = "MOVE!";
const char *const MOVE_NAGS[] PROGMEM = { M_0, M_1, M_2 };
const uint8_t MOVE_NAG_COUNT = 3;

const char W_0[] PROGMEM = "water!";
const char W_1[] PROGMEM = "sip!";
const char W_2[] PROGMEM = "H2O!";
const char *const H2O_NAGS[] PROGMEM = { W_0, W_1, W_2 };
const uint8_t H2O_NAG_COUNT = 3;

const char P_0[] PROGMEM = "nice move!";
const char P_1[] PROGMEM = "logged!";
const char *const MOVE_YAY[] PROGMEM = { P_0, P_1 };
const uint8_t MOVE_YAY_COUNT = 2;

const char S_0[] PROGMEM = "sip sip!";
const char S_1[] PROGMEM = "hydrated!";
const char *const H2O_YAY[] PROGMEM = { S_0, S_1 };
const uint8_t H2O_YAY_COUNT = 2;

const unsigned long CHATTER_MS = 3200;
uint8_t chatterIdx = 0;
uint8_t actionMsgIdx = 0;
unsigned long lastChatterAt = 0;

void loadDogName(uint8_t index) {
  if (index >= DOG_NAME_COUNT) index = 0;
  strcpy_P(nameBuf, (PGM_P)pgm_read_word(&DOG_NAMES[index]));
}

int clampLevel(int value) {
  if (value < 0) return 0;
  if (value > 100) return 100;
  return value;
}

void saveState() {
  EEPROM.update(EEPROM_ADDR_MAGIC, EEPROM_MAGIC);
  EEPROM.update(EEPROM_ADDR_FUN, (uint8_t)clampLevel(fun));
  EEPROM.update(EEPROM_ADDR_FOOD, (uint8_t)clampLevel(food));
  EEPROM.update(EEPROM_ADDR_CATX, (uint8_t)constrain(dogX, 0, 96));
  EEPROM.update(EEPROM_ADDR_NAME, nameIndex);
  EEPROM.update(EEPROM_ADDR_AGE_LOW, lowByte(ageDays));
  EEPROM.update(EEPROM_ADDR_AGE_HIGH, highByte(ageDays));
  EEPROM.update(EEPROM_ADDR_LEVEL, dogLevel);
  EEPROM.update(EEPROM_ADDR_PETS, petsSinceLevel);
  EEPROM.update(EEPROM_ADDR_FEEDS, feedsSinceLevel);
  EEPROM.update(EEPROM_ADDR_TOTAL_PETS, totalPets);
  EEPROM.update(EEPROM_ADDR_TOTAL_FEEDS, totalFeeds);
  EEPROM.update(EEPROM_ADDR_STYLE, careStyle);
  stateDirty = false;
  lastSave = millis();
}

void loadState() {
  if (EEPROM.read(EEPROM_ADDR_MAGIC) != EEPROM_MAGIC) {
    nameIndex = random(DOG_NAME_COUNT);
    ageDays = 1;
    dogLevel = 1;
    petsSinceLevel = 0;
    feedsSinceLevel = 0;
    totalPets = 0;
    totalFeeds = 0;
    careStyle = STYLE_BALANCED;
    loadDogName(nameIndex);
    saveState(); // first boot: store defaults
    return;
  }
  fun = clampLevel(EEPROM.read(EEPROM_ADDR_FUN));
  food = clampLevel(EEPROM.read(EEPROM_ADDR_FOOD));
  dogX = constrain(EEPROM.read(EEPROM_ADDR_CATX), 0, 96);

  nameIndex = EEPROM.read(EEPROM_ADDR_NAME);
  if (nameIndex >= DOG_NAME_COUNT) {
    nameIndex = random(DOG_NAME_COUNT);
    markDirty();
  }
  loadDogName(nameIndex);

  ageDays = word(
    EEPROM.read(EEPROM_ADDR_AGE_HIGH),
    EEPROM.read(EEPROM_ADDR_AGE_LOW)
  );
  if (ageDays == 0 || ageDays == 0xFFFF) {
    ageDays = 1;
    markDirty();
  }

  dogLevel = EEPROM.read(EEPROM_ADDR_LEVEL);
  if (dogLevel < 1 || dogLevel > MAX_DOG_LEVEL) {
    dogLevel = 1;
    markDirty();
  }
  petsSinceLevel = EEPROM.read(EEPROM_ADDR_PETS);
  feedsSinceLevel = EEPROM.read(EEPROM_ADDR_FEEDS);
  totalPets = EEPROM.read(EEPROM_ADDR_TOTAL_PETS);
  totalFeeds = EEPROM.read(EEPROM_ADDR_TOTAL_FEEDS);
  careStyle = EEPROM.read(EEPROM_ADDR_STYLE);
  if (careStyle > STYLE_CHONKY) {
    careStyle = STYLE_BALANCED;
    markDirty();
  }
}

void markDirty() {
  stateDirty = true;
}

// Estimate board supply from the 1.1V internal bandgap (no battery pin needed).
uint16_t readVccMv() {
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2);
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA, ADSC)) {
    // wait
  }
  uint16_t adc = ADC;
  if (adc == 0) return 5000;
  return (uint16_t)(1125300L / adc);  // 1.1V * 1023 * 1000 / adc
}

void updateBattery(unsigned long now) {
  if (lastBatteryAt != 0 && now - lastBatteryAt < BATTERY_MS) return;
  lastBatteryAt = now;
  uint16_t mv = readVccMv();
  if (mv <= 3300) batteryPct = 0;
  else if (mv >= 5000) batteryPct = 100;
  else batteryPct = (uint8_t)((mv - 3300UL) * 100UL / 1700UL);
}

void factoryReset(unsigned long now) {
  fun = 70;
  food = 70;
  dogX = 48;
  pose = SIT;
  idlePose = SIT;
  actionUntil = 0;
  lastAgeDayAt = now;
  lastFunDrain = now;
  lastFoodDrain = now;
  lastIdleChange = now;
  nameIndex = random(DOG_NAME_COUNT);
  ageDays = 1;
  dogLevel = 1;
  petsSinceLevel = 0;
  feedsSinceLevel = 0;
  totalPets = 0;
  totalFeeds = 0;
  careStyle = STYLE_BALANCED;
  showingLevelUp = false;
  clickCount = 0;
  showingReset = true;
  loadDogName(nameIndex);
  saveState();
  startAction(HAPPY, now);
}

// 5s hold: real OLED sleep (panel off), not a soft pause.
void togglePower(unsigned long now) {
  powerOn = !powerOn;
  longPressDone = true;   // consume this hold, ignore as tap
  clickCount = 0;
  holdMs = 0;
  if (!powerOn) {
    if (stateDirty) saveState();
    display.clearDisplay();
    display.display();
    // Hardware display sleep — pixels + charge pump off.
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    digitalWrite(LED_BUILTIN, LOW);
  } else {
    display.ssd1306_command(SSD1306_DISPLAYON);
    // Waking up: don't punish the dog for the time it was off.
    lastAgeDayAt = now;
    lastFunDrain = now;
    lastFoodDrain = now;
    lastIdleChange = now;
    lastInteractAt = now;
    lastFrameUpdate = now;
    startAction(HAPPY, now);
  }
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Same OLED bring-up path as the working hello_world test
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (true) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
    }
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("boop..."));
  display.display();
  delay(250);

  randomSeed(analogRead(A0));
  loadState();
  loadDogName(nameIndex);
  lastInteractAt = millis();
  updateBattery(lastInteractAt);
  rollChatter(lastInteractAt);
}

void noteInteraction(unsigned long now) {
  lastInteractAt = now;
}

void refreshCareStyle() {
  // Lifetime pattern: pets-heavy, feeds-heavy, or balanced.
  if (totalPets > totalFeeds + 2) careStyle = STYLE_PLAYFUL;
  else if (totalFeeds > totalPets + 2) careStyle = STYLE_CHONKY;
  else careStyle = STYLE_BALANCED;
}

void tryLevelUp(unsigned long now) {
  if (dogLevel >= MAX_DOG_LEVEL) return;
  uint8_t need = CARE_BASE + (uint8_t)((dogLevel - 1) * CARE_LEVEL_STEP);
  if (petsSinceLevel < need || feedsSinceLevel < need) return;

  // Lock in personality from how this level was earned.
  if (petsSinceLevel > feedsSinceLevel + 1) careStyle = STYLE_PLAYFUL;
  else if (feedsSinceLevel > petsSinceLevel + 1) careStyle = STYLE_CHONKY;
  else careStyle = STYLE_BALANCED;

  dogLevel++;
  petsSinceLevel = 0;
  feedsSinceLevel = 0;
  showingLevelUp = true;
  levelUpUntil = now + 2400;
  markDirty();
  saveState();
  startAction(HAPPY, now);
}

void startAction(Pose p, unsigned long now) {
  actionPose = p;
  actionUntil = now + ACTION_MS;
  if (p == PLAYING) actionMsgIdx = random(MOVE_YAY_COUNT);
  else if (p == EATING) actionMsgIdx = random(H2O_YAY_COUNT);
  else actionMsgIdx = random(5);
  noteInteraction(now);
}

void playWithDog(unsigned long now) {
  // Single press: user took a movement / stretch break.
  noteInteraction(now);
  if (food <= CRITICAL_LEVEL) {
    // Dehydrated — nudge water first
    fun = clampLevel(fun + 3);
    markDirty();
    saveState();
    startAction(MISERABLE, now);
    return;
  }
  if (fun >= FULL_LEVEL) {
    rejectWasFood = false;
    startAction(REJECT, now);
    return;
  }
  fun = clampLevel(fun + 30);  // resets ~20 min move timer
  food = clampLevel(food - 1);
  if (petsSinceLevel < 255) petsSinceLevel++;
  if (totalPets < 255) totalPets++;
  refreshCareStyle();
  markDirty();
  saveState();
  startAction(PLAYING, now);
  tryLevelUp(now);
}

void feedDog(unsigned long now) {
  // Double press: user drank water.
  noteInteraction(now);
  if (food >= FULL_LEVEL) {
    rejectWasFood = true;
    startAction(REJECT, now);
    return;
  }
  food = clampLevel(food + 30);  // resets ~20 min water timer
  fun = clampLevel(fun + 2);
  if (feedsSinceLevel < 255) feedsSinceLevel++;
  if (totalFeeds < 255) totalFeeds++;
  refreshCareStyle();
  markDirty();
  saveState();
  startAction(EATING, now);
  tryLevelUp(now);
}

void readButton(unsigned long now) {
  bool reading = digitalRead(BUTTON_PIN);
  if (reading != rawButton) {
    rawButton = reading;
    lastButtonEdge = now;
  }
  if (now - lastButtonEdge > DEBOUNCE_MS && stableButton != rawButton) {
    stableButton = rawButton;
    if (stableButton == LOW) {
      holdStartedAt = now;
      longPressDone = false;
    } else {
      unsigned long held = now - holdStartedAt;
      holdMs = 0;
      // Long holds are commands, not pet/feed taps.
      if (longPressDone) {
        clickCount = 0;               // reset already fired live
      } else if (held >= POWER_HOLD_MS) {
        togglePower(now);             // 5s release -> flip on/off
        clickCount = 0;
      } else {
        clickCount++;
        lastClickAt = now;
      }
    }
  }

  if (stableButton == LOW && !longPressDone) {
    holdMs = now - holdStartedAt;
  } else if (stableButton == HIGH) {
    holdMs = 0;
  }

  // Full reset only while awake, needs a very long 10s hold.
  if (powerOn && stableButton == LOW && !longPressDone && holdMs >= LONG_PRESS_MS) {
    longPressDone = true;
    holdMs = 0;
    clickCount = 0;
    factoryReset(now);
    return;
  }

  if (longPressDone || stableButton == LOW) return;

  // Wait for the multi-tap window so 1 / 2 / 3 presses can be told apart.
  if (clickCount == 0 || now - lastClickAt <= MULTI_GAP_MS) return;

  if (clickCount >= 3) {
    statsUntil = now + STATS_SHOW_MS;
    clickCount = 0;
  } else if (clickCount == 2) {
    feedDog(now);
    clickCount = 0;
  } else {
    playWithDog(now);
    clickCount = 0;
  }
}

void drawHeart(int x, int y, bool filled = true);
void printCentered(int y, const char *text);
const char *flashStatus(const char *p);

void drawGauge(int y, const char *label, int value, bool blink) {
  display.setTextSize(1);
  display.setCursor(0, y);
  display.print(label);

  const int heartCount = (value + 9) / 10;
  for (int i = 0; i < 10; i++) {
    drawHeart(29 + i * 10, y, !blink && i < heartCount);
  }
}

uint8_t careNeedForLevel() {
  if (dogLevel >= MAX_DOG_LEVEL) return 1;
  return CARE_BASE + (uint8_t)((dogLevel - 1) * CARE_LEVEL_STEP);
}

int careProgressPercent() {
  if (dogLevel >= MAX_DOG_LEVEL) return 100;
  uint8_t need = careNeedForLevel();
  uint8_t balanced = petsSinceLevel < feedsSinceLevel ? petsSinceLevel : feedsSinceLevel;
  return (int)((balanced * 100UL) / need);
}

void drawCareGauge(int y) {
  display.setTextSize(1);
  display.setCursor(0, y);
  display.print(F("CARE"));
  int pct = careProgressPercent();
  display.drawRect(29, y - 1, 100, 9, WHITE);
  int fillW = (pct * 96) / 100;
  if (fillW > 0) display.fillRect(31, y + 1, fillW, 5, WHITE);
}

// Evolution name for the level just reached, branched by care pattern.
const char *levelUpTitle() {
  if (careStyle == STYLE_PLAYFUL) return PSTR("MOVE UP!");
  if (careStyle == STYLE_CHONKY) return PSTR("SIP UP!");
  return PSTR("LEVEL UP!");
}

void drawLevelUpBanner() {
  display.fillRect(2, 16, 124, 26, BLACK);
  display.drawRect(2, 16, 124, 26, WHITE);

  char line[16];
  snprintf(line, sizeof(line), "LEVEL %u!", dogLevel);
  printCentered(20, line);
  printCentered(31, flashStatus(levelUpTitle()));
}

void drawPetHeader() {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(nameBuf);

  char meta[18];
  snprintf(meta, sizeof(meta), "Day%u Lv%u", ageDays, dogLevel);
  display.setCursor(SCREEN_WIDTH - strlen(meta) * 6, 0);
  display.print(meta);

  char bat[6];
  snprintf(bat, sizeof(bat), "%u%%", batteryPct);
  display.setCursor(SCREEN_WIDTH - strlen(bat) * 6, 8);
  display.print(bat);
}

void printCentered(int y, const char *text) {
  int len = strlen(text);
  display.setTextSize(1);
  display.setCursor((SCREEN_WIDTH - len * 6) / 2, y);
  display.print(text);
}

void drawHeart(int x, int y, bool filled) {
  if (filled) {
    display.drawLine(x + 1, y, x + 2, y, WHITE);
    display.drawLine(x + 4, y, x + 5, y, WHITE);
    display.drawLine(x, y + 1, x + 6, y + 2, WHITE);
    display.drawLine(x + 1, y + 3, x + 5, y + 3, WHITE);
    display.drawLine(x + 2, y + 4, x + 4, y + 4, WHITE);
    display.drawPixel(x + 3, y + 5, WHITE);
  } else {
    display.drawLine(x + 1, y, x + 2, y, WHITE);
    display.drawLine(x + 4, y, x + 5, y, WHITE);
    display.drawPixel(x, y + 1, WHITE);
    display.drawPixel(x + 3, y + 1, WHITE);
    display.drawPixel(x + 6, y + 1, WHITE);
    display.drawPixel(x, y + 2, WHITE);
    display.drawPixel(x + 6, y + 2, WHITE);
    display.drawPixel(x + 1, y + 3, WHITE);
    display.drawPixel(x + 5, y + 3, WHITE);
    display.drawPixel(x + 2, y + 4, WHITE);
    display.drawPixel(x + 4, y + 4, WHITE);
    display.drawPixel(x + 3, y + 5, WHITE);
  }
}

void drawBowl(int x, int y, bool full) {
  display.drawLine(x, y, x + 9, y, WHITE);
  display.drawLine(x, y, x + 2, y + 4, WHITE);
  display.drawLine(x + 9, y, x + 7, y + 4, WHITE);
  display.drawLine(x + 2, y + 4, x + 7, y + 4, WHITE);
  if (full) display.drawLine(x + 2, y + 2, x + 7, y + 2, WHITE);
}

// Copy a flash string into statusBuf and return it (keeps captions out of SRAM).
const char *flashStatus(const char *p) {
  strncpy_P(statusBuf, p, sizeof(statusBuf) - 1);
  statusBuf[sizeof(statusBuf) - 1] = 0;
  return statusBuf;
}

const char *flashTableMsg(const char *const *table, uint8_t count, uint8_t idx) {
  if (count == 0) return flashStatus(PSTR(""));
  return flashStatus((const char *)pgm_read_word(&table[idx % count]));
}

void rollChatter(unsigned long now) {
  chatterIdx = random(CHATTER_COUNT);
  lastChatterAt = now;
}

const char *statusText() {
  if (showingLevelUp) return flashStatus(PSTR("LEVEL UP!"));
  if (showingReset) return flashStatus(PSTR("reset.."));
  if (holdMs >= POWER_HOLD_MS && holdMs < LONG_PRESS_MS) {
    return flashStatus(PSTR("hold: reset"));
  }
  if (holdMs >= POWER_HINT_MS && holdMs < POWER_HOLD_MS) {
    return flashStatus(PSTR("hold: off"));
  }
  if (showingBonk) return flashStatus(PSTR("BONK"));

  switch (pose) {
    case PLAYING:
      return flashTableMsg(MOVE_YAY, MOVE_YAY_COUNT, actionMsgIdx);
    case EATING:
      return flashTableMsg(H2O_YAY, H2O_YAY_COUNT, actionMsgIdx);
    case REJECT:
      if (rejectWasFood) return flashStatus(PSTR("just sipped"));
      return flashStatus(PSTR("just moved"));
    case MISERABLE:
      return flashTableMsg(H2O_NAGS, H2O_NAG_COUNT, chatterIdx);
    case HUNGRY:
      return flashTableMsg(H2O_NAGS, H2O_NAG_COUNT, chatterIdx);
    case BORED:
      return flashTableMsg(MOVE_NAGS, MOVE_NAG_COUNT, chatterIdx);
    case SLEEP:
      return flashStatus(PSTR(""));  // quiet while sleeping
    case HAPPY:
    case GROOM:
    case STRETCH:
    case WALK_R:
    case WALK_L:
    case SIT:
    default:
      return flashTableMsg(CHATTER, CHATTER_COUNT, chatterIdx);
  }
}

void drawFace(int hx, int hy, bool faceRight) {
  int e0 = faceRight ? hx + 2 : hx - 5;
  int e1 = faceRight ? hx + 7 : hx;
  int nose = faceRight ? hx + 10 : hx - 8;
  if (pose == SLEEP) {
    display.drawLine(e0, hy, e0 + 3, hy, BLACK);
    display.drawLine(e1, hy, e1 + 3, hy, BLACK);
  } else {
    display.fillRect(e0, hy - 1, 3, 3, BLACK);
    display.fillRect(e1, hy - 1, 3, 3, BLACK);
    display.drawPixel(e0 + 1, hy - 1, WHITE);
    display.drawPixel(e1 + 1, hy - 1, WHITE);
  }
  display.fillRect(nose, hy + 2, 2, 2, BLACK);
}

void drawLegs(int x, int y, uint8_t tall) {
  bool walk = (pose == WALK_L || pose == WALK_R || pose == PLAYING);
  int a = (walk && (animPhase & 1)) ? 2 : 0;
  int b = (walk && !(animPhase & 1)) ? 2 : 0;
  display.fillRect(x + 8, y + 14 + a, 3, tall, WHITE);
  display.fillRect(x + 14, y + 14 + b, 3, tall, WHITE);
  display.fillRect(x + 20, y + 14 + a, 3, tall, WHITE);
  display.fillRect(x + 25, y + 14 + b, 3, tall, WHITE);
}

void drawGear(int x, int y, int hx) {
  // Shared level gear — keeps flash small, still scales with level.
  if (dogLevel >= 2) {
    display.fillRect(x + 10, y + 11, 12, 2, BLACK);
  }
  if (dogLevel >= 3) {
    display.fillRect(hx - 5, y + 1, 10, 2, WHITE);
  }
  if (dogLevel >= 4) {
    display.fillRect(hx - 5, y - 3, 11, 3, WHITE);
  }
  if (dogLevel >= 5) {
    display.fillRect(hx - 6, y - 6, 13, 3, WHITE);
    display.fillRect(hx - 1, y - 8, 3, 2, WHITE);
    if (animFrame) display.drawPixel(x + 16, y - 9, WHITE);
  }
}

void drawSportFox(int x, int y) {
  bool faceRight = (pose == WALK_R || pose == REJECT);
  int hx = faceRight ? x + 22 : x + 10;
  uint8_t earH = 2 + dogLevel;
  display.fillTriangle(hx - 4, y + 4, hx - 1, y + 4, hx - 3, y + 4 - earH, WHITE);
  display.fillTriangle(hx + 1, y + 4, hx + 4, y + 4, hx + 3, y + 3 - earH, WHITE);
  display.fillCircle(hx, y + 7, 5, WHITE);
  display.fillRect(x + 9, y + 8, 14, 6 + dogLevel / 2, WHITE);
  drawLegs(x, y, 4 + (dogLevel >= 3));
  int tx = faceRight ? x + 4 : x + 28;
  display.fillTriangle(tx, y + 10, tx + (faceRight ? -4 - dogLevel : 4 + dogLevel), y + 6,
                       tx, y + 14, WHITE);
  drawFace(hx, y + 7, faceRight);
  drawGear(x, y, hx);
  if (dogLevel >= 2) {
    int bx = x + (faceRight ? 30 : -2) + ((animPhase & 1) ? 1 : 0);
    display.fillCircle(bx, y + 7, 2, WHITE);
  }
  if (dogLevel >= 3) display.fillRect(hx - 5, y + 5, 11, 2, BLACK); // shades
}

void drawChonkLoaf(int x, int y) {
  bool faceRight = (pose == WALK_R || pose == REJECT);
  int hx = faceRight ? x + 20 : x + 12;
  display.fillCircle(x + 16, y + 13, 6 + dogLevel, WHITE);
  display.fillCircle(hx, y + 7, 5, WHITE);
  display.fillCircle(hx - 4, y + 2, 2, WHITE);
  display.fillCircle(hx + 4, y + 2, 2, WHITE);
  display.fillRect(x + 8, y + 17, 5, 2, WHITE);
  display.fillRect(x + 19, y + 17, 5, 2, WHITE);
  drawFace(hx, y + 7, faceRight);
  display.fillRect(faceRight ? hx + 3 : hx - 5, y + 8, 3, 2, BLACK);
  drawGear(x, y, hx);
  if (dogLevel >= 3) {
    display.fillTriangle(x + 8, y + 9, x + 24, y + 9, x + 16, y + 16, WHITE);
  }
  if (dogLevel >= 4) display.fillCircle(hx, y - 2, 4, WHITE);
}

void drawBalancedDog(int x, int y) {
  bool faceRight = (pose == WALK_R || pose == REJECT);
  int hx = faceRight ? x + 22 : x + 10;
  display.fillRect(hx - 7, y + 3, 4, 6 + dogLevel / 2, WHITE);
  display.fillRect(hx + 3, y + 3, 4, 6 + dogLevel / 2, WHITE);
  display.fillCircle(hx, y + 7, 5 + (dogLevel >= 3), WHITE);
  display.fillRect(x + 8, y + 9, 17, 7, WHITE);
  drawLegs(x, y, 3 + (dogLevel >= 3));
  int tw = (animPhase & 1) ? -2 : 2;
  display.fillTriangle(x + 27, y + 11, x + 31, y + 8 + tw, x + 30, y + 14, WHITE);
  drawFace(hx, y + 7, faceRight);
  drawGear(x, y, hx);
  if (dogLevel >= 3) {
    display.fillTriangle(x + 27, y + 5, x + 34, y + 8, x + 28, y + 17, WHITE);
  }
  if (dogLevel >= 4) {
    display.fillTriangle(hx - 4, y + 1, hx + 4, y + 1, hx, y - 6, WHITE);
  }
}

void drawPet(int x, int y) {
  int dy = 3 - (int)dogLevel;
  if (careStyle == STYLE_PLAYFUL) {
    drawSportFox(x, y + dy - 1);
  } else if (careStyle == STYLE_CHONKY) {
    drawChonkLoaf(x, y + dy + 1);
  } else {
    drawBalancedDog(x, y + dy);
  }
}

void drawPettingHand(int x, int y) {
  display.drawLine(x + 4, y - 5, x + 4, y, WHITE);
  display.drawLine(x + 5, y - 5, x + 5, y, WHITE);
  display.fillRect(x + 2, y, 7, 4, WHITE);
}

void drawBonkStars(int x, int y) {
  display.drawPixel(x + 12, y - 6, WHITE);
  display.drawPixel(x + 20, y - 4, WHITE);
}

void loop() {
  unsigned long currentMillis = millis();

  // While off: OLED is asleep; only poll the button slowly for a 5s wake hold.
  if (!powerOn) {
    readButton(currentMillis);
    delay(50);
    return;
  }

  updateBattery(currentMillis);

  // Count age while powered and preserve completed days in EEPROM.
  if (currentMillis - lastAgeDayAt >= DAY_MS) {
    lastAgeDayAt += DAY_MS;
    ageDays++;
    markDirty();
  }

  // 1. Needs Drain Engine
  if (currentMillis - lastFunDrain >= FUN_DRAIN_MS) {
    lastFunDrain = currentMillis;
    fun = clampLevel(fun - 1);
    markDirty();
  }
  if (currentMillis - lastFoodDrain >= FOOD_DRAIN_MS) {
    lastFoodDrain = currentMillis;
    food = clampLevel(food - 1);
    markDirty();
  }

  // Persist needs/position so they survive unplugging
  if (stateDirty && currentMillis - lastSave >= SAVE_MS) {
    saveState();
  }

  // 2. Button: 1 = move break, 2 = drink water, 3 = gauges
  readButton(currentMillis);

  // 3. Animation ticks (slower while napping)
  bool phaseAdvanced = false;
  unsigned long frameMs = FRAME_MS;
  if (pose == SLEEP) frameMs = FRAME_MS * 3;
  if (currentMillis - lastFrameUpdate > frameMs) {
    animPhase = (animPhase + 1) & 3;
    animFrame = (animPhase & 1);
    lastFrameUpdate = currentMillis;
    phaseAdvanced = true;
  }

  bool wantsNap = (currentMillis - lastInteractAt >= NAP_AFTER_MS)
                  && (fun > LOW_LEVEL)
                  && (food > LOW_LEVEL);

  // 4. Idle wandering only while awake
  if (!wantsNap && currentMillis - lastIdleChange > IDLE_CHANGE_MS) {
    goofThought = random(8);
    if (fun > 75 && food > 75 && random(2) == 0) {
      idlePose = HAPPY;
    } else {
      idlePose = IDLE_CHOICES[random(IDLE_CHOICE_COUNT)];
    }
    lastIdleChange = currentMillis;
    rollChatter(currentMillis);
  }

  // Fresh buddy lines while awake (not while sleeping)
  if (pose != SLEEP && currentMillis - lastChatterAt >= CHATTER_MS) {
    if (pose == BORED) chatterIdx = random(MOVE_NAG_COUNT);
    else if (pose == HUNGRY || pose == MISERABLE) chatterIdx = random(H2O_NAG_COUNT);
    else rollChatter(currentMillis);
    lastChatterAt = currentMillis;
  }

  if (showingLevelUp && currentMillis >= levelUpUntil) {
    showingLevelUp = false;
  }

  if (showingBonk && currentMillis >= bonkUntil) {
    showingBonk = false;
  }

  // 5. Mood resolver:
  // action > hunger/boredom wake-up > nap after idle > normal idle
  if (currentMillis < actionUntil) {
    pose = actionPose;
  } else {
    showingReset = false;
    if (fun <= CRITICAL_LEVEL && food <= CRITICAL_LEVEL) {
      pose = MISERABLE;
    } else if (food <= LOW_LEVEL) {
      pose = HUNGRY; // wakes from nap when hungry
    } else if (fun <= LOW_LEVEL) {
      pose = BORED;  // wakes from nap when bored
    } else if (wantsNap) {
      pose = SLEEP;
      idlePose = SLEEP;
    } else {
      pose = idlePose;
    }
  }

  // 6. Position Physics
  if (phaseAdvanced) {
    if (pose == WALK_R) {
      dogX += 1;
      if (dogX > 96) {
        dogX = 96;
        idlePose = WALK_L;
        showingBonk = true;
        bonkUntil = currentMillis + 700;
      }
      markDirty();
    } else if (pose == WALK_L) {
      dogX -= 1;
      if (dogX < 0) {
        dogX = 0;
        idlePose = WALK_R;
        showingBonk = true;
        bonkUntil = currentMillis + 700;
      }
      markDirty();
    } else if (pose == HAPPY && animPhase == 0) {
      dogX = constrain(dogX + (goofThought & 1 ? 1 : -1), 0, 96);
    } else if (pose == SIT && animPhase == 0 && random(5) == 0) {
      dogX = constrain(dogX + (random(2) ? 1 : -1), 0, 96);
    }
  }

  // ==========================================
  //   RENDER SCREEN
  // ==========================================
  display.clearDisplay();
  display.setTextColor(WHITE);

  // Persistent name and powered-on age
  drawPetHeader();

  // Need gauges: auto when low, or triple-tap to inspect anytime
  bool showStats = (currentMillis < statsUntil);
  if (showStats || fun <= LOW_LEVEL) {
    drawGauge(9, "MOVE", fun, fun <= CRITICAL_LEVEL && animFrame);
  }
  if (showStats || food <= LOW_LEVEL) {
    drawGauge(17, "H2O", food, food <= CRITICAL_LEVEL && animFrame);
  }
  if (showStats) {
    drawCareGauge(25);
    display.setTextSize(1);
    display.setCursor(0, 34);
    if (careStyle == STYLE_PLAYFUL) display.print(F("M"));
    else if (careStyle == STYLE_CHONKY) display.print(F("S"));
    else display.print(F("B"));
  }

  // Level-up gets its own banner; otherwise show the mood caption
  if (showingLevelUp) {
    drawLevelUpBanner();
  } else {
    printCentered(showStats ? 42 : 27, statusText());
  }

  // 2s..5s fills toward power-off; 5s..10s fills toward factory reset.
  if (holdMs >= POWER_HINT_MS && holdMs < LONG_PRESS_MS) {
    unsigned long start = (holdMs < POWER_HOLD_MS) ? POWER_HINT_MS : POWER_HOLD_MS;
    unsigned long end = (holdMs < POWER_HOLD_MS) ? POWER_HOLD_MS : LONG_PRESS_MS;
    int fillW = (int)(((holdMs - start) * 100UL) / (end - start));
    if (fillW > 100) fillW = 100;
    display.drawRect(14, 50, 100, 4, WHITE);
    if (fillW > 0) display.fillRect(14, 50, fillW, 4, WHITE);
  }

  // Draw Floor Ground Line
  display.drawLine(0, 60, 128, 60, WHITE);

  // Motion bob
  const int8_t softBob[4] = {0, -2, 1, -1};
  const int8_t hopBob[4] = {0, -3, 1, -4};
  const int8_t bigBob[4] = {0, -5, 0, -6};
  const int8_t tip[4] = {0, 1, 0, -1};

  const int y = dogY;
  int x = dogX;

  if (showingBonk) {
    x += (animPhase & 1) ? 2 : -2;
  }

  switch (pose) {
    case SIT:
      drawPet(x + tip[animPhase], y + softBob[animPhase]);
      break;

    case WALK_R:
      drawPet(x + tip[animPhase], y + hopBob[animPhase]);
      if (showingBonk) drawBonkStars(x, y);
      break;

    case WALK_L:
      drawPet(x + tip[animPhase], y + hopBob[animPhase]);
      if (showingBonk) drawBonkStars(x, y);
      break;

    case SLEEP: {
      int breath = (animPhase <= 1) ? 0 : 1;
      drawPet(x + ((animPhase == 3) ? 1 : 0), y + breath);
      break;
    }

    case GROOM:
      drawPet(x, y + softBob[animPhase]);
      break;

    case STRETCH:
      drawPet(x + ((animPhase & 1) ? 2 : -2), y + softBob[animPhase]);
      break;

    case HAPPY:
      drawPet(x + tip[animPhase], y + bigBob[animPhase]);
      if (animPhase & 1) drawHeart(x + 34, y - 3);
      break;

    case PLAYING: {
      const int8_t handDrop[4] = {0, 3, 4, 2};
      int by = y + ((animPhase <= 1) ? animPhase : 2);
      drawPet(x, by);
      drawPettingHand(x + 5, y - 7 + handDrop[animPhase]);
      if (animPhase & 1) drawHeart(x + 34, y - 4);
      break;
    }

    case EATING: {
      int nod = (animPhase & 1) ? 2 : 0;
      drawPet(x + tip[animPhase], y + nod);
      drawBowl(x - 12, y + 14, true);
      break;
    }

    case REJECT: {
      int shake = (animPhase == 0) ? -3 : ((animPhase == 2) ? 3 : 0);
      drawPet(x + shake, y + softBob[animPhase]);
      break;
    }

    case HUNGRY:
      drawPet(x + tip[animPhase], y + softBob[animPhase]);
      drawBowl(x - 12, y + 14, false);
      break;

    case BORED:
      drawPet(x + tip[animPhase], y + ((animPhase == 2) ? 1 : 0));
      break;

    case MISERABLE:
      drawPet(x + ((animPhase & 1) ? 2 : -2), y + hopBob[animPhase]);
      drawBonkStars(x, y);
      break;
  }

  display.display();
  delay(20);
}
