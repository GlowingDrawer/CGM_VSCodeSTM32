#pragma once
#include "adc.hpp"

class AdcSpiSigma24_Generic : public IAdc {
public:
    // drdy 可选：若你没接 DRDY，就传 nullptr/0 并让 dataReady() 永远 true
    AdcSpiSigma24_Generic(SpiDevice& dev, const GpioPin* drdy = nullptr)
        : dev_(dev), drdy_(drdy) {}

    Status init(const AdcConfig& cfg) override {
        if (cfg.vref <= 0.0f || cfg.bits < 16 || cfg.bits > 24) return Status::InvalidArg;
        cfg_ = cfg;
        // ====== 需要按具体 ADC 配置寄存器 ======
        // configure_();
        return Status::Ok;
    }

    bool dataReady() override {
        if (!drdy_) return true;
        // 常见 DRDY 低有效：低表示数据就绪（按你芯片改）
        return (drdy_->read() == false);
    }

    Status readRaw(int32_t& raw) override {
        // ====== 需要你按具体 ADC “读数据命令”修改 ======
        // 这里用“直接读 3 字节数据寄存器”的占位形式
        uint8_t tx[3] = { readCommand_(), 0x00, 0x00 }; // placeholder
        uint8_t rx[3] = {0};

        Status s = dev_.transfer(tx, rx, sizeof(rx));
        if (s != Status::Ok) return s;

        int32_t val = (static_cast<int32_t>(rx[0]) << 16) |
                      (static_cast<int32_t>(rx[1]) << 8)  |
                       static_cast<int32_t>(rx[2]);

        if (cfg_.bipolar) {
            // 24-bit 符号扩展
            if (val & 0x800000) val |= 0xFF000000;
        }
        raw = val;
        return Status::Ok;
    }

    Status readVoltage(float& volts) override {
        int32_t raw = 0;
        Status s = readRaw(raw);
        if (s != Status::Ok) return s;

        // 双极性：满量程约 ±(2^(bits-1)-1)
        const float fs = static_cast<float>((1u << (cfg_.bits - 1)) - 1u);
        float norm = static_cast<float>(raw) / fs; // -1..+1
        volts = norm * (cfg_.vref / static_cast<float>(cfg_.gain));
        return Status::Ok;
    }

private:
    uint8_t readCommand_() const {
        // placeholder：很多 ADC 是 0x00 或特定读命令
        return 0x00;
    }

    SpiDevice& dev_;
    const GpioPin* drdy_{nullptr};
    AdcConfig cfg_{};
};
