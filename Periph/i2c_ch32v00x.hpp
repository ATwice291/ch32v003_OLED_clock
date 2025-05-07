#pragma once

#include <cstdint>
#include "ch32v00x.h"
#include <cassert>
#include "rcc_ch32v00x.hpp"
#include "systick_ch32v00x.hpp"

enum struct I2cInstance {i2c1, i2c2, i2c3, i2c4, i2c5, i2c6, i2c7, i2c8};
enum struct I2cSpeed {standart, fast};
enum struct I2cDuty {duty_2_1, duty_16_9};
enum struct I2cMode {master, slave};
enum struct I2cMemAddrSize {oneByte, twoBytes};

enum struct I2cEvent {
    masterModeSelect = 0x00030001, /* EVT5 - BUSY, MSL and SB flag */
    masterTxModeSelected = 0x00070082,  /* EVT6 - BUSY, MSL, ADDR, TXE and TRA flags */
    masterRxModeSelected = 0x00030002,  /* EVT6 - BUSY, MSL and ADDR flags */
    masterModeAddress10 = 0x00030008,  /* EVT9 - BUSY, MSL and ADD10 flags */
    /* Master Receive mode */ 
    masterByteReceived = 0x00030040,  /* EVT7 - BUSY, MSL and RXNE flags */
    /* Master Transmitter mode*/
    masterByteTransmitting = 0x00070080, /* EVT8 - TRA, BUSY, MSL, TXE flags */
    masterByteTransmitted = 0x00070084,  /* EVT8_2 - TRA, BUSY, MSL, TXE and BTF flags */
    slaveRxAddressMatched = 0x00020002, /* EVT1 - BUSY and ADDR flags */
    slaveTxAddressMatched = 0x00060082, /* EVT1 - TRA, BUSY, TXE and ADDR flags */
    /* Slave Receiver mode*/ 
    slaveByteReceived = 0x00020040,  /* EVT2 - BUSY and RXNE flags */
    stopDetected = 0x00000010,  /* EVT4 - STOPF flag */
    /* Slave Transmitter mode -----------------------*/
    slaveByteTransmitted = 0x00060084,  /* EVT3 - TRA, BUSY, TXE and BTF flags */
    slaveByteTransmitting = 0x00060080,  /* EVT3 - TRA, BUSY and TXE flags */
    ackFailure = 0x00000400  /* EVT3_2 - AF flag */
};

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
    bool (*acknowledgePolling)(uint8_t devAddress, uint32_t timeout);
    void (*transmit)(uint8_t devAddress, const uint8_t *data, uint16_t size, uint32_t timeout);
    void (*receive)(uint8_t devAddress, uint8_t *data, uint16_t size, uint32_t timeout);
    void (*memoryWrite)(uint8_t devAddress, uint16_t memAddress, I2cMemAddrSize addressSize, const uint8_t *data, uint16_t size, uint32_t timeout);
    void (*memoryRead)(uint8_t devAddress, uint16_t memAddress, I2cMemAddrSize addressSize, uint8_t *data, uint16_t size, uint32_t timeout);
};

