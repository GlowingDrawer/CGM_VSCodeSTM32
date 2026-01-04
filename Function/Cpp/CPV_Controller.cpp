#include "CPV_Controller.h"
#include <math.h>

namespace NS_DAC {

    const float CPV_Controller::dacStepsPerVolt = 1240.91f;

    static inline uint16_t SafeTicksFromMs(float tick_s, uint16_t ms) {
        if (tick_s <= 0.0f) return 1;
        float tick_ms = tick_s * 1000.0f;
        if (tick_ms < 0.001f) tick_ms = 0.001f;
        int ticks = (int)floorf((ms / tick_ms) + 0.5f);
        if (ticks < 1) ticks = 1;
        return (uint16_t)ticks;
    }

    static inline uint16_t ClampTick(uint16_t t, uint16_t maxExclusive) {
        if (maxExclusive == 0) return 0;
        if (t >= maxExclusive) return (uint16_t)(maxExclusive - 1u);
        return t;
    }

    void CPV_Controller::ResetParam(const CV_VoltParams& voltParams,
                                    const CV_Params& cvParams,
                                    const CPV_MethodParams& methodParams) {
        // tick 周期
        scanDuration_s_ = cvParams.scanDuration;
        adcIndex_ = methodParams.adcDmaIndex;

        // 电位范围 -> 12bit code（复用 CV 的 offset 逻辑）
        float vMax = voltParams.highVolt + voltParams.voltOffset;
        float vMin = voltParams.lowVolt  + voltParams.voltOffset;
        if (vMax < vMin) {
            float tmp = vMax; vMax = vMin; vMin = tmp;
        }
        maxVal12_ = Clamp12_((int32_t)lrintf(vMax * dacStepsPerVolt));
        minVal12_ = Clamp12_((int32_t)lrintf(vMin * dacStepsPerVolt));

        // 步进与脉冲（mV -> V -> code）
        float stepV = methodParams.step_mV * 0.001f;
        float pulseV = methodParams.pulseAmp_mV * 0.001f;

        int32_t stepCode = (int32_t)lrintf(stepV * dacStepsPerVolt);
        if (stepCode < 1) stepCode = 1;
        stepCode12_ = (uint16_t)stepCode;

        int32_t pulseCode = (int32_t)lrintf(pulseV * dacStepsPerVolt);
        if (pulseCode < 1) pulseCode = 1;
        pulseAmpCode12_ = (uint16_t)pulseCode;

        // 每阶段 tick 数
        baseTicks_  = SafeTicksFromMs(scanDuration_s_, methodParams.baseHold_ms);
        pulseTicks_ = SafeTicksFromMs(scanDuration_s_, methodParams.pulseWidth_ms);

        // 采样 tick（默认末端采样）
        uint16_t baseSampleMs  = (methodParams.baseSample_ms  == 0) ? (uint16_t)(methodParams.baseHold_ms  ? (methodParams.baseHold_ms - 1) : 0) : methodParams.baseSample_ms;
        uint16_t pulseSampleMs = (methodParams.pulseSample_ms == 0) ? (uint16_t)(methodParams.pulseWidth_ms ? (methodParams.pulseWidth_ms - 1) : 0) : methodParams.pulseSample_ms;

        baseSampleTick_  = ClampTick(SafeTicksFromMs(scanDuration_s_, baseSampleMs),  baseTicks_);
        pulseSampleTick_ = ClampTick(SafeTicksFromMs(scanDuration_s_, pulseSampleMs), pulseTicks_);

        // 扫描方向：复用 CV 的 ScanDIR
        dir_ = (cvParams.scanDIR == ScanDIR::FORWARD) ? +1 : -1;

        // 起始 base code：正向从 min 起，反向从 max 起
        baseCode12_ = (dir_ > 0) ? minVal12_ : maxVal12_;
        outCode12_  = baseCode12_;

        // 状态清零
        phase_ = Phase::BASE;
        tickInPhase_ = 0;
        cyclesTarget_ = methodParams.cycles;
        cyclesDone_ = 0;
        stepSeq_ = 0;
        haveBase_ = false;
        overflow_ = false;

        // 清空缓冲
        head_ = tail_ = 0;

        inited_ = true;
        running_ = true;
    }

