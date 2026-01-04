#include <IRQnManage.h>

// ========================= TIM IRQnManage =========================

std::array<TIM_IRQnManage::IRQnStruct, TIM_IRQnManage::Size_TIM> TIM_IRQnManage::Handlers = {
    TIM_IRQnManage::IRQnStruct{TIM1},
    TIM_IRQnManage::IRQnStruct{TIM2},
    TIM_IRQnManage::IRQnStruct{TIM3},
    TIM_IRQnManage::IRQnStruct{TIM4},
    TIM_IRQnManage::IRQnStruct{TIM5},
    TIM_IRQnManage::IRQnStruct{TIM6},
    TIM_IRQnManage::IRQnStruct{TIM7},
    TIM_IRQnManage::IRQnStruct{TIM8}
};

uint8_t TIM_IRQnManage::GetHandlerIndexFromType(TIM_TypeDef *TIMx) {
    uint8_t idx = Size_TIM; // invalid
    for (uint8_t i = 0; i < Handlers.size(); i++) {
        if (TIMx == Handlers[i].TIMx) {
            idx = i;
            break;
        }
    }
    return idx;
}

uint8_t TIM_IRQnManage::GetTimItIndexFromType(TIM::IT tim_it) {
    uint8_t idx = To_uint8(TIM::IT_Index::END); // invalid
    for (uint8_t i = 0; i < TIM::ITMap.size(); i++) {
        if (TIM::ITMap[i] == tim_it) {
            idx = i;
            break;
        }
    }
    return idx;
}

static inline bool IsTimIt_Update(uint16_t it) {
#ifdef TIM_IT_Update
    return it == TIM_IT_Update;
#else
    (void)it; return false;
#endif
}

static inline bool IsTimIt_CC(uint16_t it) {
#ifdef TIM_IT_CC1
    return (it == TIM_IT_CC1) || (it == TIM_IT_CC2) || (it == TIM_IT_CC3) || (it == TIM_IT_CC4);
#else
    (void)it; return false;
#endif
}

static inline bool IsTimIt_TRG_COM(uint16_t it) {
#ifdef TIM_IT_Trigger
    // 触发/通信事件对应的 IT（不同工程可能未用到）
    return (it == TIM_IT_Trigger) || (it == TIM_IT_COM);
#else
    // 仅保留 COM
#ifdef TIM_IT_COM
    return (it == TIM_IT_COM);
#else
    (void)it; return false;
#endif
#endif
}

IRQn TIM_IRQnManage::GetIRQn(TIM_TypeDef *TIMx){
    return TIM::GetIRQn(TIMx);
}

IRQn TIM_IRQnManage::GetIRQn(TIM_TypeDef *TIMx, TIM::IT tim_it){
    // 对 TIM2~TIM7：通常为“单 IRQ 线”，直接走 TIM::GetIRQn
    // 对 TIM1/TIM8：根据 IT 类型选择 UP / CC / TRG_COM / BRK 等 IRQn
    const uint16_t it = To_uint16(tim_it);

#if defined(TIM1) && defined(TIM1_UP_IRQn) && defined(TIM1_CC_IRQn) && defined(TIM1_TRG_COM_IRQn) && defined(TIM1_BRK_IRQn)
    if (TIMx == TIM1) {
        if (IsTimIt_Update(it)) return TIM1_UP_IRQn;
        if (IsTimIt_CC(it))     return TIM1_CC_IRQn;
        if (IsTimIt_TRG_COM(it))return TIM1_TRG_COM_IRQn;
        return TIM1_UP_IRQn; // 默认回到 UP
    }
#endif

#if defined(TIM8) && defined(TIM8_UP_IRQn) && defined(TIM8_CC_IRQn) && defined(TIM8_TRG_COM_IRQn) && defined(TIM8_BRK_IRQn)
    if (TIMx == TIM8) {
        if (IsTimIt_Update(it)) return TIM8_UP_IRQn;
        if (IsTimIt_CC(it))     return TIM8_CC_IRQn;
        if (IsTimIt_TRG_COM(it))return TIM8_TRG_COM_IRQn;
        return TIM8_UP_IRQn;
    }
#endif

    return TIM::GetIRQn(TIMx);
}

