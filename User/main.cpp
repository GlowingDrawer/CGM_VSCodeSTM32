#include "main.h"
#include <CustomWaveCPP.h>
#include <ShowCPP.h>
#include <BTCPP.h>
#include <functional>
#include <cJson.h>

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

        // 每200ms切换一次动画帧，同一行刷新
        // bt.Printf("\r%s", animStr[waitAnimCount % 4]);
        // waitAnimCount++;
        Delay_ms(1000); // 控制动画刷新速度
    }

    const auto & adc = NS_DAC::GetADCRef();

    // 获取DAC即将发送的电压值（12Bit）
    const float& currentValRef12Bit = NS_DAC::GetCvValToSendRef();
    // ADC DMA缓冲区引用
    const auto & current16BitBuf = adc.GetDmaBufferRef();
    // ADC DMA缓冲区数据计算所得的电流引用（单位：mA）
    // 当前缓冲区[1]对应的电流值
    const auto & currentValBuf = adc.GetCurrentBufRef();
    cJSON * jsonRoot = cJSON_CreateObject();
    const auto & sysUpdataTimes = NS_DAC::SystemController::GetInstance().GetUpdataTimes();
    cJSON_AddNumberToObject(jsonRoot, "Seconds", 1);
    cJSON_AddNumberToObject(jsonRoot, "Uric", current16BitBuf[0]);
    cJSON_AddNumberToObject(jsonRoot, "Ascorbic", current16BitBuf[1]);
    cJSON_AddNumberToObject(jsonRoot, "Glucose", current16BitBuf[2]);
    cJSON_AddNumberToObject(jsonRoot, "Volt", currentValRef12Bit);
    

    // 修改数字
    cJSON *Seconds = cJSON_GetObjectItemCaseSensitive(jsonRoot, "Seconds");
    cJSON *Uric = cJSON_GetObjectItemCaseSensitive(jsonRoot, "Uric");
    cJSON *Ascrobic = cJSON_GetObjectItemCaseSensitive(jsonRoot, "Ascorbic");
    cJSON *Glucose = cJSON_GetObjectItemCaseSensitive(jsonRoot, "Glucose");
    cJSON *Volt = cJSON_GetObjectItemCaseSensitive(jsonRoot, "Volt");

    char msgBuf[64];
    uint16_t delayTimeMs = 50;
    uint32_t seconds = 1.0f;
    float secondPerTimes = delayTimeMs;

    while (1) {
        if (bt.GetRxFlag()) { 
            currentCommand = ProcessCommand(bt);
        }
        if (currentCommand != CommandType::PAUSE) {
            if (cJSON_IsNumber(Seconds)) {
                NS_DAC::SystemController::GetInstance().UpdateTimes();
                seconds = sysUpdataTimes * secondPerTimes;
                Seconds->valuedouble = seconds; // 对于数字，修改valuedouble
                Seconds->valueint = seconds;    // 同时更新valueint
            }
            if (cJSON_IsNumber(Uric)) {
                Uric->valuedouble = current16BitBuf[0]; // 对于数字，修改valuedouble
                Uric->valueint = current16BitBuf[0];    // 同时更新valueint
            }
            if (cJSON_IsNumber(Ascrobic)) {
                Ascrobic->valuedouble = current16BitBuf[1]; // 对于数字，修改valuedouble
                Ascrobic->valueint = current16BitBuf[1];    // 同时更新valueint
            }
            if (cJSON_IsNumber(Glucose)) {
                Glucose->valuedouble = current16BitBuf[2]; // 对于数字，修改valuedouble
                Glucose->valueint = current16BitBuf[2];    // 同时更新valueint
            }
            if (cJSON_IsNumber(Volt)) {
                Volt->valuedouble = (uint16_t)currentValRef12Bit; // 对于数字，修改valuedouble
                Volt->valueint = (uint16_t)currentValRef12Bit;    // 同时更新valueint
            }
            
            char * modified_json = cJSON_PrintUnformatted(jsonRoot);
            bt.Printf("%s\r\n", modified_json);
            free(modified_json);
            modified_json = NULL;
        }
        Delay_ms(200);
    }

    cJSON_Delete(jsonRoot);
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


