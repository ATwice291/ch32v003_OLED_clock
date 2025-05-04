#include "main.hpp"
#include "interrupts.hpp"

uint32_t ticks;
uint32_t mStat;


int main(void) {

    if(!RccPllHsi::init()) {
        for(;;){}
    }
    ticks = 0;
    SysTickTimer::init();
    BlueLed::init();
    InputPin::init();
    for (;;) {
        ticks = SysTickTimer::getTicks();
        if(InputPin::read()) {
            BlueLed::toggle();
            SysTickTimer::delayMs(500);
        }
    }
}