bool TIM_IRQnManage::Add(TIM_TypeDef *TIMx,
                         TIM::IT tim_it,
                         void (* func)(void),
                         uint8_t pre_priority,
                         uint8_t sub_priority,
                         FunctionalState state)
{
    // (1) 绑定回调
    bool result = false;
    for (auto& item : TIM_IRQnManage::Handlers) {
        if (item.TIMx == TIMx) {
            const auto it_index = GetTimItIndexFromType(tim_it);
            if (it_index >= To_uint8(TIM::IT_Index::END)) {
                result = false;
                break;
            }
            if (item.itFuncs[it_index] != nullptr) {
                result = false;
                break;
            }
            item.itFuncs[it_index] = func;
            result = true;
            break;
        }
    }

    // (2) 使能/设置优先级（对 TIM1/TIM8：依据 tim_it 选择正确 IRQn）
    TIM_IRQnManage::SetPriority(TIMx, tim_it, pre_priority, sub_priority, state);
    return result;
}


// TIM 分发：遍历 ITMap，发现置位就清除并执行回调
template<TIM::Index tim_index>
static void HandlerTIM_IRQ(TIM_TypeDef* TIMx) {
    for(uint8_t i = 0; i < TIM::ITMap.size(); ++i) {
        const uint16_t tim_it = To_uint16(TIM::ITMap[i]);
        if(TIM_GetITStatus(TIMx, tim_it) != RESET) {
            TIM_ClearITPendingBit(TIMx, tim_it);
            if (auto & func = TIM_IRQnManage::GetHandlers(tim_index).itFuncs[i]) {
                func();
            }
        }
    }
}

// ---- 常规定时器（单 IRQ 线） ----
extern "C" void TIM2_IRQHandler(void) { HandlerTIM_IRQ<TIM::Index::T2>(TIM2); }
extern "C" void TIM3_IRQHandler(void) { HandlerTIM_IRQ<TIM::Index::T3>(TIM3); }
extern "C" void TIM4_IRQHandler(void) { HandlerTIM_IRQ<TIM::Index::T4>(TIM4); }
extern "C" void TIM5_IRQHandler(void) { HandlerTIM_IRQ<TIM::Index::T5>(TIM5); }
extern "C" void TIM6_IRQHandler(void) { HandlerTIM_IRQ<TIM::Index::T6>(TIM6); }
extern "C" void TIM7_IRQHandler(void) { HandlerTIM_IRQ<TIM::Index::T7>(TIM7); }

// ---- TIM1 / TIM8：多 IRQ 线（UP / CC / TRG_COM / BRK），统一分发到同一处理函数 ----
// 说明：你的工程启动文件（startup_stm32f10x_xx.s）里用哪个名字，就需要提供对应 ISR。
// 这里同时提供“拆分向量名”和你原来用的“TIM1_IRQHandler/TIM8_IRQHandler”以兼容不同模板。

// TIM1
extern "C" void TIM1_IRQHandler(void) { HandlerTIM_IRQ<TIM::Index::T1>(TIM1); }

#if defined(TIM1_UP_IRQn)
extern "C" void TIM1_UP_IRQHandler(void) { HandlerTIM_IRQ<TIM::Index::T1>(TIM1); }
#endif
#if defined(TIM1_CC_IRQn)
extern "C" void TIM1_CC_IRQHandler(void) { HandlerTIM_IRQ<TIM::Index::T1>(TIM1); }
#endif
#if defined(TIM1_TRG_COM_IRQn)
extern "C" void TIM1_TRG_COM_IRQHandler(void) { HandlerTIM_IRQ<TIM::Index::T1>(TIM1); }
#endif
#if defined(TIM1_BRK_IRQn)
extern "C" void TIM1_BRK_IRQHandler(void) { HandlerTIM_IRQ<TIM::Index::T1>(TIM1); }
#endif