template<typename params, typename Rcc, typename SysTickMs>
class I2c {
    static_assert(params::getMode() == I2cMode::master, "This library works only in Master mode yet");
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
        constexpr uint16_t frequency = Rcc::getAPB1Clock() / 1000000;
        static_assert((frequency >= 4) && (frequency <= 48), "Incorrect frerquency");
        return frequency;
    }
    static constexpr uint16_t getCR2Config() {
        return getFrequency();
    }
    static constexpr uint16_t getClockConfig() {
        uint16_t result;
        if constexpr (params::getSpeed() == I2cSpeed::fast) {
            if constexpr (params::getDuty() == I2cDuty::duty_16_9) {
                result = (uint16_t)(Rcc::getAPB1Clock() / (FAST_SPEED * 25));
                result |= I2C_CKCFGR_DUTY;
            } else {
                result = (uint16_t)(Rcc::getAPB1Clock() / (FAST_SPEED * 3));
            }
            result |= I2C_CKCFGR_FS;
        } else {
            result = (uint16_t)(Rcc::getAPB1Clock() / (STANDART_SPEED << 1));
        }
        if ((result & I2C_CKCFGR_CCR) == 0) {
            result |= 0x01;
        }
        return result;
    }
    static bool timeIsUp(uint32_t tickStart, uint32_t timeout) {
        return (SysTickMs::getTicks() - tickStart >= timeout);
    }
    static bool status(I2cEvent event) {
        uint16_t status1 = (getInstance()->STAR1);
        uint16_t status2 = (getInstance()->STAR2);
        uint32_t lastEvent = ((status2<<16) | (status1)) & FLAG_MASK;
        uint32_t u32event = static_cast<uint32_t>(event);
        return ((lastEvent & u32event) == u32event);
    }
    static bool isAcknowledgeFailed() {
        if((getInstance()->STAR1 & I2C_STAR1_AF) == I2C_STAR1_AF) {
            getInstance()->STAR1 &= (~I2C_STAR1_AF);
            errorCode |= ERROR_AF;
            return true;
        }
        return false;
    }
    static void masterPrepare(uint32_t tickStart, uint32_t timeout) {
        while((getInstance()->STAR2 & I2C_STAR2_BUSY) == I2C_STAR2_BUSY) {
            if(timeIsUp(tickStart, timeout)) {
                errorCode |= ERROR_TIMEOUT;
                return;
            }
            if(isAcknowledgeFailed()) {
                generateStop();
            }
        }
        getInstance()->CTLR1 &= (~I2C_CTLR1_POS);
        getInstance()->CTLR1 &= (~I2C_CTLR1_STOP);
    }
    static void masterTxMode(uint8_t address, uint32_t tickStart, uint32_t timeout) {
        generateStart();
        while(!status(I2cEvent::masterModeSelect)) {
            if(timeIsUp(tickStart, timeout)) {
                errorCode |= ERROR_TIMEOUT;
                generateStop();
                return;
            }
        }
        getInstance()->DATAR = static_cast<uint8_t>(address & (~I2C_OADDR1_ADD0));
        while((getInstance()->STAR1 & I2C_STAR1_ADDR) != I2C_STAR1_ADDR) {
            if(timeIsUp(tickStart, timeout)) {
                errorCode |= ERROR_TIMEOUT;
                generateStop();
                return;
            }
            if(isAcknowledgeFailed()) {
                generateStop();
                return;
            }
        }
    }
    static void masterRxMode(uint8_t address, uint32_t tickStart, uint32_t timeout) {
        generateStart();
        while(!status(I2cEvent::masterModeSelect)) {
            if(timeIsUp(tickStart, timeout)) {
                errorCode |= ERROR_TIMEOUT;
                generateStop();
                return;
            }
        }
        getInstance()->DATAR = static_cast<uint8_t>(address | (I2C_OADDR1_ADD0));
        while((getInstance()->STAR1 & I2C_STAR1_ADDR) != I2C_STAR1_ADDR) {
            if(timeIsUp(tickStart, timeout)) {
                errorCode |= ERROR_TIMEOUT;
                generateStop();
                return;
            }
            if(isAcknowledgeFailed()) {
                generateStop();
                return;
            }
        }
    }
    static void masterSendBytes(const uint8_t *data, uint16_t size, uint32_t tickStart, uint32_t timeout) {
        while(size > 0) {
            getInstance()->DATAR = *data;
            ++data;
            --size;
            while(!status(I2cEvent::masterByteTransmitted)) {
                if(timeIsUp(tickStart, timeout)) {
                    errorCode |= ERROR_TIMEOUT;
                    generateStop();
                    return;
                }
            }
        }
    }
    static void masterReceiveBytes(uint8_t *data, uint16_t size, uint32_t tickStart, uint32_t timeout) {
        while(size > 0) {
            while((getInstance()->STAR1 & I2C_STAR1_RXNE) != I2C_STAR1_RXNE) {
                if(timeIsUp(tickStart, timeout)) {
                    errorCode |= ERROR_TIMEOUT;
                    generateStop();
                    return;
                }
            }
            if(size == 2) {
                getInstance()->CTLR1 &= (~I2C_CTLR1_ACK);
            }
            *data = getInstance()->DATAR;
            ++data;
            --size;
        }
    }
