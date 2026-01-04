#pragma once
#include "stm32f10x.h"
#include <InitArg.h>
#include <array>
#include <stdint.h>

namespace NS_DAC {

    // CPV/DPV: 每个台阶点叠加一个正脉冲；建议在脉冲前与脉冲末端各采一次电流
    // 注意：本控制器默认以 DAC 的 TIM Update IRQ 为“时间基准”，每次 IRQ 代表一个 tick，
    // tick 周期由 DAC_ChanController::scanDuration 决定。

    struct CPV_MethodParams {
        // 台阶电位步进（mV）
        float step_mV = 5.0f;

        // 正脉冲幅值（mV，叠加在台阶基线电位上）
        float pulseAmp_mV = 25.0f;

        // 基线保持时间（ms）：每个台阶在输出基线电位时的总保持时间（含采样）
        uint16_t baseHold_ms = 50;

        // 脉冲宽度（ms）：输出 base + pulseAmp 的保持时间（含采样）
        uint16_t pulseWidth_ms = 50;

        // 在“基线阶段”内第多少毫秒采样（ms，相对该阶段起点），默认采样在阶段末端
        // 若设为 0 或超出 baseHold_ms，则内部会自动钳制到 baseHold_ms-1
        uint16_t baseSample_ms = 0;

        // 在“脉冲阶段”内第多少毫秒采样（ms，相对该阶段起点），默认采样在阶段末端
        uint16_t pulseSample_ms = 0;

        // 采样 ADC DMA 缓冲的通道索引（你的工程里 main 用了 0/1/2 三路，默认 0）
        uint8_t adcDmaIndex = 0;

        // 循环次数：1=正向到顶点再反向回起点算 1 次；0=无限循环
        uint16_t cycles = 1;
    };

    struct CPV_Point {
        // 基线电位（12bit DAC code）
        uint16_t baseCode12 = 0;
        // 脉冲电位（12bit DAC code）
        uint16_t pulseCode12 = 0;

        // 采样到的 ADC 原始值（直接取 DMA 缓冲）
        uint16_t iBaseAdcRaw = 0;
        uint16_t iPulseAdcRaw = 0;

        // 差分（脉冲末 - 脉冲前），仍是 ADC raw 差值（如需换算成电流，由上位机/主循环统一做）
        int32_t  idAdcRaw = 0;

        // 步进序号（每前进一步+1；反向也+1，方便上位机按时间排序）
        uint32_t stepSeq = 0;

        // 当前扫描方向：+1 正向；-1 反向
        int8_t dir = +1;
    };

    class CPV_Controller {
    public:
        // 与 CV_Controller 一致：DAC 每伏特对应的步长（12bit，经验常数）
        static const float dacStepsPerVolt;

        CPV_Controller() = default;

        // 使用 CV_VoltParams / CV_Params 复用你现有的电位范围、偏置、tick 周期、扫描方向
        // 说明：voltParams.highVolt/lowVolt 是“相对参比”的目标范围；voltOffset 用于平移到 DAC 正区间。
        void ResetParam(const CV_VoltParams& voltParams,
                        const CV_Params& cvParams,
                        const CPV_MethodParams& methodParams);

        // 启停（通常由 SystemController::Start/暂停/恢复来控制定时器，本标志用于逻辑停止）
        void Start() { running_ = true; }
        void Stop()  { running_ = false; }

        bool IsRunning() const { return running_; }

        // ISR 中调用：传入当前 ADC raw（建议从 GetADCDmaBufRef()[adcIndex] 读），返回本 tick 应输出的 DAC code
        // 注意：该函数内部会在“采样点”消费 adcRaw，并在脉冲采样完成后推入一个 CPV_Point 到环形缓冲。
        uint16_t IsrUpdate(uint16_t adcRaw);

        // 当前要输出的 DAC 值（12bit）
        uint16_t GetValToSend() const { return outCode12_; }
        const uint16_t* GetValToSendPtr() const { return &outCode12_; }
        const uint16_t& GetValToSendRef() const { return outCode12_; }

        // 取出一个点（主循环中调用）。若无数据返回 false。
        bool PopPoint(CPV_Point& out);

        // 只读配置（用于 DAC_ChanController::AssignValAboutWaveForm 获取 scanDuration 等）
        float GetScanDuration_s() const { return scanDuration_s_; }
        uint8_t GetAdcDmaIndex() const { return adcIndex_; }

        // 是否发生过数据覆盖（生产快于消费时会丢最旧点）
        bool HadOverflow() const { return overflow_; }
        void ClearOverflow() { overflow_ = false; }

    private:
        enum class Phase : uint8_t { BASE = 0, PULSE = 1 };

        // 环形缓冲：单生产者（ISR）/单消费者（main），不使用锁
        static constexpr uint16_t kBufSize = 128;

        inline uint16_t Next_(uint16_t i) const { return (uint16_t)((i + 1u) % kBufSize); }

        void PushPointIsr_(const CPV_Point& p);

        // 台阶推进与换向/计圈
        void AdvanceBase_();

        // 钳制到 DAC 12bit + 电位范围
        inline uint16_t Clamp12_(int32_t v) const {
            if (v < 0) return 0;
            if (v > 4095) return 4095;
            return (uint16_t)v;
        }
        inline uint16_t ClampRange_(int32_t v) const {
            if (v < (int32_t)minVal12_) return minVal12_;
            if (v > (int32_t)maxVal12_) return maxVal12_;
            return (uint16_t)v;
        }

    private:
        bool running_ = false;
        bool inited_ = false;

        // 复用 CV 的 tick 周期（秒）
        float scanDuration_s_ = 0.001f;

        // 电位范围（12bit）
        uint16_t maxVal12_ = 4095;
        uint16_t minVal12_ = 0;

        // 方法参数（转换后的 12bit 步进/脉冲）
        uint16_t stepCode12_ = 1;
        uint16_t pulseAmpCode12_ = 1;

        // 每阶段的 tick 数与采样 tick（均为“阶段内计数”，从 0 开始）
        uint16_t baseTicks_ = 1;
        uint16_t pulseTicks_ = 1;
        uint16_t baseSampleTick_ = 0;
        uint16_t pulseSampleTick_ = 0;

        // 当前状态
        Phase phase_ = Phase::BASE;
        uint16_t tickInPhase_ = 0;

        // 当前台阶基线
        uint16_t baseCode12_ = 2048;
        // 当前输出（可能是 base 或 base+pulse）
        uint16_t outCode12_ = 2048;

        // 扫描方向与圈数
        int8_t dir_ = +1;
        uint16_t cyclesTarget_ = 1;   // 0=无限
        uint16_t cyclesDone_ = 0;

        // 采样与点生成
        uint16_t lastBaseRaw_ = 0;
        uint16_t lastPulseRaw_ = 0;
        bool haveBase_ = false;

        uint32_t stepSeq_ = 0;

        // ADC 采样通道
        uint8_t adcIndex_ = 0;

        // 点缓冲
        std::array<CPV_Point, kBufSize> buf_{};
        volatile uint16_t head_ = 0;
        volatile uint16_t tail_ = 0;
        volatile bool overflow_ = false;
    };

} // namespace NS_DAC
