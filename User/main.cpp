#include "main.h"
#include "DACManager.h"
#include "ADCManager.h"
#include "BTCPP.h"
#include <cstdio>
#include <cstdint>
#include <cstring>

// ==================== Forward Declarations ====================

static inline void SendJsonLine(
    USART_Controller& usart,
    uint32_t ms,
    uint16_t uric_raw,
    uint16_t ascorbic_raw,
    uint16_t glucose_raw,
    uint16_t code12
);

enum class CommandType {
    START,
    PAUSE,
    RESUME,
    UNKNOWN
};

static int  my_stricmp(const char* s1, const char* s2);
static void trim_inplace(char* s);

// 从 Receiver() 拼“完整一行命令”（支持 \n / \r\n），并额外容错：
// 若发送端不带换行，收到完整的 START/PAUSE/RESUME 也会立即返回。
static bool TryReadCommandLine(USART_Controller& usart, char* outLine, uint16_t outCap);

static CommandType ProcessCommandLine(
    USART_Controller& usart,
    const char* line,
    CommandType lastState
);

// ============================== main ==============================

int main(void) {

    // 【必须添加】初始化系统滴答定时器，建议放在第一行
    SysTickTimer::Init();

    OLED_Init();

    auto& bt = GetStaticBt();
    USART_IRQnManage::Add(bt.GetParams().USART, USART::IT::RXNE, BT_IRQHandler, 1, 3);
    bt.Start();

    CommandType currentCommand = CommandType::UNKNOWN;
    bt.Printf("Waiting For Command... (START)\r\n");

    // 等待 START（更快响应）
    while (currentCommand != CommandType::START) {
        char line[64];
        if (TryReadCommandLine(bt, line, sizeof(line))) {
            currentCommand = ProcessCommandLine(bt, line, currentCommand);
        }
        Delay_ms(20);
    }

    // Show.patched：必须在主循环调用 adc.Service()
    auto& adc = NS_DAC::GetADC();
    const auto& current16BitBuf = adc.GetDmaBufferRef();

    // CV（你现在把它当作 12bit 码值用：0..4095）
    const auto& cvValRef = NS_DAC::GetCvValToSendRef();

    // 系统计数：每调用一次 UpdateTimes() 就 +1（你工程现有逻辑）
    const auto& sysUpdataTimes = NS_DAC::SystemController::GetInstance().GetUpdataTimes();

    const uint16_t sendPeriodMs = 50;  // 20Hz

    // 假设你有一个 GetTick() 返回系统运行的毫秒数
    // 如果没有，可以基于 SysTick 中断实现一个 volatile uint32_t uwTick;
    uint32_t lastReportTime = 0;
    const uint32_t REPORT_INTERVAL = 50; // 50ms

    while (1) {
        // 1) 命令优先处理
        char line[64];
        if (TryReadCommandLine(bt, line, sizeof(line))) {
            currentCommand = ProcessCommandLine(bt, line, currentCommand);
        }

        // 2) 关键：主循环刷新 OLED（Show.patched 需要）
        adc.Service();

        uint32_t now = SysTickTimer::GetTick();
        if (now - lastReportTime >= REPORT_INTERVAL)
        {
            lastReportTime = now;

            if (currentCommand != CommandType::PAUSE)
            {
                NS_DAC::SystemController::GetInstance().UpdateTimes();

                // Ms：整数毫秒（更适合上位机处理）
                uint32_t ms = (uint32_t)sysUpdataTimes * (uint32_t)sendPeriodMs;

                // ADC 原始值
                uint16_t uric_raw     = current16BitBuf[0];
                uint16_t ascorbic_raw = current16BitBuf[1];
                uint16_t glucose_raw  = current16BitBuf[2];

                // 12bit 码值（你当前称为“电压”其实是 code）
                uint16_t code12 = (uint16_t)cvValRef;

                SendJsonLine(bt, ms, uric_raw, ascorbic_raw, glucose_raw, code12);
            }
            
        }
        
        Delay_ms(1);
    }
}

// ==================== Function Definitions ====================

static inline void SendJsonLine(
    USART_Controller& usart,
    uint32_t ms,
    uint16_t uric_raw,
    uint16_t ascorbic_raw,
    uint16_t glucose_raw,
    uint16_t code12
) {
    // 全整数格式化：比浮点 printf 省 ROM/CPU
    char outBuf[160];

    // 字段：Ms + 3路 ADC 原始值 + Code12
    int n = std::snprintf(outBuf, sizeof(outBuf),
        "{\"Ms\":%lu,\"Uric\":%u,\"Ascorbic\":%u,\"Glucose\":%u,\"Code12\":%u}\n",
        (unsigned long)ms,
        (unsigned)uric_raw,
        (unsigned)ascorbic_raw,
        (unsigned)glucose_raw,
        (unsigned)code12
    );

    if (n <= 0) return;

    if (n >= (int)sizeof(outBuf)) {
        // 极端截断：保证换行
        outBuf[sizeof(outBuf) - 2] = '\n';
        outBuf[sizeof(outBuf) - 1] = '\0';
        n = (int)sizeof(outBuf) - 1;
    }

    usart.Write(reinterpret_cast<const uint8_t*>(outBuf), (uint16_t)n);
}

