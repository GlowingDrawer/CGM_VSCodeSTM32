#include "SysTickTimer.h"

// 1) 静态成员定义
volatile uint32_t SysTickTimer::msTicks = 0;

// 2) C++ 接口实现
void SysTickTimer::Init() {
    // SystemCoreClock 通常在 system_stm32f10x.c 中定义
    // 配置 SysTick 为 1ms 中断一次
    if (SysTick_Config(SystemCoreClock / 1000U)) {
        // 初始化失败：通常是 SystemCoreClock 未正确配置
        while (1) {
        }
    }
}

uint32_t SysTickTimer::GetTick() {
    return msTicks;
}

void SysTickTimer::DelayMs(uint32_t ms) {
    const uint32_t start = GetTick();
    // 利用无符号溢出回绕特性，差值比较是安全的
    while ((GetTick() - start) < ms) {
        // busy wait
    }
}

void SysTickTimer::IncTick() {
    ++msTicks;
}

// 3) C 接口实现
extern "C" {

void SysTickTimer_Init_C(void) {
    SysTickTimer::Init();
}

uint32_t SysTickTimer_GetTick_C(void) {
    return SysTickTimer::GetTick();
}

void SysTickTimer_DelayMs_C(uint32_t ms) {
    SysTickTimer::DelayMs(ms);
}

void C_Delay_ms(uint32_t ms) {
    SysTickTimer::DelayMs(ms);
}

// 4) SysTick ISR
void SysTick_Handler(void) {
    SysTickTimer::IncTick();
}

} // extern "C"
