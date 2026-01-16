#pragma once
#include "stm32f10x.h"
#include "WaveDataManager.h"
#include <IRQnManage.h>

namespace NS_DAC {

    enum class DAC_Channel : uint32_t { CH1 = DAC_Channel_1, CH2 = DAC_Channel_2 };

    // 运行模式定义 (对外接口)
    enum class RunMode { CV, DPV, IT };

    // 硬件配置参数包
    // tim 允许为 nullptr：表示该通道不依赖定时器触发/中断（例如偏置常量输出）
    struct HW_Config {
        DAC_Channel dacChan;
        TIM_TypeDef* tim;
    };

    // 单个 DAC 通道控制器
    class DAC_ChanController {
    private:
        // 实际解析后的硬件资源
        struct ResolvedHW {
            DAC_Channel dacChan;
            TIM_TypeDef* tim;
            DMA_Channel_TypeDef* dmaChan;
            uint32_t dacTrigger;
            uint16_t timDmaSrc;
        } hw;

        WaveDataManager dataMgr;

        bool isPaused = false;
        bool useDMA = true;      // 当前模式是否使用 DMA

        // 自动查找硬件映射
        void ResolveHardware(const HW_Config& cfg);

        // 底层驱动
        void SetupGPIO();
        void SetupDAC();
        void SetupDMA();
        void SetupTIM(float period);

    public:
        explicit DAC_ChanController(const HW_Config& cfg);

        // 初始化配置
        void InitAsCV(const CV_VoltParams& v, const CV_Params& c);
        void InitAsDPV(const DPV_Params& d);
        void InitAsConstant(uint16_t val);

        // 控制接口
        void Start();
        void Stop();
        void Pause();
        void Resume();

        // 由 IRQnManage 分发调用：此处不要再读/清 TIM 标志位
        void TIM_IRQHandler();

        WaveDataManager& GetDataMgr() { return dataMgr; }
    };

    // 全局实例容器
    class DAC_Manager {
    public:
        static DAC_ChanController Chan_Scan;     // 扫描通道
        static DAC_ChanController Chan_Constant; // 偏置通道
        static void Init();
    };

    // 系统总控 (持有参数缓存)
    class SystemController {
    private:
        RunMode currentMode = RunMode::CV;
        bool isRunning = false;
        bool isPaused = false;
        uint32_t updateTimes = 0;

        // 参数缓存 (用于 Start 时应用)
        CV_VoltParams cachedCV_Volt;
        CV_Params cachedCV_Params;
        DPV_Params cachedDPV_Params;
        uint16_t cachedConstantVal = 2048;

        SystemController() = default;

    public:
        static SystemController& GetInstance();

        void SetMode(RunMode mode);
        void SetCVParams(const CV_VoltParams& v, const CV_Params& c);
        void SetDPVParams(const DPV_Params& d);
        void SetConstantVal(uint16_t val);

        RunMode GetMode() const { return currentMode; }

        void Start();
        void Stop();
        void Pause();
        void Resume();

        void UpdateTick() { if (isRunning && !isPaused) updateTimes++; }
        uint32_t GetTickCount() const { return updateTimes; }
        void ClearTick() { updateTimes = 0; }
    };

    // 兼容旧上位机：输出当前通道的 12bit Code（引用）
    const uint16_t& GetCvValToSendRef();

    // DPV 采样标记（bit0=I1, bit1=I2），主循环读取后清零
    uint8_t ConsumeDpvSampleFlags();

} // namespace NS_DAC
