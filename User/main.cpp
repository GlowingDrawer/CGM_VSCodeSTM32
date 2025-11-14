#include "main.h"
#include <CustomWaveCPP.h>
#include <ShowCPP.h>
#include <BTCPP.h>
#include <functional>
// #include <cJson.h>


// ------ 轻量JSON发送：零分配、每行一帧，末尾\n -------------
static inline void SendJsonLine(
    USART_Controller& usart,
    float seconds,
    uint16_t uric_raw,
    uint16_t ascorbic_raw,
    uint16_t glucose_raw,
    float volt
) {
    // 估算：一行最长 < 128 字符，留冗余
    // 形如：{"Seconds":12.345,"Uric":1234,"Ascorbic":567,"Glucose":890,"Volt":1.2345}\n
    char outBuf[160];

    // 数值保留位数可按需要调整：Seconds 3位小数、Volt 4位
    // 注意：统一用英文小数点（snprintf 默认如此）
    int n = snprintf(outBuf, sizeof(outBuf),
        "{\"Seconds\":%.3f,\"Uric\":%u,\"Ascorbic\":%u,\"Glucose\":%u,\"Volt\":%.4f}\n",
        seconds, (unsigned)uric_raw, (unsigned)ascorbic_raw, (unsigned)glucose_raw, volt
    );

    if (n <= 0) return;                  // 格式化失败（极少见）
    if (n >= (int)sizeof(outBuf)) {      // 极端异常：被截断；保证有换行，仍尝试发送
        outBuf[sizeof(outBuf) - 2] = '\n';
        outBuf[sizeof(outBuf) - 1] = '\0';
        n = sizeof(outBuf) - 1;
    }

    // 建议优先用 Write 直接发原始字节（最快），若没有 Write 就用 Printf("%s")
    // 版本 A：原始发送（若你的 BT 控制器有 Write 接口，推荐）
    // usart.Write((uint8_t*)outBuf, (uint16_t)n);

    // 版本 B：兼容你现有的 bt.Printf（最小改动）
    usart.Printf("%s", outBuf);
}


enum class CommandType {
    START,
    PAUSE,
    RESUME,
    UNKNOWN
};

// 处理来自蓝牙的信号
CommandType ProcessCommand(USART_Controller& usart);

// 定义等待动画的状态变量
uint8_t waitAnimCount = 0;
const char* animStr[4] = {"Waiting For Command   ", 
                          "Waiting For Command.  ", 
                          "Waiting For Command.. ", 
                          "Waiting For Command..."};

int main(void){
    OLED_Init();
    std::function<void()> a;

    auto & bt = GetStaticBt();

    USART_IRQnManage::Add(bt.GetParams().USART, USART::IT::RXNE, BT_IRQHandler, 1, 3);
    bt.Start();

    // 等待启动指令
    CommandType currentCommand = CommandType::UNKNOWN;
    bt.Printf("Waiting For Command...\r\n");
    while (currentCommand != CommandType::START)
    {
        if (bt.GetRxFlag()) {
            currentCommand = ProcessCommand(bt);
        }
        Delay_ms(1000);
    }

    const auto & adc = NS_DAC::GetADCRef();

    // 获取 DAC 即将发送的电压（建议保持为“伏特/或同上位机的单位”）
    const float& currentValRef12Bit = NS_DAC::GetCvValToSendRef();
    // ADC DMA 缓冲区（原始整数）
    const auto & current16BitBuf = adc.GetDmaBufferRef();
    // 系统更新计数
    const auto & sysUpdataTimes = NS_DAC::SystemController::GetInstance().GetUpdataTimes();

    // —— 时间与延时设定（保持一致）——
    uint16_t delayTimeMs = 100;                         // 每帧 100ms
    const float secondPerTimesMs = delayTimeMs / 1.0f; // 100ms/次
    float secondsMs = 0.0f;

    while (1) {
        if (bt.GetRxFlag()) {
            currentCommand = ProcessCommand(bt);
        }

        if (currentCommand != CommandType::PAUSE) {
            // 更新时间（按系统计数 × 周期，或换成更精准的硬件时间戳）
            NS_DAC::SystemController::GetInstance().UpdateTimes();
            secondsMs = sysUpdataTimes * secondPerTimesMs;

            // 读取一次当前数据（注意越界保护）
            uint16_t uric_raw      = current16BitBuf[0];
            uint16_t ascorbic_raw  = current16BitBuf[1];
            uint16_t glucose_raw   = current16BitBuf[2];
            float    volt_value    = currentValRef12Bit; // 别再强转 uint16_t 了

            // 可选：简单“数值 sanity 检查”，避免把异常/NaN 发送到上位机
            auto sane = [](float x){ return (x == x) && (x > -1e6f) && (x < 1e6f); };
            if (!sane(volt_value)) {
                // 跳过此帧或用上一次正常值（自行选择策略）
                // 这里选择跳过
                goto SLEEP_AND_NEXT;
            }

            // —— 发送“每行一帧”的 JSON（零分配）——
            SendJsonLine(bt, secondsMs, uric_raw, ascorbic_raw, glucose_raw, volt_value);
        }

SLEEP_AND_NEXT:
        Delay_ms(delayTimeMs);
    }
}


char * ToJsonString(char * buffer, uint16_t buffer_size, const char * key, const char * value)  {
    snprintf(buffer, buffer_size, "\"%s\":\"%s\"", key, value);
    return buffer;
}

char * ToJsonString(char * buffer, uint16_t buffer_size, const char * key, const uint16_t value)  {
    snprintf(buffer, buffer_size, "\"%s\":\"%d\"", key, value);
    return buffer;
}

// 四舍五入保留3位小数
float round_to_three_decimal(float value) {
    return (float)((int)(value * 1000)) / 1000;
}


CommandType ProcessCommand(USART_Controller& usart){
    uint8_t commandBuf[64] = {0};
    uint16_t len = usart.Receiver(commandBuf, 63);
    static CommandType commandType = CommandType::UNKNOWN;
    if (len == 0) {
        usart.Printf("No data received.\r\n");
    } else {
        // 移除字符串末尾的换行符（增强兼容性）
        for (uint16_t i = 0; i < len; i++) {
            if (commandBuf[i] == '\r' || commandBuf[i] == '\n') {
                commandBuf[i] = '\0';
                len = i;  // 更新有效长度
                break;
            }
        }
        // 不区分大小写比较（增强容错性）
        if (strcasecmp((char*)commandBuf, "START") == 0) {
            usart.Printf("Starting application...\r\n");
            commandType = CommandType::START;
            NS_DAC::SystemController::GetInstance().Start();
        } else if (strcasecmp((char*)commandBuf, "PAUSE") == 0) {
            usart.Printf("Pause command received.\r\n");
            if (commandType != CommandType::START && commandType != CommandType::RESUME)
            {
                usart.Printf("Error: Command type is not START or RESUME.\r\n");
            } else {
                commandType = CommandType::PAUSE;
                NS_DAC::SystemController::GetInstance().Pause();
            }
        } else if (strcasecmp((char*)commandBuf, "RESUME") == 0) {
            usart.Printf("Resume command received.\r\n");
            if (commandType != CommandType::PAUSE) {
                usart.Printf("Resume command ignored. Device is not paused.\r\n");
            } else {
                commandType = CommandType::RESUME;
                NS_DAC::SystemController::GetInstance().Resume();
            }
        } else {
            usart.Printf("Unknown command: %s. Use START, PAUSE or RESUME.\r\n", commandBuf);
            commandType = CommandType::UNKNOWN;
        }
    }

    return commandType;
}


