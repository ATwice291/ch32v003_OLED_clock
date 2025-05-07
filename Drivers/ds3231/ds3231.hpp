#pragma once

#include <cstdint>
#include "../../Periph/i2c_ch32v00x.hpp"

using DateStruct = struct {
    uint8_t date;
    uint8_t month;
    uint8_t year;
};
using TimeStruct = struct {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
};

class DS3231 {
private:
    void setControlRegister() {
        _i2c.memoryWrite(_devAddress, 0x0E, I2cMemAddrSize::oneByte, &_raw[0x0E], 1, 10);
    }
    //osMutexId _rtcMutex;
    void initMutex() {
        //_rtcMutex = osMutexNew(nullptr);
        //if (_rtcMutex == nullptr) {
        //    while (1);
        //}
    }
    void lock() {
        //osMutexWait(_rtcMutex, osWaitForever);
    }
    void unlock() {
        //osMutexRelease(_rtcMutex);
    }
public:
    static constexpr uint16_t DATA_SIZE = 19;

    DS3231(I2CInterface i2c, uint8_t devAddress) 
    : _i2c(i2c), _devAddress(devAddress) {}

    void init() {
        initMutex();
        readData();
        _raw[0x0E] = 0x04;
        setControlRegister();
    }
    bool isTimeWrong() {
        return _data.OSF;
    }
    DateStruct getDate() {
        return {static_cast<uint8_t>(_data.dateTens*10 + _data.date), 
                static_cast<uint8_t>(_data.monthTens*10 + _data.month),
                static_cast<uint8_t>(_data.yearTens*10 + _data.year)};
    }
    TimeStruct getTime() {
        return {static_cast<uint8_t>(_data.hourTens*10 + _data.hours), 
                static_cast<uint8_t>(_data.minuteTens*10 + _data.minutes),
                static_cast<uint8_t>(_data.secondTens*10 + _data.seconds)};
    }
    int8_t getTemperature() {
        return _data.temperature;
    }
    void setDate(DateStruct dateStruct) {
        lock();
        _data.dateTens = dateStruct.date / 10;
        _data.date = dateStruct.date % 10;
        _data.monthTens = dateStruct.month / 10;
        _data.month = dateStruct.month % 10;
        _data.yearTens = dateStruct.year / 10;
        _data.year = dateStruct.year % 10;
        _data.OSF = 0;
        unlock();
    }
    void setTime(TimeStruct timeStruct) {
        lock();
        _data.hourTens = timeStruct.hours / 10;
        _data.hours = timeStruct.hours % 10;
        _data.minuteTens = timeStruct.minutes / 10;
        _data.minutes = timeStruct.minutes % 10;
        _data.secondTens = timeStruct.seconds / 10;
        _data.seconds = timeStruct.seconds % 10;
        _data.OSF = 0;
        unlock();
    }
    void readData() {
        lock();
        _i2c.memoryRead(_devAddress, 0x00, I2cMemAddrSize::oneByte, _raw, DATA_SIZE, 10);
        unlock();
    }
    void writeData() {
        lock();
        _i2c.memoryWrite(_devAddress, 0x00, I2cMemAddrSize::oneByte, _raw, DATA_SIZE, 10);
        unlock();
    }
private:
    I2CInterface _i2c;
    uint8_t _devAddress;
    union {
        struct {
            unsigned seconds :4;
            unsigned secondTens :4;
            unsigned minutes :4;
            unsigned minuteTens :4;
            unsigned hours :4;
            unsigned hourTens :4;
            uint8_t day;
            unsigned date :4;
            unsigned dateTens :4;
            unsigned month :4;
            unsigned monthTens :3;
            unsigned century :1;
            unsigned year :4;
            unsigned yearTens :4;
            uint8_t alarm1_sec;
            uint8_t alarm1_min;
            uint8_t alarm1_hour;
            uint8_t alarm1_date;
            uint8_t alarm2_min;
            uint8_t alarm2_hour;
            uint8_t alarm2_date;
            unsigned A1IE :1;
            unsigned A2IE :1;
            unsigned INTCN :1;
            unsigned RS1_2 :2;
            unsigned CONV :1;
            unsigned BBSQW :1;
            unsigned EOSC :1;
            unsigned A1F :1;
            unsigned A2F :1;
            unsigned BSY :1;
            unsigned EN32KHZ :1;
            unsigned reserved1 :3;
            unsigned OSF :1;
            int8_t ageOffset;
            int8_t temperature;
            unsigned reserved2 :6;
            unsigned temperatureLSB :2;
        }_data;
        uint8_t _raw[DS3231::DATA_SIZE];
    };
};