// TIM8
extern "C" void TIM8_IRQHandler(void) { HandlerTIM_IRQ<TIM::Index::T8>(TIM8); }

#if defined(TIM8_UP_IRQn)
extern "C" void TIM8_UP_IRQHandler(void) { HandlerTIM_IRQ<TIM::Index::T8>(TIM8); }
#endif
#if defined(TIM8_CC_IRQn)
extern "C" void TIM8_CC_IRQHandler(void) { HandlerTIM_IRQ<TIM::Index::T8>(TIM8); }
#endif
#if defined(TIM8_TRG_COM_IRQn)
extern "C" void TIM8_TRG_COM_IRQHandler(void) { HandlerTIM_IRQ<TIM::Index::T8>(TIM8); }
#endif
#if defined(TIM8_BRK_IRQn)
extern "C" void TIM8_BRK_IRQHandler(void) { HandlerTIM_IRQ<TIM::Index::T8>(TIM8); }
#endif


// ========================= DMA IRQnManage =========================

// (IRQnManage.h) 中声明的静态表
const std::array<std::array<uint32_t, DMA_IRQnManage::Size_DMA_IT>, DMA_IRQnManage::Size_DMA> DMA_IRQnManage::dmaITArr = {{
    {{DMA1_IT_TC1, DMA1_IT_HT1, DMA1_IT_TE1}},  // CH1
    {{DMA1_IT_TC2, DMA1_IT_HT2, DMA1_IT_TE2}},  // CH2
    {{DMA1_IT_TC3, DMA1_IT_HT3, DMA1_IT_TE3}},  // CH3
    {{DMA1_IT_TC4, DMA1_IT_HT4, DMA1_IT_TE4}},  // CH4
    {{DMA1_IT_TC5, DMA1_IT_HT5, DMA1_IT_TE5}},  // CH5
    {{DMA1_IT_TC6, DMA1_IT_HT6, DMA1_IT_TE6}},  // CH6
    {{DMA1_IT_TC7, DMA1_IT_HT7, DMA1_IT_TE7}}   // CH7
}};

std::array<DMA_IRQnManage::IRQnStruct, DMA_IRQnManage::Size_DMA> DMA_IRQnManage::Handlers = {
    DMA_IRQnManage::IRQnStruct{DMA1_Channel1},
    DMA_IRQnManage::IRQnStruct{DMA1_Channel2},
    DMA_IRQnManage::IRQnStruct{DMA1_Channel3},
    DMA_IRQnManage::IRQnStruct{DMA1_Channel4},
    DMA_IRQnManage::IRQnStruct{DMA1_Channel5},
    DMA_IRQnManage::IRQnStruct{DMA1_Channel6},
    DMA_IRQnManage::IRQnStruct{DMA1_Channel7}
};

uint8_t DMA_IRQnManage::GetDmaChanIndexFromType(DMA_Channel_TypeDef* dma1_chanx) {
    uint8_t index = Size_DMA; // invalid
    for (uint8_t i = 0; i < Handlers.size(); i++) {
        if (Handlers[i].dma1Chanx == dma1_chanx) {
            index = i;
            break;
        }
    }
    return index;
}

uint8_t DMA_IRQnManage::GetDmaItIndexFromType(DMA::IT dma_it) {
    uint8_t index = Size_DMA_IT; // invalid
    for (uint8_t i = 0; i < DMA::ITMap.size(); i++) {
        if (DMA::ITMap[i] == dma_it) {
            index = i;
            break;
        }
    }
    return index;
}