    void CPV_Controller::PushPointIsr_(const CPV_Point& p) {
        uint16_t next = Next_(head_);
        if (next == tail_) {
            // 缓冲满：丢最旧点
            tail_ = Next_(tail_);
            overflow_ = true;
        }
        buf_[head_] = p;
        head_ = next;
    }

    bool CPV_Controller::PopPoint(CPV_Point& out) {
        uint16_t t = tail_;
        if (t == head_) return false;
        out = buf_[t];
        tail_ = Next_(t);
        return true;
    }

    void CPV_Controller::AdvanceBase_() {
        // 单次推进：到顶点换向；回到起点完成一圈
        if (dir_ > 0) {
            if ((uint32_t)baseCode12_ + (uint32_t)stepCode12_ >= (uint32_t)maxVal12_) {
                baseCode12_ = maxVal12_;
                dir_ = -1;
            } else {
                baseCode12_ = (uint16_t)(baseCode12_ + stepCode12_);
            }
        } else {
            if (baseCode12_ <= (uint16_t)(minVal12_ + stepCode12_)) {
                baseCode12_ = minVal12_;

                // 完成“正向+反向”一圈（回到起点）
                if (cyclesTarget_ != 0) {
                    cyclesDone_++;
                    if (cyclesDone_ >= cyclesTarget_) {
                        running_ = false;
                        outCode12_ = baseCode12_;
                        return;
                    }
                }
                dir_ = +1;
            } else {
                baseCode12_ = (uint16_t)(baseCode12_ - stepCode12_);
            }
        }
    }

    uint16_t CPV_Controller::IsrUpdate(uint16_t adcRaw) {
        if (!inited_) return outCode12_;
        if (!running_) return outCode12_;

        // 1) 在当前阶段的指定 tick 采样（采样发生在“输出更新前”的时刻：更贴近该阶段末端稳定值）
        if (phase_ == Phase::BASE) {
            if (tickInPhase_ == baseSampleTick_) {
                lastBaseRaw_ = adcRaw;
                haveBase_ = true;
            }
        } else { // PULSE
            if (tickInPhase_ == pulseSampleTick_) {
                lastPulseRaw_ = adcRaw;

                // 推入一个点（以本台阶的 base/pulse 为准）
                CPV_Point p;
                p.baseCode12 = baseCode12_;
                // 脉冲电位（钳制到范围内）
                p.pulseCode12 = ClampRange_((int32_t)baseCode12_ + (int32_t)pulseAmpCode12_);
                p.iBaseAdcRaw = lastBaseRaw_;
                p.iPulseAdcRaw = lastPulseRaw_;
                p.idAdcRaw = (int32_t)p.iPulseAdcRaw - (int32_t)p.iBaseAdcRaw;
                p.stepSeq = stepSeq_;
                p.dir = dir_;

                // 只有在拿到一次 baseline 后才认为点有效；否则仍推入但 id 可能无意义
                PushPointIsr_(p);
            }
        }

        // 2) 计算下一时隙应输出的 DAC 值（阶段切换时更新 outCode12_）
        // outCode12_ 表示“本次 IRQ 返回后将保持”的输出值
        if (phase_ == Phase::BASE) {
            outCode12_ = baseCode12_;

            // BASE 阶段计数推进
            tickInPhase_++;
            if (tickInPhase_ >= baseTicks_) {
                // 进入 PULSE 阶段
                phase_ = Phase::PULSE;
                tickInPhase_ = 0;
                outCode12_ = ClampRange_((int32_t)baseCode12_ + (int32_t)pulseAmpCode12_);
            }
        } else { // PULSE
            outCode12_ = ClampRange_((int32_t)baseCode12_ + (int32_t)pulseAmpCode12_);

            tickInPhase_++;
            if (tickInPhase_ >= pulseTicks_) {
                // 脉冲结束：回到 BASE 并推进到下一台阶
                phase_ = Phase::BASE;
                tickInPhase_ = 0;

                stepSeq_++;
                AdvanceBase_();

                outCode12_ = baseCode12_;
                haveBase_ = false; // 下一台阶重新 baseline
            }
        }

        return outCode12_;
    }

} // namespace NS_DAC
