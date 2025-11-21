#pragma once

#include <coco/platform/Loop_TIM2.hpp>
#include <coco/platform/SwdDevice_SPI.hpp>
#include <coco/board/config.hpp>


using namespace coco;


// drivers for SpiMasterTest
struct Drivers {
    Loop_TIM2 loop{APB1_TIMER_CLOCK};

    using SwdDevice = SwdDevice_SPI;
    SwdDevice swd{loop,
        gpio::PB3 | gpio::AF5 | gpio::Config::SPEED_MEDIUM, // SPI1 SCK (CN9 4) (don't forget to lookup the alternate function number in the data sheet!)
        gpio::PB4 | gpio::AF5 | gpio::Config::SPEED_MEDIUM, // SPI1 MISO (CN9 6)
        gpio::PB5 | gpio::AF5 | gpio::Config::SPEED_MEDIUM | gpio::Config::PULL_UP, // SPI1 MOSI (CN9 5)
        spi::SPI1_INFO,
        spi::ClockConfig::DIV_8};
    SwdDevice::Buffer<4> buffer1{swd};
    SwdDevice::Buffer<4> buffer2{swd};
};

Drivers drivers;

// Interrupt handlers (check in startup code if the handler name exists to prevent typos)
extern "C" {
void SPI1_IRQHandler() {
    drivers.swd.SPI_IRQHandler();
}
}
