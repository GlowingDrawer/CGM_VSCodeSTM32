#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "BTCPP.h"
#include "USART.h"
#include "MyDAC.h"
#include "DACManager.h"
#include "ADCManager.h"
#include "InitArg.h"


#define WE_uA_Port GPIO_Pin_1       // GPIOA_Pin_1
#define WE_mA_Port GPIO_Pin_2       // GPIOA_Pin_2


char * ToJsonString(char * buffer, uint16_t buffer_size, const char * key, const char * value);
char * ToJsonString(char * buffer, uint16_t buffer_size, const char * key, const uint16_t value);
// 四舍五入保留3位小数
float round_to_three_decimal(float value);

