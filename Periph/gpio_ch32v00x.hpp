#pragma once

#include <cstdint>
#include "ch32v00x.h"
#include <cassert>

enum struct GpioPort { 
    A = GPIOA_BASE,
    C = GPIOC_BASE,
    D = GPIOD_BASE};
enum struct GpioPin : uint8_t { P0, P1, P2, P3, P4, P5, P6, P7};
enum struct GpioMode : uint8_t { In=0b00, Out2M=0b01, Out10M=0b10, Out50M=0b11};
enum struct GpioCnf : uint8_t { Analog=0b00, Float=0b01, Pull=0b10, PP=0b100, OD=0b101, AltPP=0b110, AltOD=0b111};
enum struct GpioPull : uint8_t { Down=0b0, Up=0b1};

template<GpioPort Port, GpioPin Pin, GpioMode Mode, GpioCnf Cnf, GpioPull Pull>
class Gpio{
private:
    static_assert(
        (Mode == GpioMode::In && static_cast<uint8_t>(Cnf) < 4) ||
        (Mode != GpioMode::In && static_cast<uint8_t>(Cnf) > 3),
        "Invalid CNF type for the specified GpioMode"
    );
    static constexpr uint8_t pin = static_cast<uint8_t>(Pin);
    static constexpr uint32_t pinMask = (1 << pin);
    static constexpr uint32_t mode = ((static_cast<uint8_t>(Mode)) | ((static_cast<uint8_t>(Cnf)&0b11) << 2)) << (pin * 4);
    static constexpr uint32_t pull = static_cast<uint8_t>(Pull) << pin;
    
    static constexpr GPIO_TypeDef* getInstance() {
        if constexpr (Port == GpioPort::A) return GPIOA;
        if constexpr (Port == GpioPort::C) return GPIOC;
        if constexpr (Port == GpioPort::D) return GPIOD;
    }
public:
    static void init() {
        // Включаем тактирование порта
        if constexpr (Port == GpioPort::A) RCC->APB2PCENR |= RCC_IOPAEN;
        if constexpr (Port == GpioPort::C) RCC->APB2PCENR |= RCC_IOPCEN;
        if constexpr (Port == GpioPort::D) RCC->APB2PCENR |= RCC_IOPDEN;

        getInstance()->OUTDR &= (~pinMask);
        getInstance()->OUTDR |= pull;

        getInstance()->CFGLR &= (~(0b1111 << (pin * 4)));
        getInstance()->CFGLR |= mode;
    }
    static bool read() {
        return (getInstance()->INDR & pinMask) != 0;
    }
    static void set() {
        getInstance()->BSHR = pinMask;
    }
    static void reset() {
       getInstance()->BCR = pinMask;
    }
    static void toggle() {
        getInstance()->OUTDR ^= pinMask;
    }
};