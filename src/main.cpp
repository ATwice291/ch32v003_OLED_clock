#include "main.hpp"
#include "interrupts.hpp"

DS3231* pExtClock;
SSD1306* pOledDisplay;

uint8_t oledBuf[SSD1306::BUFFER_SIZE];

static const uint8_t font8x8[10][8] = {
    {0x3C, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3C}, // 0
    {0x08, 0x18, 0x08, 0x08, 0x08, 0x08, 0x08, 0x1C}, // 1
    {0x3C, 0x42, 0x02, 0x1C, 0x20, 0x40, 0x42, 0x7E}, // 2
    {0x3C, 0x42, 0x02, 0x1C, 0x02, 0x02, 0x42, 0x3C}, // 3
    {0x04, 0x0C, 0x14, 0x24, 0x44, 0x7E, 0x04, 0x04}, // 4
    {0x7E, 0x40, 0x40, 0x7C, 0x02, 0x02, 0x42, 0x3C}, // 5
    {0x3C, 0x42, 0x40, 0x7C, 0x42, 0x42, 0x42, 0x3C}, // 6
    {0x7E, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x40}, // 7
    {0x3C, 0x42, 0x42, 0x3C, 0x42, 0x42, 0x42, 0x3C}, // 8
    {0x3C, 0x42, 0x42, 0x42, 0x3E, 0x02, 0x42, 0x3C}  // 9
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
void drawNumber(uint8_t number, uint8_t x, uint8_t y);
void drawSmallNumber(uint8_t number, uint8_t x, uint8_t y);

int main(void) {
    if(!RccPllHsi::init()) {
        for(;;){}
    }
    
    SysTickMsTimer::init();
    BlueLed::init();
    InputPin::init();

    I2c1SDA::init();
    I2c1SCL::init();
    I2c1::init();
    uint8_t _raw[20];
    
    DS3231 ExtClock(I2c1::getInterface(), 0x68<<1);
    pExtClock = &ExtClock;
    ExtClock.init();
    
    SSD1306 OledDisplay(I2c1::getInterface(), 0x3C<<1, oledBuf);
    pOledDisplay = &OledDisplay;
    OledDisplay.init();

    uint8_t data[10];
    uint8_t size = 0;
    
    OledDisplay.fill(1);
    OledDisplay.updateScreen();
    SysTickMsTimer::delayMs(500);
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
    if(blincCounter == 10) {
      blincCounter = 0;
      isBlink = !isBlink;
    }
    SysTickMsTimer::delayMs(40);
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

  drawNumber(time.hours/10, x, y);
  drawNumber(time.hours%10, x+16, y);

  pOledDisplay->drawPixel(x+35, y+5, 1);
  pOledDisplay->drawPixel(x+35, y+6, 1);
  pOledDisplay->drawPixel(x+36, y+5, 1);
  pOledDisplay->drawPixel(x+36, y+6, 1);

  pOledDisplay->drawPixel(x+35, y+10, 1);
  pOledDisplay->drawPixel(x+35, y+11, 1);
  pOledDisplay->drawPixel(x+36, y+10, 1);
  pOledDisplay->drawPixel(x+36, y+11, 1);

  drawNumber(time.minutes/10, x+40, y);
  drawNumber(time.minutes%10, x+56, y);

  pOledDisplay->drawPixel(x+75, y+5, 1);
  pOledDisplay->drawPixel(x+75, y+6, 1);
  pOledDisplay->drawPixel(x+76, y+5, 1);
  pOledDisplay->drawPixel(x+76, y+6, 1);

  pOledDisplay->drawPixel(x+75, y+10, 1);
  pOledDisplay->drawPixel(x+75, y+11, 1);
  pOledDisplay->drawPixel(x+76, y+10, 1);
  pOledDisplay->drawPixel(x+76, y+11, 1);

  drawNumber(time.seconds/10, x+80, y);
  drawNumber(time.seconds%10, x+96, y);
}

void showDateWithoutYear(DS3231* rtc, uint8_t x, uint8_t y) {
  DateStruct date = rtc->getDate();
  
  drawSmallNumber(date.date/10, x, y);
  drawSmallNumber(date.date%10, x+8, y);
  pOledDisplay->drawPixel(x+16, y+7, 1);
  drawSmallNumber(date.month/10, x+18, y);
  drawSmallNumber(date.month%10, x+26, y);
}

void showDateWithYear(DS3231* rtc, uint8_t x, uint8_t y) {
  showDateWithoutYear(rtc, x, y);
  uint8_t year = rtc->getDate().year;
  
  pOledDisplay->drawPixel(x+34, y+7, 1);
  drawSmallNumber(2, x+36, y);
  drawSmallNumber(0, x+44, y);
  drawSmallNumber(year/10, x+52, y);
  drawSmallNumber(year%10, x+60, y);
}

void showTemperature(DS3231* rtc, uint8_t x, uint8_t y) {
  int8_t temperature = rtc->getTemperature();

  drawSmallNumber(temperature/10, x, y);
  drawSmallNumber(temperature%10, x+8, y);
  
  pOledDisplay->drawPixel(x+17, y, 1);
  pOledDisplay->drawPixel(x+18, y, 1);
  pOledDisplay->drawPixel(x+16, y+1, 1);
  pOledDisplay->drawPixel(x+19, y+1, 1);
  pOledDisplay->drawPixel(x+16, y+2, 1);
  pOledDisplay->drawPixel(x+19, y+2, 1);
  pOledDisplay->drawPixel(x+17, y+3, 1);
  pOledDisplay->drawPixel(x+18, y+3, 1);
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
    width = 15; height = 9;
    break;
  case SetupState::MONTH:
    x = 32; y = 9;
    width = 15; height = 9;
    break;
  case SetupState::YEAR:
    x = 50; y = 9;
    width = 31; height = 9;
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

void drawNumber(uint8_t number, uint8_t x, uint8_t y) {
  if(number > 9) return;
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
void drawSmallNumber(uint8_t number, uint8_t x, uint8_t y) {
  if(number > 9) return;
  const uint8_t* ptr = &font8x8[number][0];
  for(uint8_t i = 0; i < 8; i++) {
    for(uint8_t j = 0; j < 8; j++) {
      pOledDisplay->drawPixel(x+j, y+i, *ptr&(1<<(7-j)));
    } 
    ptr++;
  }
}