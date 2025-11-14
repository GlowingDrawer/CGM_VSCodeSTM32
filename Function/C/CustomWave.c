#include "stm32f10x.h"                  // Device header
#include "CustomWave.h"
#include "OLED.h"
#include "Delay.h"
#include "misc.h"
#include "math.h"


#define DAC_DHR12R1_Address      0x4000740B
#define DAC_DHR12R2_Address      0x40007420
#define CV_BUFFER_SIZE 24

const float cv_step = 1240.91f;
const float cv_max_volt = 1.8f, cv_min_volt = 0.2f,  cv_step_time = 0.05f;
const float cv_init_volt = 1.0f, cv_scan_rate = 0.050, cv_re_volt = 0.2;
uint16_t cv_step_cnt = 0;
float scan_step = 0.0f;
float DAC1_1_currentVal[2] = {666, 100};
uint16_t cv_max_value = 4095, cv_min_value = 0, cv_init_value = 0;


int Get_DAC_Value(void) {
    return DAC1_1_currentVal[0];
}

const uint16_t TriangleWave12bit[32] = {
                    0, 124, 256, 384, 512, 640, 768, 896, 1024, 1152, 1280, 1408, 1536, 1664, 1792, 1920,
                    2048, 1920, 1792, 1664, 1536, 1408, 1280, 1152, 1024, 896, 768, 640, 512, 384, 256, 124
};

const uint16_t Custom12bit[32] = {
                      2047, 2447, 2831, 3185, 3498, 3750, 3939, 4056, 4095, 4056,
                      3939, 3750, 3495, 3185, 2831, 2447, 2047, 1647, 1263, 909, 
                      599, 344, 155, 38, 0, 38, 155, 344, 599, 909, 1263, 1647};
uint16_t Custom12bitChannel1[CV_BUFFER_SIZE] = {0};
uint16_t Custom12bitChannel2[32] = {0};
uint32_t DualCustom12bit[32] = {0};

void CustomWave_Arg_Init(){

    // DAC进行CV时的步进值
    scan_step = cv_step * cv_scan_rate * cv_step_time;
    // DAC进行CV时的最大和最小值
    cv_max_value = cv_max_volt * cv_step;
    cv_min_value = cv_min_volt * cv_step;
    cv_init_value = cv_init_volt * cv_step;
    // DAC步进时需要的位数
    DAC1_1_currentVal[0] = cv_init_volt * cv_step;
}

void FillCustomWave(void) { 
    uint32_t Idx = 0;    
    /* Fill Sine32bit table */
    for (Idx = 0; Idx < 32; Idx++)
    {
        DualCustom12bit[Idx] = (Custom12bit[Idx] << 16) + (Custom12bit[Idx]);
    }
}

void FillChannelWave(uint32_t DAC_Channel_x) {
    if (DAC_Channel_x == DAC_Channel_1) {
        uint16_t Idx = 0;
        uint16_t cv_currentVal = 0;
        for (Idx = 0; Idx < CV_BUFFER_SIZE && cv_currentVal <= cv_max_value; Idx++)
        {
            Custom12bitChannel1[Idx] = cv_currentVal;
            cv_currentVal += scan_step;
        }
        Custom12bitChannel1[++Idx] = cv_max_value;
        for ( ; Idx < CV_BUFFER_SIZE && cv_currentVal >= cv_min_value; Idx++)
        {
            Custom12bitChannel1[Idx] = cv_currentVal;
            cv_currentVal -= scan_step;
        }
        cv_step_cnt = Idx - 1;
    } else if (DAC_Channel_x == DAC_Channel_2) {
        uint16_t Idx = 0;
        for (Idx = 0; Idx < 32; Idx++)
        {
            Custom12bitChannel2[Idx] = TriangleWave12bit[Idx];
        }
    }
    
}

