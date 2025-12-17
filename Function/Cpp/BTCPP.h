#pragma once

#include "stm32f10x.h"
#include <stdio.h>
#include <InitArg.h>
#include "USART.h"
#include <cstdarg>
#include <array>
#include <cstring>

// USART 控制类：支持中断接收环形缓冲、阻塞式发送、按行读取、溢出统计
class USART_Controller {
public:
    // 构造函数：指定 USART 外设和接收缓冲区大小（最大 256）
    USART_Controller(USART_TypeDef* USARTx = USART3,
                     BaudRate baudrate      = BaudRate::BAUD_115200,
                     uint16_t buf_size      = 256);

    const USART::USART_Params& GetParams() const { return btParams; }

    // 读取一次 RX 标志，并自动清零（仅表示“发生过接收”，不表示是否存在完整一行）
    bool GetRxFlag() {
        bool flag = static_cast<bool>(isRXNE);
        isRXNE = 0;
        return flag;
    }

    // RX 可读字节数（head/tail 差值）
    uint16_t Available() const;

    // 当前 RX 缓冲区中是否存在一条以 '\n' 结束的完整行
    bool HasLine() const;

    // 清空 RX 缓冲（丢弃所有已接收数据）
    void FlushRx();

    // 按行读取：当且仅当缓冲区内存在 '\n' 时返回 true，并把一行写入 out（不含 '\r'/'\n'，自动 '\0' 结尾）
    // 若一行长度 >= out_max，会截断并丢弃本行剩余部分，truncated=true，同时 lineOverflowCount++。
    bool ReadLine(char* out, uint16_t out_max, bool* truncated = nullptr);

    // 统计信息（便于定位“偶发半包/丢字节”）
    uint32_t GetRxOverflowCount() const { return m_rxOverflowCount; }   // 环形缓冲满导致丢字节
    uint32_t GetRxErrorCount() const    { return m_rxErrorCount; }      // ORE/FE/NE/PE 等错误计数
    uint32_t GetLineOverflowCount() const { return m_lineOverflowCount; } // ReadLine 截断计数
    void ClearStats() { m_rxOverflowCount = m_rxErrorCount = m_lineOverflowCount = 0; }

    bool Start();
    void Continue();
    void Stop();

    // 初始化串口（RCC / GPIO / USART）
    bool Init();

    // 发送原始数据（阻塞式）
    void Send(const uint8_t* data, uint16_t length);
    void Send(const char* str);

    // Printf 风格发送（依赖你已有的 My_USART_Printf）
    template<typename... Args>
    void Printf(const char* format, Args... args) {
        My_USART_Printf(this->btParams.USART, format, args...);
    }

    // 底层写接口：直接发送任意字节流（JSON 等优先用这个）
    void Write(const uint8_t* data, uint16_t length);

    // 从环形接收缓冲区读取最多 max_length 字节（不等待、不保证读到一整行）
    uint16_t Receiver(uint8_t* buffer, uint16_t max_length);

    // 中断处理函数（在 USARTx IRQHandler 中调用）
    void IRQHandler();

    // 重新绑定 USART 和波特率
    void Reset(USART_TypeDef* usartx,
               BaudRate baudrate = BaudRate::BAUD_115200);

    // 传统接口（内部也是基于 Write 实现；保留签名以兼容旧代码）
    void SendByte(uint8_t Byte);
    void SendArray(uint8_t* Array, uint16_t Length);
    void SendString(char* String);
    void SendNumber(uint32_t Number, uint8_t Length);

private:
    // 发生过 RXNE 的标志（volatile，避免优化导致“读不到”）
    volatile uint8_t isRXNE = 0;

    USART::USART_Params btParams{};
    uint32_t m_baudrate = 0;

    // 接收缓冲区（固定 256 字节），实际使用大小由 m_bufferSize 控制
    std::array<uint8_t, 256> m_rxBuffer{};
    uint16_t           m_bufferSize = 256;
    volatile uint16_t  m_rxHead  = 0;
    volatile uint16_t  m_rxTail  = 0;

    // 统计：RX 丢字节 / RX 错误 / 行截断
    volatile uint32_t m_rxOverflowCount = 0;
    volatile uint32_t m_rxErrorCount    = 0;
    volatile uint32_t m_lineOverflowCount = 0;

    void RCC_Config();
    void GPIO_Config();
    void USART_Config();

    static uint16_t IncIndex(uint16_t idx, uint16_t mod) {
        ++idx;
        return (idx >= mod) ? 0 : idx;
    }

    uint32_t Pow(uint32_t X, uint32_t Y);
};

// 单例接口：和你原有 GetStaticBt 一致
inline USART_Controller& GetStaticBt(USART_TypeDef* USARTx = USART3) {
    static USART_Controller instance(USARTx, BaudRate::BAUD_115200);
    return instance;
}

// 中断封装：在 NVIC 中断向量里直接调用 BT_IRQHandler()
inline void BT_IRQHandler() {
    GetStaticBt().IRQHandler();
}
