// AnnoyingDogPet.ino
//
// Virtual dog pet with two needs: PLAY and FOOD.
// Wiring: one push button between D2 and GND (uses the internal pull-up).
//   single press  -> play with the dog
//   double press  -> feed the dog
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
const uint8_t EEPROM_MAGIC = 0xC8; // bumped when level fields were added
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
const unsigned long SAVE_MS = 2000; // write at most every 2s when dirty
bool stateDirty = false;
unsigned long lastSave = 0;
unsigned long lastAgeDayAt = 0;

// --- LEVEL / GROWTH ---
const uint8_t MAX_DOG_LEVEL = 5;
// Base care needed at Lv1->2; each next level adds CARE_LEVEL_STEP more pets AND feeds.
const uint8_t CARE_BASE = 6;
const uint8_t CARE_LEVEL_STEP = 3;
uint8_t dogLevel = 1;
uint8_t petsSinceLevel = 0;
uint8_t feedsSinceLevel = 0;

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

void tryLevelUp(unsigned long now) {
  if (dogLevel >= MAX_DOG_LEVEL) return;
  // Higher levels need more consistent petting AND feeding.
  uint8_t need = CARE_BASE + (uint8_t)((dogLevel - 1) * CARE_LEVEL_STEP);
  if (petsSinceLevel < need || feedsSinceLevel < need) return;

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

void drawGauge(int y, const char *label, int value, bool blink) {
  display.setTextSize(1);
  display.setCursor(0, y);
  display.print(label);

  const int heartCount = (value + 9) / 10;
  for (int i = 0; i < 10; i++) {
    drawHeart(29 + i * 10, y, !blink && i < heartCount);
  }
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
  if (showingLevelUp) return flashStatus(PSTR("LEVEL UP!!"));
  if (showingReset) return flashStatus(PSTR("uhhh... who am i"));
  if (holdMs >= HOLD_HINT_MS) return flashStatus(PSTR("hold to reset"));
  if (showingBonk) return flashStatus(PSTR("BONK"));

  switch (pose) {
    case PLAYING:   return flashStatus(PSTR("pet pet"));
    case EATING:    return flashStatus(PSTR("nom nom"));
    case REJECT:
      return flashStatus(rejectWasFood ? PSTR("no. full. rock.") : PSTR("too silly already"));
    case MISERABLE: return flashStatus(PSTR("i am a pancake"));
    case HUNGRY:    return flashStatus(PSTR("feed me"));
    case BORED:     return flashStatus(PSTR("play?"));
    case SLEEP:
    case GROOM:
    case STRETCH:
    case HAPPY:
    case WALK_R:
    case WALK_L:
    case SIT:
    default:
      return flashStatus(PSTR(""));
  }
}

void drawLevelLook(int x, int y) {
  // Growing drip: accessories unlock with consistent care.
  if (dogLevel >= 2) {
    // Collar + tag
    display.drawLine(x + 8, y + 12, x + 22, y + 12, WHITE);
    display.fillRect(x + 14, y + 11, 3, 3, WHITE);
  }
  if (dogLevel >= 3) {
    // Bandana flap
    display.drawLine(x + 6, y + 11, x + 11, y + 11, WHITE);
    display.drawPixel(x + 7, y + 13, WHITE);
    display.drawPixel(x + 8, y + 14, WHITE);
    display.drawPixel(x + 9, y + 13, WHITE);
  }
  if (dogLevel >= 4) {
    // Party hat
    display.drawLine(x + 10, y + 1, x + 14, y - 5, WHITE);
    display.drawLine(x + 18, y + 1, x + 14, y - 5, WHITE);
    display.drawLine(x + 10, y + 1, x + 18, y + 1, WHITE);
    display.drawPixel(x + 14, y - 6, WHITE);
  }
  if (dogLevel >= 5) {
    // Crown + sparkles for max goof royalty
    display.fillRect(x + 9, y - 2, 12, 3, WHITE);
    display.drawPixel(x + 11, y - 4, WHITE);
    display.drawPixel(x + 14, y - 5, WHITE);
    display.drawPixel(x + 17, y - 4, WHITE);
    if (animFrame) {
      display.drawPixel(x + 1, y + 3, WHITE);
      display.drawPixel(x + 29, y + 5, WHITE);
      display.drawPixel(x + 4, y - 1, WHITE);
    }
  }
}

void drawDog(int x, int y, const unsigned char *bitmap) {
  // Lv1 pup sits a little lower / smaller vibe
  int dy = (dogLevel == 1) ? 2 : ((dogLevel >= 4) ? -1 : 0);
  display.drawBitmap(x, y + dy, bitmap, SPRITE_W, SPRITE_H, WHITE);
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
  unsigned long frameMs = (pose == SLEEP) ? (FRAME_MS * 3) : FRAME_MS;
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

  // 6. Position Physics — clumsy hops, wall bonks, silly scoots
  if (phaseAdvanced) {
    if (pose == WALK_R) {
      dogX += 1 + (animPhase == 2 ? 1 : 0); // occasional overstep
      if (dogX > 96) {
        dogX = 96;
        idlePose = WALK_L;
        showingBonk = true;
        bonkUntil = currentMillis + 700;
      }
      markDirty();
    } else if (pose == WALK_L) {
      dogX -= 1 + (animPhase == 1 ? 1 : 0);
      if (dogX < 0) {
        dogX = 0;
        idlePose = WALK_R;
        showingBonk = true;
        bonkUntil = currentMillis + 700;
      }
      markDirty();
    } else if (pose == HAPPY && animPhase == 0) {
      // Accidental scoot while wagging
      dogX = constrain(dogX + (goofThought & 1 ? 1 : -1), 0, 96);
    } else if (pose == SIT && animPhase == 0 && random(5) == 0) {
      // Randomly tip a pixel because brain empty
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

  // Draw Need Gauges only once a need gets low (critical ones blink for attention)
  if (fun <= LOW_LEVEL) drawGauge(9, "PLAY", fun, fun <= CRITICAL_LEVEL && animFrame);
  if (food <= LOW_LEVEL) drawGauge(17, "FOOD", food, food <= CRITICAL_LEVEL && animFrame);

  // Draw Mood Caption
  printCentered(27, statusText());

  // Hold progress bar while preparing a factory reset
  if (holdMs >= HOLD_HINT_MS) {
    int fillW = (int)((holdMs * 100UL) / LONG_PRESS_MS);
    if (fillW > 100) fillW = 100;
    display.drawRect(14, 36, 100, 6, WHITE);
    if (fillW > 0) display.fillRect(14, 36, fillW, 6, WHITE);
  }

  // Draw Floor Ground Line
  display.drawLine(0, 60, 128, 60, WHITE);

  // Switch Art Sprites — exaggerated derpy motion
  const int8_t softBob[4] = {0, -2, 1, -1};
  const int8_t hopBob[4]  = {0, -3, 1, -4};
  const int8_t bigBob[4]  = {1, -5, 0, -6};
  const int8_t tip[4]     = {0, 1, 0, -1}; // sideways brain-empty tip
  const int y = dogY;
  int x = dogX;

  if (showingBonk) {
    x += (animPhase & 1) ? 2 : -2;
  }

  switch (pose) {
    case SIT:
      drawDog(x + tip[animPhase], y + softBob[animPhase], (animPhase & 1) ? dog_sit2 : dog_sit1);
      if (animPhase == 2) drawPant(x, y + softBob[animPhase]);
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
      int breath = (animPhase <= 1) ? 0 : 2;
      drawDog(x + ((animPhase == 3) ? 1 : 0), y + breath, dog_sleep);
      display.setTextSize(1);
      display.setCursor(x + 34, y - 1 - animPhase);
      display.print(F("zZ"));
      break;
    }

    case GROOM:
      drawDog(x, y + softBob[animPhase], (animPhase & 1) ? dog_groom2 : dog_groom1);
      display.drawPixel(x + animPhase, y + 10, WHITE);
      display.drawPixel(x + 3 + animPhase, y + 8 + (animPhase & 1), WHITE);
      display.drawPixel(x + 1, y + 13, WHITE);
      break;

    case STRETCH:
      drawDog(x + ((animPhase & 1) ? 2 : -2), y + softBob[animPhase], (animPhase & 1) ? dog_stretch2 : dog_stretch1);
      break;

    case HAPPY:
      drawDog(x + tip[animPhase], y + bigBob[animPhase], (animPhase & 1) ? dog_happy2 : dog_happy1);
      drawPant(x, y + bigBob[animPhase]);
      drawHeart(x + 34, y - 3 - animPhase);
      drawHeart(x - 6, y - 6 + (animPhase & 1));
      if (animPhase == 0) {
        display.setTextSize(1);
        display.setCursor(x + 12, y - 10);
        display.print("!");
      }
      break;

    case PLAYING: {
      // Hand pats the dog's head; dog squishes happily on contact.
      const int8_t handDrop[4] = {0, 3, 4, 2};
      const int8_t dogSquish[4] = {0, 1, 2, 1};
      int by = y + dogSquish[animPhase];
      drawDog(x, by, (animPhase & 1) ? dog_happy2 : dog_happy1);
      drawPettingHand(x + 5, y - 8 + handDrop[animPhase]);

      if (animPhase == 1 || animPhase == 2) {
        drawPant(x, by);
        drawHeart(x + 34, y - 5);
      }
      break;
    }

    case EATING: {
      int nod = (animPhase & 1) ? 2 : 0;
      drawDog(x + tip[animPhase], y + nod, (animPhase & 1) ? dog_eat : dog_sit1);
      drawBowl(x - 12, y + 14, true);
      // Messy crumbs everywhere
      display.drawPixel(x - 9, y + 10 - nod, WHITE);
      display.drawPixel(x - 5, y + 7 + nod, WHITE);
      display.drawPixel(x + 2, y + 8, WHITE);
      display.drawPixel(x + 30, y + 12, WHITE);
      break;
    }

    case REJECT: {
      int shake = (animPhase == 0) ? -3 : ((animPhase == 2) ? 3 : ((animPhase & 1) ? -1 : 1));
      drawDog(x + shake, y + softBob[animPhase], (animPhase & 1) ? dog_reject2 : dog_reject1);
      display.setTextSize(1);
      display.setCursor(x + 30, y - 7);
      display.print(F("NO"));
      break;
    }

    case HUNGRY:
      drawDog(x + tip[animPhase], y + softBob[animPhase], dog_beg);
      drawBowl(x - 12, y + 14, false);
      drawPant(x, y + softBob[animPhase]);
      display.setTextSize(1);
      display.setCursor(x + 10, y - 9 - (animPhase & 1));
      display.print(F("!"));
      break;

    case BORED:
      drawDog(x + tip[animPhase], y + ((animPhase == 2) ? 1 : 0), dog_bored);
      display.setTextSize(1);
      display.setCursor(x + 34, y - (animPhase & 1));
      display.print(F("..."));
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
