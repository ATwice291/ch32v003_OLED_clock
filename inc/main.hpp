#pragma once

#include "rcc_ch32v00x.hpp"
#include "gpio_ch32v00x.hpp"
#include "systick_ch32v00x.hpp"
#include "i2c_ch32v00x.hpp"

#include "ds3231.hpp"
#include "ssd1306.hpp"

using SysClkHsi = SysClock<SysClockSource::HSI>;
using RccPllHsi = Rcc<SysClkHsi, AhbPsc::AHB1>;
using SysTickMsTimer = SysTickMs<RccPllHsi>;

using I2c1SDA = Gpio<GpioPort::C, GpioPin::P1, GpioMode::Out50M, GpioCnf::AltOD, GpioPull::Up>;
using I2c1SCL = Gpio<GpioPort::C, GpioPin::P2, GpioMode::Out50M, GpioCnf::AltOD, GpioPull::Up>;
using InputPin = Gpio<GpioPort::C, GpioPin::P3, GpioMode::In, GpioCnf::Pull, GpioPull::Up>;
using I2c1Params = I2cParams<I2cInstance::i2c1, I2cSpeed::fast>;
using I2c1 = I2c<I2c1Params, RccPllHsi, SysTickMsTimer>;

using ModeButton = Gpio<GpioPort::C, GpioPin::P0, GpioMode::In, GpioCnf::Pull, GpioPull::Up>;
using PlusButton = Gpio<GpioPort::C, GpioPin::P3, GpioMode::In, GpioCnf::Pull, GpioPull::Up>;
using MinusButton = Gpio<GpioPort::C, GpioPin::P4, GpioMode::In, GpioCnf::Pull, GpioPull::Up>;