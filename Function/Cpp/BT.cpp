#include "stm32f10x.h"      // Device header
#include "BTCPP.h"
#include "GPIO.h"
#include <cstring>

// ===== 构造函数 & 复位配置 =====
USART_Controller::USART_Controller(USART_TypeDef* usartx,
                                   BaudRate baudrate,
                                   uint16_t buffer_size)
    : m_baudrate(static_cast<uint32_t>(baudrate)),
      m_bufferSize(buffer_size),
      m_rxHead(0),
      m_rxTail(0) {
    // 限制 m_bufferSize 不超过物理缓冲区大小
    if (m_bufferSize == 0 || m_bufferSize > m_rxBuffer.size()) {
        m_bufferSize = static_cast<uint16_t>(m_rxBuffer.size());
    }
    this->Reset(usartx, baudrate);
}

void USART_Controller::Reset(USART_TypeDef* usartx, BaudRate baudrate) {
    bool found = false;
    for (auto& item : USART::USART_ConfigMap) {
        if (item.USART == usartx) {
            btParams          = item;
            btParams.baudrate = static_cast<uint32_t>(baudrate);
            m_baudrate        = btParams.baudrate;
            found             = true;
            break;
        }
    }

    // 若未命中配置表，至少保留 USART 指针与波特率，Init() 会据此返回 false/true
    if (!found) {
        btParams.USART     = usartx;
        btParams.baudrate  = static_cast<uint32_t>(baudrate);
        m_baudrate         = btParams.baudrate;
    }
}

// ===== 启停控制 =====
bool USART_Controller::Start() {
    bool result = this->Init();
    this->Continue();
    return result;
}

void USART_Controller::Continue() {
    // 建议：NVIC 优先级/使能可在外部统一配置；这里仅开启 RXNE
    USART_ITConfig(btParams.USART, USART_IT_RXNE, ENABLE);
    USART_Cmd(btParams.USART, ENABLE);
}

void USART_Controller::Stop() {
    USART_ITConfig(btParams.USART, USART_IT_RXNE, DISABLE);
    USART_Cmd(btParams.USART, DISABLE);
}

// ===== 初始化：RCC / GPIO / USART =====
bool USART_Controller::Init() {
    if (m_bufferSize == 0) return false;
    if (btParams.USART == nullptr) return false;

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
    // 你当前工程用 GPIOA_Init / GPIOB_Init 封装（保持一致）
    if (this->btParams.GPIOx == GPIOA) {
        GPIOA_Init(this->btParams.USART_TX, GPIO_Mode_AF_PP, GPIO_Speed_50MHz);
        GPIOA_Init(this->btParams.USART_RX, GPIO_Mode_IPU, GPIO_Speed_50MHz);
    } else if (this->btParams.GPIOx == GPIOB) {
        GPIOB_Init(this->btParams.USART_TX, GPIO_Mode_AF_PP, GPIO_Speed_50MHz);
        GPIOB_Init(this->btParams.USART_RX, GPIO_Mode_IPU, GPIO_Speed_50MHz);
    } else {
        // 若后续用到 GPIOC 等，可在这里补一个通用 GPIO_InitTypeDef 分支
        GPIO_InitTypeDef gpio{};
        gpio.GPIO_Speed = GPIO_Speed_50MHz;

        gpio.GPIO_Pin  = this->btParams.USART_TX;
        gpio.GPIO_Mode = GPIO_Mode_AF_PP;
        GPIO_Init(this->btParams.GPIOx, &gpio);

        gpio.GPIO_Pin  = this->btParams.USART_RX;
        gpio.GPIO_Mode = GPIO_Mode_IPU;
        GPIO_Init(this->btParams.GPIOx, &gpio);
    }
}

void USART_Controller::USART_Config() {
    USART_InitTypeDef USART_InitStruct{};
    USART_InitStruct.USART_BaudRate            = btParams.baudrate;
    USART_InitStruct.USART_WordLength          = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits            = USART_StopBits_1;
    USART_InitStruct.USART_Parity              = USART_Parity_No;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;

    USART_Init(btParams.USART, &USART_InitStruct);
}

// ===== 发送相关 =====

// 统一的底层写函数：阻塞式发送任意字节流
void USART_Controller::Write(const uint8_t* data, uint16_t length) {
    if (data == nullptr || length == 0 || btParams.USART == nullptr) {
        return;
    }

    for (uint16_t i = 0; i < length; ++i) {
        while (USART_GetFlagStatus(this->btParams.USART, USART_FLAG_TXE) == RESET) {
            // busy wait
        }
        USART_SendData(this->btParams.USART, data[i]);
    }

    // 等待本帧完全发送出去（避免上位机/蓝牙端“尾部截断”的偶发问题）
    while (USART_GetFlagStatus(this->btParams.USART, USART_FLAG_TC) == RESET) {
        // wait
    }
}

void USART_Controller::Send(const uint8_t* data, uint16_t length) {
    Write(data, length);
}

void USART_Controller::Send(const char* str) {
    if (!str) return;
    Write(reinterpret_cast<const uint8_t*>(str),
          static_cast<uint16_t>(strlen(str)));
}

