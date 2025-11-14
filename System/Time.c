#include "stm32f10x.h"                  // Device header
#include "Time.h"

#define DAC_BUFFER_SIZE 256  // 数据缓冲区大小
uint16_t dacBuffer[DAC_BUFFER_SIZE];  // DAC数据数组


void TIM1_Init(void) {

    // 1. 使能时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);   // 触发定时器
    
    TIM_TimeBaseInitTypeDef TIM_InitStruct;
    // 2. 配置定时器（设置触发频率）
    TIM_InitStruct.TIM_Period = 100 - 1;         // 自动重装载值
    TIM_InitStruct.TIM_Prescaler = 72 - 1;       // 分频系数（72MHz / 72 = 1MHz）
    TIM_InitStruct.TIM_ClockDivision = 0;
    TIM_InitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_InitStruct);
    TIM_SelectOutputTrigger(TIM1, TIM_TRGOSource_Update);  // 触发信号源
    TIM_Cmd(TIM1, ENABLE);  // 启动定时器2
    
    
}

void TIM2_Init(void) {

    // 1. 使能时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);   // 触发定时器
    
    TIM_TimeBaseInitTypeDef TIM_InitStruct;
    // 2. 配置定时器（设置触发频率）
    TIM_InitStruct.TIM_Period = 100 - 1;         // 自动重装载值
    TIM_InitStruct.TIM_Prescaler = 72 - 1;       // 分频系数（72MHz / 72 = 1MHz）
    TIM_InitStruct.TIM_ClockDivision = 0;
    TIM_InitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_InitStruct);
    TIM_SelectOutputTrigger(TIM2, TIM_TRGOSource_Update);  // 触发信号源
    TIM_Cmd(TIM2, ENABLE);  // 启动定时器2
    
    
}

