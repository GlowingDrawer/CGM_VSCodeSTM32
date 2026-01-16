#pragma once
#include "stm32f10x.h"
#include <InitArg.h>
#include <array>
#include <functional>

class TIM_IRQnManage
{
public:
    static const uint8_t Size_TIM = To_uint8(TIM::Index::END);
   
private:
    struct IRQnStruct
    {
        const TIM_TypeDef *TIMx = nullptr;
        void (*itFuncs[To_uint8(TIM::IT_Index::END)])() = {nullptr};

        IRQnStruct() = default;
        IRQnStruct(TIM_TypeDef *timx) : TIMx(timx) {
            for (auto i = To_uint8(TIM::IT_Index::UP); i < To_uint8(TIM::IT_Index::END); i++) {  itFuncs[i] = nullptr; }
        }
    };
    static std::array<IRQnStruct, Size_TIM> Handlers;

    static const uint8_t GetHandlerIndexFromType(TIM_TypeDef *TIMx);
    static const uint8_t GetTimItIndexFromType(TIM::IT it);

public:
    TIM_IRQnManage() = default;
    static bool Add(TIM_TypeDef *TIMx, TIM::IT tim_it, void (* func)(void),  uint8_t pre_priority, uint8_t sub_priority, FunctionalState state = ENABLE);

    static inline const std::array<IRQnStruct, Size_TIM> & GetHandlers() { return Handlers; }
    static inline const IRQnStruct & GetHandlers(TIM::Index index){ return TIM_IRQnManage::Handlers[To_uint8(index)]; }
    static IRQn GetIRQn(TIM_TypeDef *TIMx);
    static void SetPriority(TIM_TypeDef *TIMx, TIM::IT tim_it, uint8_t pre_priority, uint8_t sub_priority, FunctionalState state = ENABLE) {
        MyNVIC::SetPriority(GetIRQn(TIMx), pre_priority, sub_priority, state);
    }

};

// IRQnManage.h 修改部分
class DMA_IRQnManage
{
public:
    
    static const uint8_t Size_DMA = To_uint8(DMA::ChanIndex::END);
    static const uint8_t Size_DMA_IT = To_uint8(DMA::IT_Index::END);
    
    static const std::array<std::array<uint32_t, Size_DMA_IT>, Size_DMA> dmaITArr;

    static uint8_t GetDmaChanIndexFromType(DMA_Channel_TypeDef* dma1_chanx);
    static IRQn GetIRQn(DMA_Channel_TypeDef* dma1_chanx);
    static uint8_t GetDmaItIndexFromType(DMA::IT dma_it);

private:
    struct IRQnStruct {
        DMA_Channel_TypeDef* dma1Chanx = nullptr;
        void (*itFuncs[To_uint8(DMA::IT_Index::END)])() = {nullptr};

        IRQnStruct() = default;
        IRQnStruct(DMA_Channel_TypeDef* chanx) : dma1Chanx(chanx) {
            std::fill(std::begin(itFuncs), std::end(itFuncs), nullptr);
        }
    };
    
    static std::array<IRQnStruct, Size_DMA> Handlers;

public:
    static bool Add(DMA_Channel_TypeDef* dma1_chanx, DMA::IT dma_it, void (*func)(void), uint8_t pre_priority, uint8_t sub_priority);
    // 保持其他方法不变

    static inline const IRQnStruct& GetHandlers(DMA::ChanIndex index) { return Handlers[To_uint8(index)]; }
    
    static void SetPriority(DMA_Channel_TypeDef* dma1_chanx, DMA::IT dma_it, uint8_t pre_priority, uint8_t sub_priority, FunctionalState state = ENABLE) {
        MyNVIC::SetPriority(GetIRQn(dma1_chanx), pre_priority, sub_priority, state);
    }
};

class USART_IRQnManage
{
public:
    static const uint8_t Size_USART = To_uint8(USART::Index::END);
    static const uint8_t Size_USART_IT = To_uint8(USART::IT_Index::END);

private:
    struct IRQnStruct {
        USART_TypeDef* USARTx = nullptr;
        void (*itFuncs[To_uint8(USART::IT_Index::END)])() = {nullptr};

        IRQnStruct() = default;
        IRQnStruct(USART_TypeDef* usartx) : USARTx(usartx) {
            std::fill(std::begin(itFuncs), std::end(itFuncs), nullptr);
        }
    };
    
    static std::array<IRQnStruct, Size_USART> Handlers;

public:
    static bool Add(USART_TypeDef* USARTx, 
                   USART::IT usart_it, 
                   std::function<void ()> func,
                   uint8_t pre_priority,
                   uint8_t sub_priority) {
        return USART_IRQnManage::Add(USARTx, usart_it, func, pre_priority, sub_priority); }
    // 核心方法
    static bool Add(USART_TypeDef* USARTx, 
                   USART::IT usart_it, 
                   void (*func)(void),
                   uint8_t pre_priority,
                   uint8_t sub_priority);

    // 辅助方法
    static uint8_t GetUsartIndexFromType(USART_TypeDef* USARTx);
    static IRQn GetIRQn(USART_TypeDef* USARTx);
    static uint8_t GetUsartItIndexFromType(USART::IT usart_it);

    // 访问器
    static inline const IRQnStruct& GetHandlers(USART::Index index) {
        return Handlers[To_uint8(index)];
    }

    // 优先级设置
    static void SetPriority(USART_TypeDef* USARTx, 
                           USART::IT usart_it,
                           uint8_t pre_priority,
                           uint8_t sub_priority,
                           FunctionalState state = ENABLE) {
        MyNVIC::SetPriority(GetIRQn(USARTx), pre_priority, sub_priority, state);
    }
};







