#pragma once

#include <cstdint>
#include "ch32v00x.h"
#include <cassert>
#include "sysClock_ch32v00x.hpp"
#include "systick_ch32v00x.hpp"

enum struct I2cInstance {i2c1, i2c2, i2c3, i2c4, i2c5, i2c6, i2c7, i2c8};
enum struct I2cSpeed {standart, fast};
enum struct I2cDuty {duty_2_1, duty_16_9};
enum struct I2cMode {master, slave};
enum struct I2cMemAddrSize {oneByte, twoBytes};

template <I2cInstance instance = I2cInstance::i2c1,
          I2cSpeed speed = I2cSpeed::fast,
          I2cDuty duty = I2cDuty::duty_2_1,
          I2cMode mode = I2cMode::master, 
          uint8_t ownAddress = 0x00>
struct I2cParams {
    static constexpr I2cInstance getInstance() {return instance;}
    static constexpr I2cSpeed getSpeed() {return speed;}
    static constexpr I2cDuty getDuty() {return duty;}
    static constexpr I2cMode getMode() {return mode;}
    static constexpr uint8_t getOwnAddress() {return ownAddress;}
};

struct I2CInterface {
    bool (*acknowledgePolling)(uint8_t devAddress, uint32_t attempts);
    void (*transmit)(uint8_t devAddress, const uint8_t *data, uint16_t size);
    void (*receive)(uint8_t devAddress, uint8_t *data, uint16_t size);
    void (*memoryWrite)(uint8_t devAddress, uint16_t memAddress, I2cMemAddrSize addressSize, const uint8_t *data, uint16_t size);
    void (*memoryRead)(uint8_t devAddress, uint16_t memAddress, I2cMemAddrSize addressSize, uint8_t *data, uint16_t size);
};

template<typename params, typename SysClock, typename SysTickTimer>
class I2c {
    static_assert(params::getMode() == i2cMode::master, "This library works only in Master mode yet");
public:
    enum ErrorCodes {
        ERROR_NONE = 0x00,
        ERROR_BERR = 0x00000001U,
        ERROR_ARLO = 0x00000002U,
        ERROR_AF = 0x00000004U,
        ERROR_OVR = 0x00000008U,
        ERROR_DMA = 0x00000010U,
        ERROR_TIMEOUT = 0x00000020U,
        ERROR_SIZE = 0x00000040U,
        ERROR_DMA_PARAM = 0x00000080U,
        WRONG_START = 0x00000200U
    };
private:
    static constexpr I2C_TypeDef* getInstance() {
        if constexpr(params::getInstance() == I2cInstance::i2c1) { return I2C1; }
        else { return nullptr; }
    }
    static void enableClock() {
        if constexpr(params::getInstance() == I2cInstance::i2c1) {RCC->APB1PCENR |= RCC_I2C1EN;}
    }
    static void disableClock() {
        if constexpr(params::getInstance() == I2cInstance::i2c1) {RCC->APB1PCENR &= (~RCC_I2C1EN);}
    }
    static void generateStart() {
        getInstance()->CTLR1 |= I2C_CTLR1_START;
    }
    static void generateStop() {
        getInstance()->CTLR1 |= I2C_CTLR1_STOP;
    }
    static void enable() {
        getInstance()->CTLR1 |= I2C_CTLR1_PE;
    }
    static void disable() {
        getInstance()->CTLR1 &= (~I2C_CTLR1_PE);
    }
    static void resetI2c() {
        getInstance()->CTLR1 |= I2C_CTLR1_SWRST;
        getInstance()->CTLR1 &= (~I2C_CTLR1_SWRST);
    }

