// Copyright (c) 2025 Perevozchikov Aleksei [ATwice291]
// Licensed under the MIT License. See LICENSE file.

#include "main.hpp"
#include "interrupts.hpp"

DS3231* pExtClock;
SSD1306* pOledDisplay;

uint8_t oledBuf[SSD1306::BUFFER_SIZE];

static constexpr uint8_t FONT_SIZE = 13;
static const uint8_t font8x8[FONT_SIZE][8] = {
    {0x3C, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3C}, // 0
    {0x08, 0x18, 0x08, 0x08, 0x08, 0x08, 0x08, 0x1C}, // 1
    {0x3C, 0x42, 0x02, 0x1C, 0x20, 0x40, 0x42, 0x7E}, // 2
    {0x3C, 0x42, 0x02, 0x1C, 0x02, 0x02, 0x42, 0x3C}, // 3
    {0x04, 0x0C, 0x14, 0x24, 0x44, 0x7E, 0x04, 0x04}, // 4
    {0x7E, 0x40, 0x40, 0x7C, 0x02, 0x02, 0x42, 0x3C}, // 5
    {0x3C, 0x42, 0x40, 0x7C, 0x42, 0x42, 0x42, 0x3C}, // 6
    {0x7E, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x40}, // 7
    {0x3C, 0x42, 0x42, 0x3C, 0x42, 0x42, 0x42, 0x3C}, // 8
    {0x3C, 0x42, 0x42, 0x42, 0x3E, 0x02, 0x42, 0x3C}, // 9
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x60}, //.
    {0x00, 0x00, 0x60, 0x60, 0x00, 0x60, 0x60, 0x00}, //:
    {0x30, 0x48, 0x48, 0x30, 0x00, 0x00, 0x00, 0x00}  //D is DEGREES
};

enum struct ClockState{
    NORMAL,
    SETUP
} clockState; 
enum struct SetupState {
    HOURS,
    MINUTES,
    SECONDS,
    DATE,
    MONTH,
    YEAR,
    STATES_COUNT
} setupState;

void normalClockState();
void setupClockState(SetupState select, bool isBlink);
void showTime(DS3231* rtc, uint8_t x, uint8_t y);
void showDateWithoutYear(DS3231* rtc, uint8_t x, uint8_t y);
void showDateWithYear(DS3231* rtc, uint8_t x, uint8_t y);
void showTemperature(DS3231* rtc, uint8_t x, uint8_t y);
void showCursor(SetupState select, bool isBlink);
bool modeButtonPressed();
bool plusButtonPressed();
bool minusButtonPressed();
uint8_t getIndexOfChar(char c);
void drawChar16x16(uint8_t x, uint8_t y, char c);
void drawChar8x8(uint8_t x, uint8_t y, char c);

int main(void) {
  if(!RccPllHsi::init()) {
      for(;;){}
  }
  SysTickMsTimer::init();

  I2c1SDA::init();
  I2c1SCL::init();
  I2c1::init();
  
  DS3231 ExtClock(I2c1::getInterface(), 0x68<<1);
  pExtClock = &ExtClock;
  ExtClock.init();
  
  SSD1306 OledDisplay(I2c1::getInterface(), 0x3C<<1, oledBuf);
  pOledDisplay = &OledDisplay;
  OledDisplay.init();

  OledDisplay.fill(0);
  OledDisplay.updateScreen();
  
  bool isBlink = false;   
  uint8_t blincCounter = 0;
  for (;;) {
    OledDisplay.fill(0);
    
    switch(clockState) {
    case ClockState::NORMAL:
      normalClockState();
      if(modeButtonPressed()) {
        clockState = ClockState::SETUP;
        setupState = SetupState::HOURS;
      }
      break;
    case ClockState::SETUP:
      setupClockState(setupState, isBlink);
      if(modeButtonPressed() && setupState == SetupState::YEAR) {
        ExtClock.writeData();
        clockState = ClockState::NORMAL;
      }
      break;
    }
    
    OledDisplay.updateScreen();
    blincCounter++;
    isBlink = ((blincCounter & 0x04) == 0x04);
    SysTickMsTimer::delayMs(50);
  }
}

void normalClockState() {
  pExtClock->readData();
  showTime(pExtClock, 10, 40);
  showDateWithoutYear(pExtClock, 14, 10);
  showTemperature(pExtClock, 97, 10);
}

void setupClockState(SetupState select, bool isBlink) {
  showTime(pExtClock, 10, 40);
  showDateWithYear(pExtClock, 14, 10);
  showCursor(select, isBlink);
  TimeStruct time = pExtClock->getTime();
  DateStruct date = pExtClock->getDate();
  if(plusButtonPressed()) {
    switch(setupState) {
    case SetupState::HOURS:
      time.hours = (time.hours+1)%24;
      break;
    case SetupState::MINUTES:
      time.minutes = (time.minutes+1)%60;
      break;
    case SetupState::SECONDS:
      time.seconds = (time.seconds+1)%60;
      break;
    case SetupState::DATE:
      date.date = 1+(date.date%31);
      break;
    case SetupState::MONTH:
      date.month = 1+(date.month%12);
      break;
    case SetupState::YEAR:
      date.year = (date.year+1)%100;
      break;
    }
    pExtClock->setDate(date);
    pExtClock->setTime(time);
  }
  if(minusButtonPressed()) {
    switch(setupState) {
    case SetupState::HOURS:
      time.hours = (time.hours+23)%24;
      break;
    case SetupState::MINUTES:
      time.minutes = (time.minutes+59)%60;
      break;
    case SetupState::SECONDS:
      time.seconds = (time.seconds+59)%60;
      break;
    case SetupState::DATE:
      date.date = 1+((date.date+29)%31);
      break;
    case SetupState::MONTH:
      date.month = 1+((date.month+10)%12);
      break;
    case SetupState::YEAR:
      date.year = (date.year-1)%100;
      break;
    }
    pExtClock->setDate(date);
    pExtClock->setTime(time);
  }
  if(setupState < SetupState::YEAR && modeButtonPressed()) {
    setupState = SetupState(static_cast<uint8_t>(setupState)+1);
  }
}

