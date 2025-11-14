#include "Show.h"
#include "OLED.h"
#include "GPIO.h"
#include "Delay.h"


uint16_t AD_Value[2];

float max_current = 999.0f, min_current = -999.0f;

typedef struct 
{
    ADC_TypeDef * ADCx;
    uint8_t Channelx;
    GPIO_TypeDef * GPIOx;
    uint16_t GPIO_Pin;
}AD_ChanTypeDef;

AD_ChanTypeDef ADCx_Channel_x[4] = {
    {ADC1, ADC_Channel_1, GPIOA, GPIO_Pin_0},
    {ADC1, ADC_Channel_2, GPIOA, GPIO_Pin_2},
    {ADC1, ADC_Channel_3, GPIOA, GPIO_Pin_3},
    {ADC1, ADC_Channel_4, GPIOA, GPIO_Pin_4},
};

void Fill_AD_ChannelStruct(AD_ChanTypeDef * ADC_ChannelStruct)
{
    if (ADC_ChannelStruct->Channelx >= ADC_Channel_0 && ADC_ChannelStruct->Channelx <= ADC_Channel_7) {
        ADC_ChannelStruct->GPIOx = GPIOA;
    } else if (ADC_ChannelStruct->Channelx >= ADC_Channel_8 && ADC_ChannelStruct->Channelx <= ADC_Channel_9) {
        ADC_ChannelStruct->GPIOx = GPIOB;
    } else if (ADC_ChannelStruct->Channelx >= ADC_Channel_10 && ADC_ChannelStruct->Channelx <= ADC_Channel_9) {
        ADC_ChannelStruct->GPIOx = GPIOC;
    }
    switch (ADC_ChannelStruct->Channelx)
    {
    case ADC_Channel_0:
        ADC_ChannelStruct->GPIO_Pin = GPIO_Pin_0;
        break;
    case ADC_Channel_1:
        ADC_ChannelStruct->GPIO_Pin = GPIO_Pin_1;
        break;
    case ADC_Channel_2:
        ADC_ChannelStruct->GPIO_Pin = GPIO_Pin_2;
        break;
    case ADC_Channel_3:
        ADC_ChannelStruct->GPIO_Pin = GPIO_Pin_3;
        break;
    case ADC_Channel_4:
        ADC_ChannelStruct->GPIO_Pin = GPIO_Pin_4;
        break;
    case ADC_Channel_5:
        ADC_ChannelStruct->GPIO_Pin = GPIO_Pin_5;
        break;
    case ADC_Channel_6:
        ADC_ChannelStruct->GPIO_Pin = GPIO_Pin_6;
        break;
    case ADC_Channel_7:
        ADC_ChannelStruct->GPIO_Pin = GPIO_Pin_7;
        break;
    case ADC_Channel_8:
        ADC_ChannelStruct->GPIO_Pin = GPIO_Pin_0;
        break;
    case ADC_Channel_9:
        ADC_ChannelStruct->GPIO_Pin = GPIO_Pin_1;
        break;
    case ADC_Channel_10:
        ADC_ChannelStruct->GPIO_Pin = GPIO_Pin_0;
        break;
    case ADC_Channel_11:
        ADC_ChannelStruct->GPIO_Pin = GPIO_Pin_1;
        break;
    case ADC_Channel_12:
        ADC_ChannelStruct->GPIO_Pin = GPIO_Pin_2;
        break;
    case ADC_Channel_13:
        ADC_ChannelStruct->GPIO_Pin = GPIO_Pin_3;
        break;
    case ADC_Channel_14:
        ADC_ChannelStruct->GPIO_Pin = GPIO_Pin_4;
        break;
    case ADC_Channel_15:
        ADC_ChannelStruct->GPIO_Pin = GPIO_Pin_5;
        break;
    default:
        break;
    }
    
};

void AD_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	uint16_t Pin = GPIO_Pin_1 | GPIO_Pin_2;
	GPIOA_Init(Pin, GPIO_Mode_AIN, GPIO_Speed_2MHz);
	
	
	
    // ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, 
    //     ADC_SampleTime_55Cycles5);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 2, 
        ADC_SampleTime_55Cycles5);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 3, 
        ADC_SampleTime_55Cycles5);
    // ADC_RegularChannelConfig(ADC1, ADC_Channel_3, 4, 
    //     ADC_SampleTime_55Cycles5);
	
	ADC_InitTypeDef ADC_InitStructure;
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_NbrOfChannel = 2;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_Init(ADC1, &ADC_InitStructure);
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	
	DMA_InitTypeDef DMA_InitStructure;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)AD_Value;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = 2;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);
    
    DMA_Cmd(DMA1_Channel1,ENABLE);
    ADC_DMACmd(ADC1, ENABLE);
	ADC_Cmd(ADC1, ENABLE);
    	
	ADC_ResetCalibration(ADC1);
	while (ADC_GetResetCalibrationStatus(ADC1) == SET);
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1) == SET)
        Delay_ms(1);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

float Init_volt_ADC1_1, Init_volt_ADC1_2, volt_right, current_right;
float Init_current_ADC1_1, Init_current_ADC1_2;
uint32_t volt_left, current_left;

int voltage_start = 10, current_start = 9;

// REF voltage   V_Ref V_sen
float volt_ref = 0, volt_sensor = 0;
void VoltRef_Modi(float ref, float sen) {
    if (ref >= 0 && ref <= 3.3) {
        volt_ref = ref;
    }
    if (sen >= 0 && sen <= 3.3) {
        volt_sensor = sen;
    }
}
void CountShow(uint8_t line, uint16_t AD_Value, uint32_t Gain){

    OLED_ShowString(1, 3, "Valu");
    OLED_ShowString(1, 8, "Current");
    OLED_ShowString(2, 1, "1:");
    OLED_ShowString(2, 12, "uA");
    OLED_ShowString(3, 1, "2:");
    OLED_ShowString(3, 12, "mA");


    if (Gain > 500 && Gain < 999999) {

    } else if (Gain > 50) {}

    OLED_ShowNum(1, 5, AD_Value, 4);
    //Count the value of Voltage and Current
    Init_volt_ADC1_1 = (float)AD_Value*(3.3/4096);
    //Init_volt_ADC1_1 = 2.32;      // Test the voltage function 1
    volt_left = (int) Init_volt_ADC1_1;
    OLED_ShowNum(1, voltage_start, volt_left, 1);
    volt_right = (Init_volt_ADC1_1 - volt_left) * 1000;
    OLED_ShowString(1, voltage_start + 1, ".");
    OLED_ShowNum(1, voltage_start + 2, volt_right, 3);
    OLED_ShowString(1, voltage_start + 5, "V");

    uint16_t mag_sensor1 = (volt_ref - volt_sensor) * 1000;

    Init_current_ADC1_1 = (float)(Init_volt_ADC1_1 - volt_sensor) * mag_sensor1;
    current_left = Init_current_ADC1_1;
    OLED_ShowNum(2, current_start, current_left, 2);
    current_right = (Init_current_ADC1_1 - current_left) * 1000;
    OLED_ShowString(2, current_start + 2, ".");
    OLED_ShowNum(2, current_start + 3, current_right, 3);
    OLED_ShowString(2, current_start + 6, "uA");


}

void CountShow_current(uint8_t line, uint16_t AD_Value, uint32_t Gain){
    
}

