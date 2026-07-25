// UltimateAnimeCatPet.ino
//
// Virtual cat pet with two needs: PLAY and FOOD.
// Wiring: one push button between D2 and GND (uses the internal pull-up).
//   single press  -> play with the cat
//   double press  -> feed the cat
#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const uint8_t BUTTON_PIN = 2;

// ==========================================
//   HAND-CRAFTED ANIME PIXEL ART (16x16)
// ==========================================

// 1. Idle / Sitting (Giant eyes, tiny body, wagging tail)
const unsigned char PROGMEM anime_sit1[] = {
  0x0c,0x30,0x1e,0x78,0x3f,0xfc,0x7f,0xfe,0x76,0xee,0x79,0x9e,0x7f,0xfe,0x3f,0xfc,
  0x1f,0xf8,0x0f,0xf0,0x0f,0xf0,0x1f,0xf8,0x1b,0xd8,0x11,0x88,0x1b,0xd8,0x00,0x00
};
const unsigned char PROGMEM anime_sit2[] = {
  0x0c,0x30,0x1e,0x78,0x3f,0xfc,0x7f,0xfe,0x76,0xee,0x79,0x9e,0x7f,0xfe,0x3f,0xfc,
  0x1f,0xf8,0x0f,0xf0,0x0f,0xf0,0x1f,0xf8,0x1b,0xd8,0x11,0x88,0x33,0xcc,0x20,0x04
};

// 2. Walking Right (Cute stubby legs moving)
const unsigned char PROGMEM anime_walk_r1[] = {
  0x03,0x00,0x07,0x80,0x0f,0xc0,0x1f,0xf0,0x3d,0xec,0x3e,0xbc,0x1f,0xfc,0x0f,0xfe,
  0x07,0xff,0x03,0xff,0x03,0xff,0x01,0xff,0x01,0x6d,0x01,0x24,0x00,0x66,0x00,0x00
};
const unsigned char PROGMEM anime_walk_r2[] = {
  0x03,0x00,0x07,0x80,0x0f,0xc0,0x1f,0xf0,0x3d,0xec,0x3e,0xbc,0x1f,0xfc,0x0f,0xfe,
  0x07,0xff,0x03,0xff,0x03,0xff,0x01,0xff,0x02,0xdb,0x02,0x49,0x00,0x33,0x00,0x00
};

// 3. Walking Left
const unsigned char PROGMEM anime_walk_l1[] = {
  0x00,0xc0,0x01,0xe0,0x03,0xf0,0x0f,0xf8,0x37,0xbc,0x3d,0x7c,0x3f,0xf8,0x7f,0xf0,
  0xff,0xe0,0xff,0xc0,0xff,0xc0,0xff,0x80,0xb6,0x80,0x24,0x80,0x66,0x00,0x00,0x00
};
const unsigned char PROGMEM anime_walk_l2[] = {
  0x00,0xc0,0x01,0xe0,0x03,0xf0,0x0f,0xf8,0x37,0xbc,0x3d,0x7c,0x3f,0xf8,0x7f,0xf0,
  0xff,0xe0,0xff,0xc0,0xff,0xc0,0xff,0x80,0xdb,0x40,0x49,0x40,0x33,0x00,0x00,0x00
};

// 4. Sleeping / Resting (Squinty happy eyes, relaxed body)
const unsigned char PROGMEM anime_sleep[] = {
  0x00,0x00,0x00,0x00,0x06,0x60,0x0f,0xf0,0x1f,0xf8,0x3f,0xfc,0x39,0x9c,0x3f,0xfc,
  0x1f,0xf8,0x1f,0xf8,0x3f,0xfc,0x7f,0xfe,0x7f,0xfe,0x33,0xcc,0x00,0x00,0x00,0x00
};

// 5. Playing (Batting a tiny yarn ball)
const unsigned char PROGMEM anime_play[] = {
  0x0c,0x30,0x1e,0x78,0x3f,0xfc,0x7f,0xfe,0x76,0xee,0x79,0x9e,0x7f,0xfe,0x3f,0xfc,
  0x1f,0xf8,0x1f,0xf0,0x0f,0xf8,0x1b,0xdc,0x31,0xce,0x23,0xc6,0x01,0xc0,0x00,0x00
};

