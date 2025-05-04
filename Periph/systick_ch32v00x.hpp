#pragma once

#include <cstdint>
#include "ch32v00x.h"

#define STK_CTLR_STE                              ((uint32_t)0x00000001)
#define STK_CTLR_STIE                             ((uint32_t)0x00000002)
#define STK_CTLR_STCLK                            ((uint32_t)0x00000004)
#define STK_CTLR_STRE                             ((uint32_t)0x00000008)
#define STK_CTLR_SWIE                             ((uint32_t)0x80000000)

template<typename SysClock>
class SysTickMs {
private:
    static constexpr uint32_t TICK_MS = 1000;
    static constexpr uint32_t PERIOD = SysClock::getAHBClock() / TICK_MS / 8;
public:
    static inline volatile uint32_t _ticks = 0;
    static void init() {
        SysTick->CNT = 0;
        SysTick->CMP = PERIOD - 1;
        SysTick->CTLR = STK_CTLR_STE | STK_CTLR_STRE | STK_CTLR_STIE;
        NVIC_EnableIRQ(SysTicK_IRQn);
    }
    static void delayMs(uint32_t ms) {
        uint32_t start = getTicks();
        while (getTicks() - start < ms) {}
    }
    static uint32_t getTicks() {
        return _ticks;
    }
    static void incrementTicks(void) {
        ++_ticks;
    }
};