static int my_stricmp(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        char c1 = (*s1 >= 'a' && *s1 <= 'z') ? (*s1 - 'a' + 'A') : *s1;
        char c2 = (*s2 >= 'a' && *s2 <= 'z') ? (*s2 - 'a' + 'A') : *s2;
        if (c1 != c2) return (unsigned char)c1 - (unsigned char)c2;
        ++s1; ++s2;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

static void trim_inplace(char* s) {
    if (!s) return;

    // trim left
    char* p = s;
    while (*p == ' ' || *p == '\t') ++p;
    if (p != s) std::memmove(s, p, std::strlen(p) + 1);

    // trim right
    size_t n = std::strlen(s);
    while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\t')) {
        s[n - 1] = '\0';
        --n;
    }
}

static bool TryReadCommandLine(USART_Controller& usart, char* outLine, uint16_t outCap) {
    static char lineBuf[64];
    static uint16_t lineLen = 0;
    static bool dropping = false;

    uint8_t rxTmp[64];
    uint16_t n = usart.Receiver(rxTmp, (uint16_t)(sizeof(rxTmp) - 1));
    if (n == 0) return false;

    for (uint16_t i = 0; i < n; ++i) {
        char ch = (char)rxTmp[i];

        // 行结束：\r 或 \n
        if (ch == '\r' || ch == '\n') {
            if (dropping) {
                dropping = false;
                lineLen = 0;
                continue;
            }
            if (lineLen == 0) continue;

            lineBuf[lineLen] = '\0';

            if (outCap > 0) {
                std::strncpy(outLine, lineBuf, outCap - 1);
                outLine[outCap - 1] = '\0';
            }

            lineLen = 0;
            return true;
        }

        if (dropping) continue;

        if (lineLen < (uint16_t)(sizeof(lineBuf) - 1)) {
            lineBuf[lineLen++] = ch;
            lineBuf[lineLen] = '\0'; // 便于无换行容错比较

            // 无换行容错：恰好匹配固定命令就立即返回
            if (lineLen == 5) {
                if (my_stricmp(lineBuf, "START") == 0 || my_stricmp(lineBuf, "PAUSE") == 0) {
                    if (outCap > 0) {
                        std::strncpy(outLine, lineBuf, outCap - 1);
                        outLine[outCap - 1] = '\0';
                    }
                    lineLen = 0;
                    return true;
                }
            } else if (lineLen == 6) {
                if (my_stricmp(lineBuf, "RESUME") == 0) {
                    if (outCap > 0) {
                        std::strncpy(outLine, lineBuf, outCap - 1);
                        outLine[outCap - 1] = '\0';
                    }
                    lineLen = 0;
                    return true;
                }
            }
        } else {
            // 行过长：丢弃直到下一次换行
            dropping = true;
            lineLen = 0;
        }
    }

    return false;
}

static CommandType ProcessCommandLine(
    USART_Controller& usart,
    const char* line,
    CommandType lastState
) {
    if (!line || !line[0]) return lastState;

    char cmd[64];
    std::strncpy(cmd, line, sizeof(cmd) - 1);
    cmd[sizeof(cmd) - 1] = '\0';
    trim_inplace(cmd);

    if (my_stricmp(cmd, "START") == 0) {
        usart.Printf("Starting application...\r\n");
        NS_DAC::SystemController::GetInstance().Start();
        return CommandType::START;
    }

    if (my_stricmp(cmd, "PAUSE") == 0) {
        if (lastState != CommandType::START && lastState != CommandType::RESUME) {
            usart.Printf("Error: PAUSE only valid after START/RESUME.\r\n");
            return lastState;
        }
        usart.Printf("Pause command received.\r\n");
        NS_DAC::SystemController::GetInstance().Pause();
        return CommandType::PAUSE;
    }

    if (my_stricmp(cmd, "RESUME") == 0) {
        if (lastState != CommandType::PAUSE) {
            usart.Printf("Resume ignored. Device is not paused.\r\n");
            return lastState;
        }
        usart.Printf("Resume command received.\r\n");
        NS_DAC::SystemController::GetInstance().Resume();
        return CommandType::RESUME;
    }

    usart.Printf("Unknown command: %s. Use START, PAUSE or RESUME.\r\n", cmd);
    return CommandType::UNKNOWN;
}