// 6. Shocked / Dizzy (Classic anime spiral/X eyes)
const unsigned char PROGMEM anime_shock[] = {
  0x0c,0x30,0x1e,0x78,0x3f,0xfc,0x7f,0xfe,0x69,0x96,0x52,0x4a,0x7f,0xfe,0x3f,0xfc,
  0x1f,0xf8,0x0f,0xf0,0x0f,0xf0,0x1f,0xf8,0x1b,0xd8,0x11,0x88,0x1b,0xd8,0x00,0x00
};

// 7. Bored / Sulking (Downcast droopy eyes)
const unsigned char PROGMEM anime_bored[] = {
  0x0c,0x30,0x1e,0x78,0x3f,0xfc,0x7f,0xfe,0x7f,0xfe,0x76,0xee,0x79,0x9e,0x3f,0xfc,
  0x1f,0xf8,0x0f,0xf0,0x0f,0xf0,0x1f,0xf8,0x1b,0xd8,0x11,0x88,0x1b,0xd8,0x00,0x00
};

// 8. Begging / Hungry (Wide pleading eyes, open mouth)
const unsigned char PROGMEM anime_beg[] = {
  0x0c,0x30,0x1e,0x78,0x3f,0xfc,0x7f,0xfe,0x76,0xee,0x79,0x9e,0x7e,0x7e,0x3f,0xfc,
  0x1f,0xf8,0x0f,0xf0,0x0f,0xf0,0x1f,0xf8,0x1b,0xd8,0x11,0x88,0x1b,0xd8,0x00,0x00
};

// 9. Munching (Head down over the bowl)
const unsigned char PROGMEM anime_eat[] = {
  0x00,0x00,0x0c,0x30,0x1e,0x78,0x3f,0xfc,0x7f,0xfe,0x76,0xee,0x7e,0x7e,0x3f,0xfc,
  0x1f,0xf8,0x0f,0xf0,0x1f,0xf8,0x1b,0xd8,0x11,0x88,0x1b,0xd8,0x00,0x00,0x00,0x00
};

// 10. Grooming (paw up by the face, lick loop)
const unsigned char PROGMEM anime_groom1[] = {
  0x0c,0x30,0x1e,0x78,0x3f,0xfc,0x7f,0xfe,0x76,0xee,0x79,0x9e,0x7f,0xfe,0x3f,0xfc,
  0x1f,0xf8,0x0f,0xf0,0x1f,0xf8,0x3b,0xd8,0x31,0x88,0x1b,0xd8,0x00,0x00,0x00,0x00
};
const unsigned char PROGMEM anime_groom2[] = {
  0x0c,0x30,0x1e,0x78,0x3f,0xfc,0x7f,0xfe,0x76,0xee,0x79,0x9e,0x7f,0xfe,0x3f,0xfc,
  0x1f,0xf8,0x1f,0xf0,0x3f,0xf8,0x1b,0xd8,0x11,0x88,0x1b,0xd8,0x00,0x00,0x00,0x00
};

// 11. Stretching (long loaf, paws out)
const unsigned char PROGMEM anime_stretch1[] = {
  0x00,0x00,0x00,0x00,0x0c,0x30,0x1e,0x78,0x3f,0xfc,0x7f,0xfe,0x76,0xee,0x79,0x9e,
  0x7f,0xfe,0x3f,0xfc,0x1f,0xf8,0x3f,0xfc,0x7b,0xde,0x61,0x86,0x00,0x00,0x00,0x00
};
const unsigned char PROGMEM anime_stretch2[] = {
  0x00,0x00,0x00,0x00,0x0c,0x30,0x1e,0x78,0x3f,0xfc,0x7f,0xfe,0x76,0xee,0x79,0x9e,
  0x7f,0xfe,0x3f,0xfc,0x1f,0xf8,0x1f,0xf8,0x3b,0xdc,0x31,0x8c,0x20,0x04,0x00,0x00
};