void CustomWave_GPIO_Config(uint16_t GPIO_Pin){
    assert_param(IS_GPIO_PIN_SOURCE(GPIO_Pin));
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void TIM2_Chan1_Config(){

    /* TIM2 Periph clock enable */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {
        .TIM_Period = 5000 - 1,                     // 10kHz/500 = 20Hz = 50ms/次
        .TIM_Prescaler = 7200-1,                    // 72MHz/7200 = 10KHz 
        .TIM_ClockDivision = TIM_CKD_DIV1,
        .TIM_CounterMode = TIM_CounterMode_Up,
    };
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    /* TIM2 TRGO selection */
    TIM_SelectOutputTrigger(TIM2, TIM_TRGOSource_Update);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    /* Enable the TIM2 global Interrupt */
    NVIC_InitTypeDef NVIC_InitStructure = {
        .NVIC_IRQChannel = TIM2_IRQn,
        .NVIC_IRQChannelPreemptionPriority = 1,
        .NVIC_IRQChannelSubPriority = 1,
        .NVIC_IRQChannelCmd = ENABLE,
    }; 
    NVIC_Init(&NVIC_InitStructure);

    

    // TIM_Cmd(TIM2, ENABLE);

}

void TIM2_IRQHandler(void)
{
    // static volatile uint32_t irq_cnt = 0;
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        // 更新DAC输出
        DAC1_1_currentVal[0] += scan_step;
        // irq_cnt++;

        // OLED_ShowString(4, 1, "DAC1_VAL:");
        // OLED_ShowNum(4, 10, DAC1_1_currentVal[0], 4);
        
        // 反转扫描方向
        if(DAC1_1_currentVal[0] >= cv_max_value){
            scan_step = -fabs(scan_step);
            DAC1_1_currentVal[0] = cv_max_value;
        }  else if (DAC1_1_currentVal[0] <= cv_min_value){
            scan_step = fabs(scan_step);
            DAC1_1_currentVal[0] = cv_min_value;    
        }
        // DAC_SetChannel1Data(DAC_Align_12b_R, DAC1_1_currentVal[0]);
        // DAC_SoftwareTriggerCmd(DAC_Channel_1, ENABLE);
    }
}

void TIM3_Chan2_Config(){

    /* TIM2 Periph clock enable */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {
        .TIM_Period = 5000 - 1,                     // 10kHz/500 = 20Hz = 50ms/次
        .TIM_Prescaler = 7200-1,                    // 72MHz/7200 = 10KHz 
        .TIM_ClockDivision = TIM_CKD_DIV1,
        .TIM_CounterMode = TIM_CounterMode_Up,
    };
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    /* TIM2 TRGO selection */
    TIM_SelectOutputTrigger(TIM3, TIM_TRGOSource_Update);

    // TIM_Cmd(TIM3, ENABLE);

    // /* Enable the TIM2 global Interrupt */
    // NVIC_InitTypeDef NVIC_InitStructure = {
    //     .NVIC_IRQChannel = TIM3_IRQn,
    //     .NVIC_IRQChannelPreemptionPriority = 1,
    //     .NVIC_IRQChannelSubPriority = 2,
    //     .NVIC_IRQChannelCmd = ENABLE,
    // };
    // NVIC_Init(&NVIC_InitStructure);

    // TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

}


void CustomWave_DAC_Config(uint32_t DAC_Channel_x, uint32_t DAC_Trigger){

    assert_param(IS_DAC_CHANNEL(DAC_Channel_x));
    assert_param(IS_DAC_TRIGGER(DAC_Trigger));
    /* DAC Periph clock enable */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

    DAC_InitTypeDef DAC_InitStructure;
    /* DAC channel1 Configuration */
    DAC_InitStructure.DAC_Trigger = DAC_Trigger;
    DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
    DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Disable;
    DAC_Init(DAC_Channel_x, &DAC_InitStructure);

    DAC_Cmd(DAC_Channel_x, ENABLE);
}

void CustomWave_DMA_Config(TIM_TypeDef* TIMx){

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    DMA_InitTypeDef DMA_InitStructure;
    
    /* DMA1 channel4 configuration */
    DMA_DeInit(DMA1_Channel3);

    DMA_InitStructure.DMA_PeripheralBaseAddr = DAC_DHR12R2_Address;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)&DualCustom12bit;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = 32;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

    DMA_Init(DMA1_Channel3, &DMA_InitStructure);
    DMA_Cmd(DMA1_Channel3, ENABLE);

    DAC_Cmd(DAC_Channel_1, ENABLE);
    DAC_Cmd(DAC_Channel_2, ENABLE);

    DAC_DMACmd(DAC_Channel_2, ENABLE);

    TIM_Cmd(TIMx, ENABLE);

}

