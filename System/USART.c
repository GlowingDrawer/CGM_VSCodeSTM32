#include "USART.h"
#include <stdio.h>
#include "Gpio.h"
#include <stdarg.h>
#include "stdlib.h"

USART_TypeDef * My_USARTx = USART1;
void Fputc_Switch(USART_TypeDef * USARTx) {
    if (USARTx == USART1) {
        My_USARTx = USARTx;
    } else if (USARTx == USART2)
    {
        /* code */
        My_USARTx = USARTx;
    } else if (USARTx == USART3)
    {
        /* code */
        My_USARTx = USARTx;
    } 
    
}

void My_USART_SendByte(USART_TypeDef * USARTx, uint8_t Byte)
{
    USART_SendData(USARTx, Byte);
    while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET);
}

void My_USART_SendArray(USART_TypeDef * USARTx, uint8_t *Array, uint16_t Length)
{
    uint16_t i;
    for (i = 0; i < Length; i++)
    {
        My_USART_SendByte(USARTx, *Array++);
    }
}

void My_USART_SendString(USART_TypeDef * USARTx, char *String)
{
    for (int i = 0; String[i] != '\0'; i++)
    {
        My_USART_SendByte(USARTx, String[i]);
    }
}

uint32_t My_USART_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--)
    {
        Result *= X;
    }
    return Result;
}

void My_USART_SendNumber(USART_TypeDef * USARTx, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        My_USART_SendByte(USARTx, Number / My_USART_Pow(10, Length - i - 1) % 10 + 0x30);
    } 
}

int fputc(int ch, FILE *f)
{
    My_USART_SendByte(My_USARTx, ch);
    return ch;
}

void My_USART_Printf(USART_TypeDef* USARTx,const char *format, ...)
{
    // char String[100];
    va_list arg;
    va_start(arg, format);
    int len = vsnprintf(NULL, 0, format, arg) + 1;  // +1 为终止符
    va_end(arg);

    char * String = (char *)malloc(len);
    // 格式化字符串
    va_start(arg, format);
    vsnprintf(String, len, format, arg);
    va_end(arg);
    
    My_USART_SendString(USARTx, String);

    free(String);
}



