#include <IRQnManage.h>

// bool MyNVIC_SetPriority(IRQn_Type irqn, uint8_t pre_priority, uint8_t sub_priority, FunctionalState state) {
//     return  MyNVIC::SetPriority(irqn, pre_priority, sub_priority, state);
// }

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

const uint8_t TIM_IRQnManage::GetHandlerIndexFromType(TIM_TypeDef *TIMx) {
    uint8_t idx;
    for (auto i = 0; i < Handlers.size(); i++) {
        if (TIMx == Handlers[i].TIMx) {
            idx = i;
            break;
        }
    }
    return idx;
}

const uint8_t TIM_IRQnManage::GetTimItIndexFromType(TIM::IT tim_it) {
    uint8_t idx;
    for (auto i = 0; i < TIM::ITMap.size(); i++) {
        if (TIM::ITMap[i] == tim_it) {
            idx = i;
            break;
        }
    }
    return idx;
}

bool TIM_IRQnManage::Add(TIM_TypeDef *TIMx, TIM::IT tim_it, void (* func)(void), uint8_t pre_priority, uint8_t sub_priority, FunctionalState state) {
    bool result = true;
    for (auto& item : TIM_IRQnManage::Handlers) {
        if (item.TIMx == TIMx) {
            const auto it_index = GetTimItIndexFromType(tim_it);
            if (item.itFuncs[it_index] != nullptr) {
                result = false;
                break;
            } else {
                item.itFuncs[it_index] = func;
            }
            break;
        }
    }
    TIM_IRQnManage::SetPriority(TIMx, tim_it, pre_priority, sub_priority, state);
    return result;
}

IRQn TIM_IRQnManage::GetIRQn(TIM_TypeDef *TIMx){
    return TIM::GetIRQn(TIMx);
}

void TempShowMsg(const char * msg = "Hello") {
    OLED_ShowString(4, 1, msg);
}

template<TIM::Index tim_index>
void HandlerTIM_IRQ(TIM_TypeDef* TIMx) {
    for(uint8_t i = 0; i < TIM::ITMap.size(); ++i) {
        if(TIM_GetITStatus(TIMx, To_uint16(TIM::ITMap[i])) != RESET) {
            TIM_ClearITPendingBit(TIMx, To_uint16(TIM::ITMap[i]));
            if (auto & func = TIM_IRQnManage::GetHandlers(tim_index).itFuncs[i]) {
                func();
            }
        };
    }
}

extern "C" void TIM1_IRQHandler(void) {
    HandlerTIM_IRQ<TIM::Index::T1>(TIM1);
}

extern "C" void TIM2_IRQHandler(void) {
    HandlerTIM_IRQ<TIM::Index::T2>(TIM2);
}

extern "C" void TIM3_IRQHandler(void) {
    HandlerTIM_IRQ<TIM::Index::T3>(TIM3);
    // JustShow("Hellos", 4, 6);
}

extern "C" void TIM4_IRQHandler(void) {
   HandlerTIM_IRQ<TIM::Index::T4>(TIM4);
}

extern "C" void TIM5_IRQHandler(void) { 
    HandlerTIM_IRQ<TIM::Index::T5>(TIM5);
}

extern "C" void TIM6_IRQHandler(void) { 
    HandlerTIM_IRQ<TIM::Index::T6>(TIM6);
}

extern "C" void TIM7_IRQHandler(void) { 
    HandlerTIM_IRQ<TIM::Index::T7>(TIM7);
}

extern "C" void TIM8_IRQHandler(void) { 
    HandlerTIM_IRQ<TIM::Index::T8>(TIM8);
}

