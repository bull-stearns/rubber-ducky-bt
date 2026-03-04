#include <Arduino.h>
#include <ArduinoBLE.h>
#include "USBHIDKeyboard.h"
#include <ctype.h>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define DATA_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define STATE_UUID "f0000001-0451-4000-b000-000000000000"
#define TOTALSIZE_UUID "f0000001-0451-4000-b000-000000000001"

USBHIDKeyboard Keyboard;

BLEService dataService(SERVICE_UUID);
BLECharacteristic dataChar(DATA_UUID, BLEWrite, 256);
BLECharacteristic stateChar(STATE_UUID, BLERead | BLEWrite | BLENotify, 16);
BLECharacteristic totalSizeChar(TOTALSIZE_UUID, BLEWrite, 256);

String buffer = "";
int expectedSize = 0;

enum BlockState {
  NONE,
  STRING,
  STRINGLN,
  REM_BLOCK
};

BlockState blockState = NONE;

uint32_t DEFAULTDELAY = 20;
uint32_t DEFAULTCHARDELAY = 20;
uint32_t CHARJITTER = 0;

uint32_t secureRandom(uint32_t min, uint32_t max) {
  uint32_t seed = analogRead(A0) ^ micros() ^ (micros() >> 16);
  seed ^= seed << 13;
  seed ^= seed >> 17;
  seed ^= seed << 5;
  return (seed % (max - min + 1)) + min;
}

void runDuckyCommand(String line) {
  line.trim();

  if (blockState == REM_BLOCK) {
    if (line == "END_REM") {
      blockState = NONE;
    }
    return;
  }

  if (blockState == STRING) {
    if (line == "END_STRING") {
      blockState = NONE;
    } else {
      for (int i = 0; i < line.length(); i++) {
        Keyboard.write(line.charAt(i));
        delay(DEFAULTCHARDELAY + secureRandom(0, CHARJITTER));
      }
    }
    return;
  }

  if (blockState == STRINGLN) {
    if (line == "END_STRINGLN") {
      blockState = NONE;
    } else {
      for (int i = 0; i < line.length(); i++) {
        Keyboard.write(line.charAt(i));
        delay(DEFAULTCHARDELAY + secureRandom(0, CHARJITTER));
      }
      Keyboard.write(KEY_RETURN);
    }
    return;
  }

  if (line.length() == 0) return;

  if (line == "REM" || line.startsWith("REM ")) return;

  if (line.startsWith("//")) return;

  if (line == "REM_BLOCK") {
    blockState = REM_BLOCK;
    return;
  }

  if (line == "STRING") {
    blockState = STRING;
    return;
  }

  if (line == "STRINGLN") {
    blockState = STRINGLN;
    return;
  }

  if (line == "LED_OFF") {
    digitalWrite(LEDR, HIGH);
    digitalWrite(LEDG, HIGH);
    digitalWrite(LEDB, HIGH);
    return;
  }

  if (line == "LED_R") {
    digitalWrite(LEDR, LOW);
    digitalWrite(LEDG, HIGH);
    digitalWrite(LEDB, HIGH);
    return;
  }

  if (line == "LED_G") {
    digitalWrite(LEDR, HIGH);
    digitalWrite(LEDG, LOW);
    digitalWrite(LEDB, HIGH);
    return;
  }

  if (line.startsWith("STRING ")) {
    String text = line.substring(7);
    for (int i = 0; i < text.length(); i++) {
      Keyboard.write(text.charAt(i));
      delay(DEFAULTCHARDELAY + secureRandom(0, CHARJITTER));
    }
    return;
  }

  if (line.startsWith("STRINGLN ")) {
    String text = line.substring(9);
    for (int i = 0; i < text.length(); i++) {
      Keyboard.write(text.charAt(i));
      delay(DEFAULTCHARDELAY + secureRandom(0, CHARJITTER));
    }
    Keyboard.write(KEY_RETURN);
    return;
  }

  if (line.startsWith("DELAY ")) {
    uint32_t d = line.substring(6).toInt();
    if (d < 20) d = 20;
    delay(d);
    return;
  }

  if (line.startsWith("DEFAULTDELAY ")) {
    DEFAULTDELAY = line.substring(13).toInt();
    return;
  }


  if (line == "ENTER") {
    Keyboard.write(KEY_RETURN);
    return;
  }
  if (line == "TAB") {
    Keyboard.write(KEY_TAB);
    return;
  }
  if (line == "SPACE") {
    Keyboard.write(' ');
    return;
  }
  if (line == "BACKSPACE") {
    Keyboard.write(KEY_BACKSPACE);
    return;
  }
  if (line == "DELETE" || line == "DEL") {
    Keyboard.write(KEY_DELETE);
    return;
  }

  if (line == "UP" || line == "UPARROW") {
    Keyboard.write(KEY_UP_ARROW);
    return;
  }
  if (line == "DOWN" || line == "DOWNARROW") {
    Keyboard.write(KEY_DOWN_ARROW);
    return;
  }
  if (line == "LEFT" || line == "LEFTARROW") {
    Keyboard.write(KEY_LEFT_ARROW);
    return;
  }
  if (line == "RIGHT" || line == "RIGHTARROW") {
    Keyboard.write(KEY_RIGHT_ARROW);
    return;
  }

  if (line.length() == 2 && line.startsWith("F")) {
    int f = line.substring(1).toInt();
    if (f >= 1 && f <= 12) {
      Keyboard.write(KEY_F1 + (f - 1));
    }
    return;
  }

  uint8_t modifiers = 0;
  char finalKey = 0;

  int lastSpace = line.lastIndexOf(' ');
  String mods = (lastSpace != -1) ? line.substring(0, lastSpace) : "";
  String key = (lastSpace != -1) ? line.substring(lastSpace + 1) : line;

  mods.toUpperCase();
  key.toUpperCase();

  if (mods.indexOf("CTRL") >= 0 || mods.indexOf("CONTROL") >= 0)
    {modifiers = 1; Keyboard.press(KEY_LEFT_CTRL);}
  if (mods.indexOf("SHIFT") >= 0)
    {modifiers = 1; Keyboard.press(KEY_LEFT_SHIFT);}
  if (mods.indexOf("ALT") >= 0)
    {modifiers = 1; Keyboard.press(KEY_LEFT_ALT);}
  if (mods.indexOf("GUI") >= 0 || mods.indexOf("WINDOWS") >= 0)
    {modifiers = 1; Keyboard.press(KEY_LEFT_GUI);}

  if (key.length() == 1) {
    finalKey = key.charAt(0);
  } else if (key == "ENTER") {
    finalKey = KEY_RETURN;
  } else if (key == "DELETE" || key == "DEL") {
    finalKey = KEY_DELETE;
  }

  if (finalKey != 0 && modifiers != 0) {
    Keyboard.press(tolower(finalKey));
    delay(5);
    Keyboard.releaseAll();
    return;
  }
}