void CustomWave_Chan1_Init(void){
    OLED_ShowString(4, 1, "CH1START");

    // RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    // GPIO_InitTypeDef GPIO_InitStructure = {
    //     .GPIO_Pin = GPIO_Pin_4,
    //     .GPIO_Mode = GPIO_Mode_AIN,
    // };
    // GPIO_Init(GPIOA, &GPIO_InitStructure);
    CustomWave_GPIO_Config(GPIO_Pin_4);

    TIM2_Chan1_Config();
    CustomWave_DAC_Config(DAC_Channel_1, DAC_Trigger_T2_TRGO);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    DMA_DeInit(DMA1_Channel3);
    DMA_InitTypeDef DMA_InitStructure = {
        .DMA_PeripheralBaseAddr = (uint32_t)DAC->DHR12R1,
        .DMA_PeripheralInc = DMA_PeripheralInc_Disable,
        .DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord,
        .DMA_MemoryBaseAddr = (uint32_t)&DAC1_1_currentVal,
        .DMA_MemoryInc = DMA_MemoryInc_Disable,
        .DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord,
        .DMA_DIR = DMA_DIR_PeripheralDST,
        .DMA_BufferSize = 2,
        .DMA_Mode = DMA_Mode_Circular,
        .DMA_Priority = DMA_Priority_Medium,
        .DMA_M2M = DMA_M2M_Disable, 
    };

    DMA_Init(DMA1_Channel3, &DMA_InitStructure);
    DMA_Cmd(DMA1_Channel3, ENABLE);

    DAC_Cmd(DAC_Channel_1, ENABLE);
    DAC_DMACmd(DAC_Channel_1, ENABLE);

    TIM_Cmd(TIM2, ENABLE);

    OLED_ShowString(4, 10, "END");

}

void CustomWave_Chan2_Init(void){
    /* DAC_Chan2_Init */

    CustomWave_GPIO_Config(GPIO_Pin_5);
    
    TIM3_Chan2_Config();
    CustomWave_DAC_Config(DAC_Channel_2, DAC_Trigger_T3_TRGO);

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    DMA_InitTypeDef DMA_InitStructure = {
        .DMA_PeripheralBaseAddr = (uint32_t)DAC->DHR12R2,
        .DMA_PeripheralInc = DMA_PeripheralInc_Disable,
        .DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord,
        .DMA_MemoryBaseAddr = (uint32_t)&Custom12bitChannel2,
        .DMA_MemoryInc = DMA_MemoryInc_Enable,
        .DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord,
        .DMA_DIR = DMA_DIR_PeripheralDST,
        .DMA_BufferSize = 32,
        .DMA_Mode = DMA_Mode_Circular,
        .DMA_Priority = DMA_Priority_High,
        .DMA_M2M = DMA_M2M_Disable, 
    };
    
    // DMA1 channel4 configuration 
    DMA_DeInit(DMA1_Channel3);

    DMA_Init(DMA1_Channel3, &DMA_InitStructure);
    // Enable DMA1 Channel2 
    DMA_Cmd(DMA1_Channel3, ENABLE);

    // Enable DAC Channel1: Once the DAC channel1 is enabled, PA.04 is 
    // automatically connected to the DAC converter. 

    DAC_Cmd(DAC_Channel_2, ENABLE);
    DAC_DMACmd(DAC_Channel_2, ENABLE);

    TIM_Cmd(TIM3, ENABLE);
    

    

    // CustomWave_DAC_Config(DAC_Channel_2, DAC_Trigger_Software);
    
    // DAC_SetChannel2Data(DAC_Align_12b_R, cv_init_value);
    // DAC_SoftwareTriggerCmd(DAC_Channel_2, ENABLE);
}

void CustomWave_DAC_Channel1_Setup(){
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);


    GPIO_InitTypeDef GPIO_InitStructure = {
        .GPIO_Pin = GPIO_Pin_4,
        .GPIO_Mode = GPIO_Mode_AIN,
    };
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {
        .TIM_Period = 5000 - 1,                     // 10kHz/500 = 20Hz = 50ms/次
        .TIM_Prescaler = 7200 - 1,                  // 72MHz/7200 = 10KHz
        .TIM_ClockDivision = TIM_CKD_DIV1,
        .TIM_CounterMode = TIM_CounterMode_Up,
    };
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    TIM_SelectOutputTrigger(TIM2, TIM_TRGOSource_Update);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    NVIC_InitTypeDef NVIC_InitStructure = {
        .NVIC_IRQChannel = TIM2_IRQn,
        .NVIC_IRQChannelPreemptionPriority = 1,
        .NVIC_IRQChannelSubPriority = 1,
        .NVIC_IRQChannelCmd = ENABLE,
    };
    NVIC_Init(&NVIC_InitStructure);

    DAC_InitTypeDef DAC_InitStructure = {
        .DAC_Trigger = DAC_Trigger_T2_TRGO,
        .DAC_WaveGeneration = DAC_WaveGeneration_None,
        .DAC_OutputBuffer = DAC_OutputBuffer_Disable,
    };
    DAC_Init(DAC_Channel_1, &DAC_InitStructure);
    DAC_Cmd(DAC_Channel_1, ENABLE);

    DMA_InitTypeDef DMA_InitStructure = {
        .DMA_PeripheralBaseAddr = (uint32_t)DAC->DHR12R1,
        .DMA_PeripheralInc = DMA_PeripheralInc_Disable,
        .DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord,
        .DMA_MemoryBaseAddr = (uint32_t)DAC1_1_currentVal,
        .DMA_MemoryInc = DMA_MemoryInc_Disable,
        .DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord,
        .DMA_DIR = DMA_DIR_PeripheralDST,
        .DMA_BufferSize = 1,
        .DMA_Mode = DMA_Mode_Circular,
        .DMA_Priority = DMA_Priority_High,
        .DMA_M2M = DMA_M2M_Disable,
    };
    DMA_Init(DMA1_Channel2, &DMA_InitStructure);
    DMA_Cmd(DMA1_Channel2, ENABLE);
    DAC_DMACmd(DAC_Channel_1, ENABLE);
    

    TIM_Cmd(TIM2, ENABLE);
}