bool DMA_IRQnManage::Add(DMA_Channel_TypeDef* dma1_chanx,
                         DMA::IT dma_it,
                         void (*func)(void),
                         uint8_t pre_priority,
                         uint8_t sub_priority,
                         FunctionalState state)
{
    const auto chanIndex = DMA_IRQnManage::GetDmaChanIndexFromType(dma1_chanx);
    const auto itIndex   = DMA_IRQnManage::GetDmaItIndexFromType(dma_it);

    if (chanIndex >= Size_DMA || itIndex >= Size_DMA_IT) return false;

    if (DMA_IRQnManage::Handlers[chanIndex].itFuncs[itIndex] != nullptr) return false;

    DMA_IRQnManage::Handlers[chanIndex].itFuncs[itIndex] = func;
    DMA_IRQnManage::SetPriority(dma1_chanx, dma_it, pre_priority, sub_priority, state);
    return true;
};

IRQn DMA_IRQnManage::GetIRQn(DMA_Channel_TypeDef* dma1_chanx) {
    return DMA::GetDmaIRQn(dma1_chanx);
}

template<DMA::ChanIndex dma_index>
static void HandlerDMA_IRQ() {
    const auto index = To_uint8(dma_index);
    for (uint8_t i = 0; i < To_uint8(DMA::IT_Index::END); ++i) {
        const auto dma_it = DMA_IRQnManage::dmaITArr[index][i];
        if (DMA_GetITStatus(dma_it)) {
            DMA_ClearITPendingBit(dma_it);
            if (auto& func = DMA_IRQnManage::GetHandlers(dma_index).itFuncs[i]) {
                func();
            }
        }
    }
}

extern "C" void DMA1_Channel1_IRQHandler(void) { HandlerDMA_IRQ<DMA::ChanIndex::D1CH1>(); }
extern "C" void DMA1_Channel2_IRQHandler(void) { HandlerDMA_IRQ<DMA::ChanIndex::D1CH2>(); }
extern "C" void DMA1_Channel3_IRQHandler(void) { HandlerDMA_IRQ<DMA::ChanIndex::D1CH3>(); }
extern "C" void DMA1_Channel4_IRQHandler(void) { HandlerDMA_IRQ<DMA::ChanIndex::D1CH4>(); }
extern "C" void DMA1_Channel5_IRQHandler(void) { HandlerDMA_IRQ<DMA::ChanIndex::D1CH5>(); }
extern "C" void DMA1_Channel6_IRQHandler(void) { HandlerDMA_IRQ<DMA::ChanIndex::D1CH6>(); }
extern "C" void DMA1_Channel7_IRQHandler(void) { HandlerDMA_IRQ<DMA::ChanIndex::D1CH7>(); }


// ========================= USART IRQnManage =========================

std::array<USART_IRQnManage::IRQnStruct, USART_IRQnManage::Size_USART>
USART_IRQnManage::Handlers = {
    USART_IRQnManage::IRQnStruct{USART1},
    USART_IRQnManage::IRQnStruct{USART2},
    USART_IRQnManage::IRQnStruct{USART3}
};

bool USART_IRQnManage::Add(USART_TypeDef* USARTx,
                           USART::IT usart_it,
                           void (*func)(void),
                           uint8_t pre_priority,
                           uint8_t sub_priority,
                           FunctionalState state)
{
    bool result = false;
    for (auto& item : Handlers) {
        if (item.USARTx == USARTx) {
            const auto it_index = GetUsartItIndexFromType(usart_it);
            if (it_index >= To_uint8(USART::IT_Index::END)) break;
            if (item.itFuncs[it_index]) break;
            item.itFuncs[it_index] = func;
            result = true;
            break;
        }
    }
    SetPriority(USARTx, usart_it, pre_priority, sub_priority, state);
    return result;
}

uint8_t USART_IRQnManage::GetUsartIndexFromType(USART_TypeDef* USARTx) {
    return USART::GetIndexFromType(USARTx);
}

uint8_t USART_IRQnManage::GetUsartItIndexFromType(USART::IT usart_it) {
    return USART::GetIT_IndexFromType(usart_it);
}