// 在DMA_IRQnManage类中定义(IRQnManage.h)
const std::array<std::array<uint32_t, DMA_IRQnManage::Size_DMA_IT>, DMA_IRQnManage::Size_DMA> DMA_IRQnManage::dmaITArr = {{
    {{DMA1_IT_TC1, DMA1_IT_HT1, DMA1_IT_TE1}},  // D1
    {{DMA1_IT_TC2, DMA1_IT_HT2, DMA1_IT_TE2}},  // D2
    {{DMA1_IT_TC3, DMA1_IT_HT3, DMA1_IT_TE3}},  // D3
    {{DMA1_IT_TC4, DMA1_IT_HT4, DMA1_IT_TE4}},  // D4
    {{DMA1_IT_TC5, DMA1_IT_HT5, DMA1_IT_TE5}},  // D5
    {{DMA1_IT_TC6, DMA1_IT_HT6, DMA1_IT_TE6}},  // D6
    {{DMA1_IT_TC7, DMA1_IT_HT7, DMA1_IT_TE7}}   // D7
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
    uint8_t index = Size_DMA;
    for (auto i = 0; i < Handlers.size(); i++) {
        if (Handlers[i].dma1Chanx == dma1_chanx) {
            index = i;
            break;
        }
    }
    return index;
}

uint8_t DMA_IRQnManage::GetDmaItIndexFromType(DMA::IT dma_it) {
    uint8_t index = Size_DMA_IT;
    for (auto i = 0; i < DMA::ITMap.size(); i++) {
        if (DMA::ITMap[i] == dma_it) {
            index = i;
            break;
        }
    }
    return index;
}

bool DMA_IRQnManage::Add(DMA_Channel_TypeDef* dma1_chanx, DMA::IT dma_it, void (*func)(void), uint8_t pre_priority, uint8_t sub_priority) {
    bool result = false;
    auto chanIndex = DMA_IRQnManage::GetDmaChanIndexFromType(dma1_chanx);
    auto itIndex = DMA_IRQnManage::GetDmaItIndexFromType(dma_it);
    if (DMA_IRQnManage::Handlers[chanIndex].itFuncs[itIndex] == nullptr) {
        DMA_IRQnManage::Handlers[chanIndex].itFuncs[itIndex] = func;
        DMA_IRQnManage::SetPriority(dma1_chanx,dma_it, pre_priority, sub_priority);
    } else {
        result = false;
    }
    return result;
};

IRQn DMA_IRQnManage::GetIRQn(DMA_Channel_TypeDef* dma1_chanx) {
    return DMA::GetDmaIRQn(dma1_chanx);
}

template<DMA::ChanIndex dma_index>
void HandlerDMA_IRQ() {
    auto index = To_uint8(dma_index);
    for (uint8_t i = 0; i < To_uint8(DMA::IT_Index::END); ++i) {
        auto dma_it = DMA_IRQnManage::dmaITArr[index][i];
        if (DMA_GetITStatus(dma_it)) {
            DMA_ClearITPendingBit(dma_it);
            if (auto& func = DMA_IRQnManage::GetHandlers(dma_index).itFuncs[i]) {
                func();
            }
        }
    }
}

// DMA中断处理函数
extern "C" void DMA1_Channel1_IRQHandler(void) {
    HandlerDMA_IRQ<DMA::ChanIndex::D1CH1>();
}

extern "C" void DMA1_Channel2_IRQHandler(void) {
    HandlerDMA_IRQ<DMA::ChanIndex::D1CH2>();
}

// 补全剩余通道的中断处理函数
extern "C" void DMA1_Channel3_IRQHandler(void) {
    HandlerDMA_IRQ<DMA::ChanIndex::D1CH3>();
}

extern "C" void DMA1_Channel4_IRQHandler(void) {
    HandlerDMA_IRQ<DMA::ChanIndex::D1CH4>();
}

extern "C" void DMA1_Channel5_IRQHandler(void) {
    HandlerDMA_IRQ<DMA::ChanIndex::D1CH5>();
}

extern "C" void DMA1_Channel6_IRQHandler(void) {
    HandlerDMA_IRQ<DMA::ChanIndex::D1CH6>();
}

extern "C" void DMA1_Channel7_IRQHandler(void) {
    HandlerDMA_IRQ<DMA::ChanIndex::D1CH7>();
}



// 在 IRQnManage.cpp 中实现
std::array<USART_IRQnManage::IRQnStruct, USART_IRQnManage::Size_USART> 
USART_IRQnManage::Handlers = {
    USART_IRQnManage::IRQnStruct{USART1},
    USART_IRQnManage::IRQnStruct{USART2},
    USART_IRQnManage::IRQnStruct{USART3}
};

bool USART_IRQnManage::Add(USART_TypeDef* USARTx, USART::IT usart_it, 
                          void (*func)(void), uint8_t pre_priority, uint8_t sub_priority)
{
    bool result = false;
    for (auto& item : Handlers) {
        if (item.USARTx == USARTx) {
            const auto it_index = GetUsartItIndexFromType(usart_it);
            if (item.itFuncs[it_index]) break;
            item.itFuncs[it_index] = func;
            result = true;
            break;
        }
    }
    SetPriority(USARTx, usart_it, pre_priority, sub_priority);
    return result;
}

uint8_t USART_IRQnManage::GetUsartIndexFromType(USART_TypeDef* USARTx)
{
    return USART::GetIndexFromType(USARTx);
}

uint8_t USART_IRQnManage::GetUsartItIndexFromType(USART::IT usart_it)
{
    return USART::GetIT_IndexFromType(usart_it);
}

IRQn USART_IRQnManage::GetIRQn(USART_TypeDef* USARTx)
{
    return USART::GetUSART_IRQnFromType(USARTx);
}

template<USART::Index usart_index>
void HandleUSART_IRQ(USART_TypeDef* usart)
{
    uint8_t index = static_cast<uint8_t>(usart_index);
    for (uint8_t i = 0; i < static_cast<uint8_t>(USART::IT_Index::END); ++i) {
        auto usart_it = static_cast<uint16_t>(USART::ITMap[i]);
        if (USART_GetITStatus(usart, usart_it)) {
            USART_ClearITPendingBit(usart, usart_it);
            if (auto& func = USART_IRQnManage::GetHandlers(usart_index).itFuncs[i]) {
                func();
            } 
        }
        
    }
}

// 中断处理函数
extern "C" void USART1_IRQHandler(void) {
    HandleUSART_IRQ<USART::Index::US1>(USART1);
}

extern "C" void USART2_IRQHandler(void) {
    HandleUSART_IRQ<USART::Index::US2>(USART2);
}

extern "C" void USART3_IRQHandler(void) {
    HandleUSART_IRQ<USART::Index::US3>(USART3);
}



