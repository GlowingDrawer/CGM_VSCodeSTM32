#include "stm32f10x.h"                  // Device header
#include "MyDAC.h"
#include "Gpio.h"
void MyDAC_Init(void){
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure = {
        .GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5,
        .GPIO_Mode = GPIO_Mode_AIN,
    };
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    DAC_InitTypeDef DAC_InitStructure;
    
    //DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_TriangleAmplitude_4095;
    DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Disable;
    DAC_InitStructure.DAC_Trigger = DAC_Trigger_Software;
    DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
    
    DAC_Init(DAC_Channel_1, &DAC_InitStructure);
    DAC_Cmd(DAC_Channel_1, ENABLE);
    DAC_Init(DAC_Channel_2, &DAC_InitStructure);
    DAC_Cmd(DAC_Channel_2, ENABLE);
    
}


void DAC_SineWave(void){
    GPIOA_Init(GPIO_Pin_4 | GPIO_Pin_5, GPIO_Mode_AIN, GPIO_Speed_10MHz);
    
    DAC_InitTypeDef DAC_InitStructure;
    DAC_InitStructure.DAC_Trigger = DAC_Trigger_T2_TRGO;
    DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
    DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Disable;
    DAC_Init(DAC_Channel_1, &DAC_InitStructure);

    /* DAC channel2 Configuration */
    DAC_Init(DAC_Channel_2, &DAC_InitStructure);

}

void DAC_SetChannelVolt(float voltage){
    uint16_t dac_value = (voltage / 3.3) * 4095;
    DAC_SetChannel1Data(DAC_Align_12b_R, dac_value);
    DAC_SoftwareTriggerCmd(DAC_Channel_1, ENABLE);
}

void DAC_SetChanne2Volt(float voltage){
    uint16_t dac_value = (voltage / 3.3) * 4095;
    DAC_SetChannel2Data(DAC_Align_12b_R, dac_value);
    DAC_SoftwareTriggerCmd(DAC_Channel_2, ENABLE);
}