// 原有接口封装到 Write 上
void USART_Controller::SendByte(uint8_t Byte) {
    Write(&Byte, 1);
}

void USART_Controller::SendArray(uint8_t* Array, uint16_t Length) {
    Write(Array, Length);
}

void USART_Controller::SendString(char* String) {
    if (!String) return;
    Send(String);  // 复用 const char* 版本
}

// ===== 接收：基础读 / 按行读 =====

uint16_t USART_Controller::Available() const {
    uint16_t head = m_rxHead;
    uint16_t tail = m_rxTail;
    uint16_t size = m_bufferSize;

    if (size == 0) return 0;
    if (head >= tail) return static_cast<uint16_t>(head - tail);
    return static_cast<uint16_t>(size - (tail - head));
}

bool USART_Controller::HasLine() const {
    uint16_t head = m_rxHead;
    uint16_t tail = m_rxTail;
    const uint16_t size = m_bufferSize;
    if (size == 0) return false;

    uint16_t idx = tail;
    while (idx != head) {
        if (m_rxBuffer[idx] == '\n') return true;
        idx = IncIndex(idx, size);
    }
    return false;
}

void USART_Controller::FlushRx() {
    m_rxTail = m_rxHead;
    isRXNE = 0;
}

bool USART_Controller::ReadLine(char* out, uint16_t out_max, bool* truncated) {
    if (truncated) *truncated = false;
    if (!out || out_max < 2) return false;

    // 没有完整行就不消费数据，避免“半包”把命令拆碎
    if (!HasLine()) return false;

    uint16_t head = m_rxHead;
    uint16_t tail = m_rxTail;
    const uint16_t size = m_bufferSize;

    uint16_t w = 0;
    bool cut = false;

    while (tail != head) {
        char c = static_cast<char>(m_rxBuffer[tail]);
        tail = IncIndex(tail, size);

        if (c == '\n') {
            break; // 一行结束
        }
        if (c == '\r') {
            continue; // 兼容 CRLF
        }

        if (w < static_cast<uint16_t>(out_max - 1)) {
            out[w++] = c;
        } else {
            // out 空间不足：丢弃本行剩余部分直到 '\n'
            cut = true;
            while (tail != head) {
                char cc = static_cast<char>(m_rxBuffer[tail]);
                tail = IncIndex(tail, size);
                if (cc == '\n') break;
            }
            break;
        }
    }

    out[w] = '\0';
    m_rxTail = tail;

    if (cut) {
        ++m_lineOverflowCount;
        if (truncated) *truncated = true;
    }
    return true;
}

uint16_t USART_Controller::Receiver(uint8_t* buffer, uint16_t max_length) {
    if (!buffer || max_length == 0) return 0;

    uint16_t bytesRead = 0;
    uint16_t head = m_rxHead;
    uint16_t tail = m_rxTail;
    const uint16_t size = m_bufferSize;

    while ((head != tail) && (bytesRead < max_length)) {
        buffer[bytesRead++] = m_rxBuffer[tail];
        tail = IncIndex(tail, size);
    }
    m_rxTail = tail;
    return bytesRead;
}

// ===== 中断 =====
void USART_Controller::IRQHandler() {
    if (btParams.USART == nullptr) return;

    // 先处理错误（FE/NE/ORE/PE），避免卡死在错误状态
    // 读 SR + DR 可清错误相关标志（StdPeriph 的建议做法）
    if (USART_GetFlagStatus(btParams.USART, USART_FLAG_ORE) != RESET ||
        USART_GetFlagStatus(btParams.USART, USART_FLAG_FE)  != RESET ||
        USART_GetFlagStatus(btParams.USART, USART_FLAG_NE)  != RESET ||
        USART_GetFlagStatus(btParams.USART, USART_FLAG_PE)  != RESET) {

        volatile uint16_t sr = btParams.USART->SR;
        (void)sr;
        volatile uint16_t dr = btParams.USART->DR;
        (void)dr;
        ++m_rxErrorCount;
        // 不 return：可能同一时刻也有 RXNE
    }

    if (USART_GetITStatus(btParams.USART, USART_IT_RXNE) != RESET) {
        uint8_t data = static_cast<uint8_t>(USART_ReceiveData(btParams.USART) & 0xFF);

        uint16_t head = m_rxHead;
        uint16_t next = IncIndex(head, m_bufferSize);

        if (next != m_rxTail) {
            m_rxBuffer[head] = data;
            m_rxHead = next;
        } else {
            // 缓冲区满：丢弃该字节并统计
            ++m_rxOverflowCount;
        }
        isRXNE = 1;
        // RXNE 标志由读 DR 自动清除；此处无需 ClearITPendingBit
    }
}

// ===== 工具函数 =====
uint32_t USART_Controller::Pow(uint32_t X, uint32_t Y) {
    uint32_t Result = 1;
    while (Y--) {
        Result *= X;
    }
    return Result;
}

void USART_Controller::SendNumber(uint32_t Number, uint8_t Length) {
    for (uint8_t i = 0; i < Length; i++) {
        SendByte(static_cast<uint8_t>(
            Number / Pow(10, Length - i - 1) % 10 + 0x30));
    }
}
