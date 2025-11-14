#ifndef DPV_CONTROLLER_H
#define DPV_CONTROLLER_H

#include "stm32f10x.h"
#include <cstdint>

// 定义最大步数(可根据实际需求调整)
#define MAX_DPV_STEPS 1000

class DPVController {
private:
    // 单例实例
    static DPVController* instance;
    
    // DPV参数结构体
    struct DPVParameters {
        float initialVoltage;    // 初始电压 (V)
        float finalVoltage;      // 终电压 (V)
        float stepPotential;     // 步长电压 (V)
        float pulseAmplitude;    // 脉冲幅度 (V)
        uint32_t pulseWidth;     // 脉冲宽度 (ms)
        uint32_t baseWidth;      // 基线宽度 (ms)
        uint32_t stepNumber;     // 实际步数
    } params;
    
    // 查表法所需的电压-时间结构
    struct TableEntry {
        uint16_t dacValue;       // DAC输出值
        uint32_t duration;       // 该电压持续时间(ms)
    };
    
    // DPV查表数据
    TableEntry dpvTable[MAX_DPV_STEPS * 2];  // 每个步骤包含2个条目
    uint32_t tableSize;                      // 实际表大小
    
    // 私有构造函数，防止外部实例化
    DPVController();
    
    // 防止拷贝和赋值
    DPVController(const DPVController&) = delete;
    DPVController& operator=(const DPVController&) = delete;
    
    // 将电压值转换为DAC输出值 (12位DAC)
    uint16_t voltageToDac(float voltage);
    
    // 毫秒级延迟
    void delayMs(uint32_t ms);

public:
    // 获取单例实例
    static DPVController* getInstance();
    
    // 初始化DAC硬件
    void initDac();
    
    // 生成DPV查表数据
    void generateTable(float initialV, float finalV, float stepV, 
                      float pulseAmp, uint32_t pulseW, uint32_t baseW);
    
    // 执行DPV扫描
    void runScan();
    
    // 获取当前查表大小(用于调试)
    uint32_t getTableSize() const { return tableSize; }
    
    // 获取当前步数(用于调试)
    uint32_t getStepCount() const { return params.stepNumber; }
};

#endif // DPV_CONTROLLER_H
