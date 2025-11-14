#ifndef __SHOW_H__
#define __SHOW_H__
#include "stm32f10x.h"


#ifdef __cplusplus
extern "C" {
#endif

extern uint16_t ADC_Value[];

void AD_Init(void);
void CountShow(uint8_t line, uint16_t AD_Value, uint32_t Gain);

#ifdef __cplusplus
}
#endif


#endif
