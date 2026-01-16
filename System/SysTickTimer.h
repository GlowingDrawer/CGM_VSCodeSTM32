#pragma once

#include "stm32f10x.h"
#include <stdint.h>

// ============================================================
// SysTickTimer
// - C 文件（.c）：只能使用下方 C 接口（避免包含 class/namespace 等 C++ 语法）
// - C++ 文件（.cpp）：可直接使用 SysTickTimer class，也可用 C 接口
// ============================================================

#ifdef __cplusplus

class SysTickTimer {
private:
    // 毫秒计数（必须 volatile，防止编译器优化）
    static volatile uint32_t msTicks;

public:
    // 初始化 SysTick：1ms 中断一次
    static void Init();

    // 获取当前毫秒 tick
    static uint32_t GetTick();

    // 阻塞延时（不要再使用 Delay_ms/Delay_us 这类会重配 SysTick 的函数）
    static void DelayMs(uint32_t ms);

    // 供 SysTick_Handler 调用
    static void IncTick();
};

#endif // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

// C 接口：供 .c 文件使用
void SysTickTimer_Init_C(void);
uint32_t SysTickTimer_GetTick_C(void);
void SysTickTimer_DelayMs_C(uint32_t ms);

// 兼容你现有 Key.c 的调用方式
void C_Delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif
