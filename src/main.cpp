#include "main.hpp"

int main(void) {
    if(!RccPllHsi::init()) {
        for(;;){}
    }
    BlueLed::init();
    InputPin::init();
    for (;;) {
        if(InputPin::read()) {
            BlueLed::toggle();
            for (volatile int i = 0; i < 30000; ++i);
        }
    }
}