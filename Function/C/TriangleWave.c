// #include "stm32f10x.h"
// #include "TriangleWave.h"

// void TriangleWave_RCC_Config(){

//   RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
//   /* DAC Periph clock enable */
//   RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
//   /* TIM2 Periph clock enable */
//   RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
// }

// void TriangleWave_GPIO_Config(){

//   GPIO_InitTypeDef GPIO_InitStructure;

//   /* Once the DAC channel is enabled, the corresponding GPIO pin is automatically 
//     connected to the DAC converter. In order to avoid parasitic consumption, 
//     the GPIO pin should be configured in analog */
//   GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_4 | GPIO_Pin_5;
//   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
//   GPIO_Init(GPIOA, &GPIO_InitStructure);
// }

// void TriangleWave_TIM_Config(){
//   TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
//       /* TIM2 Configuration */
//   TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
//   TIM_TimeBaseStructure.TIM_Period = 200-1;          
//   TIM_TimeBaseStructure.TIM_Prescaler = 3600-1;       
//   TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;    
//   TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  
//   TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

//   /* TIM2 TRGO selection */
//   TIM_SelectOutputTrigger(TIM2, TIM_TRGOSource_Update);

//   TIM_Cmd(TIM2, ENABLE);
// }

// void TriangleWave_DAC_Config(){

//   DAC_InitTypeDef DAC_InitStructure;
//   /* DAC channel1 Configuration */
//   DAC_InitStructure.DAC_Trigger = DAC_Trigger_T2_TRGO;
//   DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_Triangle;
//   DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_TriangleAmplitude_4095;
//   DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Disable;
//   DAC_Init(DAC_Channel_1, &DAC_InitStructure);

//   /* DAC channel2 Configuration */
//   DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_TriangleAmplitude_1023;
//   DAC_Init(DAC_Channel_2, &DAC_InitStructure);

//   /* Enable DAC Channel1: Once the DAC channel1 is enabled, PA.04 is 
//     automatically connected to the DAC converter. */
//   DAC_Cmd(DAC_Channel_1, ENABLE);

//   /* Enable DAC Channel2: Once the DAC channel2 is enabled, PA.05 is 
//     automatically connected to the DAC converter. */
//   DAC_Cmd(DAC_Channel_2, ENABLE);

//   /* Set DAC dual channel DHR12RD register */
//   DAC_SetDualChannelData(DAC_Align_12b_R, 0x100, 0x100);


// }

// void TriangleWave_Init(){

//   TriangleWave_RCC_Config();
//   TriangleWave_GPIO_Config();
//   TriangleWave_TIM_Config();
//   TriangleWave_DAC_Config();
// }

