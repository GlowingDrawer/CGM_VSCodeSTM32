#ifndef __BT_H
#define __BT_H

#include <stdio.h>


#ifdef __cplusplus
 extern "C" {
#endif 
// 时钟使能配置
void BT_RCC_Config(void);
//GPIO配置
void BT_GPIO_Config(void);
//USART Config
void BT_USART_Config(void);
//中断配置
void BT_NVIC_Config(void);
//蓝牙初始化配置
void BT_Init(USART_TypeDef *USARTx);

//发送数据代码
void BT_SendByte(uint8_t Byte);

void BT_SendArray(uint8_t *Array, uint16_t Length);

void BT_SendString(char *String);

uint32_t BT_Pow(uint32_t X, uint32_t Y);

void BT_SendNumber(uint32_t Number, uint8_t Length);

void BT_Printf(char * format, ...);

#ifdef __cplusplus
}
#endif 

#endif