IRQn USART_IRQnManage::GetIRQn(USART_TypeDef* USARTx) {
    return USART::GetUSART_IRQnFromType(USARTx);
}

static inline void Usart_ClearIdle(USART_TypeDef* usart) {
    // 读 SR 再读 DR 清除 IDLE（也会顺带处理部分错误标志）
    volatile uint32_t tmp = usart->SR;
    tmp = usart->DR;
    (void)tmp;
}

static inline void Usart_ClearErr(USART_TypeDef* usart) {
    // ORE/NE/FE/PE：典型清除序列也是 SR -> DR
    volatile uint32_t tmp = usart->SR;
    tmp = usart->DR;
    (void)tmp;
}

template<USART::Index usart_index>
static void HandleUSART_IRQ(USART_TypeDef* usart)
{
    for (uint8_t i = 0; i < static_cast<uint8_t>(USART::IT_Index::END); ++i) {
        const uint16_t usart_it = static_cast<uint16_t>(USART::ITMap[i]);

        if (USART_GetITStatus(usart, usart_it) == RESET) continue;

        // 1) RXNE：不要 ClearITPendingBit，读取 DR 才能清
        if (usart_it == USART_IT_RXNE) {
            if (auto& func = USART_IRQnManage::GetHandlers(usart_index).itFuncs[i]) {
                func(); // 期望 func 内部读取 USART_ReceiveData()
            } else {
                (void)USART_ReceiveData(usart);
            }
            continue;
        }

        // 2) IDLE：必须 SR->DR 清除（ClearITPendingBit 不可靠）
#ifdef USART_IT_IDLE
        if (usart_it == USART_IT_IDLE) {
            Usart_ClearIdle(usart);
            if (auto& func = USART_IRQnManage::GetHandlers(usart_index).itFuncs[i]) func();
            continue;
        }
#endif

        // 3) 错误类（ORE/NE/FE/PE）：SR->DR 清除
#ifdef USART_IT_ORE
        if (usart_it == USART_IT_ORE) {
            Usart_ClearErr(usart);
            if (auto& func = USART_IRQnManage::GetHandlers(usart_index).itFuncs[i]) func();
            continue;
        }
#endif
#ifdef USART_IT_NE
        if (usart_it == USART_IT_NE) {
            Usart_ClearErr(usart);
            if (auto& func = USART_IRQnManage::GetHandlers(usart_index).itFuncs[i]) func();
            continue;
        }
#endif
#ifdef USART_IT_FE
        if (usart_it == USART_IT_FE) {
            Usart_ClearErr(usart);
            if (auto& func = USART_IRQnManage::GetHandlers(usart_index).itFuncs[i]) func();
            continue;
        }
#endif
#ifdef USART_IT_PE
        if (usart_it == USART_IT_PE) {
            Usart_ClearErr(usart);
            if (auto& func = USART_IRQnManage::GetHandlers(usart_index).itFuncs[i]) func();
            continue;
        }
#endif

        // 4) TXE：通常由“写 DR / 关闭 TXE 中断”结束，不要依赖 ClearITPendingBit
#ifdef USART_IT_TXE
        if (usart_it == USART_IT_TXE) {
            if (auto& func = USART_IRQnManage::GetHandlers(usart_index).itFuncs[i]) func();
            continue;
        }
#endif

        // 5) 其他：按你原逻辑清 pending bit 再回调
        USART_ClearITPendingBit(usart, usart_it);
        if (auto& func = USART_IRQnManage::GetHandlers(usart_index).itFuncs[i]) {
            func();
        }
    }
}

extern "C" void USART1_IRQHandler(void) { HandleUSART_IRQ<USART::Index::US1>(USART1); }
extern "C" void USART2_IRQHandler(void) { HandleUSART_IRQ<USART::Index::US2>(USART2); }
extern "C" void USART3_IRQHandler(void) { HandleUSART_IRQ<USART::Index::US3>(USART3); }
