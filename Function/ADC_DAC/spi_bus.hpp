#pragma once
#include "stm32f10x_spi.h"
#include <cstddef>
#include <cstdint>

enum class Status : uint8_t {
    Ok = 0,
    Timeout,
    BusError,
    InvalidArg,
    NotReady,
};

class SpiBus {
public:
    SpiBus(SPI_TypeDef* spi, uint32_t timeout_cycles = 100000)
        : spi_(spi), timeout_(timeout_cycles) {}

    // 全双工传输 1 byte
    Status transfer8(uint8_t tx, uint8_t& rx) {
        if (!waitFlag_(SPI_I2S_FLAG_TXE, true)) return Status::Timeout;
        SPI_I2S_SendData(spi_, tx);

        if (!waitFlag_(SPI_I2S_FLAG_RXNE, true)) return Status::Timeout;
        rx = static_cast<uint8_t>(SPI_I2S_ReceiveData(spi_));
        return Status::Ok;
    }

    // 传输 N bytes（tx 可为空表示发 0xFF；rx 可为空表示丢弃）
    Status transfer(const uint8_t* tx, uint8_t* rx, size_t n) {
        for (size_t i = 0; i < n; ++i) {
            uint8_t r = 0;
            uint8_t t = tx ? tx[i] : 0xFF;
            Status s = transfer8(t, r);
            if (s != Status::Ok) return s;
            if (rx) rx[i] = r;
        }
        return Status::Ok;
    }

    void setTimeout(uint32_t timeout_cycles) { timeout_ = timeout_cycles; }

private:
    bool waitFlag_(uint16_t flag, bool set) {
        uint32_t t = timeout_;
        while (t--) {
            FlagStatus fs = SPI_I2S_GetFlagStatus(spi_, flag);
            if (set && fs == SET) return true;
            if (!set && fs == RESET) return true;
        }
        return false;
    }

    SPI_TypeDef* spi_;
    uint32_t timeout_;
};
