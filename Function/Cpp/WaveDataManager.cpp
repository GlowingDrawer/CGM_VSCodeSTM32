#include "WaveDataManager.h"
#include "DacMath.h"
#include <cmath>

namespace NS_DAC {

    void CV_Controller::Init(const CV_VoltParams& v, const CV_Params& c) {
        cvParams = c;

        maxVal = DacMath::VoltToCode(v.highVolt, v.voltOffset);
        minVal = DacMath::VoltToCode(v.lowVolt,  v.voltOffset);

        // initVal：相对电位 0V 对应的码值
        cvParams.initVal = DacMath::VoltToCode(0.0f, v.voltOffset);

        currentVal = (float)cvParams.initVal;
        valBuf = cvParams.initVal;

        // step(code) = (code/V) * (V/s) * (s)
        stepVal = DacMath::STEP_PER_V * c.rate * c.duration;
        if (c.dir == ScanDIR::REVERSE) stepVal = -stepVal;
        if (stepVal == 0.0f) stepVal = (c.dir == ScanDIR::REVERSE) ? -1.0f : 1.0f;
    }

    void CV_Controller::ResetToInit() {
        currentVal = (float)cvParams.initVal;
        valBuf = cvParams.initVal;
    }

    void CV_Controller::UpdateCurrentVal() {
        currentVal += stepVal;

        // 三角波回弹逻辑
        if (stepVal > 0.0f) {
            if (currentVal >= maxVal) {
                currentVal = (float)maxVal;
                stepVal = -std::fabs(stepVal);
            }
        } else {
            if (currentVal <= minVal) {
                currentVal = (float)minVal;
                stepVal = std::fabs(stepVal);
            }
        }

        valBuf = (uint16_t)currentVal;
    }

    void WaveDataManager::SetupCV(const CV_VoltParams& v, const CV_Params& c) {
        cvCtrl.Init(v, c);
    }

    void WaveDataManager::SetupDPV(const DPV_Params& p) {
        dpvCtrl.SetParams(p);
    }

    void WaveDataManager::SetupConstant(uint16_t val) {
        constantVal = val;
    }

    void WaveDataManager::SwitchMode(GenMode mode) {
        currentMode = mode;
        dpvSampleFlags = 0;

        switch (mode) {
            case GenMode::CV_SCAN:
                cvCtrl.ResetToInit();
                unifiedValToSend = cvCtrl.GetValToSend();
                break;

            case GenMode::DPV_PULSE:
                dpvCtrl.Start();
                unifiedValToSend = dpvCtrl.GetCurrentCode();
                break;

            case GenMode::CONSTANT:
                unifiedValToSend = constantVal;
                break;

            default:
                break;
        }
    }

    bool WaveDataManager::UpdateNextStep() {
        bool updated = false;

        switch (currentMode) {
            case GenMode::CV_SCAN:
                cvCtrl.UpdateCurrentVal();
                unifiedValToSend = cvCtrl.GetValToSend();
                updated = true;
                break;

            case GenMode::DPV_PULSE: {
                const bool changed = dpvCtrl.StepTick();
                unifiedValToSend = dpvCtrl.GetCurrentCode();
                updated = changed;

                // 汇总采样标记（bit0/bit1）
                dpvSampleFlags |= dpvCtrl.ConsumeSampleFlags();
                break;
            }

            case GenMode::CONSTANT:
                if (unifiedValToSend != constantVal) {
                    unifiedValToSend = constantVal;
                    updated = true;
                }
                break;

            default:
                break;
        }

        return updated;
    }

} // namespace NS_DAC
