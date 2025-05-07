#pragma once

#include <cstdint>
#include "ch32v00x.h"
#include <cassert>
#include <array>

#if !defined  (HSI_VALUE)
  #define HSI_VALUE    ((uint32_t)24000000)
#endif /* HSI_VALUE */

enum struct HseBypass {OFF, ON};
enum struct PllSource {HSI, HSE};
enum struct SysClockSource {HSI, HSE, PLL};
enum struct ClockSafety {OFF, ON}; 
enum struct AhbPsc {AHB1, AHB2, AHB3, AHB4, AHB5, AHB6, AHB7, AHB8, AHB16, AHB32, AHB64, AHB128, AHB256};
enum struct AdcPsc {ADC2, ADC4, ADC6, ADC8, ADC12, ADC16, ADC24, ADC32, ADC48, ADC64, ADC96, ADC128};

struct Hsi {
    static constexpr uint32_t getClock() { return HSI_VALUE; }
};

template <uint32_t clock, HseBypass bypass>
struct Hse {
    static_assert(clock >= 4000000 && clock <= 25000000, "Invalid HSE clock frequency!");
    static constexpr uint32_t getClock() { return clock;}
    static constexpr HseBypass getBypass() { return bypass;}
};

template <PllSource source, typename SourceType = Hsi>
struct Pll;

template<typename HsiType>
struct Pll<PllSource::HSI, HsiType> {
    using hsi = HsiType;
    static constexpr PllSource getSource() { return PllSource::HSI; }
    static constexpr uint32_t getClock() { return hsi::getClock() * 2; }
};

template<typename HseType>
struct Pll<PllSource::HSE, HseType> {
    using hse = HseType;
    static constexpr PllSource getSource() { return PllSource::HSE; }
    static constexpr uint32_t getClock() { return hse::getClock() * 2; }
};

template<SysClockSource source, typename SourceType = Hsi, ClockSafety css = ClockSafety::OFF>
struct SysClock; 

template<typename HsiType>
struct SysClock<SysClockSource::HSI, HsiType, ClockSafety::OFF> {
    using hsi = HsiType;
    static constexpr SysClockSource getSource() { return SysClockSource::HSI; }
    static constexpr ClockSafety getCss() { return ClockSafety::OFF; }
    static constexpr uint32_t getClock() { return hsi::getClock(); }
};

template<typename HseType, ClockSafety css>
struct SysClock<SysClockSource::HSE, HseType, css> {
    using hse = HseType;
    static constexpr SysClockSource getSource() { return SysClockSource::HSE; }
    static constexpr ClockSafety getCss() { return css; }
    static constexpr uint32_t getClock() { return hse::getClock(); }
};

template<typename PllType, ClockSafety css>
struct SysClock<SysClockSource::PLL, PllType, css> {
    using pll = PllType;
    static constexpr SysClockSource getSource() { return SysClockSource::PLL; }
    static constexpr ClockSafety getCss() { return css; }
    static constexpr uint32_t getClock() { return pll::getClock(); }
};