// 12. Happy bounce (sparkly eyes)
const unsigned char PROGMEM anime_happy1[] = {
  0x0c,0x30,0x1e,0x78,0x3f,0xfc,0x7f,0xfe,0x6a,0xae,0x75,0x5e,0x7f,0xfe,0x3f,0xfc,
  0x1f,0xf8,0x0f,0xf0,0x0f,0xf0,0x1f,0xf8,0x1b,0xd8,0x11,0x88,0x1b,0xd8,0x00,0x00
};
const unsigned char PROGMEM anime_happy2[] = {
  0x0c,0x30,0x1e,0x78,0x3f,0xfc,0x7f,0xfe,0x6a,0xae,0x75,0x5e,0x7f,0xfe,0x3f,0xfc,
  0x1f,0xf8,0x0f,0xf0,0x0f,0xf0,0x1f,0xf8,0x1b,0xd8,0x11,0x88,0x33,0xcc,0x20,0x04
};

// 13. Reject / "no thanks" (turned head, closed mouth pout)
const unsigned char PROGMEM anime_reject1[] = {
  0x0c,0x30,0x1e,0x78,0x3f,0xfc,0x7f,0xfe,0x7b,0xde,0x7d,0xbe,0x7f,0xfe,0x3f,0xfc,
  0x1f,0xf8,0x0f,0xf0,0x0f,0xf0,0x1f,0xf8,0x1b,0xd8,0x11,0x88,0x1b,0xd8,0x00,0x00
};
const unsigned char PROGMEM anime_reject2[] = {
  0x18,0x18,0x3c,0x3c,0x7f,0xfe,0xff,0xff,0xf7,0xef,0xfb,0xdf,0xff,0xff,0x7f,0xfe,
  0x3f,0xfc,0x1f,0xf8,0x1f,0xf8,0x3f,0xfc,0x37,0xec,0x22,0x44,0x36,0x6c,0x00,0x00
};

enum Pose {
  WALK_R, WALK_L, SIT, SLEEP, GROOM, STRETCH, HAPPY,
  PLAYING, EATING, BORED, HUNGRY, MISERABLE, REJECT
};
Pose pose = SIT;
Pose idlePose = SIT;

const Pose IDLE_CHOICES[] = { WALK_R, WALK_L, SIT, SLEEP, GROOM, STRETCH, HAPPY };
const uint8_t IDLE_CHOICE_COUNT = sizeof(IDLE_CHOICES) / sizeof(IDLE_CHOICES[0]);

int catX = 56;
const int catY = 44; // Locks them perfectly to the bottom grassline
unsigned long lastIdleChange = 0;
unsigned long lastFrameUpdate = 0;
unsigned long lastFunDrain = 0;
unsigned long lastFoodDrain = 0;
unsigned long actionUntil = 0;
Pose actionPose = PLAYING;
bool animFrame = false;

// --- NEEDS ENGINE ---
int fun = 70;   // playfulness, 0..100
int food = 70;  // fullness, 0..100
const int LOW_LEVEL = 25;
const int CRITICAL_LEVEL = 12;
const int FULL_LEVEL = 90;                // too satisfied / too full to accept more
const unsigned long FUN_DRAIN_MS = 10000;  // one point of boredom every 10 seconds
const unsigned long FOOD_DRAIN_MS = 15000; // one point of hunger every 15 seconds
const unsigned long ACTION_MS = 1800;      // how long a play/feed reaction lasts
const unsigned long IDLE_CHANGE_MS = 3200; // swap cute idle loops often

// --- BUTTON ENGINE (single press = play, double press = feed) ---
const unsigned long DEBOUNCE_MS = 30;
const unsigned long DOUBLE_GAP_MS = 600; // time allowed between clicks for a double
bool rawButton = HIGH;
bool stableButton = HIGH;
unsigned long lastButtonEdge = 0;
uint8_t clickCount = 0;
unsigned long lastClickAt = 0;

// --- EEPROM (survives power-off) ---
const uint8_t EEPROM_MAGIC = 0xC7;
const int EEPROM_ADDR_MAGIC = 0;
const int EEPROM_ADDR_FUN = 1;
const int EEPROM_ADDR_FOOD = 2;
const int EEPROM_ADDR_CATX = 3;
const unsigned long SAVE_MS = 2000; // write at most every 2s when dirty
bool stateDirty = false;
unsigned long lastSave = 0;

