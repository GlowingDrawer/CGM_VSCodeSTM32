#pragma once
#include "stm32f10x.h"
#include "DPVController.h"
#include <algorithm> // std::swap

namespace NS_DAC {

    // 运行模式
    enum class GenMode { IDLE, CV_SCAN, DPV_PULSE, CONSTANT };
    enum class ScanDIR : uint8_t { FORWARD, REVERSE };

    // CV 参数定义（相对电位 + 中点偏置 voltOffset）
    struct CV_VoltParams {
        float highVolt, lowVolt, voltOffset; // voltOffset=中点偏置（默认 1.65V）
        CV_VoltParams(float h=0.8f, float l=-0.8f, float off=1.5f)
            : highVolt(h), lowVolt(l), voltOffset(off) {
            if (highVolt < lowVolt) std::swap(highVolt, lowVolt);
        }
    };

    // CV 扫描参数
    struct CV_Params {
        float duration;     // 每步持续时间（s）
        float rate;         // 扫描速率（V/s）
        ScanDIR dir;
        // ADCManager 使用的 initVal（码值）
        uint16_t initVal = 1.5 * 1240.91f;

        CV_Params(float d=0.05f, float r=0.05f, ScanDIR s=ScanDIR::FORWARD)
            : duration(d), rate(r), dir(s) {}
    };

    // CV 控制器（码值三角波）
    class CV_Controller {
    private:
        float currentVal = 2048.0f;
        float stepVal = 0.0f;
        uint16_t maxVal=4095, minVal=0;

        // ADCManager 需要地址稳定的缓存
        volatile uint16_t valBuf = 2048;

    public:
        CV_Params cvParams;

        void Init(const CV_VoltParams& v, const CV_Params& c);
        void ResetToInit();
        void UpdateCurrentVal();

        uint16_t GetValToSend() const { return (uint16_t)valBuf; }

        // 兼容 ADCManager 的接口
        const CV_Params& GetCvParams() const { return cvParams; }
        const uint16_t* GetBufferPtr() const { return (const uint16_t*)&valBuf; }
    };

    // 波形数据管理器：统一输出 unifiedValToSend
    class WaveDataManager {
    private:
        GenMode currentMode = GenMode::IDLE;

        // 对外统一的 DMA/手写 DAC 数据源
        volatile uint16_t unifiedValToSend = 2048;

        // DPV 采样标记（bit0=I1, bit1=I2），由主循环读取后清零
        volatile uint8_t dpvSampleFlags = 0;

        // 子控制器
        CV_Controller cvCtrl;
        DPVController dpvCtrl;

        // 常量输出
        uint16_t constantVal = 2048;

    public:
        void SetupCV(const CV_VoltParams& v, const CV_Params& c);
        void SetupDPV(const DPV_Params& p);
        void SetupConstant(uint16_t val);

        void SwitchMode(GenMode mode);
        GenMode GetMode() const { return currentMode; }

        // 核心更新函数（由定时器中断调用）
        // 返回：unifiedValToSend 是否发生变化（用于非 DMA 模式手写 DAC）
        bool UpdateNextStep();

        // 数据获取接口
        volatile uint16_t* GetDMAAddr() { return &unifiedValToSend; }
        uint16_t GetCurrentData() const { return unifiedValToSend; }

        // DPV 采样标记读取（读后清零）
        uint8_t ConsumeDpvSampleFlags() {
            uint8_t f = dpvSampleFlags;
            dpvSampleFlags = 0;
            return f;
        }

        // 获取子控制器
        CV_Controller& GetCV() { return cvCtrl; }
        DPVController& GetDPV() { return dpvCtrl; }
    };

} // namespace NS_DAC
