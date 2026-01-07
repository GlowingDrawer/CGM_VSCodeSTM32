#include "DPVController.h"
#include "ADCManager.h" // 获取 ADC 数据用

DPVController* DPVController::instance = nullptr;

DPVController* DPVController::getInstance() {
    if (instance == nullptr) instance = new DPVController();
    return instance;
}

DPVController::DPVController() {}

void DPVController::Init(TIM_TypeDef* tim, uint16_t dac_chan) {
    this->timer = tim;
    this->dacChannel = dac_chan;
    // 初始化定时器硬件 TIM_TimeBaseInit... 
    // 设定预分频使得计数器频率为 10kHz (0.1ms 一个 tick) 或 1kHz (1ms 一个 tick)
    // 建议设为 1kHz (1ms) 方便计算
}

// 复用 CV 的参数转换逻辑
int16_t DPVController::VoltToCode(float v) {
    // 假设您有 dacStepsPerVolt 和 offset，这里仅做示例
    // 实际请调用 NS_DAC::CV_Controller 里的转换逻辑
    const float k = 1240.9f; 
    const float offset = 2048.0f; 
    return (int16_t)(v * k + offset);
}

void DPVController::SetParams(const DPV_Params& p) {
    this->params = p;
    
    // 预计算 DAC 码值 (定点化)
    codeStart = VoltToCode(p.startVolt);
    codeEnd   = VoltToCode(p.endVolt);
    
    // 计算增量 (注意 stepVolt 是相对值)
    codeStep  = (int16_t)(p.stepVolt * 1240.9f);
    codePulse = (int16_t)(p.pulseAmp * 1240.9f);
}

void DPVController::Start() {
    currentBaseCode = codeStart;
    state = DPV_State::WAIT_BASE_TIME;
    
    // 1. 输出初始基准电压
    if (dacChannel == DAC_Channel_1) 
        DAC_SetChannel1Data(DAC_Align_12b_R, currentBaseCode);
    else 
        DAC_SetChannel2Data(DAC_Align_12b_R, currentBaseCode);
    DAC_SoftwareTriggerCmd(dacChannel, ENABLE);

    // 2. 设定定时器中断时间 = (周期 - 脉宽)
    // 假设 ARR 单位是 ms
    TIM_SetAutoreload(timer, params.pulsePeriodMs - params.pulseWidthMs);
    TIM_SetCounter(timer, 0);
    TIM_Cmd(timer, ENABLE);
    TIM_ITConfig(timer, TIM_IT_Update, ENABLE);
}

void DPVController::Stop() {
    TIM_Cmd(timer, DISABLE);
    state = DPV_State::IDLE;
}

// === 核心 ISR (替代 runScan) ===
void DPVController::ISR_Handler() {
    // 清除中断标志位由外部或在此处处理
    TIM_ClearITPendingBit(timer, TIM_IT_Update);

    if (state == DPV_State::WAIT_BASE_TIME) {
        // === 基电位阶段结束 -> 采样 I1 -> 输出脉冲 ===
        
        // 1. 采样 I1 (脉冲前基准电流)
        // 从 ADCManager 获取最新 DMA 数据
        auto& adcBuf = NS_DAC::GetADC().GetDmaBufferRef();
        adcRaw_Base = adcBuf[0]; // 假设通道0

        // 2. 输出脉冲电压
        uint16_t outVal = currentBaseCode + codePulse;
        if (dacChannel == DAC_Channel_1) DAC_SetChannel1Data(DAC_Align_12b_R, outVal);
        else DAC_SetChannel2Data(DAC_Align_12b_R, outVal);
        DAC_SoftwareTriggerCmd(dacChannel, ENABLE);

        // 3. 设定下一次中断时间 (脉宽)
        TIM_SetAutoreload(timer, params.pulseWidthMs);
        state = DPV_State::WAIT_PULSE_TIME;
    }
    else if (state == DPV_State::WAIT_PULSE_TIME) {
        // === 脉冲阶段结束 -> 采样 I2 -> 回到基电位并迈步 ===
        
        // 1. 采样 I2 (脉冲电流)
        auto& adcBuf = NS_DAC::GetADC().GetDmaBufferRef();
        adcRaw_Pulse = adcBuf[0];

        // 2. 准备数据发送
        // 简单的差分计算 (注意处理负数或溢出)
        if (adcRaw_Pulse > adcRaw_Base) lastResultDiff = adcRaw_Pulse - adcRaw_Base;
        else lastResultDiff = 0; // 或处理为负
        
        lastResultCode = currentBaseCode; // 记录当前的基准电压
        dataReady = true;

        // 3. 迈向下一步电压
        currentBaseCode += codeStep;

        // 4. 检查是否结束
        bool finished = (codeStep > 0) ? (currentBaseCode >= codeEnd) : (currentBaseCode <= codeEnd);
        
        if (finished) {
            Stop();
            // 可以设一个标志位通知 main 结束了
        } else {
            // 输出新的基准电压
            if (dacChannel == DAC_Channel_1) DAC_SetChannel1Data(DAC_Align_12b_R, currentBaseCode);
            else DAC_SetChannel2Data(DAC_Align_12b_R, currentBaseCode);
            DAC_SoftwareTriggerCmd(dacChannel, ENABLE);

            // 设定下一次中断时间 (基线时长)
            TIM_SetAutoreload(timer, params.pulsePeriodMs - params.pulseWidthMs);
            state = DPV_State::WAIT_BASE_TIME;
        }
    }
}