public:
    static inline uint32_t errorCode;
    static void init() {
        RCC->APB2PCENR |= RCC_AFIOEN;
        enableClock();
        resetI2c();
        getInstance()->CTLR2 = getCR2Config();
        getInstance()->CKCFGR = getClockConfig();
        enable();
    }
    static bool acknowledgePolling(uint8_t devAddress, uint32_t timeout) {
        uint32_t tickStart = SysTickMs::getTicks();
        for(;;) {
            errorCode = ERROR_NONE;
            masterPrepare(tickStart, timeout);
            masterRxMode(devAddress, tickStart, timeout);
            if(errorCode == ERROR_NONE) {
                generateStop();
                return true;
            }
        }
        return false;
    }
    static void transmit(uint8_t devAddress, const uint8_t *data, uint16_t size, uint32_t timeout) {
        uint32_t tickStart = SysTickMs::getTicks();
        errorCode = ERROR_NONE;
        masterPrepare(tickStart, timeout);
        masterTxMode(devAddress, tickStart, timeout);
        if(errorCode != ERROR_NONE) {
            return;
        }
        masterSendBytes(data, size, tickStart, timeout);
        if(errorCode != ERROR_NONE) {
            return;
        }
        generateStop();   
    }
    static void receive(uint8_t devAddress, uint8_t *data, uint16_t size, uint32_t timeout) {
        uint32_t tickStart = SysTickMs::getTicks();
        errorCode = ERROR_NONE;
        masterPrepare(tickStart, timeout);
        masterRxMode(devAddress, tickStart, timeout);
        if(errorCode != ERROR_NONE) {
            return;
        }
        getInstance()->CTLR1 |= (I2C_CTLR1_ACK);
        masterReceiveBytes(data, size, tickStart, timeout);
        if(errorCode != ERROR_NONE) {
            return;
        }
        generateStop();  
    }
    static void memoryWrite(uint8_t devAddress, uint16_t memAddress, I2cMemAddrSize addressSize, const uint8_t *data, uint16_t size, uint32_t timeout) {
        uint32_t tickStart = SysTickMs::getTicks();
        errorCode = ERROR_NONE;
        masterPrepare(tickStart, timeout);
        masterTxMode(devAddress, tickStart, timeout);
        if(errorCode != ERROR_NONE) {
            return;
        }
        uint8_t memAddr[2];
        uint16_t addrSize = 0;
        if(addressSize == I2cMemAddrSize::twoBytes) {
            memAddr[addrSize++] = (memAddress>>8) & 0xFF;
        }
        memAddr[addrSize++] = memAddress & 0xFF;
        masterSendBytes(memAddr, addrSize, tickStart, timeout);
        masterSendBytes(data, size, tickStart, timeout);
        if(errorCode != ERROR_NONE) {
            return;
        }
        generateStop();  
    }
    static void memoryRead(uint8_t devAddress, uint16_t memAddress, I2cMemAddrSize addressSize, uint8_t *data, uint16_t size, uint32_t timeout) {
        uint32_t tickStart = SysTickMs::getTicks();
        errorCode = ERROR_NONE;
        masterPrepare(tickStart, timeout);
        masterTxMode(devAddress, tickStart, timeout);
        if(errorCode != ERROR_NONE) {
            return;
        }
        uint8_t memAddr[2];
        uint16_t addrSize = 0;
        if(addressSize == I2cMemAddrSize::twoBytes) {
            memAddr[addrSize++] = (memAddress>>8) & 0xFF;
        }
        memAddr[addrSize++] = memAddress & 0xFF;
        masterSendBytes(memAddr, addrSize, tickStart, timeout);
        masterRxMode(devAddress, tickStart, timeout);
        if(errorCode != ERROR_NONE) {
            return;
        }
        status(I2cEvent::masterModeSelect);
        getInstance()->CTLR1 |= (I2C_CTLR1_ACK);
        masterReceiveBytes(data, size, tickStart, timeout);
        if(errorCode != ERROR_NONE) {
            return;
        }
        generateStop(); 
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
    static constexpr uint32_t FLAG_MASK = 0x00FFFFFF;
};
