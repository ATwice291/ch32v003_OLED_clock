#pragma once

#include "main.hpp"

extern "C" void HardFault_Handler(void) __attribute__((interrupt));
extern "C" void SysTick_Handler(void) __attribute__((interrupt));


extern "C" void HardFault_Handler(void) {
	for(;;){}
}

extern "C" void SysTick_Handler(void) {
    SysTick->SR = 0;
    SysTickTimer::incrementTicks();
}