// ADHDBuddy.ino
//
// ADHD companion pet: two reminders that drain over ~20 minutes.
//   BODY (fun)  -> body check / stretch break
//   UNMASK (food) -> take the mask off / soft reset
// Wiring: one push button between D2 and GND (uses the internal pull-up).
//   single press  -> logged a body check
//   double press  -> logged an unmask
//   triple press  -> show BODY / UNMASK / care gauges
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
//   ANNOYING-DOG STYLE (1-bit bitmaps)
// ==========================================
// Packed 32px wide (4 bytes/row); logical draw width is SPRITE_W.
const uint8_t SPRITE_W = 28;
const uint8_t SPRITE_H = 18;
const uint8_t SLEEP_W = 32;
const uint8_t SLEEP_H = 14;

const unsigned char PROGMEM dog_sit[] = {
  0x00,0x00,0x00,0x00,0x0d,0xec,0x00,0x00,0x1f,0xff,0xe0,0x00,
  0x3b,0xbf,0xe0,0x00,0x3f,0xff,0xe0,0x00,0x3d,0xff,0xfe,0x00,
  0x38,0xff,0xfe,0x00,0x30,0x1f,0xfe,0x00,0x3f,0xff,0xfc,0x00,
  0x3f,0xff,0xfc,0x00,0x3f,0xff,0xf8,0x00,0x3f,0xff,0xf8,0x00,
  0x3f,0xff,0xf8,0x00,0x0c,0xc3,0x30,0x00,0x0c,0x82,0x30,0x00,
  0x08,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
// Trot frames: opposite diagonal legs extend each step.
const unsigned char PROGMEM dog_walk1[] = {
  0x00,0x00,0x00,0x00,0x0d,0xec,0x00,0x00,0x1f,0xff,0xe0,0x00,
  0x3b,0xbf,0xe0,0x00,0x3f,0xff,0xe0,0x00,0x3d,0xff,0xfe,0x00,
  0x38,0xff,0xfe,0x00,0x30,0x1f,0xfe,0x00,0x3f,0xff,0xfc,0x00,
  0x3f,0xff,0xfc,0x00,0x3f,0xff,0xf8,0x00,0x3f,0xff,0xf8,0x00,
  0x3f,0xff,0xf8,0x00,0x1c,0x63,0x8c,0x00,0x0c,0x61,0x8c,0x00,
  0x0c,0x01,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
const unsigned char PROGMEM dog_walk2[] = {
  0x00,0x00,0x00,0x00,0x0d,0xec,0x00,0x00,0x1f,0xff,0xe0,0x00,
  0x3b,0xbf,0xe0,0x00,0x3f,0xff,0xe0,0x00,0x3d,0xff,0xfe,0x00,
  0x38,0xff,0xfe,0x00,0x30,0x1f,0xfe,0x00,0x3f,0xff,0xfc,0x00,
  0x3f,0xff,0xfc,0x00,0x3f,0xff,0xf8,0x00,0x3f,0xff,0xf8,0x00,
  0x3f,0xff,0xf8,0x00,0x0c,0x71,0x8e,0x00,0x0c,0x61,0x8c,0x00,
  0x00,0x60,0x0c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
// Lying loaf sleep pose.
const unsigned char PROGMEM dog_sleep[] = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xff,0xff,0x80,
  0x03,0xff,0xff,0xc0,0x07,0x77,0xff,0xe0,0x07,0xbf,0xff,0xe0,
  0x07,0x1f,0xff,0xf0,0x07,0xff,0xff,0xf0,0x07,0xff,0xff,0xe0,
  0x03,0xff,0xff,0x40,0x01,0xff,0xfe,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

enum Pose {
  WALK_R, WALK_L, SIT, SLEEP, GROOM, STRETCH, HAPPY,
  PLAYING, EATING, BORED, HUNGRY, MISERABLE, REJECT,
  ZOOMIES, SPIN, TIPOVER, SNIFF, DANCE
};
Pose pose = SIT;
Pose idlePose = SIT;

// Weighted toward silly bits so idle feels goofy, not just pacing.
const Pose IDLE_CHOICES[] = {
  WALK_R, WALK_L, SIT,
  GROOM, STRETCH, HAPPY,
  ZOOMIES, ZOOMIES, SPIN, SPIN,
  TIPOVER, SNIFF, DANCE, DANCE, HAPPY
};
const uint8_t IDLE_CHOICE_COUNT = sizeof(IDLE_CHOICES) / sizeof(IDLE_CHOICES[0]);

int dogX = 50;
const int dogY = 40; // 28x18 annoying-dog style above the ground line
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
// fun  = body-check energy
// food = unmask / soft-reset energy
int fun = 70;
int food = 70;
const int LOW_LEVEL = 25;                 // start nagging
const int CRITICAL_LEVEL = 12;            // loud nag
const int FULL_LEVEL = 90;                // already topped up
// ~20 min from topped (~100) down to LOW (25) ≈ 75 points
const unsigned long FUN_DRAIN_MS = (20UL * 60UL * 1000UL) / 75UL;   // body-check reminder
const unsigned long FOOD_DRAIN_MS = (20UL * 60UL * 1000UL) / 75UL;  // unmask reminder
const unsigned long ACTION_MS = 2200;      // celebration after logging check/unmask
const unsigned long IDLE_CHANGE_MS = 1800;
const unsigned long NAP_AFTER_MS = 50000;
const unsigned long FRAME_MS = 130;
const unsigned long DAY_MS = 86400000UL;
const unsigned long BATTERY_MS = 2000;

// Board VCC as battery % (uses internal 1.1V bandgap; no extra wiring).
// 5.0V = 100%, 3.3V = 0%.
unsigned long lastBatteryAt = 0;
uint8_t batteryPct = 100;

// --- BUTTON ENGINE ---
// single tap = body check, double tap = unmask, triple tap = gauges
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
const char C_1[] PROGMEM = "unmask...";
const char C_2[] PROGMEM = "body check?";
const char C_3[] PROGMEM = "slow down";
const char C_4[] PROGMEM = "sensory time?";
const char C_5[] PROGMEM = "no rush";
const char C_6[] PROGMEM = "it's ok";
const char C_7[] PROGMEM = "zone out...";
const char *const CHATTER[] PROGMEM = {
  C_0, C_1, C_2, C_3, C_4, C_5, C_6, C_7
};
const uint8_t CHATTER_COUNT = 8;

const char M_0[] PROGMEM = "body check?";
const char M_1[] PROGMEM = "stretch!";
const char M_2[] PROGMEM = "check in!";
const char *const MOVE_NAGS[] PROGMEM = { M_0, M_1, M_2 };
const uint8_t MOVE_NAG_COUNT = 3;

const char W_0[] PROGMEM = "unmask?";
const char W_1[] PROGMEM = "take off..";
const char W_2[] PROGMEM = "soft face?";
const char *const H2O_NAGS[] PROGMEM = { W_0, W_1, W_2 };
const uint8_t H2O_NAG_COUNT = 3;

const char P_0[] PROGMEM = "checked!";
const char P_1[] PROGMEM = "body ok!";
const char *const MOVE_YAY[] PROGMEM = { P_0, P_1 };
const uint8_t MOVE_YAY_COUNT = 2;

const char S_0[] PROGMEM = "unmasked!";
const char S_1[] PROGMEM = "soft now!";
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
  EEPROM.update(EEPROM_ADDR_CATX, (uint8_t)constrain(dogX, 0, 100));
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
  dogX = constrain(EEPROM.read(EEPROM_ADDR_CATX), 0, 100);

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
  dogX = 50;
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
  levelUpUntil = now + 3000;
  markDirty();
  saveState();
  if (careStyle == STYLE_PLAYFUL) startAction(DANCE, now);
  else if (careStyle == STYLE_CHONKY) startAction(HAPPY, now);
  else startAction(SPIN, now);
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
  // Single press: user did a body check.
  noteInteraction(now);
  if (food <= CRITICAL_LEVEL) {
    // Mask still on hard — nudge unmask first
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
  fun = clampLevel(fun + 30);  // resets ~20 min body-check timer
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
  // Double press: user unmasked / soft reset.
  noteInteraction(now);
  if (food >= FULL_LEVEL) {
    rejectWasFood = true;
    startAction(REJECT, now);
    return;
  }
  food = clampLevel(food + 30);  // resets ~20 min unmask timer
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

  int hx = (int)strlen(label) * 6 + 2;
  int spacing = (SCREEN_WIDTH - hx) / 10;
  if (spacing > 10) spacing = 10;
  if (spacing < 7) spacing = 7;

  const int heartCount = (value + 9) / 10;
  for (int i = 0; i < 10; i++) {
    drawHeart(hx + i * spacing, y, !blink && i < heartCount);
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
  if (careStyle == STYLE_PLAYFUL) return PSTR("ATHLETE!");
  if (careStyle == STYLE_CHONKY) return PSTR("SOFT MODE!");
  return PSTR("TRUE FORM!");
}

void drawSpark(int x, int y) {
  display.drawPixel(x, y, WHITE);
  display.drawPixel(x - 1, y, WHITE);
  display.drawPixel(x + 1, y, WHITE);
  display.drawPixel(x, y - 1, WHITE);
  display.drawPixel(x, y + 1, WHITE);
}

void drawLevelUpBanner() {
  display.fillRect(2, 12, 124, 34, BLACK);
  display.drawRect(2, 12, 124, 34, WHITE);

  // Style-flavored frame accents.
  if (careStyle == STYLE_PLAYFUL) {
    // Lightning bolts in the corners.
    display.drawLine(5, 15, 9, 22, WHITE);
    display.drawLine(9, 22, 7, 22, WHITE);
    display.drawLine(7, 22, 11, 30, WHITE);
    display.drawLine(122, 15, 118, 22, WHITE);
    display.drawLine(118, 22, 120, 22, WHITE);
    display.drawLine(120, 22, 116, 30, WHITE);
    if (animFrame) {
      display.drawPixel(14, 18, WHITE);
      display.drawPixel(113, 18, WHITE);
    }
  } else if (careStyle == STYLE_CHONKY) {
    // Soft sparkles + little hearts.
    drawSpark(8, 18);
    drawSpark(119, 18);
    if (animFrame) {
      drawHeart(10, 36);
      drawHeart(110, 36);
    } else {
      display.drawPixel(16, 40, WHITE);
      display.drawPixel(111, 40, WHITE);
    }
  } else {
    // Twin stars.
    drawSpark(8, 20);
    drawSpark(119, 20);
    display.drawPixel(14, 38, WHITE);
    display.drawPixel(113, 38, WHITE);
    if (animFrame) {
      display.drawPixel(20, 16, WHITE);
      display.drawPixel(107, 16, WHITE);
    }
  }

  char line[16];
  snprintf(line, sizeof(line), "LEVEL %u!", dogLevel);
  printCentered(20, line);
  printCentered(32, flashStatus(levelUpTitle()));
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
      if (rejectWasFood) return flashStatus(PSTR("just unmasked"));
      return flashStatus(PSTR("just checked"));
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
    case ZOOMIES:
    case SPIN:
    case TIPOVER:
    case SNIFF:
    case DANCE:
    case WALK_R:
    case WALK_L:
    case SIT:
    default:
      return flashTableMsg(CHATTER, CHATTER_COUNT, chatterIdx);
  }
}

void drawBitmapHFlip(int x, int y, const unsigned char *bitmap, uint8_t w, uint8_t h) {
  // Tiny horizontal flip so walk-right can reuse the left-facing sprite.
  uint8_t bytesPerRow = (w + 7) / 8;
  for (uint8_t row = 0; row < h; row++) {
    for (uint8_t col = 0; col < w; col++) {
      uint8_t b = pgm_read_byte(bitmap + row * bytesPerRow + (col >> 3));
      if (b & (0x80 >> (col & 7))) {
        display.drawPixel(x + (w - 1 - col), y + row, WHITE);
      }
    }
  }
}

void drawAnnoyingDog(int x, int y, bool faceRight, bool walking) {
  if (pose == SLEEP) {
    // Flat loaf sleep sprite (wider, shorter).
    int sx = x - 2;
    int sy = y + 4;
    if (faceRight) drawBitmapHFlip(sx, sy, dog_sleep, SLEEP_W, SLEEP_H);
    else display.drawBitmap(sx, sy, dog_sleep, SLEEP_W, SLEEP_H, WHITE);
    // Soft Z breath mark
    if (animPhase & 1) {
      display.drawPixel(sx + SLEEP_W + 1, sy, WHITE);
      display.drawPixel(sx + SLEEP_W + 2, sy - 1, WHITE);
    }
    return;
  }

  const unsigned char *bmp = dog_sit;
  // Alternate diagonal leg pairs each tick for a trot.
  if (walking) bmp = (animPhase & 1) ? dog_walk2 : dog_walk1;
  if (faceRight) drawBitmapHFlip(x, y, bmp, SPRITE_W, SPRITE_H);
  else display.drawBitmap(x, y, bmp, SPRITE_W, SPRITE_H, WHITE);

  // Baked face already has neutral eyes + T-mouth. Only tweak for moods.
  // Anchors match left-facing 1px eyes at cols 5 / 9.
  int e0 = faceRight ? x + (SPRITE_W - 1 - 5) : x + 5;
  int e1 = faceRight ? x + (SPRITE_W - 1 - 9) : x + 9;
  int mx = faceRight ? x + (SPRITE_W - 8) : x + 5;
  int my = y + 6;

  switch (pose) {
    case HAPPY:
    case DANCE:
      // Close eyes into ^ ^ over baked holes, keep T-mouth.
      display.fillRect(e0, y + 3, 2, 2, WHITE);
      display.fillRect(e1, y + 3, 2, 2, WHITE);
      display.drawPixel(e0, y + 5, BLACK);
      display.drawPixel(e0 + 1, y + 4, BLACK);
      display.drawPixel(e1, y + 5, BLACK);
      display.drawPixel(e1 + 1, y + 4, BLACK);
      break;
    case PLAYING:
    case ZOOMIES:
      display.fillRect(e0, y + 3, 2, 2, WHITE);
      display.fillRect(e1, y + 3, 2, 2, WHITE);
      display.drawPixel(e0, y + 3, BLACK);
      display.drawPixel(e1, y + 3, BLACK);
      display.drawPixel(e0, y + 4, BLACK);
      display.drawPixel(e1, y + 4, BLACK);
      break;
    case BORED:
    case TIPOVER:
      display.fillRect(e0, y + 3, 2, 2, WHITE);
      display.fillRect(e1, y + 3, 2, 2, WHITE);
      display.drawPixel(e0, y + 4, BLACK);
      display.drawPixel(e1, y + 5, BLACK);
      break;
    case HUNGRY:
      display.fillRect(mx, my, 3, 1, WHITE);
      display.fillRect(mx, my + 1, 3, 1, BLACK);
      break;
    case EATING:
      display.fillRect(mx, my, 3, 1, WHITE);
      if (animPhase & 1) display.fillRect(mx, my, 3, 1, BLACK);
      else display.drawLine(mx, my, mx + 2, my, BLACK);
      break;
    case MISERABLE:
      display.fillRect(e0, y + 3, 2, 2, WHITE);
      display.fillRect(e1, y + 3, 2, 2, WHITE);
      display.drawLine(e0, y + 3, e0 + 1, y + 4, BLACK);
      display.drawLine(e0 + 1, y + 3, e0, y + 4, BLACK);
      display.drawLine(e1, y + 3, e1 + 1, y + 4, BLACK);
      display.drawLine(e1 + 1, y + 3, e1, y + 4, BLACK);
      break;
    case REJECT:
      display.fillRect(e0, y + 3, 2, 2, WHITE);
      display.fillRect(e1, y + 3, 2, 2, WHITE);
      display.drawLine(e0, y + 3, e0 + 1, y + 4, BLACK);
      display.drawLine(e1 + 1, y + 3, e1, y + 4, BLACK);
      display.drawPixel(e0, y + 4, BLACK);
      display.drawPixel(e1, y + 4, BLACK);
      break;
    case GROOM:
    case SNIFF:
      if (animPhase & 1) {
        display.fillRect(e1, y + 3, 2, 2, WHITE);
        display.drawPixel(e1, y + 4, BLACK);
      }
      break;
    case SPIN:
      // Dizzy dots instead of eyes.
      display.fillRect(e0, y + 3, 2, 2, WHITE);
      display.fillRect(e1, y + 3, 2, 2, WHITE);
      display.drawPixel(e0 + (animPhase & 1), y + 4, BLACK);
      display.drawPixel(e1 + ((animPhase + 1) & 1), y + 4, BLACK);
      break;
    default:
      // Sit / walk / stretch: keep baked neutral face.
      break;
  }
}

void drawLevelGear(int x, int y, bool faceRight) {
  if (dogLevel < 2 || pose == SLEEP) return;

  if (careStyle == STYLE_PLAYFUL) {
    // Body-check athlete path: sweatband -> kicks -> cape -> lightning.
    // Sweatband
    display.fillRect(x + 5, y + 2, 10, 2, WHITE);
    if (dogLevel >= 3) {
      // Sneakers
      display.fillRect(x + 3, y + 15, 5, 2, WHITE);
      display.fillRect(x + 16, y + 15, 5, 2, WHITE);
      display.drawPixel(x + 7, y + 15, BLACK);
      display.drawPixel(x + 20, y + 15, BLACK);
      // Speed dashes when moving
      if (pose == WALK_L || pose == WALK_R || pose == ZOOMIES || pose == DANCE) {
        int dx = faceRight ? -6 : 30;
        display.drawPixel(x + dx, y + 8 + (animPhase & 1), WHITE);
        display.drawPixel(x + dx + (faceRight ? -2 : 2), y + 10, WHITE);
        display.drawPixel(x + dx + (faceRight ? -3 : 3), y + 12, WHITE);
      }
    }
    if (dogLevel >= 4) {
      // Flutter cape
      int cx = faceRight ? x - 2 : x + SPRITE_W - 2;
      int flap = (animPhase & 1) ? 2 : 0;
      display.fillTriangle(cx, y + 4, cx + (faceRight ? -5 : 5), y + 8 + flap,
                           cx + (faceRight ? -2 : 2), y + 14, WHITE);
    }
    if (dogLevel >= 5) {
      // Lightning crest + spark aura
      int lx = x + 12;
      display.drawLine(lx, y - 5, lx + 3, y - 1, WHITE);
      display.drawLine(lx + 3, y - 1, lx + 1, y - 1, WHITE);
      display.drawLine(lx + 1, y - 1, lx + 4, y + 3, WHITE);
      if (animFrame) {
        drawSpark(x + 1, y + 3);
        drawSpark(x + SPRITE_W - 2, y + 5);
      }
    } else {
      // Bouncing training ball (lv2-4 companion)
      int bx = faceRight ? x + SPRITE_W + 1 : x - 4;
      int by = y + 8 + ((animPhase == 1 || animPhase == 3) ? -2 : 0);
      display.fillRect(bx, by, 3, 3, WHITE);
      display.drawPixel(bx + 1, by + 1, BLACK);
    }

  } else if (careStyle == STYLE_CHONKY) {
    // Unmask / soft path: scarf -> bloom -> beanie -> glow.
    // Scarf / mask ribbon
    display.fillRect(x + 6, y + 8, 12, 2, WHITE);
    int tipX = faceRight ? x + SPRITE_W - 1 : x + 2;
    display.drawLine(tipX, y + 10, tipX + (faceRight ? 3 : -3), y + 13 + (animPhase & 1), WHITE);
    display.drawPixel(tipX + (faceRight ? 2 : -2), y + 12, WHITE);

    if (dogLevel >= 3) {
      // Soft belly puff
      display.fillCircle(x + 12, y + 12, 3, WHITE);
      // Tiny bloom by ear
      int fx = faceRight ? x + 3 : x + SPRITE_W - 6;
      display.drawPixel(fx + 1, y + 1, WHITE);
      display.drawPixel(fx, y + 2, WHITE);
      display.drawPixel(fx + 2, y + 2, WHITE);
      display.drawPixel(fx + 1, y + 3, WHITE);
    }
    if (dogLevel >= 4) {
      // Cozy beanie
      display.fillRect(x + 6, y - 1, 12, 3, WHITE);
      display.fillRect(x + 8, y - 3, 8, 2, WHITE);
      display.fillRect(x + 16, y - 2, 3, 3, WHITE); // pom
    }
    if (dogLevel >= 5) {
      // Soft glow rings + floaty sparkles
      display.drawCircle(x + 12, y + 8, 14, WHITE);
      if (animFrame) display.drawCircle(x + 12, y + 8, 11, WHITE);
      drawSpark(x - 1, y + 2 + (animPhase & 1));
      drawSpark(x + SPRITE_W + 1, y + 6 - (animPhase & 1));
      if (animPhase == 0) drawHeart(x + 30, y);
      if (animPhase == 2) drawHeart(x - 6, y + 2);
    }

  } else {
    // Balanced path: tag -> wing -> crown -> orbit stars.
    // Collar tag
    display.fillRect(x + 9, y + 9, 6, 2, WHITE);
    display.fillRect(x + 11, y + 11, 2, 2, WHITE);
    display.drawPixel(x + 11, y + 11, BLACK);

    if (dogLevel >= 3) {
      // Little wing
      int wx = faceRight ? x + SPRITE_W - 4 : x;
      int flap = (animPhase & 1) ? 1 : 0;
      display.fillTriangle(wx, y + 6, wx + (faceRight ? 7 : -7), y + 4 - flap,
                           wx + (faceRight ? 5 : -5), y + 10, WHITE);
    }
    if (dogLevel >= 4) {
      // Crown
      display.fillRect(x + 7, y - 1, 10, 2, WHITE);
      display.drawPixel(x + 8, y - 3, WHITE);
      display.drawPixel(x + 11, y - 4, WHITE);
      display.drawPixel(x + 14, y - 3, WHITE);
      display.drawLine(x + 8, y - 2, x + 14, y - 2, WHITE);
    }
    if (dogLevel >= 5) {
      // Halo + orbiting stars
      display.drawCircle(x + 12, y - 2, 6, WHITE);
      int ox = ((animPhase & 1) ? 10 : -8);
      int oy = ((animPhase == 0 || animPhase == 2) ? -4 : 2);
      drawSpark(x + 12 + ox, y + 6 + oy);
      drawSpark(x + 12 - ox, y + 8 - oy);
    } else if (dogLevel >= 2 && animFrame) {
      drawSpark(x + SPRITE_W + 1, y + 2);
    }
  }
}

void drawPet(int x, int y) {
  bool faceRight = (pose == WALK_R || pose == REJECT ||
                    (pose == ZOOMIES && (goofThought & 1)) ||
                    (pose == SPIN && (animPhase & 1)) ||
                    (pose == DANCE && (animPhase == 1 || animPhase == 2)));
  bool walking = (pose == WALK_L || pose == WALK_R || pose == PLAYING ||
                  pose == STRETCH || pose == ZOOMIES || pose == DANCE);
  int dy = (dogLevel <= 1) ? 2 : 0;

  drawAnnoyingDog(x, y + dy, faceRight, walking);
  drawLevelGear(x, y + dy, faceRight);
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

  // 2. Button: 1 = body check, 2 = unmask, 3 = gauges
  readButton(currentMillis);

  // 3. Animation ticks (faster during zoomies / dance)
  bool phaseAdvanced = false;
  unsigned long frameMs = FRAME_MS;
  if (pose == SLEEP) frameMs = FRAME_MS * 3;
  else if (pose == ZOOMIES || pose == DANCE) frameMs = FRAME_MS * 2 / 3;
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
    if (fun > 75 && food > 75 && random(3) == 0) {
      idlePose = DANCE;
    } else {
      idlePose = IDLE_CHOICES[random(IDLE_CHOICE_COUNT)];
    }
    // Zoom direction: bit0 = face/run right
    if (idlePose == ZOOMIES) {
      goofThought = (dogX < 64) ? 1 : 0;
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
      if (dogX > 100) {
        dogX = 100;
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
    } else if (pose == ZOOMIES) {
      int step = (goofThought & 1) ? 2 : -2;
      dogX += step;
      if (dogX > 100) {
        dogX = 100;
        goofThought &= ~1;
        showingBonk = true;
        bonkUntil = currentMillis + 500;
      } else if (dogX < 0) {
        dogX = 0;
        goofThought |= 1;
        showingBonk = true;
        bonkUntil = currentMillis + 500;
      }
      markDirty();
    } else if (pose == DANCE && animPhase == 0) {
      dogX = constrain(dogX + (goofThought & 1 ? 2 : -2), 0, 100);
    } else if (pose == SPIN && animPhase == 0) {
      dogX = constrain(dogX + (random(2) ? 1 : -1), 0, 100);
    } else if (pose == HAPPY && animPhase == 0) {
      dogX = constrain(dogX + (goofThought & 1 ? 1 : -1), 0, 100);
    } else if (pose == SIT && animPhase == 0 && random(5) == 0) {
      dogX = constrain(dogX + (random(2) ? 1 : -1), 0, 100);
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
    drawGauge(9, "BODY", fun, fun <= CRITICAL_LEVEL && animFrame);
  }
  if (showStats || food <= LOW_LEVEL) {
    drawGauge(17, "UNMASK", food, food <= CRITICAL_LEVEL && animFrame);
  }
  if (showStats) {
    drawCareGauge(25);
    display.setTextSize(1);
    display.setCursor(0, 34);
    if (careStyle == STYLE_PLAYFUL) display.print(F("C"));
    else if (careStyle == STYLE_CHONKY) display.print(F("U"));
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
      // Small ground bob only — legs already animate in the sprite.
      drawPet(x + tip[animPhase], y + ((animPhase & 1) ? 1 : 0));
      if (showingBonk) drawBonkStars(x, y);
      break;

    case WALK_L:
      drawPet(x + tip[animPhase], y + ((animPhase & 1) ? 1 : 0));
      if (showingBonk) drawBonkStars(x, y);
      break;

    case SLEEP: {
      // Loaf rests on the floor; tiny side-to-side breath.
      drawPet(x + ((animPhase == 3) ? 1 : 0), y + 2);
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

    case ZOOMIES:
      drawPet(x + tip[animPhase], y + hopBob[animPhase]);
      if (showingBonk) drawBonkStars(x, y);
      break;

    case SPIN:
      drawPet(x + tip[animPhase] * 2, y + softBob[animPhase]);
      if (animPhase == 0 || animPhase == 2) {
        display.drawPixel(x + 14, y - 2, WHITE);
        display.drawPixel(x + 18, y - 4, WHITE);
      }
      break;

    case TIPOVER:
      // Floppy tip — looks like it forgot gravity for a second.
      drawPet(x + tip[animPhase] * 3, y + 4 + softBob[animPhase]);
      break;

    case SNIFF:
      drawPet(x + ((animPhase & 1) ? 1 : 0), y + 5 + ((animPhase & 1) ? 1 : 0));
      break;

    case DANCE:
      drawPet(x + tip[animPhase] * 2, y + bigBob[animPhase]);
      if (animPhase & 1) drawHeart(x + 32, y - 4);
      if (animPhase == 2) drawHeart(x - 2, y - 2);
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
