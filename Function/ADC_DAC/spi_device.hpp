#pragma once
#include "spi_bus.hpp"
#include "gpio_pin.hpp"

class SpiDevice {
public:
    SpiDevice(SpiBus& bus, GpioPin cs) : bus_(bus), cs_(cs) {}

    Status write(const uint8_t* tx, size_t n) {
        cs_.low();
        Status s = bus_.transfer(tx, nullptr, n);
        cs_.high();
        return s;
    }

    Status transfer(const uint8_t* tx, uint8_t* rx, size_t n) {
        cs_.low();
        Status s = bus_.transfer(tx, rx, n);
        cs_.high();
        return s;
    }

private:
    SpiBus& bus_;
    GpioPin cs_;
};
