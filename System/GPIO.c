#include "stm32f10x.h"                  // Device header
#include "GPIO.h"

void GPIOA_Init(uint16_t Pin, GPIOMode_TypeDef Mode, GPIOSpeed_TypeDef Speed){
    assert_param(IS_GPIO_PIN_SOURCE(Pin));
    assert_param(IS_GPIO_MODE(Mode));
    assert_param(IS_GPIO_SPEED(Speed));

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = Mode;
    GPIO_InitStructure.GPIO_Pin = Pin;
    GPIO_InitStructure.GPIO_Speed = Speed;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
}

void GPIOB_Init(uint16_t Pin, GPIOMode_TypeDef Mode, GPIOSpeed_TypeDef Speed){
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = Mode;
    GPIO_InitStructure.GPIO_Pin = Pin;
    GPIO_InitStructure.GPIO_Speed = Speed;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
}

void GPIOC_Init(uint16_t Pin, GPIOMode_TypeDef Mode, GPIOSpeed_TypeDef Speed){
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = Mode;
    GPIO_InitStructure.GPIO_Pin = Pin;
    GPIO_InitStructure.GPIO_Speed = Speed;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
}

void ADC_Reset(ADC_TypeDef * ADCx)
{
	ADC_ResetCalibration(ADCx);
	while (ADC_GetResetCalibrationStatus(ADCx));
	ADC_StartCalibration(ADCx);
	while (ADC_GetCalibrationStatus(ADCx));
}