    static constexpr uint16_t getFrequency() {
        constexpr uint16_t frequency = SysClock::getAPB1Clock() / 1000000;
        static_assert(frequency >= 4 && frequency <= 48, "Incorrect frerquency");
        return frequency;
    }
    static constexpr uint16_t getCR2Config() {
        return getFrequency();
    }
    static constexpr uint16_t getClockConfig() {
        constexpr uint16_t result;
        if constexpr (params::getSpeed() == I2cSpeed::fast) {
            if constexpr (params::getDuty() = I2cDuty::duty_16_9) {
                result = (uint16_t)(SysClock::getAPB1Clock() / (FAST_SPEED * 25));
                result |= I2C_CKCFGR_DUTY;
            } else {
                result = (uint16_t)(SysClock::getAPB1Clock() / (FAST_SPEED * 3));
            }
            result |= I2C_CKCFGR_FS;
        } else {
            result = (uint16_t)(SysClock::getAPB1Clock() / (STANDART_SPEED << 1));
        }
        if constexpr ((result & I2C_CKCFGR_CCR) == 0) {
            result |= 0x01;
        }
        return result;
    }
    static void masterPrepare() {
        waitForSR2FlagReset(I2C_SR2_BUSY);
        if(errorCode == ERROR_TIMEOUT) {
            return;
        }
        if((getInstance()->CR1 & I2C_CR1_PE) != I2C_CR1_PE) {
            enable();
        }
        getInstance()->CR1 &= (~I2C_CR1_POS);
        getInstance()->CR1 &= (~I2C_CR1_STOP);
    }
public:
    static inline uint32_t errorCode;
    static void init() {
        enableClock();
        resetI2c();
        getInstance()->CTLR2 = getCR2Config();
        getInstance()->CKCFGR = getClockConfig();
        enable();
    }
    static bool acknowledgePolling(uint8_t devAddress, uint32_t attempts) {
        while(attempts--) {
            errorCode = ERROR_NONE;
            masterPrepare();
            masterRequestRead(devAddress);
            if(errorCode == ERROR_NONE) {
                generateStop();
                return true;
            }
        }
        return false;
    }
    static void transmit(uint8_t devAddress, const uint8_t *data, uint16_t size) {
        errorCode = ERROR_NONE;
        masterPrepare();
        masterRequestWrite(devAddress);
        if(errorCode != ERROR_NONE) {
            return;
        }
        clearAddressFlag();
        masterSendBytes(data, size);
        generateStop();   
    }
    static void receive(uint8_t devAddress, uint8_t *data, uint16_t size) {
        errorCode = ERROR_NONE;
        masterPrepare();
        masterRequestRead(devAddress);
        if(errorCode != ERROR_NONE) {
            return;
        }
        masterReceiveBytes(data, size);
    }
    static void memoryWrite(uint8_t devAddress, uint16_t memAddress, I2cMemAddrSize addressSize, const uint8_t *data, uint16_t size) {
        errorCode = ERROR_NONE;
        masterPrepare();
        masterRequestWrite(devAddress);
        if(errorCode != ERROR_NONE) {
            return;
        }
        clearAddressFlag();
        uint8_t memAddr[2];
        uint16_t addrSize = 0;
        if(addressSize == I2cMemAddrSize::oneByte) {
            memAddr[addrSize++] = memAddress & 0xFF;
        } else {
            memAddr[addrSize++] = (memAddress>>8) & 0xFF;
            memAddr[addrSize++] = memAddress & 0xFF;
        }
        masterSendBytes(memAddr, addrSize);
        masterSendBytes(data, size);
        generateStop();  
    }
    static void memoryRead(uint8_t devAddress, uint16_t memAddress, I2cMemAddrSize addressSize, uint8_t *data, uint16_t size) {
        errorCode = ERROR_NONE;
        masterPrepare();
        masterRequestWrite(devAddress);
        if(errorCode != ERROR_NONE) {
            return;
        }
        clearAddressFlag();
        uint8_t memAddr[2];
        uint16_t addrSize = 0;
        if(addressSize == I2cMemAddrSize::oneByte) {
            memAddr[addrSize++] = memAddress & 0xFF;
        } else {
            memAddr[addrSize++] = (memAddress>>8) & 0xFF;
            memAddr[addrSize++] = memAddress & 0xFF;
        }
        masterSendBytes(memAddr, addrSize);
        masterRequestRead(devAddress);
        if(errorCode != ERROR_NONE) {
            return;
        }
        masterReceiveBytes(data, size);
    }
    static I2CInterface getInterface() {
        return {acknowledgePolling,
                transmit,
                receive,
                memoryWrite,
                memoryRead};
    }
private:
    static constexpr uint32_t STANDART_SPEED = 100000; 
    static constexpr uint32_t FAST_SPEED = 400000; 
    static constexpr uint32_t TIMEOUT = 10000;
};