int clampLevel(int value) {
  if (value < 0) return 0;
  if (value > 100) return 100;
  return value;
}

void saveState() {
  EEPROM.update(EEPROM_ADDR_MAGIC, EEPROM_MAGIC);
  EEPROM.update(EEPROM_ADDR_FUN, (uint8_t)clampLevel(fun));
  EEPROM.update(EEPROM_ADDR_FOOD, (uint8_t)clampLevel(food));
  EEPROM.update(EEPROM_ADDR_CATX, (uint8_t)constrain(catX, 0, 112));
  stateDirty = false;
  lastSave = millis();
}

void loadState() {
  if (EEPROM.read(EEPROM_ADDR_MAGIC) != EEPROM_MAGIC) {
    saveState(); // first boot: store defaults
    return;
  }
  fun = clampLevel(EEPROM.read(EEPROM_ADDR_FUN));
  food = clampLevel(EEPROM.read(EEPROM_ADDR_FOOD));
  catX = constrain(EEPROM.read(EEPROM_ADDR_CATX), 0, 112);
}

void markDirty() {
  stateDirty = true;
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  randomSeed(analogRead(A0));
  loadState();
}

void startAction(Pose p, unsigned long now) {
  actionPose = p;
  actionUntil = now + ACTION_MS;
}

void playWithCat(unsigned long now) {
  if (food <= CRITICAL_LEVEL) {
    // Too weak to chase the yarn ball, it just flops over
    fun = clampLevel(fun + 3);
    markDirty();
    saveState();
    startAction(MISERABLE, now);
    return;
  }
  if (fun >= FULL_LEVEL) {
    // Already zoomies'd out — turns away from the yarn
    startAction(REJECT, now);
    return;
  }
  fun = clampLevel(fun + 20);
  food = clampLevel(food - 4); // running around burns a snack
  markDirty();
  saveState();
  startAction(PLAYING, now);
}

