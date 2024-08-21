#include <Keyboard.h>

#define DEBOUNCE_DELAY 50

struct Button {
  char key;
  bool currentState;
  bool lastState;
};

const int rowCount = 4;
const int columnCount = 4;

const int rowPins[rowCount] = { D7, D8, D9, D10 };
const int colPins[columnCount] = { D6, D5, D4, D3 };

Button buttons[rowCount][columnCount];

void setup() {
  Serial.begin(9600);
  Keyboard.begin();

  // Initialize LED.
  {
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(LED_BUILTIN, LOW);

#ifdef ARDUINO_ARCH_RP2040
    pinMode(PIN_LED_R, OUTPUT);
    pinMode(PIN_LED_R, LOW);

    pinMode(PIN_LED_G, OUTPUT);
    pinMode(PIN_LED_G, LOW);

    pinMode(PIN_LED_B, OUTPUT);
    pinMode(PIN_LED_B, LOW);
#endif
  }

  // Initialize buttons.
  {
    buttons[0][0] = { KEY_KP_1, HIGH, HIGH };
    buttons[0][1] = { KEY_KP_2, HIGH, HIGH };
    buttons[0][2] = { KEY_KP_3, HIGH, HIGH };
    buttons[0][3] = { 'z', HIGH, HIGH };

    buttons[1][0] = { KEY_KP_4, HIGH, HIGH };
    buttons[1][1] = { KEY_KP_5, HIGH, HIGH };
    buttons[1][2] = { KEY_KP_6, HIGH, HIGH };
    buttons[1][3] = { 'x', HIGH, HIGH };

    buttons[2][0] = { KEY_KP_7, HIGH, HIGH };
    buttons[2][1] = { KEY_KP_8, HIGH, HIGH };
    buttons[2][2] = { KEY_KP_9, HIGH, HIGH };
    buttons[2][3] = { 'c', HIGH, HIGH };

    buttons[3][0] = { KEY_KP_0, HIGH, HIGH };
    buttons[3][1] = { KEY_TAB, HIGH, HIGH };
    buttons[3][2] = { KEY_BACKSPACE, HIGH, HIGH };
    buttons[3][3] = { 'v', HIGH, HIGH };
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
}

void loop() {

  bool anyPressed = false;

  // Check each row.
  for (int i = 0; i < rowCount; i++) {
    digitalWrite(rowPins[i], LOW);

    // Check each column.
    for (int j = 0; j < columnCount; j++) {

      // Fetch and check button state.
      bool state = digitalRead(colPins[j]);
      Button &button = buttons[i][j];

      if (state != button.lastState) {
        unsigned long debounceTime = millis();
        while (millis() - debounceTime < DEBOUNCE_DELAY) {
          state = digitalRead(colPins[j]);
        }
        if (state != button.currentState) {
          button.currentState = state;
          if (button.currentState == LOW) {
            // Key pressed
            Keyboard.press(button.key);
            Serial.print("Key pressed: ");
            Serial.println(button.key);

          } else {
            // Key released
            Keyboard.release(button.key);
            Serial.print("Key released: ");
            Serial.println(button.key);
          }
        }
      }

      if (button.currentState == LOW) anyPressed = true;

      // Cache state.
      button.lastState = state;
    }

    digitalWrite(rowPins[i], HIGH);
  }

  // Light LED.
  if (anyPressed == true) {
    pinMode(LED_BUILTIN, HIGH);
  } else {
    pinMode(LED_BUILTIN, LOW);
  }
}