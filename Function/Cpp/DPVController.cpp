#include "DPVController.h"
#include <cmath>



// 初始化静态实例指针
DPVController* DPVController::instance = nullptr;

// 私有构造函数
DPVController::DPVController() : tableSize(0) {
    // 初始化参数
    params.initialVoltage = 0.0f;
    params.finalVoltage = 0.0f;
    params.stepPotential = 0.0f;
    params.pulseAmplitude = 0.0f;
    params.pulseWidth = 0;
    params.baseWidth = 0;
    params.stepNumber = 0;
}

// 获取单例实例
DPVController* DPVController::getInstance() {
    if (instance == nullptr) {
        instance = new DPVController();
    }
    return instance;
}

// 初始化DAC硬件
void DPVController::initDac() {
    // 使能DAC和GPIOA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
    
    // 配置PA4为DAC输出 (模拟输入)
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 配置DAC
    DAC_InitTypeDef DAC_InitStructure;
    DAC_InitStructure.DAC_Trigger = DAC_Trigger_None;
    DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
    DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;
    DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
    DAC_Init(DAC_Channel_1, &DAC_InitStructure);
    
    // 使能DAC通道1
    DAC_Cmd(DAC_Channel_1, ENABLE);
}

// 将电压值转换为DAC输出值 (12位DAC)
uint16_t DPVController::voltageToDac(float voltage) {
    // 参考电压为3.3V
    float dacValue = (voltage / 3.3f) * 4095.0f;
    
    // 限制在有效范围内
    if (dacValue < 0) return 0;
    if (dacValue > 4095) return 4095;
    
    return static_cast<uint16_t>(dacValue);
}

// 毫秒级延迟
void DPVController::delayMs(uint32_t ms) {
    // 简单的延迟实现，可根据需要替换为更精确的定时器延迟
    for (uint32_t i = 0; i < ms; i++) {
        for (uint32_t j = 0; j < 72000; j++) {
            __NOP(); // 空操作，仅用于消耗时间
        }
    }
}

// 生成DPV查表数据
void DPVController::generateTable(float initialV, float finalV, float stepV, 
                                 float pulseAmp, uint32_t pulseW, uint32_t baseW) {
    // 保存参数
    params.initialVoltage = initialV;
    params.finalVoltage = finalV;
    params.stepPotential = stepV;
    params.pulseAmplitude = pulseAmp;
    params.pulseWidth = pulseW;
    params.baseWidth = baseW;
    
    // 计算步数
    if (stepV != 0) {
        params.stepNumber = static_cast<uint32_t>(fabs((finalV - initialV) / stepV)) + 1;
        // 检查是否超过最大步数限制
        if (params.stepNumber > MAX_DPV_STEPS) {
            params.stepNumber = MAX_DPV_STEPS;
        }
    } else {
        params.stepNumber = 1;
    }
    
    // 生成查表数据
    tableSize = 0;
    float currentVoltage = initialV;
    
    for (uint32_t i = 0; i < params.stepNumber; i++) {
        // 添加基线电压条目
        dpvTable[tableSize].dacValue = voltageToDac(currentVoltage);
        dpvTable[tableSize].duration = baseW;
        tableSize++;
        
        // 添加脉冲电压条目
        dpvTable[tableSize].dacValue = voltageToDac(currentVoltage + pulseAmp);
        dpvTable[tableSize].duration = pulseW;
        tableSize++;
        
        // 更新到下一步电压
        if (finalV > initialV) {
            currentVoltage += stepV;
            if (currentVoltage > finalV) break;
        } else {
            currentVoltage -= stepV;
            if (currentVoltage < finalV) break;
        }
    }
}

// 执行DPV扫描
void DPVController::runScan() {
    // 遍历查表并输出电压
    for (uint32_t i = 0; i < tableSize; i++) {
        // 输出当前电压
        DAC_SetChannel1Data(DAC_Align_12b_R, dpvTable[i].dacValue);
        
        // 在脉冲阶段结束前适合进行电流采样
        // 例如：if (i % 2 == 1) { triggerCurrentSampling(); }
        
        // 保持当前电压指定时间
        delayMs(dpvTable[i].duration);
    }
    
    // 扫描结束后输出0V
    DAC_SetChannel1Data(DAC_Align_12b_R, voltageToDac(0.0f));
}
