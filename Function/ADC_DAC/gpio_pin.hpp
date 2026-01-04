#pragma once
#include "stm32f10x_gpio.h"

struct GpioPin {
    GPIO_TypeDef* port;
    uint16_t pin;

    inline void high() const { GPIO_SetBits(port, pin); }
    inline void low()  const { GPIO_ResetBits(port, pin); }
    inline bool read() const { return GPIO_ReadInputDataBit(port, pin) != Bit_RESET; }
};