void setup() {
  Serial.begin(115200);
  pinMode(D3, INPUT_PULLUP);

  delay(500);

  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);

  digitalWrite(LEDR, HIGH);
  digitalWrite(LEDG, HIGH);
  digitalWrite(LEDB, HIGH);

  if (!BLE.begin()) {
    Serial.println("BLE init failed!");
    while (1);
  }

  BLE.setLocalName("Arduiboard");
  BLE.setDeviceName("Arduiboard");

  dataService.addCharacteristic(dataChar);
  dataService.addCharacteristic(stateChar);
  dataService.addCharacteristic(totalSizeChar);
  BLE.addService(dataService);

  BLE.advertise();
  Serial.println("Waiting for BLE connection...");
  stateChar.writeValue("READY");
}

void loop() {
  BLEDevice central = BLE.central();

  if (central) {
    Serial.println("Connected to central.");

    buffer = "";
    expectedSize = 0;

    while (central.connected()) {
      if (totalSizeChar.written()) {
        int len = totalSizeChar.valueLength();
        char sizeStr[len + 1];
        memcpy(sizeStr, totalSizeChar.value(), len);
        sizeStr[len] = '\0';

        String sizeString(sizeStr);
        expectedSize = sizeString.toInt();
        Serial.print("Expecting ");
        Serial.print(expectedSize);
        Serial.println(" bytes...");

        buffer = "";
        stateChar.writeValue("READY");
      }

      if (dataChar.written()) {
        int len = dataChar.valueLength();
        uint8_t temp[len + 1];
        dataChar.readValue(temp, len);
        temp[len] = '\0';

        buffer += String((char*)temp);

        Serial.print("Received chunk buffer size: ");
        Serial.println(buffer.length());

        if (buffer.length() >= expectedSize) {
          Serial.println("Full data received.");
          Keyboard.begin();
          int lineStart = 0;
          while (true) {
            int newlineIndex = buffer.indexOf('\n', lineStart);
            if (newlineIndex == -1) break;
            String line = buffer.substring(lineStart, newlineIndex);
            runDuckyCommand(line);
            delay(DEFAULTDELAY);
            lineStart = newlineIndex + 1;
          }

          if (lineStart < buffer.length()) {
            runDuckyCommand(buffer.substring(lineStart));
          }
          Keyboard.flush();
          Keyboard.end();

          buffer = "";
          expectedSize = 0;
        }
      }
      stateChar.writeValue("READY");
    }

    Serial.println("Disconnected from central.");
    stateChar.writeValue("READY");
  }
}

