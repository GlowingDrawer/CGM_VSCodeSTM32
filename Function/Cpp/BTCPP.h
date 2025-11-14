#pragma once
#include "stm32f10x.h"
#include <stdio.h>
#include <InitArg.h>
#include "USART.h"
#include <cstdarg>

class USART_Controller {
public:
    
    // 构造函数：指定USART外设和最大接收缓存大小
    USART_Controller(USART_TypeDef* USARTx = USART3, BaudRate baudrate = BaudRate::BAUD_115200, uint16_t buf_size = 256);

    const USART::USART_Params& GetParams() const { return btParams; }
    bool GetRxFlag(){
        if (this->isRXNE) {
            this->isRXNE = false;
            return true;
        }
        return this->isRXNE;
    }

    bool Start();
    void Continue();
    void Stop();

    // 初始化蓝牙串口
    bool Init();
    
    // 发送数据
    void Send(const uint8_t* data, uint16_t length);
    void Send(const char* str);
    template<typename... Args>
    void Printf(const char * format, Args... args) {  My_USART_Printf(this->btParams.USART, format, args...); }
    
    // 接收数据
    uint16_t Receiver(uint8_t* buffer, uint16_t max_lenghth);
    
    // 中断处理函数（需要放在类外实际中断服务例程中调用）
    void IRQHandler();


    void Reset(USART_TypeDef* usartx, BaudRate baudratem = BaudRate::BAUD_115200);

    // 基础发送函数
    void SendByte(uint8_t Byte);
    
    // 数据块发送
    void SendArray(uint8_t *Array, uint16_t Length);
    
    // 字符串发送（需以'\0'结尾）
    void SendString(char *String);
    
    // 数字发送（十进制ASCII形式）
    void SendNumber(uint32_t Number, uint8_t Length);

private:
    bool isRXNE = false;

    USART::USART_Params btParams;

    uint32_t m_baudrate;
    
    // 接收缓冲区
    std::array<uint8_t, 256> m_rxBuffer = {0};
    uint16_t m_bufferSize;
    volatile uint16_t m_rxHead;
    volatile uint16_t m_rxTail;
    
    void RCC_Config();
    void GPIO_Config();
    void USART_Config();

    uint32_t Pow(uint32_t X, uint32_t Y);
};

inline USART_Controller & GetStaticBt(USART_TypeDef* USARTx = USART3) {
    static USART_Controller instance(USARTx, BaudRate::BAUD_115200);
    return instance;
}

inline void BT_IRQHandler() {
    GetStaticBt().IRQHandler();
}
    

// // 时钟使能配置
// void BT_RCC_Config(void);
// //GPIO配置
// void BT_GPIO_Config(void);
// //USART Config
// void BT_USART_Config(void);
// //中断配置
// void BT_NVIC_Config(void);
// //蓝牙初始化配置
// bool BT_Arg_Init(USART::USART_Params* params, USART_TypeDef *USARTx);
// void BT_Init(USART_TypeDef *USARTx);

// //发送数据代码
// void BT_SendByte(uint8_t Byte);

// void BT_SendArray(uint8_t *Array, uint16_t Length);

// void BT_SendString(char *String);

// uint32_t BT_Pow(uint32_t X, uint32_t Y);

// void BT_SendNumber(uint32_t Number, uint8_t Length);

// extern "C" void BT_Printf(char * format, ...);



