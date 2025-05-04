#pragma once

#include "sysClock_ch32v00x.hpp"
#include "gpio_ch32v00x.hpp"
#include "systick_ch32v00x.hpp"

using SysClkHsi = SysClock<SysClockSource::HSI>;

using Hse24 = Hse<24000000, HseBypass::OFF>;
using SysClkHse = SysClock<SysClockSource::HSE, Hse24, ClockSafety::ON>;

using RccPllHsi = Rcc<SysClkHsi, AhbPsc::AHB3>;
//using RccPllHse = Rcc<SysClkHse, AhbPsc::AHB3>;

using SysTickTimer = SysTickMs<RccPllHsi>;

using BlueLed = Gpio<GpioPort::D, GpioPin::P6, GpioMode::Out2M, GpioCnf::PP, GpioPull::Down>;
using InputPin = Gpio<GpioPort::C, GpioPin::P3, GpioMode::In, GpioCnf::Pull, GpioPull::Up>;