void feedCat(unsigned long now) {
  if (food >= FULL_LEVEL) {
    // Too stuffed — pushes the bowl away
    startAction(REJECT, now);
    return;
  }
  food = clampLevel(food + 24);
  fun = clampLevel(fun + 4); // dinner is its own kind of fun
  markDirty();
  saveState();
  startAction(EATING, now);
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
      clickCount++;
      lastClickAt = now;
    }
  }

  // Two presses within the gap -> feed. One press, then wait out the gap -> play.
  if (clickCount >= 2) {
    feedCat(now);
    clickCount = 0;
  } else if (clickCount == 1 && stableButton == HIGH && now - lastClickAt > DOUBLE_GAP_MS) {
    playWithCat(now);
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

const char *statusText() {
  switch (pose) {
    case PLAYING:   return "WHEEE! ^o^";
    case EATING:    return "NOM NOM NOM";
    case REJECT:    return food >= FULL_LEVEL ? "too full!! no!!" : "already happy!!";
    case MISERABLE: return "too weak... feed me!";
    case HUNGRY:    return "STARVING! press x2";
    case BORED:     return "sooo bored... press 1x";
    case SLEEP:     return "napping...";
    case GROOM:     return "groom groom~";
    case STRETCH:   return "nyaaa~ stretch";
    case HAPPY:     return "purr purr~";
    default:
      if (fun > 75 && food > 75) return "purrfectly happy!";
      return "";
  }
}

void loop() {
  unsigned long currentMillis = millis();

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

  // 3. Frame Swapper (320ms gives the perfect slow kawaii bop)
  if (currentMillis - lastFrameUpdate > 320) {
    animFrame = !animFrame;
    lastFrameUpdate = currentMillis;
  }

  // 4. Idle wandering, only used when no need is screaming
  if (currentMillis - lastIdleChange > 4500) {
    idlePose = (Pose)random(4); // WALK_R, WALK_L, SIT, SLEEP
    lastIdleChange = currentMillis;
  }

  // 5. Mood resolver: a fresh action beats a need, a need beats idling
  if (currentMillis < actionUntil) {
    pose = actionPose;
  } else if (fun <= CRITICAL_LEVEL && food <= CRITICAL_LEVEL) {
    pose = MISERABLE;
  } else if (food <= LOW_LEVEL) {
    pose = HUNGRY;
  } else if (fun <= LOW_LEVEL) {
    pose = BORED;
  } else {
    pose = idlePose;
  }

  // 6. Position Physics
  if (pose == WALK_R) {
    catX++;
    if (catX > 112) { catX = 112; idlePose = WALK_L; }
    markDirty();
  }
  if (pose == WALK_L) {
    catX--;
    if (catX < 0) { catX = 0; idlePose = WALK_R; }
    markDirty();
  }

  // ==========================================
  //   RENDER SCREEN
  // ==========================================
  display.clearDisplay();
  display.setTextColor(WHITE);

  // Draw Need Gauges only once a need gets low (critical ones blink for attention)
  if (fun <= LOW_LEVEL) drawGauge(1, "PLAY", fun, fun <= CRITICAL_LEVEL && animFrame);
  if (food <= LOW_LEVEL) drawGauge(12, "FOOD", food, food <= CRITICAL_LEVEL && animFrame);

  // Draw Mood Caption
  printCentered(24, statusText());

  // Draw Floor Ground Line
  display.drawLine(0, 60, 128, 60, WHITE);

  // Switch Art Sprites based on active status
  switch (pose) {
    case SIT:
      display.drawBitmap(catX, catY, animFrame ? anime_sit1 : anime_sit2, 16, 16, WHITE);
      break;

    case WALK_R:
      display.drawBitmap(catX, catY, animFrame ? anime_walk_r1 : anime_walk_r2, 16, 16, WHITE);
      break;

    case WALK_L:
      display.drawBitmap(catX, catY, animFrame ? anime_walk_l1 : anime_walk_l2, 16, 16, WHITE);
      break;

    case SLEEP:
      display.drawBitmap(catX, catY, anime_sleep, 16, 16, WHITE);
      // Sweaty/Zz marks floating up
      display.setTextSize(1);
      display.setCursor(catX + 18, catY - (animFrame ? 4 : 2));
      display.print("zZ");
      break;

    case PLAYING:
      display.drawBitmap(catX, catY, anime_play, 16, 16, WHITE);
      // Small pixel ball rolling in front of cat
      display.fillCircle(catX - 4, catY + 12 + (animFrame ? -1 : 0), 2, WHITE);
      // Happy hearts popping off
      drawHeart(catX + 17, catY - (animFrame ? 6 : 3));
      drawHeart(catX - 8, catY - (animFrame ? 2 : 5));
      break;

    case EATING:
      display.drawBitmap(catX, catY, anime_eat, 16, 16, WHITE);
      drawBowl(catX + 17, catY + 10, true);
      // Kibble bouncing out of the bowl
      display.drawPixel(catX + 20, catY + 6 - (animFrame ? 2 : 0), WHITE);
      display.drawPixel(catX + 24, catY + 4 + (animFrame ? 2 : 0), WHITE);
      break;

    case HUNGRY:
      display.drawBitmap(catX, catY, anime_beg, 16, 16, WHITE);
      drawBowl(catX + 17, catY + 10, false);
      // Begging exclamation
      display.setTextSize(1);
      display.setCursor(catX + 6, catY - (animFrame ? 10 : 8));
      display.print("!");
      break;

    case BORED:
      display.drawBitmap(catX, catY, anime_bored, 16, 16, WHITE);
      // Slow sulking dots drifting off
      display.setTextSize(1);
      display.setCursor(catX + 17, catY - (animFrame ? 2 : 0));
      display.print("...");
      break;

    case MISERABLE:
      display.drawBitmap(catX, catY, anime_shock, 16, 16, WHITE);
      // Anime exclamation marks!
      display.setTextSize(1);
      display.setCursor(catX + 6, catY - 8);
      display.print("!!");
      break;
  }

  display.display();
  delay(20);
}
