#include "DPVController.h"
#include "DacMath.h"
#include <cmath>

void DPVController::SetParams(const DPV_Params& p) {
    params = p;

    // 参数保护：宽度必须 < 周期
    if (params.pulseWidthMs >= params.pulsePeriodMs) {
        params.pulseWidthMs = (params.pulsePeriodMs > 1) ? (params.pulsePeriodMs - 1) : 1;
    }
    if (params.pulseWidthMs == 0) params.pulseWidthMs = 1;
    if (params.pulsePeriodMs == 0) params.pulsePeriodMs = 1;

    pulseWidthMs = params.pulseWidthMs;
    baseTimeMs   = (uint16_t)(params.pulsePeriodMs - params.pulseWidthMs);
    if (baseTimeMs == 0) baseTimeMs = 1;

    sampleLeadMs = params.sampleLeadMs;
    if (sampleLeadMs == 0) sampleLeadMs = 1;
    // 采样提前量不能超过阶段长度
    if (sampleLeadMs >= baseTimeMs)   sampleLeadMs = (baseTimeMs   > 1) ? (baseTimeMs   - 1) : 1;
    if (sampleLeadMs >= pulseWidthMs) sampleLeadMs = (pulseWidthMs > 1) ? (pulseWidthMs - 1) : 1;

    // 电位->码值（相对电位 + midVolt）
    currentBaseCode = (int32_t)DacMath::VoltToCode(params.startVolt, params.midVolt);
    codeEnd         = (int32_t)DacMath::VoltToCode(params.endVolt,   params.midVolt);

    // step：按 end-start 决定方向，按 |stepVolt| 决定幅值
    const float vdir = params.endVolt - params.startVolt;
    const int dir = (vdir >= 0.0f) ? 1 : -1;

    const float stepMag = std::fabs(params.stepVolt);
    int32_t step = DacMath::DeltaVoltToCodeSigned(stepMag);
    if (step == 0) step = 1;
    codeStep = dir * (step > 0 ? step : -step);

    // pulse：允许为负
    codePulse = DacMath::DeltaVoltToCodeSigned(params.pulseAmp);
}

void DPVController::Start() {
    sampleFlags = 0;
    timerCount = 0;

    state = DPV_State::WAIT_BASE_TIME;
    currentOutputCode = DacMath::Clamp12(currentBaseCode);
}

void DPVController::Stop() {
    state = DPV_State::IDLE;
}

bool DPVController::StepTick() {
    if (state == DPV_State::IDLE) return false;

    bool dacChanged = false;

    if (state == DPV_State::WAIT_BASE_TIME) {
        // 采样点：基电位阶段末尾（避开跳变）
        if (baseTimeMs > sampleLeadMs && timerCount == (uint16_t)(baseTimeMs - sampleLeadMs)) {
            sampleFlags |= 0x01; // I1
        }

        timerCount++;
        if (timerCount >= baseTimeMs) {
            timerCount = 0;
            state = DPV_State::WAIT_PULSE_TIME;

            // 进入脉冲阶段：输出 base + pulse
            const int32_t pulseCode = currentBaseCode + codePulse;
            currentOutputCode = DacMath::Clamp12(pulseCode);
            dacChanged = true;
        }
        return dacChanged;
    }

    // WAIT_PULSE_TIME
    if (pulseWidthMs > sampleLeadMs && timerCount == (uint16_t)(pulseWidthMs - sampleLeadMs)) {
        sampleFlags |= 0x02; // I2
    }

    timerCount++;
    if (timerCount >= pulseWidthMs) {
        timerCount = 0;

        // 脉冲结束：若当前 base 已到终点，则回到 base 并停止
        if (currentBaseCode == codeEnd) {
            currentOutputCode = DacMath::Clamp12(currentBaseCode);
            Stop();
            return true;
        }

        // 进入下一阶梯 base（直接从脉冲电位跳到下一 base）
        int32_t nextBase = currentBaseCode + codeStep;
        if (codeStep > 0 && nextBase > codeEnd) nextBase = codeEnd;
        if (codeStep < 0 && nextBase < codeEnd) nextBase = codeEnd;

        currentBaseCode = nextBase;
        currentOutputCode = DacMath::Clamp12(currentBaseCode);

        state = DPV_State::WAIT_BASE_TIME;
        dacChanged = true;
    }

    return dacChanged;
}
