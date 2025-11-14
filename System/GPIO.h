#ifndef __GPIO_H
#define __GPIO_H

#ifdef __cplusplus
 extern "C" {
#endif 

void GPIOA_Init(uint16_t Pin, GPIOMode_TypeDef Mode, GPIOSpeed_TypeDef Speed);
void GPIOB_Init(uint16_t Pin, GPIOMode_TypeDef Mode, GPIOSpeed_TypeDef Speed);
void GPIOC_Init(uint16_t Pin, GPIOMode_TypeDef Mode, GPIOSpeed_TypeDef Speed);
void ADC_Reset(ADC_TypeDef * ADCx);

     #ifdef __cplusplus
 }
#endif 


#endif
