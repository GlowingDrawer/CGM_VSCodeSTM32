#ifndef __USART_H
#define __USART_H
#include "stm32f10x.h"


#ifdef __cplusplus
extern "C" {
#endif

void Fputc_Switch(USART_TypeDef * USARTx);

void My_USART_SendByte(USART_TypeDef * USARTx, uint8_t Byte);

void My_USART_SendArray(USART_TypeDef * USARTx, uint8_t *Array, uint16_t Length);

void My_USART_SendString(USART_TypeDef * USARTx, char *String);

uint32_t My_USART_Pow(uint32_t X, uint32_t Y);

void My_USART_SendNumber(USART_TypeDef * USARTx, uint32_t Number, uint8_t Length);

void My_USART_Printf(USART_TypeDef* USARTx,const char *format, ...);




#ifdef __cplusplus
}
#endif

#endif


