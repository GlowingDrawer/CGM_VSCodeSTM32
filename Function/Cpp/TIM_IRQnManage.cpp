#include "TIM_IRQnManage.h"

std::array<TIM_IRQnManage::TIM_IT, TIM_IRQnManage::NUM_TIM_IT> TIM_IRQnManage::TIM_IT_Array = {TIM_IT::UP, TIM_IT::CC1, TIM_IT::CC2, TIM_IT::CC3, TIM_IT::CC4, TIM_IT::COM};
const std::array<TIM_IRQnManage::TIM_IRQn, 9> TIM_IRQnManage::timIRQnMap = {
    TIM_IRQn{TIM1, IRQn::TIM1_UP_IRQn},
    TIM_IRQn{TIM2, IRQn::TIM2_IRQn},
    TIM_IRQn{TIM3, IRQn::TIM3_IRQn},
    TIM_IRQn{TIM4, IRQn::TIM4_IRQn},
    TIM_IRQn{TIM5, IRQn::TIM5_IRQn},
    TIM_IRQn{TIM6, IRQn::TIM6_IRQn},
    TIM_IRQn{TIM7, IRQn::TIM7_IRQn},
    TIM_IRQn{TIM8, IRQn::TIM8_UP_IRQn}
};

std::array<TIM_IRQnManage::IRQnStruct, 9> TIM_IRQnManage::Handlers = {
    TIM_IRQnManage::IRQnStruct{nullptr},
    TIM_IRQnManage::IRQnStruct{TIM1},
    TIM_IRQnManage::IRQnStruct{TIM2},
    TIM_IRQnManage::IRQnStruct{TIM3},
    TIM_IRQnManage::IRQnStruct{TIM4},
    TIM_IRQnManage::IRQnStruct{TIM5},    
    TIM_IRQnManage::IRQnStruct{TIM6},
    TIM_IRQnManage::IRQnStruct{TIM7},
    TIM_IRQnManage::IRQnStruct{TIM8}
};

static uint8_t HandlersIndex(TIM_TypeDef *TIMx) {
    int idx = 0;
    for (idx = 0; idx < 9; idx++) {
        if (TIMx == TIM_IRQnManage::GetHandlers(idx).TIMx) {
            break;
        }
    }
    return idx;
}


bool TIM_IRQnManage::Add(TIM_TypeDef *TIMx, void (* func)(void), TIM_IT_Index tim_it_idx) {
    bool result = true;
    for (auto& item : TIM_IRQnManage::Handlers) {
        if (item.TIMx == TIMx) {
            const auto it_idx = static_cast<uint8_t>(tim_it_idx);
            if (item.itFuncs[it_idx] != nullptr) {
                result = false;
            } else {
                item.itFuncs[it_idx] = func;
            }
            break;
        }
    }
    return result;
}

IRQn TIM_IRQnManage::GetIRQn(TIM_TypeDef *TIMx){
    IRQn result = static_cast<IRQn>(99);
    for (auto &item : TIM_IRQnManage::timIRQnMap) {
        if (item.TIMx == TIMx) {
            result = item.irqn;
            break;
        }
    }
    return result;
}

void TempShowMsg(const char * msg = "Hello") {
    OLED_ShowString(4, 1, msg);
}

template<TIM_IRQnManage::TIM_Index tim_index>
void HandleIRQ(TIM_TypeDef* TIMx) {
    constexpr uint8_t idx = To_uint8(tim_index);
    for(uint8_t i = 0; i < To_uint8(TIM_IRQnManage::TIM_IT_Index::END); ++i) {
        if(TIM_GetITStatus(TIMx, To_uint8(TIM_IRQnManage::TIM_IT_Array[i])) != RESET) {
            TIM_ClearITPendingBit(TIMx, To_uint8(TIM_IRQnManage::TIM_IT_Array[i]));
            if (auto & func = TIM_IRQnManage::GetHandlers(idx).itFuncs[i]) func();
        };
        
    }

}

extern "C" void TIM1_IRQHandler(void) {
    HandleIRQ<TIM_IRQnManage::TIM_Index::T1>(TIM1);
}

extern "C" void TIM2_IRQHandler(void) {
    HandleIRQ<TIM_IRQnManage::TIM_Index::T2>(TIM2);
}

extern "C" void TIM3_IRQHandler(void) {
    HandleIRQ<TIM_IRQnManage::TIM_Index::T3>(TIM3);
}

extern "C" void TIM4_IRQHandler(void) {
   HandleIRQ<TIM_IRQnManage::TIM_Index::T4>(TIM4);
}

extern "C" void TIM5_IRQHandler(void) { 
    HandleIRQ<TIM_IRQnManage::TIM_Index::T5>(TIM5);
}

extern "C" void TIM6_IRQHandler(void) { 
    HandleIRQ<TIM_IRQnManage::TIM_Index::T6>(TIM6);
}

extern "C" void TIM7_IRQHandler(void) { 
    HandleIRQ<TIM_IRQnManage::TIM_Index::T7>(TIM7);
}

extern "C" void TIM8_IRQHandler(void) { 
    HandleIRQ<TIM_IRQnManage::TIM_Index::T8>(TIM8);
}

extern
