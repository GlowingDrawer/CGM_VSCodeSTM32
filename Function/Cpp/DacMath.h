#pragma once
#include <stdint.h>

namespace DacMath {

// 12-bit DAC, Vref assumed 3.3V
constexpr float VREF_V      = 3.3f;
constexpr float MAX_CODE_F  = 4095.0f;                 // 0..4095
constexpr float STEP_PER_V  = (MAX_CODE_F / VREF_V);   // ~1240.909

inline int32_t RoundToI32(float x) {
    return (x >= 0.0f) ? (int32_t)(x + 0.5f) : (int32_t)(x - 0.5f);
}

inline uint16_t Clamp12(int32_t code) {
    if (code < 0) return 0;
    if (code > 4095) return 4095;
    return (uint16_t)code;
}

// Relative electrode potential (v_rel) mapped to DAC code around midVolt.
// Example: midVolt=1.65V -> v_rel=0 maps to ~2048.
inline uint16_t VoltToCode(float v_rel, float midVolt = 1.65f) {
    const float v_abs = v_rel + midVolt;
    return Clamp12(RoundToI32(v_abs * STEP_PER_V));
}

// Voltage increment mapped to signed DAC code increment.
inline int32_t DeltaVoltToCodeSigned(float dv) {
    return RoundToI32(dv * STEP_PER_V);
}

} // namespace DacMath