void showTime(DS3231* rtc, uint8_t x, uint8_t y) {
  TimeStruct time = rtc->getTime();

  drawChar16x16(x,    y, '0'+time.hours/10);
  drawChar16x16(x+16, y, '0'+time.hours%10);

  drawChar16x16(x+32, y, ':');
  
  drawChar16x16(x+40, y, '0'+time.minutes/10);
  drawChar16x16(x+56, y, '0'+time.minutes%10);

  drawChar16x16(x+72, y, ':');
  
  drawChar16x16(x+80, y, '0'+time.seconds/10);
  drawChar16x16(x+96, y, '0'+time.seconds%10);
}

void showDateWithoutYear(DS3231* rtc, uint8_t x, uint8_t y) {
  DateStruct date = rtc->getDate();

  drawChar8x8(x, y, '0'+date.date/10);
  drawChar8x8(x+8, y, '0'+date.date%10);

  drawChar8x8(x+16, y, '.');  

  drawChar8x8(x+20, y, '0'+date.month/10);
  drawChar8x8(x+28, y, '0'+date.month%10);
}

void showDateWithYear(DS3231* rtc, uint8_t x, uint8_t y) {
  showDateWithoutYear(rtc, x, y);
  uint8_t year = rtc->getDate().year;
  
  drawChar8x8(x+36, y, '.');
  
  drawChar8x8(x+40, y, '2');
  drawChar8x8(x+48, y, '0');
  drawChar8x8(x+56, y, '0'+year/10);
  drawChar8x8(x+64, y, '0'+year%10);
}

void showTemperature(DS3231* rtc, uint8_t x, uint8_t y) {
  int8_t temperature = rtc->getTemperature();

  drawChar8x8(x+0, y, '0'+temperature/10);
  drawChar8x8(x+8, y, '0'+temperature%10);

  drawChar8x8(x+16, y, 'D');
}

void showCursor(SetupState select, bool isBlink) {
  uint8_t x, y, width, height;
  switch(select) {
  case SetupState::HOURS:
    x = 10; y = 39;
    width = 31; height = 17;
    break;
  case SetupState::MINUTES:
    x = 50; y = 39;
    width = 31; height = 17;
    break;
  case SetupState::SECONDS:
    x = 90; y = 39;
    width = 31; height = 17;
    break;
  case SetupState::DATE:
    x = 14; y = 9;
    width = 16; height = 9;
    break;
  case SetupState::MONTH:
    x = 34; y = 9;
    width = 16; height = 9;
    break;
  case SetupState::YEAR:
    x = 54; y = 9;
    width = 32; height = 9;
    break;
  }
  if(isBlink) {
    for(uint8_t i = x; i < x+width; i++) {
      for(uint8_t j  = y; j < y+height; j++) {
        pOledDisplay->invertPixel(i, j);
      }
    }
  }
}

bool modeButtonPressed() {
  static bool lastState = false;
  bool currState = ModeButton::read();
  bool pressed = lastState && !currState;
  lastState = currState;
  return pressed;
}

bool plusButtonPressed() {
  static bool lastState = false;
  bool currState = PlusButton::read();
  bool pressed = lastState && !currState;
  lastState = currState;
  return pressed;
}

bool minusButtonPressed() {
  static bool lastState = false;
  bool currState = MinusButton::read();
  bool pressed = lastState && !currState;
  lastState = currState;
  return pressed;
}

uint8_t getIndexOfChar(char c) {
  static const char map[] = "0123456789.:D";
  for(uint8_t i = 0; i < FONT_SIZE; ++i) {
    if(c == map[i]) {
      return i;
    }
  }
  return 10; // '.' for all unknown symbols
}

void drawChar16x16(uint8_t x, uint8_t y, char c) {
  uint8_t number = getIndexOfChar(c);
  const uint8_t* ptr = &font8x8[number][0];
  for(uint8_t i = 0; i < 8; i++) {
    for(uint8_t j = 0; j < 8; j++) {
      pOledDisplay->drawPixel(x+2*j, y+2*i, *ptr&(1<<(7-j)));
      pOledDisplay->drawPixel(x+2*j+1, y+2*i, *ptr&(1<<(7-j)));
      pOledDisplay->drawPixel(x+2*j, y+2*i+1, *ptr&(1<<(7-j)));
      pOledDisplay->drawPixel(x+2*j+1, y+2*i+1, *ptr&(1<<(7-j)));
    } 
    ptr++;
  }
}
void drawChar8x8(uint8_t x, uint8_t y, char c) {
  uint8_t number = getIndexOfChar(c);
  const uint8_t* ptr = &font8x8[number][0];
  for(uint8_t i = 0; i < 8; i++) {
    for(uint8_t j = 0; j < 8; j++) {
      pOledDisplay->drawPixel(x+j, y+i, *ptr&(1<<(7-j)));
    } 
    ptr++;
  }
}