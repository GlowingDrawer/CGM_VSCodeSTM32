#include "stm32f10x.h"                  // Device header
#include "BT.h"
#include <stdio.h>
#include "Gpio.h"
#include <stdarg.h>

#define BT_USARTx USART3

struct BT_ARG {
    USART_TypeDef * USART;          // Serial of BT
    uint32_t RCC_Periph_USART;      // 串口外设时钟
    uint32_t GPIO_Periph;           // GPIO外设
    GPIO_TypeDef * GPIOx;          
    uint16_t USART_TX;              // USART_TX使用的GPIO_Pin
    uint16_t USART_RX;              // USART_RX使用的GPIO_Pin
    uint32_t baudrate;              // 波特率
};

//蓝牙参数结构体初始化
struct BT_ARG BT_ARG_STRUCTURE = {
    USART3, RCC_APB1Periph_USART3,                              // USART_ARG
    RCC_APB2Periph_GPIOB, GPIOB, GPIO_Pin_10, GPIO_Pin_11,      // GPIO_ARG                                            // GPIO外设
    115200,                                                     // 波特率
    // 更改程序时记得检查中断配置和中断服务函数
};

int BT_Arg_Init(USART_TypeDef * USARTx){
    if (USARTx == USART1) {
        BT_ARG_STRUCTURE.USART = USART1;
        BT_ARG_STRUCTURE.RCC_Periph_USART = RCC_APB2Periph_USART1;
        BT_ARG_STRUCTURE.GPIO_Periph = RCC_APB2Periph_GPIOA;
        BT_ARG_STRUCTURE.GPIOx = GPIOA;
        BT_ARG_STRUCTURE.USART_TX = GPIO_Pin_9;
        BT_ARG_STRUCTURE.USART_RX = GPIO_Pin_10;
    } else if (USARTx == USART2) {
        BT_ARG_STRUCTURE.USART = USART2;
        BT_ARG_STRUCTURE.RCC_Periph_USART = RCC_APB1Periph_USART2;
        BT_ARG_STRUCTURE.GPIO_Periph = RCC_APB2Periph_GPIOA;
        BT_ARG_STRUCTURE.GPIOx = GPIOA;
        BT_ARG_STRUCTURE.USART_TX = GPIO_Pin_2;
        BT_ARG_STRUCTURE.USART_RX = GPIO_Pin_3;
    } else if (USARTx == USART3) {
        BT_ARG_STRUCTURE.USART = USART3;
        BT_ARG_STRUCTURE.RCC_Periph_USART = RCC_APB1Periph_USART3;
        BT_ARG_STRUCTURE.GPIO_Periph = RCC_APB2Periph_GPIOB;
        BT_ARG_STRUCTURE.GPIOx = GPIOB;
        BT_ARG_STRUCTURE.USART_TX = GPIO_Pin_10;
        BT_ARG_STRUCTURE.USART_RX = GPIO_Pin_11;
        return 1;
    }
    return 0;
};


// 时钟使能配置
void BT_RCC_Config(){
    RCC_APB1PeriphClockCmd(BT_ARG_STRUCTURE.RCC_Periph_USART, ENABLE);
    RCC_APB2PeriphClockCmd(BT_ARG_STRUCTURE.GPIO_Periph, ENABLE);
};

//GPIO配置
void BT_GPIO_Config(){
    if (BT_ARG_STRUCTURE.GPIOx == GPIOA) {
        GPIOA_Init(BT_ARG_STRUCTURE.USART_TX, GPIO_Mode_AF_PP, GPIO_Speed_50MHz);
        GPIOA_Init(BT_ARG_STRUCTURE.USART_RX, GPIO_Mode_IPU, GPIO_Speed_50MHz);
    } else if (BT_ARG_STRUCTURE.GPIOx == GPIOB) {
        GPIOB_Init(BT_ARG_STRUCTURE.USART_TX, GPIO_Mode_AF_PP, GPIO_Speed_50MHz);
        GPIOB_Init(BT_ARG_STRUCTURE.USART_RX, GPIO_Mode_IPU, GPIO_Speed_50MHz);
    }
    
};

//USART Config
void BT_USART_Config(void){
    
    USART_InitTypeDef USART_InitStructure;
    // USART参数配置
    USART_InitStructure.USART_BaudRate = BT_ARG_STRUCTURE.baudrate; // 如9600
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART3, &USART_InitStructure);
    
    USART_ITConfig(BT_ARG_STRUCTURE.USART, USART_IT_RXNE, ENABLE);
}

void BT_NVIC_Config(void){
    
    NVIC_InitTypeDef NVIC_InitStructure;
    // 配置NVIC
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

// 中断服务函数
#define RX_BUFFER_SIZE 256
uint8_t rx_buffer[RX_BUFFER_SIZE];
uint16_t rx_index = 0;
void USART3_IRQHandler(void){
    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) {
        // 读取接收到的数据
        uint8_t data = USART_ReceiveData(USART3);
        
        // 存储到缓冲区（示例：简单回传）
        rx_buffer[rx_index++] = data;
        if (rx_index >= RX_BUFFER_SIZE) rx_index = 0; // 防止溢出
        
        // 可选：回传数据（Echo）
        USART_SendData(USART3, data);
        while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET); // 等待发送完成
    }
}


//蓝牙初始化配置
void BT_Init(USART_TypeDef * USARTx){

    BT_Arg_Init(USARTx);

    BT_RCC_Config();
    BT_GPIO_Config();
    BT_USART_Config();
    BT_NVIC_Config();
    
    USART_Cmd(BT_ARG_STRUCTURE.USART, ENABLE);
};

//发送数据代码
void BT_SendByte(uint8_t Byte)
{
    USART_SendData(BT_ARG_STRUCTURE.USART, Byte);
    while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
}

void BT_SendArray(uint8_t *Array, uint16_t Length)
{
    uint16_t i;
    for (i = 0; i < Length; i++)
    {
        BT_SendByte(*Array++);
    }
}

void BT_SendString(char *String)
{
    for (int i = 0; String[i] != '\0'; i++)
    {
        BT_SendByte(String[i]);
    }
}

uint32_t BT_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--)
    {
        Result *= X;
    }
    return Result;
}

void BT_SendNumber(uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        BT_SendByte(Number / BT_Pow(10, Length - i - 1) % 10 + 0x30);
    } 
}

void BT_Printf(char * format, ...)
{
    char String[100];
    va_list arg;
    va_start(arg, format);
    vsprintf(String, format, arg);
    va_end(arg);
    BT_SendString(String);
}