template<typename sysClock, AhbPsc ahbPsc, AdcPsc adcPsc = AdcPsc::ADC128>
class Rcc {
private:
    static constexpr uint16_t TIMEOUT = 60000;
    static constexpr uint32_t calcAhb() {
        constexpr std::array<uint32_t, 13> values = {
            1, //AHB1, 
            2, //AHB2, 
            3, //AHB3, 
            4, //AHB4, 
            5, //AHB5, 
            6, //AHB6, 
            7, //AHB7, 
            8, //AHB8, 
            16, //AHB16, 
            32, //AHB32, 
            64, //AHB64, 
            128, //AHB128, 
            256  //AHB256
        };
        return values[static_cast<uint32_t>(ahbPsc)];
    }
    static constexpr uint32_t calcHPRE() {
        constexpr std::array<uint32_t, 13> values = {
            RCC_HPRE_DIV1, //AHB1, 
            RCC_HPRE_DIV2, //AHB2, 
            RCC_HPRE_DIV3, //AHB3, 
            RCC_HPRE_DIV4, //AHB4, 
            RCC_HPRE_DIV5, //AHB5, 
            RCC_HPRE_DIV6, //AHB6, 
            RCC_HPRE_DIV7, //AHB7, 
            RCC_HPRE_DIV8, //AHB8, 
            RCC_HPRE_DIV16, //AHB16, 
            RCC_HPRE_DIV32, //AHB32, 
            RCC_HPRE_DIV64, //AHB64, 
            RCC_HPRE_DIV128, //AHB128, 
            RCC_HPRE_DIV256  //AHB256
        };
        return values[static_cast<uint32_t>(ahbPsc)];
    }
    static constexpr uint32_t calcAdc() {
        constexpr std::array<uint32_t, 12> values = {
            2, //ADC2, 
            4, //ADC4, 
            6, //ADC6, 
            8, //ADC8, 
            12, //ADC12, 
            16, //ADC16, 
            24, //ADC24, 
            32, //ADC32, 
            48, //ADC48, 
            64, //ADC64, 
            96, //ADC96, 
            128 //ADC128
        };
        return values[static_cast<uint32_t>(adcPsc)];
    }
    static constexpr uint32_t calcADCPRE() {
        constexpr std::array<uint32_t, 12> values = {
            RCC_ADCPRE_DIV2 | RCC_PPRE2_DIV1, //ADC2, 
            RCC_ADCPRE_DIV4 | RCC_PPRE2_DIV1, //ADC4, 
            RCC_ADCPRE_DIV6 | RCC_PPRE2_DIV1, //ADC6, 
            RCC_ADCPRE_DIV8 | RCC_PPRE2_DIV1, //ADC8, 
            RCC_ADCPRE_DIV6 | RCC_PPRE2_DIV2, //ADC12, 
            RCC_ADCPRE_DIV8 | RCC_PPRE2_DIV2, //ADC16, 
            RCC_ADCPRE_DIV6 | RCC_PPRE2_DIV4, //ADC24, 
            RCC_ADCPRE_DIV8 | RCC_PPRE2_DIV4, //ADC32, 
            RCC_ADCPRE_DIV6 | RCC_PPRE2_DIV8, //ADC48, 
            RCC_ADCPRE_DIV8 | RCC_PPRE2_DIV8, //ADC64, 
            RCC_ADCPRE_DIV6 | RCC_PPRE2_DIV16, //ADC96, 
            RCC_ADCPRE_DIV8 | RCC_PPRE2_DIV16 //ADC128
        };
        return values[static_cast<uint32_t>(adcPsc)];
    }
    static constexpr uint32_t _HPRE = calcHPRE();
    static constexpr uint32_t _ADCPRE = calcADCPRE();
    static void setPrescalers() {
        RCC->CFGR0 &= 0xFFFF070F;
        RCC->CFGR0 |= (_HPRE | _ADCPRE);
    }
    static bool enableHSE() {
        RCC->CTLR |= RCC_HSEON;
        uint16_t i = 0;
        while (!(RCC->CTLR & RCC_HSERDY)) {
            if(++i > TIMEOUT) {
                return false;
            }
        }
        return true;
    }
    static bool enableHSEBypass() {
        RCC->CTLR &= (~(RCC_HSEON));
        RCC->CTLR |= (RCC_HSEBYP);
        return enableHSE();
    }
    static void disableHSE() {
        RCC->CTLR &= (~(RCC_HSEON));
        RCC->CTLR &= (~(RCC_HSEBYP));
        while ((RCC->CTLR & RCC_HSERDY)) {}
    }
    static bool enablePLL(PllSource pllSrc) {
        disablePLL();
        RCC->CFGR0 &= (~RCC_PLLSRC);
        RCC->CFGR0 |= ((pllSrc == PllSource::HSE) ? RCC_PLLSRC : 0);
        RCC->CTLR |= RCC_PLLON;
        uint16_t i = 0;
        while (!(RCC->CTLR & RCC_PLLRDY)) {
            if(++i > TIMEOUT) {
                return false;
            }
        }
        return true;
    }
    static void disablePLL() {
        RCC->CTLR &= (~RCC_PLLON);
        while ((RCC->CTLR & RCC_PLLRDY)) {}
    }
    static constexpr uint32_t calcNewFlashLatency() {
        return getSysClock()/24000000 - (getSysClock()%24000000==0?1:0);
    }
    static uint32_t getFlashLatency() {
        return (FLASH->ACTLR & FLASH_ACTLR_LATENCY);
    }
    static void setFlashLatency(uint32_t latency) {
        FLASH->ACTLR &= (~FLASH_ACTLR_LATENCY);
        FLASH->ACTLR |= (latency & FLASH_ACTLR_LATENCY);
    }
    static void switchSysClock(uint32_t source) {
        uint32_t oldLatency = getFlashLatency();
        uint32_t newLatency = calcNewFlashLatency();
        if(newLatency > oldLatency) {
            setFlashLatency(newLatency);
        }
        RCC->CFGR0 &= (~RCC_SW);
        RCC->CFGR0 |= source;
        while ((RCC->CFGR0 & RCC_SWS) != (source << 2)) {}
        if(newLatency < oldLatency) {
            setFlashLatency(newLatency);
        }
    }
public:
    static constexpr uint32_t getSysClock() {
        if constexpr(sysClock::getSource() == SysClockSource::HSI) {
            return sysClock::hsi::getClock();
        } else if constexpr(sysClock::getSource() == SysClockSource::HSE) {
            return sysClock::hse::getClock();
        } else if constexpr(sysClock::getSource() == SysClockSource::PLL) {
            return sysClock::pll::getClock();
        } else {
            return 0;
        }
    }
    static bool init() {
        setPrescalers();
        switchSysClock(RCC_SW_HSI);
        if constexpr(sysClock::getSource() == SysClockSource::HSI) {
            disablePLL();
            disableHSE();
        } else if constexpr(sysClock::getSource() == SysClockSource::HSE) {
            if constexpr(sysClock::hse::getBypass() == HseBypass::ON) {
                if(!enableHSEBypass()) { return false; }
            } else {
                if(!enableHSE()) { return false; }
            }
            switchSysClock(RCC_SW_HSE);
            disablePLL();
        } else if constexpr(sysClock::getSource() == SysClockSource::PLL) {
            if constexpr (sysClock::pll::getSource() == PllSource::HSE) { 
                if constexpr(sysClock::pll::hse::getBypass() == HseBypass::ON) {
                    if(!enableHSEBypass()) { return false; }
                } else {
                    if(!enableHSE()) { return false; }
                }
            }
            if(!enablePLL(sysClock::pll::getSource())) { return false; }
            switchSysClock(RCC_SW_PLL);
            if constexpr (sysClock::pll::getSource() == PllSource::HSI) {
                disableHSE();
            }
        }
        if constexpr(sysClock::getCss() == ClockSafety::ON) {
            RCC->CTLR |= RCC_CSSON;
        } else {
            RCC->CTLR &= (~RCC_CSSON);
        }
        return true;
    }
    static constexpr uint32_t getAHBClock() {
        return getSysClock() / calcAhb();
    }
    static constexpr uint32_t getAPB1Clock() {
        return getAHBClock();
    }
    static constexpr uint32_t getAPB2Clock() {
        return getAHBClock();
    }
};

