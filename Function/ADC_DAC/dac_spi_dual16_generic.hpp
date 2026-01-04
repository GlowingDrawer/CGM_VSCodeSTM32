#pragma once
#include "dac.hpp"

class DacSpiDual16_Generic : public IDac {
public:
    explicit DacSpiDual16_Generic(SpiDevice& dev) : dev_(dev) {}

    Status init(const DacConfig& cfg) override {
        if (cfg.vref <= 0.0f || cfg.bits == 0 || cfg.bits > 16) return Status::InvalidArg;
        cfg_ = cfg;
        fullscale_ = (1u << cfg_.bits) - 1u;
        return Status::Ok;
    }

    Status writeCode(DacChannel ch, uint32_t code) override {
        if (code > fullscale_) code = fullscale_;
        uint8_t frame[3];
        buildFrame_(ch, static_cast<uint16_t>(code), frame);
        return dev_.write(frame, sizeof(frame));
    }

    Status writeVoltage(DacChannel ch, float volts) override {
        volts = std::clamp(volts, 0.0f, cfg_.vref);
        uint32_t code = static_cast<uint32_t>((volts / cfg_.vref) * fullscale_ + 0.5f);
        return writeCode(ch, code);
    }

private:
    void buildFrame_(DacChannel ch, uint16_t code, uint8_t out[3]) {
        // ====== 需要你按具体DAC手册修改的部分 ======
        // 这里给“常见三字节：CMD/ADDR + DATA_H + DATA_L”的占位示例
        uint8_t addr = (ch == DacChannel::CH0) ? 0x00 : 0x01;
        uint8_t cmd  = 0x30; // placeholder：比如 write+update
        out[0] = static_cast<uint8_t>(cmd | (addr & 0x0F));
        out[1] = static_cast<uint8_t>((code >> 8) & 0xFF);
        out[2] = static_cast<uint8_t>(code & 0xFF);
    }

    SpiDevice& dev_;
    DacConfig cfg_{};
    uint32_t fullscale_{0};
};
