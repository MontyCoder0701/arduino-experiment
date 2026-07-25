// AnnoyingDogPet.ino
//
// Virtual dog pet with two needs: PLAY and FOOD.
// Wiring: one push button between D2 and GND (uses the internal pull-up).
//   single press  -> pet the dog
//   double press  -> feed the dog
//   hold 3 seconds -> show hunger / care gauges
//   hold 5 seconds -> wipe memory and start a brand-new dog
#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const uint8_t BUTTON_PIN = 2;

// ==========================================
//   ANNOYING DOG PIXEL ART (32x20)
// ==========================================
const uint8_t SPRITE_W = 32;
const uint8_t SPRITE_H = 20;

const unsigned char PROGMEM dog_sit1[] = {
  0x00,0x00,0x00,0x00,0x37,0xf3,0x00,0x00,0x3f,0xff,0x00,0x00,
  0x3f,0xff,0xc0,0x00,0xf7,0xbf,0x80,0x00,0xff,0xff,0xf8,0x03,
  0xf8,0x7f,0xfe,0x06,0xc9,0xef,0xff,0xff,0xf0,0x1f,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfc,
  0x3f,0xff,0xff,0xfc,0x38,0x70,0x0e,0x1c,0x38,0x70,0x0e,0x1c,
  0x30,0x70,0x0e,0x10,0x00,0x60,0x08,0x00
};
const unsigned char PROGMEM dog_sit2[] = {
  0x00,0x00,0x00,0x00,0x37,0xf3,0x00,0x00,0x3f,0xff,0x00,0x00,
  0x3f,0xff,0xc0,0x00,0xf7,0xbf,0x80,0x06,0xff,0xff,0xf8,0x03,
  0xf8,0x7f,0xfe,0x06,0xc9,0xef,0xff,0xf0,0xf0,0x1f,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfc,
  0x3f,0xff,0xff,0xfc,0x38,0x70,0x0e,0x1c,0x38,0x70,0x0e,0x1c,
  0x30,0x70,0x0e,0x10,0x00,0x60,0x08,0x00
};
const unsigned char PROGMEM dog_walk_r1[] = {
  0x00,0x00,0x00,0x00,0x00,0x00,0xcf,0xec,0x00,0x00,0xff,0xfc,
  0x00,0x03,0xff,0xfc,0x00,0x01,0xfd,0xef,0xc0,0x1f,0xff,0xff,
  0x60,0x7f,0xfe,0x1f,0xff,0xff,0xf7,0x93,0xff,0xff,0xf8,0x0f,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x3f,0xff,0xff,0xff,
  0x3f,0xff,0xff,0xfc,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,
  0x0c,0x0c,0x0c,0x0c,0x00,0x00,0x00,0x00
};
const unsigned char PROGMEM dog_walk_r2[] = {
  0x00,0x00,0x00,0x00,0x00,0x00,0xcf,0xec,0x00,0x00,0xff,0xfc,
  0x00,0x03,0xff,0xfc,0x00,0x01,0xfd,0xef,0xc0,0x1f,0xff,0xff,
  0x60,0x7f,0xfe,0x1f,0xff,0xff,0xf7,0x93,0xff,0xff,0xf8,0x0f,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x3f,0xff,0xff,0xff,
  0x3f,0xff,0xff,0xfc,0x0f,0x0f,0xf0,0xf0,0x0f,0x0f,0xf0,0xf0,
  0x06,0x06,0x60,0x60,0x00,0x00,0x00,0x00
};
const unsigned char PROGMEM dog_walk_l1[] = {
  0x00,0x00,0x00,0x00,0x37,0xf3,0x00,0x00,0x3f,0xff,0x00,0x00,
  0x3f,0xff,0xc0,0x00,0xf7,0xbf,0x80,0x00,0xff,0xff,0xf8,0x03,
  0xf8,0x7f,0xfe,0x06,0xc9,0xef,0xff,0xff,0xf0,0x1f,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfc,
  0x3f,0xff,0xff,0xfc,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,
  0x30,0x30,0x30,0x30,0x00,0x00,0x00,0x00
};
const unsigned char PROGMEM dog_walk_l2[] = {
  0x00,0x00,0x00,0x00,0x37,0xf3,0x00,0x00,0x3f,0xff,0x00,0x00,
  0x3f,0xff,0xc0,0x00,0xf7,0xbf,0x80,0x00,0xff,0xff,0xf8,0x03,
  0xf8,0x7f,0xfe,0x06,0xc9,0xef,0xff,0xff,0xf0,0x1f,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfc,
  0x3f,0xff,0xff,0xfc,0x0f,0x0f,0xf0,0xf0,0x0f,0x0f,0xf0,0xf0,
  0x06,0x06,0x60,0x60,0x00,0x00,0x00,0x00
};
const unsigned char PROGMEM dog_sleep[] = {
  0x00,0x00,0x00,0x00,0x37,0xf3,0x00,0x00,0x3f,0xff,0x00,0x00,
  0x3f,0xff,0xc0,0x00,0xf7,0xbf,0x80,0x00,0xff,0xff,0xf8,0x03,
  0xff,0xff,0xfe,0x06,0xe3,0x1f,0xff,0xff,0xf0,0x1f,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfc,
  0x3f,0xff,0xff,0xfc,0x38,0x70,0x0e,0x1c,0x38,0x70,0x0e,0x1c,
  0x30,0x70,0x0e,0x10,0x00,0x60,0x08,0x00
};
const unsigned char PROGMEM dog_play[] = {
  0x00,0x00,0x00,0x00,0x37,0xf3,0x00,0x00,0x3f,0xff,0x00,0x00,
  0x3f,0xff,0xc0,0x00,0xf7,0xbf,0x80,0x00,0xff,0xff,0xf8,0x03,
  0xf8,0x7f,0xfe,0x06,0xc9,0xef,0xff,0xff,0xf0,0x1f,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfc,
  0x3f,0xff,0xff,0xfc,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,
  0x30,0x30,0x30,0x30,0x00,0x00,0x00,0x00
};
const unsigned char PROGMEM dog_shock[] = {
  0x00,0x00,0x00,0x00,0x37,0xf3,0x00,0x00,0x3f,0xff,0x00,0x00,
  0x3f,0xff,0xc0,0x00,0xf7,0xbf,0x80,0x00,0xff,0xff,0xf8,0x03,
  0xe3,0x1f,0xfe,0x06,0xe3,0x1f,0xff,0xff,0xf0,0x1f,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfc,
  0x3f,0xff,0xff,0xfc,0x38,0x70,0x0e,0x1c,0x38,0x70,0x0e,0x1c,
  0x30,0x70,0x0e,0x10,0x00,0x60,0x08,0x00
};
const unsigned char PROGMEM dog_bored[] = {
  0x00,0x00,0x00,0x00,0x37,0xf3,0x00,0x00,0x3f,0xff,0x00,0x00,
  0x3f,0xff,0xc0,0x00,0xf7,0xbf,0x80,0x00,0xff,0xff,0xf8,0x03,
  0xfc,0xff,0xfe,0x06,0xc9,0xef,0xff,0xff,0xf0,0x1f,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfc,
  0x3f,0xff,0xff,0xfc,0x38,0x70,0x0e,0x1c,0x38,0x70,0x0e,0x1c,
  0x30,0x70,0x0e,0x10,0x00,0x60,0x08,0x00
};
const unsigned char PROGMEM dog_beg[] = {
  0x00,0x00,0x00,0x00,0x37,0xf3,0x00,0x00,0x3f,0xff,0x00,0x00,
  0x3f,0xff,0xc0,0x00,0xf7,0xbf,0x80,0x00,0xff,0xff,0xf8,0x03,
  0xe0,0x1f,0xfe,0x06,0xc1,0x0f,0xff,0xff,0xe0,0x1f,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfc,
  0x3f,0xff,0xff,0xfc,0x38,0x70,0x0e,0x1c,0x38,0x70,0x0e,0x1c,
  0x30,0x70,0x0e,0x10,0x00,0x60,0x08,0x00
};
const unsigned char PROGMEM dog_eat[] = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x3f,0xff,0xc0,0x00,0xf7,0xbf,0x80,0x00,0xff,0xff,0xf8,0x03,
  0xf8,0x7f,0xfe,0x06,0xc9,0xef,0xff,0xff,0xf0,0x1f,0xff,0xff,
  0xf0,0x3f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfc,
  0x3f,0xff,0xff,0xfc,0x38,0x70,0x0e,0x1c,0x38,0x70,0x0e,0x1c,
  0x30,0x70,0x0e,0x10,0x00,0x60,0x08,0x00
};
const unsigned char PROGMEM dog_groom1[] = {
  0x00,0x00,0x00,0x00,0x37,0xf3,0x00,0x00,0x3f,0xff,0x00,0x00,
  0x3f,0xff,0xc0,0x00,0xf7,0xbf,0x80,0x00,0xff,0xff,0xf8,0x03,
  0xf8,0x7f,0xfe,0x06,0xc9,0xef,0xff,0xff,0xf0,0x1f,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfc,
  0x3f,0xff,0xff,0xfc,0x38,0x70,0x0e,0x1c,0x38,0x70,0x0e,0x1c,
  0x30,0x70,0x0e,0x10,0x00,0x60,0x08,0x00
};
const unsigned char PROGMEM dog_groom2[] = {
  0x00,0x00,0x00,0x00,0x37,0xf3,0x00,0x00,0x3f,0xff,0x00,0x00,
  0x3f,0xff,0xc0,0x00,0xf7,0xbf,0x80,0x00,0xff,0xff,0xf8,0x03,
  0xf8,0x7f,0xfe,0x06,0xc9,0xef,0xff,0xff,0xf0,0x1f,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfc,
  0x3f,0xff,0xff,0xfc,0x38,0x70,0x0e,0x1c,0x38,0x70,0x0e,0x1c,
  0x30,0x70,0x0e,0x10,0x00,0x60,0x08,0x00
};
const unsigned char PROGMEM dog_stretch1[] = {
  0x00,0x00,0x00,0x00,0x37,0xf3,0x00,0x00,0x3f,0xff,0x00,0x00,
  0x3f,0xff,0xc0,0x00,0xf7,0xbf,0x80,0x00,0xff,0xff,0xf8,0x03,
  0xf8,0x7f,0xfe,0x06,0xc9,0xef,0xff,0xff,0xf0,0x1f,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfc,
  0x3f,0xff,0xff,0xfc,0xf0,0xf0,0x0f,0x0f,0xf0,0xf0,0x0f,0x0f,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
const unsigned char PROGMEM dog_stretch2[] = {
  0x00,0x00,0x00,0x00,0x37,0xf3,0x00,0x00,0x3f,0xff,0x00,0x00,
  0x3f,0xff,0xc0,0x00,0xf7,0xbf,0x80,0x00,0xff,0xff,0xf8,0x03,
  0xf8,0x7f,0xfe,0x06,0xc9,0xef,0xff,0xff,0xf0,0x1f,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfc,
  0x3f,0xff,0xff,0xfc,0x78,0x78,0x1e,0x1e,0x78,0x78,0x1e,0x1e,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
const unsigned char PROGMEM dog_happy1[] = {
  0x00,0x00,0x00,0x00,0x37,0xf3,0x00,0x00,0x3f,0xff,0x00,0x00,
  0x3f,0xff,0xc0,0x00,0xf7,0xbf,0x80,0x00,0xff,0xff,0xf8,0x03,
  0xf8,0x7f,0xfe,0x06,0xc9,0xef,0xff,0xff,0xf0,0x1f,0xff,0xff,
  0xfc,0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfc,
  0x3f,0xff,0xff,0xfc,0x38,0x70,0x0e,0x1c,0x38,0x70,0x0e,0x1c,
  0x30,0x70,0x0e,0x10,0x00,0x60,0x08,0x00
};
const unsigned char PROGMEM dog_happy2[] = {
  0x00,0x00,0x00,0x00,0x37,0xf3,0x00,0x00,0x3f,0xff,0x00,0x00,
  0x3f,0xff,0xc0,0x00,0xf7,0xbf,0x80,0x06,0xff,0xff,0xf8,0x03,
  0xf8,0x7f,0xfe,0x06,0xc9,0xef,0xff,0xf0,0xf0,0x1f,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfc,
  0x3f,0xff,0xff,0xfc,0x38,0x70,0x0e,0x1c,0x38,0x70,0x0e,0x1c,
  0x30,0x70,0x0e,0x10,0x00,0x60,0x08,0x00
};
const unsigned char PROGMEM dog_reject1[] = {
  0x00,0x00,0x00,0x00,0x00,0x00,0xcf,0xec,0x00,0x00,0xff,0xfc,
  0x00,0x03,0xff,0xfc,0x00,0x01,0xfd,0xef,0xc0,0x1f,0xff,0xff,
  0x60,0x7f,0xfe,0x1f,0xff,0xff,0xf7,0x93,0xff,0xff,0xf8,0x0f,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x3f,0xff,0xff,0xff,
  0x3f,0xff,0xff,0xfc,0x38,0x70,0x0e,0x1c,0x38,0x70,0x0e,0x1c,
  0x08,0x70,0x0e,0x0c,0x00,0x10,0x06,0x00
};
const unsigned char PROGMEM dog_reject2[] = {
  0x00,0x00,0x00,0x00,0x00,0x00,0xcf,0xec,0x00,0x00,0xff,0xfc,
  0x00,0x03,0xff,0xfc,0x60,0x01,0xfd,0xef,0xc0,0x1f,0xff,0xff,
  0x60,0x7f,0xfe,0x1f,0x0f,0xff,0xf7,0x93,0xff,0xff,0xf8,0x0f,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x3f,0xff,0xff,0xff,
  0x3f,0xff,0xff,0xfc,0x38,0x70,0x0e,0x1c,0x38,0x70,0x0e,0x1c,
  0x08,0x70,0x0e,0x0c,0x00,0x10,0x06,0x00
};

enum Pose {
  WALK_R, WALK_L, SIT, SLEEP, GROOM, STRETCH, HAPPY,
  PLAYING, EATING, BORED, HUNGRY, MISERABLE, REJECT
};
Pose pose = SIT;
Pose idlePose = SIT;

const Pose IDLE_CHOICES[] = { WALK_R, WALK_L, SIT, GROOM, STRETCH, HAPPY };
const uint8_t IDLE_CHOICE_COUNT = sizeof(IDLE_CHOICES) / sizeof(IDLE_CHOICES[0]);

int dogX = 48;
const int dogY = 40; // Locks 32x20 dog to the bottom grassline
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

// --- NEEDS ENGINE ---
int fun = 70;   // playfulness, 0..100
int food = 70;  // fullness, 0..100
const int LOW_LEVEL = 25;
const int CRITICAL_LEVEL = 12;
const int FULL_LEVEL = 90;                // too satisfied / too full to accept more
const unsigned long FUN_DRAIN_MS = 10000;  // one point of boredom every 10 seconds
const unsigned long FOOD_DRAIN_MS = 15000; // one point of hunger every 15 seconds
const unsigned long ACTION_MS = 2200;      // how long a play/feed reaction lasts
const unsigned long IDLE_CHANGE_MS = 2400; // swap goofy idle loops often
const unsigned long NAP_AFTER_MS = 20000;  // no interaction -> dog naps
const unsigned long FRAME_MS = 130;        // frantic derpy animation tick
const unsigned long DAY_MS = 86400000UL;   // one powered-on day

// --- BUTTON ENGINE ---
// single tap = pet, double tap = feed
// hold 3s = show hunger/care gauges
// hold 5s = full reset
const unsigned long DEBOUNCE_MS = 40;
const unsigned long DOUBLE_GAP_MS = 600;
const unsigned long STATS_HOLD_MS = 3000;  // show gauges
const unsigned long LONG_PRESS_MS = 5000;  // factory reset
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

const char NAME_0[] PROGMEM = "Bonk";
const char NAME_1[] PROGMEM = "Doof";
const char NAME_2[] PROGMEM = "Goof";
const char NAME_3[] PROGMEM = "Blob";
const char NAME_4[] PROGMEM = "Chonk";
const char NAME_5[] PROGMEM = "Boop";
const char NAME_6[] PROGMEM = "Derp";
const char NAME_7[] PROGMEM = "Potato";
const char NAME_8[] PROGMEM = "Bean";
const char NAME_9[] PROGMEM = "Dummy";
const char *const DOG_NAMES[] PROGMEM = {
  NAME_0, NAME_1, NAME_2, NAME_3, NAME_4,
  NAME_5, NAME_6, NAME_7, NAME_8, NAME_9
};
const uint8_t DOG_NAME_COUNT = 10;
uint8_t nameIndex = 0;
uint16_t ageDays = 1;
char statusBuf[22]; // reusable RAM for captions copied from flash
char nameBuf[12];

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
  noteInteraction(now);
}

void playWithDog(unsigned long now) {
  noteInteraction(now);
  if (food <= CRITICAL_LEVEL) {
    // Too weak for pets, it just flops over
    fun = clampLevel(fun + 3);
    markDirty();
    saveState();
    startAction(MISERABLE, now);
    return;
  }
  if (fun >= FULL_LEVEL) {
    // Already fully satisfied — politely dodges the hand
    rejectWasFood = false;
    startAction(REJECT, now);
    return;
  }
  fun = clampLevel(fun + 20);
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
  noteInteraction(now);
  if (food >= FULL_LEVEL) {
    // Too stuffed — pushes the bowl away
    rejectWasFood = true;
    startAction(REJECT, now);
    return;
  }
  food = clampLevel(food + 24);
  fun = clampLevel(fun + 4); // dinner is its own kind of fun
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
      // Stats hold or reset hold should not count as pet/feed.
      if (longPressDone || held >= STATS_HOLD_MS) {
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

  if (stableButton == LOW && !longPressDone && holdMs >= LONG_PRESS_MS) {
    longPressDone = true;
    holdMs = 0;
    clickCount = 0;
    factoryReset(now);
    return;
  }

  if (longPressDone || stableButton == LOW) return;

  if (clickCount >= 2) {
    feedDog(now);
    clickCount = 0;
  } else if (clickCount == 1 && now - lastClickAt > DOUBLE_GAP_MS) {
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
  if (careStyle == STYLE_PLAYFUL) {
    switch (dogLevel) {
      case 2:  return PSTR("BALL ROOKIE");
      case 3:  return PSTR("ZOOM CADET");
      case 4:  return PSTR("TRACK STAR");
      default: return PSTR("ZOOM CHAMPION");
    }
  }
  if (careStyle == STYLE_CHONKY) {
    switch (dogLevel) {
      case 2:  return PSTR("SNACK PUP");
      case 3:  return PSTR("BIG LOAF");
      case 4:  return PSTR("CHEF CHONK");
      default: return PSTR("SUPREME LOAF");
    }
  }
  switch (dogLevel) {
    case 2:  return PSTR("GOOD PUP");
    case 3:  return PSTR("FINE DOG");
    case 4:  return PSTR("PARTY DOG");
    default: return PSTR("ROYAL DOG");
  }
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

const char *statusText() {
  if (showingLevelUp) {
    if (careStyle == STYLE_PLAYFUL) return flashStatus(PSTR("SPORTS DOG!!"));
    if (careStyle == STYLE_CHONKY) return flashStatus(PSTR("CHONK EVOLVED"));
    return flashStatus(PSTR("GOOD DOG!!"));
  }
  if (showingReset) return flashStatus(PSTR("uhhh... who am i"));
  if (holdMs >= STATS_HOLD_MS && holdMs < LONG_PRESS_MS) {
    return flashStatus(PSTR("hold to reset"));
  }
  if (showingBonk) return flashStatus(PSTR("BONK"));

  switch (pose) {
    case PLAYING:
      if (careStyle == STYLE_PLAYFUL) return flashStatus(PSTR("ZOOMIES!!"));
      if (careStyle == STYLE_CHONKY) return flashStatus(PSTR("oof.. pet"));
      return flashStatus(PSTR("pet pet"));
    case EATING:
      if (careStyle == STYLE_PLAYFUL) return flashStatus(PSTR("fuel up!"));
      if (careStyle == STYLE_CHONKY) return flashStatus(PSTR("MORE nom"));
      return flashStatus(PSTR("nom nom"));
    case REJECT:
      if (rejectWasFood) {
        return flashStatus(careStyle == STYLE_CHONKY ? PSTR("too round already") : PSTR("no. full. rock."));
      }
      return flashStatus(careStyle == STYLE_PLAYFUL ? PSTR("already zoomed") : PSTR("too silly already"));
    case MISERABLE: return flashStatus(PSTR("i am a pancake"));
    case HUNGRY:
      return flashStatus(careStyle == STYLE_CHONKY ? PSTR("FEED. NOW.") : PSTR("feed me"));
    case BORED:
      return flashStatus(careStyle == STYLE_PLAYFUL ? PSTR("THROW BALL") : PSTR("play?"));
    case HAPPY:
      if (careStyle == STYLE_PLAYFUL) return flashStatus(PSTR("wag athlete"));
      if (careStyle == STYLE_CHONKY) return flashStatus(PSTR("loaf mode"));
      return flashStatus(PSTR(""));
    case SLEEP:
    case GROOM:
    case STRETCH:
    case WALK_R:
    case WALK_L:
    case SIT:
    default:
      return flashStatus(PSTR(""));
  }
}

void drawFormBody(int x, int y) {
  // Distinct silhouette per evolution form (visible on mono OLED).
  if (careStyle == STYLE_CHONKY && dogLevel >= 2) {
    uint8_t r = 4 + (dogLevel >= 4 ? 2 : 0) + (dogLevel >= 5 ? 1 : 0);
    display.fillCircle(x + 16, y + 15, r, WHITE);
    display.fillRect(x + 9, y + 15, 16, 4, WHITE);
    display.fillRect(x + 8, y + 18, 5, 2, WHITE);
    display.fillRect(x + 20, y + 18, 5, 2, WHITE);
  } else if (careStyle == STYLE_PLAYFUL && dogLevel >= 2) {
    display.fillRect(x + 10, y + 15, 12, 2, BLACK);
    int bx = x - 5 + ((animPhase == 1) ? 2 : ((animPhase == 3) ? -1 : 0));
    int by = y + 9 - ((animPhase & 1) ? 2 : 0);
    display.fillCircle(bx, by, 2, WHITE);
    display.drawPixel(bx, by, BLACK);
  } else if (careStyle == STYLE_BALANCED && dogLevel >= 2) {
    display.fillRect(x + 12, y - 1, 4, 2, WHITE);
    display.drawPixel(x + 13, y - 2, WHITE);
  }
}

void drawLevelLook(int x, int y) {
  if (dogLevel >= 2) {
    if (careStyle == STYLE_PLAYFUL) {
      display.fillRect(x + 7, y + 11, 17, 3, BLACK);
      display.fillCircle(x + 3, y + 14, 2, WHITE);
      display.drawPixel(x + 3, y + 14, BLACK);
    } else if (careStyle == STYLE_CHONKY) {
      display.fillRect(x + 6, y + 12, 20, 3, BLACK);
      display.fillRect(x + 14, y + 15, 5, 4, WHITE);
    } else {
      display.fillRect(x + 7, y + 11, 17, 3, BLACK);
      display.drawLine(x + 7, y + 11, x + 23, y + 11, WHITE);
      display.drawLine(x + 7, y + 13, x + 23, y + 13, WHITE);
      display.fillRect(x + 14, y + 14, 4, 5, WHITE);
      display.drawPixel(x + 15, y + 16, BLACK);
    }
  }

  if (dogLevel >= 3) {
    if (careStyle == STYLE_PLAYFUL) {
      display.fillRect(x + 4, y + 2, 15, 3, WHITE);
      display.fillRect(x + 4, y + 3, 15, 1, BLACK);
    } else if (careStyle == STYLE_CHONKY) {
      display.fillTriangle(x + 5, y + 9, x + 18, y + 9, x + 11, y + 18, WHITE);
      display.drawLine(x + 8, y + 12, x + 14, y + 12, BLACK);
    } else {
      display.fillTriangle(x - 3, y + 8, x + 6, y + 10, x - 3, y + 18, WHITE);
      display.drawLine(x - 2, y + 10, x - 2, y + 16, BLACK);
    }
  }

  if (dogLevel == 4) {
    if (careStyle == STYLE_PLAYFUL) {
      display.fillRect(x + 5, y - 3, 18, 4, WHITE);
      display.fillRect(x + 1, y - 1, 9, 2, WHITE);
    } else if (careStyle == STYLE_CHONKY) {
      display.fillRect(x + 9, y - 2, 12, 4, WHITE);
      display.fillCircle(x + 15, y - 6, 5, WHITE);
      display.fillRect(x + 12, y - 4, 2, 2, BLACK);
    } else {
      display.fillTriangle(x + 6, y + 1, x + 20, y + 1, x + 13, y - 10, WHITE);
      display.fillRect(x + 6, y + 1, 15, 2, WHITE);
      display.fillRect(x + 12, y - 12, 3, 3, WHITE);
    }
  }

  if (dogLevel >= 5) {
    if (careStyle == STYLE_PLAYFUL) {
      display.fillCircle(x + 14, y - 5, 4, WHITE);
      display.fillRect(x + 12, y - 1, 5, 6, WHITE);
      display.drawPixel(x + 14, y - 5, BLACK);
      display.fillTriangle(x + 28, y + 4, x + 36, y + 6, x + 30, y + 16, WHITE);
      if (animFrame) {
        display.fillCircle(x - 4, y + 6, 2, WHITE);
        display.drawPixel(x - 4, y + 6, BLACK);
      }
    } else if (careStyle == STYLE_CHONKY) {
      display.fillRect(x + 7, y - 3, 16, 4, WHITE);
      display.fillRect(x + 9, y - 6, 3, 3, WHITE);
      display.fillRect(x + 14, y - 7, 3, 4, WHITE);
      display.fillRect(x + 19, y - 6, 3, 3, WHITE);
      display.fillCircle(x - 2 + (animPhase & 1), y + 8, 1, WHITE);
      display.fillCircle(x + 31, y + 6 + (animPhase == 2 ? 1 : 0), 1, WHITE);
      display.fillCircle(x + 28, y + 14, 1, WHITE);
    } else {
      display.fillRect(x + 7, y - 3, 16, 4, WHITE);
      display.fillRect(x + 7, y - 6, 3, 3, WHITE);
      display.fillRect(x + 13, y - 8, 4, 5, WHITE);
      display.fillRect(x + 20, y - 6, 3, 3, WHITE);
      display.fillTriangle(x + 27, y + 5, x + 35, y + 8, x + 29, y + 19, WHITE);
      display.drawLine(x + 28, y + 8, x + 30, y + 16, BLACK);
      if (animFrame) {
        display.drawPixel(x + 3, y - 4, WHITE);
        display.drawPixel(x + 24, y - 5, WHITE);
      }
    }
  }
}

void drawDog(int x, int y, const unsigned char *bitmap) {
  int dy = (dogLevel <= 1) ? 4 : 0;
  if (careStyle == STYLE_PLAYFUL && dogLevel >= 2) dy -= 1;
  if (careStyle == STYLE_CHONKY && dogLevel >= 2) dy += 1;
  if (dogLevel >= 5 && careStyle == STYLE_BALANCED) dy -= 1;

  display.drawBitmap(x, y + dy, bitmap, SPRITE_W, SPRITE_H, WHITE);
  drawFormBody(x, y + dy);
  drawLevelLook(x, y + dy);
}

void drawPant(int x, int y) {
  // Big dumb tongue
  display.drawLine(x + 7, y + 11, x + 7, y + 14, WHITE);
  display.drawPixel(x + 8, y + 14, WHITE);
  display.drawPixel(x + 6, y + 14, WHITE);
}

void drawPettingHand(int x, int y) {
  // Wrist coming down from above
  display.drawLine(x + 4, y - 5, x + 4, y, WHITE);
  display.drawLine(x + 5, y - 5, x + 5, y, WHITE);

  // Palm and four little fingers
  display.fillRect(x + 2, y, 7, 4, WHITE);
  display.drawPixel(x, y + 2, WHITE);
  display.drawPixel(x + 1, y + 3, WHITE);
  display.drawPixel(x + 9, y + 1, WHITE);
  display.drawPixel(x + 10, y + 2, WHITE);
}

void drawBonkStars(int x, int y) {
  display.setTextSize(1);
  display.setCursor(x + 10, y - 8);
  display.print(animPhase & 1 ? "*" : "+");
  display.setCursor(x + 20, y - 6);
  display.print(animPhase & 1 ? "+" : "*");
}

void loop() {
  unsigned long currentMillis = millis();

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

  // 2. Button: one press plays, two presses feed
  readButton(currentMillis);

  // 3. Animation ticks (slower while napping)
  bool phaseAdvanced = false;
  unsigned long frameMs = FRAME_MS;
  if (pose == SLEEP) frameMs = FRAME_MS * 3;
  else if (careStyle == STYLE_PLAYFUL) frameMs = FRAME_MS - 30;
  else if (careStyle == STYLE_CHONKY) frameMs = FRAME_MS + 40;
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

  // 6. Position Physics — form-specific movement
  if (phaseAdvanced) {
    int step = (careStyle == STYLE_PLAYFUL) ? 2 : ((careStyle == STYLE_CHONKY) ? ((animPhase & 1) ? 1 : 0) : 1);
    if (pose == WALK_R) {
      dogX += step + ((careStyle == STYLE_PLAYFUL && animPhase == 2) ? 1 : 0);
      if (dogX > 96) {
        dogX = 96;
        idlePose = WALK_L;
        showingBonk = true;
        bonkUntil = currentMillis + 700;
      }
      markDirty();
    } else if (pose == WALK_L) {
      dogX -= step + ((careStyle == STYLE_PLAYFUL && animPhase == 1) ? 1 : 0);
      if (dogX < 0) {
        dogX = 0;
        idlePose = WALK_R;
        showingBonk = true;
        bonkUntil = currentMillis + 700;
      }
      markDirty();
    } else if (pose == HAPPY && animPhase == 0) {
      int scoot = (careStyle == STYLE_PLAYFUL) ? 2 : 1;
      dogX = constrain(dogX + (goofThought & 1 ? scoot : -scoot), 0, 96);
    } else if (pose == SIT && animPhase == 0 && careStyle != STYLE_CHONKY && random(5) == 0) {
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

  // Need gauges: auto when low, or hold 3s to inspect anytime
  bool showStats = (holdMs >= STATS_HOLD_MS);
  if (showStats || fun <= LOW_LEVEL) {
    drawGauge(9, "PLAY", fun, fun <= CRITICAL_LEVEL && animFrame);
  }
  if (showStats || food <= LOW_LEVEL) {
    drawGauge(17, "FOOD", food, food <= CRITICAL_LEVEL && animFrame);
  }
  if (showStats) {
    drawCareGauge(25);
    display.setTextSize(1);
    display.setCursor(0, 34);
    if (careStyle == STYLE_PLAYFUL) display.print(F("SPORT"));
    else if (careStyle == STYLE_CHONKY) display.print(F("CHONK"));
    else display.print(F("BALANCED"));
  }

  // Level-up gets its own banner; otherwise show the mood caption
  if (showingLevelUp) {
    drawLevelUpBanner();
  } else {
    printCentered(showStats ? 42 : 27, statusText());
  }

  // After 3s, remaining hold fills toward factory reset at 5s
  if (holdMs >= STATS_HOLD_MS && holdMs < LONG_PRESS_MS) {
    unsigned long span = LONG_PRESS_MS - STATS_HOLD_MS;
    int fillW = (int)(((holdMs - STATS_HOLD_MS) * 100UL) / span);
    if (fillW > 100) fillW = 100;
    display.drawRect(14, 50, 100, 4, WHITE);
    if (fillW > 0) display.fillRect(14, 50, fillW, 4, WHITE);
  }

  // Draw Floor Ground Line
  display.drawLine(0, 60, 128, 60, WHITE);

  // Evolution-form motion profiles
  int8_t softBob[4];
  int8_t hopBob[4];
  int8_t bigBob[4];
  int8_t tip[4];
  if (careStyle == STYLE_PLAYFUL) {
    softBob[0]=0; softBob[1]=-3; softBob[2]=1; softBob[3]=-2;
    hopBob[0]=0; hopBob[1]=-5; hopBob[2]=0; hopBob[3]=-6;
    bigBob[0]=0; bigBob[1]=-7; bigBob[2]=-1; bigBob[3]=-8;
    tip[0]=0; tip[1]=2; tip[2]=0; tip[3]=-2;
  } else if (careStyle == STYLE_CHONKY) {
    softBob[0]=0; softBob[1]=0; softBob[2]=1; softBob[3]=0;
    hopBob[0]=0; hopBob[1]=-1; hopBob[2]=1; hopBob[3]=-1;
    bigBob[0]=0; bigBob[1]=-2; bigBob[2]=1; bigBob[3]=-1;
    tip[0]=0; tip[1]=2; tip[2]=0; tip[3]=-2; // waddle
  } else {
    softBob[0]=0; softBob[1]=-2; softBob[2]=1; softBob[3]=-1;
    hopBob[0]=0; hopBob[1]=-3; hopBob[2]=1; hopBob[3]=-4;
    bigBob[0]=1; bigBob[1]=-5; bigBob[2]=0; bigBob[3]=-6;
    tip[0]=0; tip[1]=1; tip[2]=0; tip[3]=-1;
  }

  const int y = dogY;
  int x = dogX;

  if (showingBonk) {
    x += (animPhase & 1) ? 2 : -2;
  }

  switch (pose) {
    case SIT:
      drawDog(x + tip[animPhase], y + softBob[animPhase], (animPhase & 1) ? dog_sit2 : dog_sit1);
      if (careStyle == STYLE_PLAYFUL && (animPhase & 1)) drawPant(x, y + softBob[animPhase]);
      else if (careStyle != STYLE_PLAYFUL && animPhase == 2) drawPant(x, y + softBob[animPhase]);
      break;

    case WALK_R:
      drawDog(x + tip[animPhase], y + hopBob[animPhase], (animPhase & 1) ? dog_walk_r2 : dog_walk_r1);
      if (showingBonk) drawBonkStars(x, y);
      break;

    case WALK_L:
      drawDog(x + tip[animPhase], y + hopBob[animPhase], (animPhase & 1) ? dog_walk_l2 : dog_walk_l1);
      if (showingBonk) drawBonkStars(x, y);
      break;

    case SLEEP: {
      int breath = (careStyle == STYLE_CHONKY)
        ? ((animPhase <= 1) ? 0 : 2)
        : ((animPhase <= 1) ? 0 : 1);
      drawDog(x + ((animPhase == 3) ? 1 : 0), y + breath, dog_sleep);
      display.setTextSize(1);
      display.setCursor(x + 34, y - 1 - (animPhase & 1));
      if (careStyle == STYLE_CHONKY) display.print(F("Zzz"));
      else if (careStyle == STYLE_PLAYFUL) display.print(F("z!"));
      else display.print(F("zZ"));
      break;
    }

    case GROOM:
      drawDog(x, y + softBob[animPhase], (animPhase & 1) ? dog_groom2 : dog_groom1);
      display.drawPixel(x + animPhase, y + 10, WHITE);
      display.drawPixel(x + 3 + animPhase, y + 8 + (animPhase & 1), WHITE);
      break;

    case STRETCH:
      drawDog(x + ((animPhase & 1) ? 2 : -2), y + softBob[animPhase], (animPhase & 1) ? dog_stretch2 : dog_stretch1);
      break;

    case HAPPY:
      if (careStyle == STYLE_PLAYFUL) {
        // Rocket bounce
        drawDog(x + tip[animPhase], y + bigBob[animPhase], (animPhase & 1) ? dog_happy2 : dog_happy1);
        drawPant(x, y + bigBob[animPhase]);
        display.fillCircle(x - 6 + animPhase, y + 8 - animPhase, 2, WHITE);
      } else if (careStyle == STYLE_CHONKY) {
        // Happy loaf wobble
        drawDog(x + tip[animPhase] * 2, y + softBob[animPhase], (animPhase & 1) ? dog_happy2 : dog_sit1);
        drawHeart(x + 34, y - 2);
      } else {
        drawDog(x + tip[animPhase], y + bigBob[animPhase], (animPhase & 1) ? dog_happy2 : dog_happy1);
        drawPant(x, y + bigBob[animPhase]);
        drawHeart(x + 34, y - 3 - animPhase);
        drawHeart(x - 6, y - 6 + (animPhase & 1));
      }
      break;

    case PLAYING: {
      if (careStyle == STYLE_PLAYFUL) {
        // Chase the ball hard
        int by = y + bigBob[animPhase];
        drawDog(x + tip[animPhase], by, (animPhase & 1) ? dog_walk_l1 : dog_play);
        int bx = x - 8 - animPhase * 2;
        int bally = y + 12 - ((animPhase & 1) ? 5 : 0);
        display.fillCircle(bx, bally, 2, WHITE);
        drawPant(x, by);
      } else if (careStyle == STYLE_CHONKY) {
        // Slow grateful squish under hand
        const int8_t handDrop[4] = {1, 3, 4, 2};
        const int8_t dogSquish[4] = {1, 2, 3, 2};
        int by = y + dogSquish[animPhase];
        drawDog(x, by, dog_sit1);
        drawPettingHand(x + 5, y - 6 + handDrop[animPhase]);
        drawHeart(x + 34, y - 2);
      } else {
        const int8_t handDrop[4] = {0, 3, 4, 2};
        const int8_t dogSquish[4] = {0, 1, 2, 1};
        int by = y + dogSquish[animPhase];
        drawDog(x, by, (animPhase & 1) ? dog_happy2 : dog_happy1);
        drawPettingHand(x + 5, y - 8 + handDrop[animPhase]);
        if (animPhase == 1 || animPhase == 2) {
          drawPant(x, by);
          drawHeart(x + 34, y - 5);
        }
      }
      break;
    }

    case EATING: {
      int nod = (careStyle == STYLE_CHONKY) ? ((animPhase & 1) ? 3 : 0) : ((animPhase & 1) ? 2 : 0);
      drawDog(x + tip[animPhase], y + nod, (animPhase & 1) ? dog_eat : dog_sit1);
      drawBowl(x - 12, y + 14, true);
      display.drawPixel(x - 9, y + 10 - nod, WHITE);
      display.drawPixel(x - 5, y + 7 + nod, WHITE);
      if (careStyle == STYLE_CHONKY) {
        display.drawPixel(x + 2, y + 8, WHITE);
        display.drawPixel(x + 30, y + 12, WHITE);
        display.drawPixel(x + 26, y + 9, WHITE);
      }
      break;
    }

    case REJECT: {
      int shake = (careStyle == STYLE_PLAYFUL)
        ? ((animPhase == 0) ? -4 : ((animPhase == 2) ? 4 : 0))
        : ((animPhase == 0) ? -2 : ((animPhase == 2) ? 2 : 0));
      drawDog(x + shake, y + softBob[animPhase], (animPhase & 1) ? dog_reject2 : dog_reject1);
      display.setTextSize(1);
      display.setCursor(x + 30, y - 7);
      if (careStyle == STYLE_CHONKY) display.print(F("urp"));
      else if (careStyle == STYLE_PLAYFUL) display.print(F("GO!"));
      else display.print(F("NO"));
      break;
    }

    case HUNGRY:
      drawDog(x + tip[animPhase], y + softBob[animPhase], dog_beg);
      drawBowl(x - 12, y + 14, false);
      if (careStyle != STYLE_CHONKY) drawPant(x, y + softBob[animPhase]);
      display.setTextSize(1);
      display.setCursor(x + 10, y - 9 - (animPhase & 1));
      display.print(careStyle == STYLE_CHONKY ? F("!!") : F("!"));
      break;

    case BORED:
      drawDog(x + tip[animPhase], y + ((animPhase == 2) ? 1 : 0), dog_bored);
      display.setTextSize(1);
      display.setCursor(x + 34, y - (animPhase & 1));
      if (careStyle == STYLE_PLAYFUL) display.print(F("run?"));
      else display.print(F("..."));
      break;

    case MISERABLE:
      drawDog(x + ((animPhase & 1) ? 2 : -2), y + hopBob[animPhase], dog_shock);
      drawBonkStars(x, y);
      display.setTextSize(1);
      display.setCursor(x + 8, y - 10);
      display.print("ow");
      break;
  }

  display.display();
  delay(20);
}
