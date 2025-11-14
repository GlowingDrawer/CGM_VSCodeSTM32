#include "stm32f10x.h"                  // Device header
#include "BTCPP.h"
#include "GPIO.h"
#include <cstring>
#include <IRQnManage.h>
#include <stdlib.h>

USART_Controller::USART_Controller(USART_TypeDef* usartx, BaudRate baudrate, uint16_t buffer_size)
    : m_rxHead(0), m_rxTail(0), m_bufferSize(buffer_size) {
    this->Reset(usartx, baudrate);
}

void USART_Controller::Reset(USART_TypeDef* usartx, BaudRate baudrate) {
    for (auto & item : USART::USART_ConfigMap) {
        if (item.USART == usartx) {
            btParams = item;
            btParams.baudrate = static_cast<uint32_t>(baudrate);
            break;
        }
    }
}

bool USART_Controller::Start() { 
    bool result = this->Init();
    this->Continue();

    return result;
}

void USART_Controller::Continue(){
    USART_ITConfig(btParams.USART, USART_IT_RXNE, ENABLE);
    USART_Cmd(btParams.USART, ENABLE);
}

void USART_Controller::Stop() { 
    USART_ITConfig(btParams.USART, USART_IT_RXNE, DISABLE);
    USART_Cmd(btParams.USART, DISABLE);
}

bool USART_Controller::Init() {
    if(m_bufferSize == 0) return false;
    bool result = true;
    // 其他USART外设的时钟使能需要根据实际情况添加
    this->RCC_Config();
    this->GPIO_Config();
    this->USART_Config();
    
    return true;
}

void USART_Controller::RCC_Config() {
    if (this->btParams.USART == USART1) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    } else {
        RCC_APB1PeriphClockCmd(btParams.RCC_Periph_USART, ENABLE);
    }
    RCC_APB2PeriphClockCmd(btParams.GPIO_Periph, ENABLE);
    
}
void USART_Controller::GPIO_Config() {
    if (this->btParams.GPIOx == GPIOA) {
        GPIOA_Init(this->btParams.USART_TX, GPIO_Mode_AF_PP, GPIO_Speed_50MHz);
        GPIOA_Init(this->btParams.USART_RX, GPIO_Mode_IPU, GPIO_Speed_50MHz);
    } else if (this->btParams.GPIOx == GPIOB) {
        GPIOB_Init(this->btParams.USART_TX, GPIO_Mode_AF_PP, GPIO_Speed_50MHz);
        GPIOB_Init(this->btParams.USART_RX, GPIO_Mode_IPU, GPIO_Speed_50MHz);
    }
}

void USART_Controller::USART_Config() {
    USART_InitTypeDef USART_InitStruct;
    
    USART_InitStruct.USART_BaudRate = btParams.baudrate;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits = USART_StopBits_1;
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    
    USART_Init(btParams.USART, &USART_InitStruct);
   
}

void USART_Controller::Send(const uint8_t* data, uint16_t length) {
    DMA_Cmd(btParams.TX_DMA_Channel, DISABLE);
    DMA_SetCurrDataCounter(btParams.TX_DMA_Channel, length);
    // DMA_MemoryTargetConfig(btParams.TX_DMA_Channel, (uint32_t)data, &);
    DMA_Cmd(btParams.TX_DMA_Channel, ENABLE);
    USART_DMACmd(btParams.USART, USART_DMAReq_Tx, ENABLE);
    // for(uint16_t i = 0; i < length; ++i) {
    //     USART_SendData(btParams.USART, data[i]);
    //     while(USART_GetFlagStatus(btParams.USART, USART_FLAG_TXE) == RESET);
    // }
}

void USART_Controller::Send(const char* str) {
    Send((const uint8_t*)str, strlen(str));
}

uint16_t USART_Controller::Receiver(uint8_t* buffer, uint16_t max_length) {
    uint16_t bytesRead = 0;
    while((m_rxHead != m_rxTail) && (bytesRead < max_length)) {
        buffer[bytesRead++] = m_rxBuffer[m_rxTail];
        m_rxTail = (m_rxTail + 1) % m_bufferSize;
    }
    return bytesRead;
}

void USART_Controller::IRQHandler() {
    uint8_t data = USART_ReceiveData(btParams.USART);
    uint16_t nextHead = (m_rxHead + 1) & 0xFF;
    if(nextHead != m_rxTail) {
        m_rxBuffer[m_rxHead] = data;
        m_rxHead = nextHead;
    }
    this->isRXNE = true;
}

//发送数据代码
void USART_Controller::SendByte(uint8_t Byte)
{
    USART_SendData(this->btParams.USART, Byte);
    while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
}

void USART_Controller::SendArray(uint8_t *Array, uint16_t Length)
{
    uint16_t i;
    for (i = 0; i < Length; i++)
    {
        SendByte(*Array++);
    }
}

void USART_Controller::SendString(char *String)
{
    for (int i = 0; String[i] != '\0'; i++)
    {
        SendByte(String[i]);
    }
}

uint32_t USART_Controller::Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--)
    {
        Result *= X;
    }
    return Result;
}

void USART_Controller::SendNumber(uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        SendByte(Number / Pow(10, Length - i - 1) % 10 + 0x30);
    } 
}
