#pragma once
#include <cstdint>
#include "spi_device.hpp"
#include "gpio_pin.hpp"

struct AdcConfig {
    float vref = 2.500f;
    uint8_t bits = 24;     // 18/24
    uint16_t sps = 200;    // 采样率（用于配置寄存器时用）
    uint8_t gain = 1;      // 若ADC带PGA
    bool bipolar = true;   // 24-bit two's complement
};

class IAdc {
public:
    virtual ~IAdc() = default;
    virtual Status init(const AdcConfig& cfg) = 0;
    virtual bool dataReady() = 0;
    virtual Status readRaw(int32_t& raw) = 0;
    virtual Status readVoltage(float& volts) = 0;
};
