#pragma once
#include <stdint.h>

// DPV 运行状态
enum class DPV_State : uint8_t {
    IDLE,
    WAIT_BASE_TIME,   // 基电位保持阶段
    WAIT_PULSE_TIME   // 脉冲电位保持阶段
};

// DPV 参数结构体（以“相对电位”为输入：0V 表示中点偏置 midVolt）
struct DPV_Params {
    float startVolt = -0.5f;
    float endVolt   =  0.5f;
    float stepVolt  =  0.005f;   // 阶梯增量（幅值）
    float pulseAmp  =  0.05f;    // 脉冲幅度（可为负，表示反向脉冲）

    uint16_t pulsePeriodMs = 50; // 周期（ms）
    uint16_t pulseWidthMs  = 10; // 脉冲宽度（ms）

    // 在“阶段结束前 sampleLeadMs”触发采样标记，避免落在跳变瞬态
    uint16_t sampleLeadMs  = 1;

    // DAC 中点偏置（V），默认 1.65V（对应 DAC≈2048）
    float midVolt = 1.65f;
};

class DPVController {
private:
    DPV_Params params{};
    DPV_State  state = DPV_State::IDLE;

    int32_t currentBaseCode = 2048;
    int32_t codeEnd         = 2048;
    int32_t codeStep        = 1;
    int32_t codePulse       = 0;

    uint16_t timerCount   = 0;
    uint16_t baseTimeMs   = 1;
    uint16_t pulseWidthMs = 1;
    uint16_t sampleLeadMs = 1;

    volatile uint8_t sampleFlags = 0; // bit0=I1, bit1=I2
    uint16_t currentOutputCode   = 2048;

public:
    void SetParams(const DPV_Params& p);
    void Start();
    void Stop();

    // 每 1ms 调一次（由 DAC 定时器中断驱动）
    // 返回：DAC 输出是否发生变化（用于非 DMA 模式下是否需要手动写 DAC）
    bool StepTick();

    uint16_t GetCurrentCode() const { return currentOutputCode; }

    // 读取并清除采样标记：bit0=I1（基电位末），bit1=I2（脉冲末）
    uint8_t ConsumeSampleFlags() {
        uint8_t f = sampleFlags;
        sampleFlags = 0;
        return f;
    }

    // 兼容旧接口：任一采样点触发即返回 true（会清除全部标记）
    bool CheckSamplingPoint() { return ConsumeSampleFlags() != 0; }

    bool IsRunning() const { return state != DPV_State::IDLE; }
};
