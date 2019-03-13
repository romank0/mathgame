/*
BSD 2-Clause License

Copyright (c) 2019, Roman Konoval
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <M5Stack.h>

#ifndef min
  #define min(a,b) (((a) < (b)) ? (a) : (b))
#endif


#define SAVE_FILE "/mathgame.sav"
#define INCR 1
#define DECR -1
#define BACKGROUND_COLOR BLACK

enum Operation {
    ADD = 0,
    SUB = 1,
    MUL = 2,
    DIV = 3
};

int level[] = {1, 1, 1, 1};

int outcome[][5] = {
    {-1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1}
};

int x;
int y;
int result;
int r[3];
Operation op;
int mistakes;
uint8_t cursor = 0;

int getR() {
  return 100 * r[2] + 10 * r[1] + r[0];
}

String getRString() {
  return String(r[2]) + String(r[1]) + String(r[0]);
}

void critical(const String& message) {
  M5.Lcd.clear(BACKGROUND_COLOR);
  M5.Lcd.setTextSize(2);
  M5.Lcd.print(message);
  while (true);
}

String getOpString() {
  switch (op) {
      case ADD:
        return F("+");
      case SUB:
        return F("-");
      case MUL:
        return F("*");
      case DIV:
        return F("/");
      default:
        critical(String("unknown operation ") + String((int)op));
  }
  
}

void showXY() {
  M5.Lcd.setTextSize(5);
  M5.Lcd.setTextColor(RED);
  M5.Lcd.drawCentreString(String(x) + String(F(" ")) + getOpString() + String(F(" ")) + String(y), M5.Lcd.width()/2, (30)-5, 1);
}

void initGame() {
  op = static_cast<Operation>(random(4)); 

  // difficulty level for the operation
  int cur_level = level[op];
  switch (op) {
      case ADD:
        x = random(1, 30 + cur_level * 15);
        y = random(1, 30 + cur_level * 15);
        result = x + y;
        break;

      case SUB:
        x = random(10, 20 + cur_level * 10);
        y = random(4, min(10 + cur_level * 10, x) - 1);
        result = x - y;
        break;

      case MUL:
        x = random(1, 4 + cur_level);
        y = random(2, 4 + cur_level);
        result = x * y;
        break;

      case DIV:
        result = random(1, cur_level + 3);
        y = random(2, cur_level + 3);
        x = result * y;
        break;
      default:
        critical(String("unknown operation ") + String((int)op));
  }

  mistakes = 0;

  r[0] = 0;
  r[1] = 0;
  r[2] = 0;
  
  draw();
}

void readGameSave(const char * path) {
    Serial.printf("Reading file: %s\n", path);

    SD.open("/");
    File file = SD.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    if (file.read(reinterpret_cast<uint8_t*>(level), sizeof(int) * 4)) {
        Serial.println("Read levels ok");
    }
    else {
        Serial.println("Read levels failed");
        return;
    }
    if (file.read(reinterpret_cast<uint8_t*>(outcome), sizeof(int) * 4 * 5)) {
        Serial.println("Read outcome ok");
    }
    else {
        Serial.println("Read outcome failed");
        return;
    }
}

void writeGameSave(const char * path){
    Serial.printf("Writing file: %s\n", path);

    File file = SD.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.write(reinterpret_cast<uint8_t*>(level), sizeof(int) * 4)){
        Serial.println("levels written");
    } else {
        Serial.println("Write levels failed");
    }
    if(file.write(reinterpret_cast<uint8_t*>(outcome), sizeof(int) * 4 * 5)){
        Serial.println("outcoms written");
    } else {
        Serial.println("Write outcomes failed");
    }
}

void showButtons() {
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.setCursor(50, 216);
  M5.Lcd.print("<");
  M5.Lcd.setCursor(140, 216);
  M5.Lcd.print("+/-");
  M5.Lcd.setCursor(240, 216);
  M5.Lcd.print(">/=");
}

void showCursor() {
  M5.Lcd.setTextSize(8);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.setCursor(98 + cursor * 40, 146);
  M5.Lcd.print("_");
}

void showR() {
  M5.Lcd.setTextSize(8);
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.fillRect(0, 120, 320, 80, BACKGROUND_COLOR);
  M5.Lcd.drawCentreString(getRString(), M5.Lcd.width()/2, 130, 1);
  showCursor();
}

void moveHistory() {
  for(int i=4; i>0; --i) {
    outcome[op][i] = outcome[op][i-1];
  }
  Serial.println(String("History for ") + String(op));
  for(int i=0; i<5; ++i) {
    Serial.println(String(outcome[op][i]));
  }
}

int calculateLevel() {
  int total_mistakes = 0;
  if (outcome[op][4] == -1) {
    return level[op];
  }
  for(int i=0; i<5; ++i) {
    total_mistakes += outcome[op][i];
  }
  if (total_mistakes < 3) {
    return level[op] + 1;
  } else if (total_mistakes > 7 && level[op] != 1) {
    return level[op] - 1;
  } else {
    return level[op];
  }
}

void showResult() {
  M5.Lcd.clear(BACKGROUND_COLOR);
  M5.Lcd.setCursor(40, 90);
  M5.Lcd.setTextSize(8);
  if (getR() == result) {
    moveHistory();
    outcome[op][0] = mistakes;
    int old_level = level[op];
    int new_level = calculateLevel();
    level[op] = new_level;
    if (new_level != old_level) {
      for(int i=0; i<5; i++) {
        outcome[op][i] = -1;
      }
    }
    writeGameSave(SAVE_FILE);
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.drawCentreString(F("GOOD!"), M5.Lcd.width()/2, 80, 1);
    if (new_level != old_level) {
      Serial.println(String("new level ") + String(new_level));
      M5.Lcd.drawCentreString(String("LEVEL=") + String(level[op]), M5.Lcd.width()/2, 160, 1);
      delay(2000);
    }
    writeGameSave(SAVE_FILE);
    delay(1000);
    initGame();  
  } else {
    mistakes += 1;
    M5.Lcd.setTextColor(RED);
    M5.Lcd.drawCentreString(F("WRONG!"), M5.Lcd.width()/2, M5.Lcd.height()/2-30, 1);
    delay(2000);
    draw();
  }
}

void draw() {
  M5.Lcd.clear(BACKGROUND_COLOR);
  showXY();
  showR();
  showButtons();
}

void setup() {
  M5.begin(true, true, true);

  if (M5.BtnA.isPressed()) {
    Serial.print("Removing save file");
    SD.remove(SAVE_FILE);
  } else {
    readGameSave(SAVE_FILE);
  }

  initGame();

  writeGameSave(SAVE_FILE);
}

void change(int pos, int delta) {
  r[pos] += delta;
  if (r[pos] == 10) r[pos] = 0;
  if (r[pos] < 0) r[pos] = 9;
  showR();
}

void loop() {
  M5.update();

  if (M5.BtnA.wasReleased()) {
    if (cursor > 0) {
      cursor -= 1;
      draw();
    }
  } else if (M5.BtnB.wasReleasefor(200)) {
    change(2 - cursor , DECR);
  } else if (M5.BtnB.wasReleased()) {
    change(2 - cursor , INCR);
  } else if (M5.BtnC.wasReleasefor(200)) {
    showResult();
  } else if (M5.BtnC.wasReleased()) {
    if (cursor < 2) {
      cursor += 1;
      draw();
    }
  }
}
