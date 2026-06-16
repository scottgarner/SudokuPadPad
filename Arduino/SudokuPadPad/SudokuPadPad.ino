#include <Keyboard.h>
#include <EEPROM.h>

#define DEBOUNCE_DELAY 50
#define EEPROM_ADDR 0
#define LAYOUT_COUNT 2

struct Color {
  PinStatus r, g, b;
};

struct Button {
  char key;
  bool currentState = HIGH;
  bool lastState = HIGH;
  unsigned long lastChange = 0;
};

enum Layout : uint8_t {
  LAYOUT_ASCENDING = 0,   // 1-2-3 on top
  LAYOUT_DESCENDING = 1,  // 7-8-9 on top
};

const int rowCount = 4;
const int columnCount = 4;
const int rowPins[rowCount] = { D7, D8, D9, D10 };
const int colPins[columnCount] = { D6, D5, D4, D3 };
Button buttons[rowCount][columnCount];

uint8_t layout = LAYOUT_ASCENDING;

const char layoutKeys[LAYOUT_COUNT][16] = {
  // LAYOUT_ASCENDING: 1-2-3 / 4-5-6 / 7-8-9
  { KEY_KP_1, KEY_KP_2, KEY_KP_3, 'z',
    KEY_KP_4, KEY_KP_5, KEY_KP_6, 'x',
    KEY_KP_7, KEY_KP_8, KEY_KP_9, 'c',
    KEY_KP_0, KEY_TAB, KEY_BACKSPACE, 'v' },
  // LAYOUT_DESCENDING: 7-8-9 / 4-5-6 / 1-2-3
  { KEY_KP_7, KEY_KP_8, KEY_KP_9, 'z',
    KEY_KP_4, KEY_KP_5, KEY_KP_6, 'x',
    KEY_KP_1, KEY_KP_2, KEY_KP_3, 'c',
    KEY_KP_0, KEY_TAB, KEY_BACKSPACE, 'v' },
};

const Color layoutColors[] = {
  { HIGH, LOW, HIGH },
  { HIGH, HIGH, LOW },
};

void setLED(Color c) {
  digitalWrite(PIN_LED_R, c.r);
  digitalWrite(PIN_LED_G, c.g);
  digitalWrite(PIN_LED_B, c.b);
}

const Color LED_OFF = { HIGH, HIGH, HIGH };

uint16_t pressedMask = 0;

void setup() {

  // Initialize LED.
  {
    pinMode(PIN_LED_R, OUTPUT);
    pinMode(PIN_LED_G, OUTPUT);
    pinMode(PIN_LED_B, OUTPUT);
  }

  // Initialize pins.
  {
    for (int i = 0; i < rowCount; i++) {
      pinMode(rowPins[i], OUTPUT);
      digitalWrite(rowPins[i], HIGH);
    }
    for (int j = 0; j < columnCount; j++) {
      pinMode(colPins[j], INPUT_PULLUP);
    }
  }

  // Wait for serial.
  Serial.begin(9600);
  while (!Serial && millis() < 2000) {
  }

  // Check for boot-mode override (hold a key while plugging in).
  {
    EEPROM.begin(256);

    // Read single key by position.
    auto held = [](int row, int col) -> bool {
      digitalWrite(rowPins[row], LOW);
      delayMicroseconds(10);
      bool pressed = (digitalRead(colPins[col]) == LOW);
      digitalWrite(rowPins[row], HIGH);
      return pressed;
    };

    if (held(3, 3)) {
      Serial.println("Entering bootloader mode...");
      rp2040.rebootToBootloader();
    } else if (held(0, 0)) {
      Serial.println("Setting ascending layout...");
      EEPROM.write(EEPROM_ADDR, LAYOUT_ASCENDING);
      EEPROM.commit();
    } else if (held(0, 1)) {
      Serial.println("Setting descending layout...");
      EEPROM.write(EEPROM_ADDR, LAYOUT_DESCENDING);
      EEPROM.commit();
    }

    // Load layout or default to ascending.
    uint8_t stored = EEPROM.read(EEPROM_ADDR);
    layout = (stored < LAYOUT_COUNT)
               ? stored
               : LAYOUT_ASCENDING;

    Serial.print("Loaded layout: ");
    Serial.println(layout);
  }

  // Initialize buttons.
  {
    buttons[0][0] = { layoutKeys[layout][0] };
    buttons[0][1] = { layoutKeys[layout][1] };
    buttons[0][2] = { layoutKeys[layout][2] };
    buttons[0][3] = { layoutKeys[layout][3] };
    buttons[1][0] = { layoutKeys[layout][4] };
    buttons[1][1] = { layoutKeys[layout][5] };
    buttons[1][2] = { layoutKeys[layout][6] };
    buttons[1][3] = { layoutKeys[layout][7] };
    buttons[2][0] = { layoutKeys[layout][8] };
    buttons[2][1] = { layoutKeys[layout][9] };
    buttons[2][2] = { layoutKeys[layout][10] };
    buttons[2][3] = { layoutKeys[layout][11] };
    buttons[3][0] = { layoutKeys[layout][12] };
    buttons[3][1] = { layoutKeys[layout][13] };
    buttons[3][2] = { layoutKeys[layout][14] };
    buttons[3][3] = { layoutKeys[layout][15] };
  }

  // Keyboard initialization.
  Keyboard.begin();
}

void loop() {

  for (int i = 0; i < rowCount; i++) {
    digitalWrite(rowPins[i], LOW);

    for (int j = 0; j < columnCount; j++) {
      bool state = digitalRead(colPins[j]);
      Button &button = buttons[i][j];

      // Raw reading changed, restart debounce timer.
      if (state != button.lastState) {
        button.lastChange = millis();
      }

      // Debounce passed, and state changed.
      if (millis() - button.lastChange >= DEBOUNCE_DELAY) {
        if (state != button.currentState) {
          button.currentState = state;

          byte buttonIndex = (i * 4) + j;

          if (button.currentState == LOW) {
            Keyboard.press(button.key);
            Serial.print("Key pressed: ");
            Serial.print(i);
            Serial.print(", ");
            Serial.println(j);

            pressedMask |= (uint16_t(1) << buttonIndex);
          } else {
            Keyboard.release(button.key);
            Serial.print("Key released: ");
            Serial.print(i);
            Serial.print(", ");
            Serial.println(j);

            pressedMask &= ~(uint16_t(1) << buttonIndex);
          }
        }
      }

      button.lastState = state;
    }

    digitalWrite(rowPins[i], HIGH);
  }

  if (pressedMask)
    setLED(layoutColors[layout]);
  } else {
    setLED(LED_OFF);
  }
}