void CustomWave_DAC_ReBuild() {

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3 | RCC_APB1Periph_DAC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure = {
        .GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5,
        .GPIO_Mode = GPIO_Mode_AIN,
    };
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {
        .TIM_Period = 5000 - 1,                     // 10kHz/500 = 20Hz = 50ms/次
        .TIM_Prescaler = 7200 - 1,                  // 72MHz/7200 = 10KHz
        .TIM_ClockDivision = TIM_CKD_DIV1,
    };
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    TIM_SelectOutputTrigger(TIM3, TIM_TRGOSource_Update);
    TIM_Cmd(TIM3, ENABLE);

    DAC_InitTypeDef DAC_InitStructure = {
        .DAC_Trigger = DAC_Trigger_T3_TRGO,
        .DAC_WaveGeneration = DAC_WaveGeneration_None,
        .DAC_OutputBuffer = DAC_OutputBuffer_Disable,
    };
    DAC_Init(DAC_Channel_1, &DAC_InitStructure);
    DAC_Init(DAC_Channel_2, &DAC_InitStructure);
    DAC_Cmd(DAC_Channel_1, ENABLE);
    DAC_Cmd(DAC_Channel_2, ENABLE);

    DMA_InitTypeDef DMA_InitStructure = {
        .DMA_PeripheralBaseAddr = (uint32_t)&DAC->DHR12R2,
        .DMA_PeripheralInc = DMA_PeripheralInc_Disable,
        .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word,
        .DMA_MemoryBaseAddr = (uint32_t)&DualCustom12bit,
        .DMA_MemoryInc = DMA_MemoryInc_Enable,
        .DMA_MemoryDataSize = DMA_MemoryDataSize_Word,
        .DMA_DIR = DMA_DIR_PeripheralDST,
        .DMA_BufferSize = 32,
        .DMA_Mode = DMA_Mode_Circular,
        .DMA_Priority = DMA_Priority_High,
        .DMA_M2M = DMA_M2M_Disable
    };
    DMA_Init(DMA1_Channel3, &DMA_InitStructure);
    DMA_Cmd(DMA1_Channel3, ENABLE);
    DAC_DMACmd(DAC_Channel_2, ENABLE);


}
void CustomWave_Init(){
    CustomWave_Arg_Init();
    FillCustomWave();
    
    FillChannelWave(DAC_Channel_1);
    FillChannelWave(DAC_Channel_2);

    
    // CustomWave_Chan1_Init();
    // CustomWave_DAC_Channel1_Setup();

    // CustomWave_Chan2_Init();
    
    CustomWave_DAC_ReBuild();
    

    // OLED_ShowNum(3, 1, cv_max_value, 4);
    // OLED_ShowNum(3, 6, cv_min_value, 4);
    // OLED_ShowNum(1, 1, cv_step_cnt, 4);
    // OLED_ShowHexNum(1, 6, (uint32_t)&DAC->DHR12R1, 8);
    // OLED_ShowString(3, 7, ".   V");
    // uint16_t count = 0, volt1 = 0, volt2 = 0;
    // while (++count < 1024) {
    //     OLED_ShowNum(2, 1, count, 4);
    //     OLED_ShowNum(3, 1, Custom12bitChannel1[count], 4);
    //     OLED_ShowNum(3, 6, (Custom12bitChannel1[count] / cv_step), 1);
    //     OLED_ShowNum(3, 8, (Custom12bitChannel1[count] % (int)cv_step / cv_step * 1000), 3);  
    //     OLED_ShowNum(4, 1, Custom12bitChannel2[count], 4);
    //     Delay_ms(50);
    // }
    

}



