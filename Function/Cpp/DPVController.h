#pragma once
#include "stm32f10x.h"
#include "DACManager.h" // 复用其中的 CV_VoltParams 等定义

// DPV 运行状态
enum class DPV_State {
    IDLE,
    WAIT_BASE_TIME,   // 处于基电位，正在倒计时
    WAIT_PULSE_TIME   // 处于脉冲电位，正在倒计时
};

// DPV 参数结构体
struct DPV_Params {
    float startVolt = -0.5f;
    float endVolt = 0.5f;
    float stepVolt = 0.005f;   // 阶梯增量
    float pulseAmp = 0.05f;    // 脉冲幅度
    uint16_t pulseWidthMs = 50;    // 脉冲宽 (ms)
    uint16_t pulsePeriodMs = 200;  // 脉冲周期 (ms)
    
    // 构造函数...
};

class DPVController {
private:
    static DPVController* instance;
    
    DPV_Params params;
    DPV_State state = DPV_State::IDLE;

    // 硬件相关
    TIM_TypeDef* timer;   // 用于 DPV 计时的定时器 (如 TIM4)
    uint16_t dacChannel;  // DAC_Channel_1 或 2

    // 运行时变量 (全部使用 DAC 码值/整数，避免 ISR 浮点运算)
    int16_t codeStart;
    int16_t codeEnd;
    int16_t codeStep;     // 阶梯 DAC 增量
    int16_t codePulse;    // 脉冲 DAC 增量
    
    int16_t currentBaseCode; // 当前基准电压 Code
    
    // 采样结果缓存
    uint16_t adcRaw_Base;  // 脉冲前采样
    uint16_t adcRaw_Pulse; // 脉冲末采样
    
    // 结果数据 (用于发送)
    volatile bool dataReady;
    uint16_t lastResultDiff; // I_pulse - I_base (绝对值或处理后)
    uint16_t lastResultCode; // 对应的电压 Code

    // 私有函数：转换电压
    int16_t VoltToCode(float v);

    DPVController();

public:
    static DPVController* getInstance();

    // 初始化参数
    void Init(TIM_TypeDef* tim, uint16_t dac_chan);

    // 设置扫描参数
    void SetParams(const DPV_Params& p);

    // 控制接口
    void Start();
    void Stop();
    void Pause();  // 暂停只是停止定时器
    void Resume(); 

    // 状态查询
    bool IsRunning() const { return state != DPV_State::IDLE; }
    bool IsDataReady() { 
        if(dataReady) { dataReady = false; return true; } 
        return false; 
    }
    
    // 获取最新计算结果 (给 main 发送用)
    void GetLastResult(uint16_t& diff, uint16_t& voltCode) {
        diff = lastResultDiff;
        voltCode = lastResultCode;
    }

    // === 核心：中断服务函数 ===
    // 需要在 stm32f10x_it.c 的 TIMx_IRQHandler 中调用
    void ISR_Handler();
};