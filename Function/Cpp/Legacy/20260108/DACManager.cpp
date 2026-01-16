#include "stm32f10x.h"                  // Device header
#include "DACManager.h"
#include "stm32f10x_tim.h"
#include "GPIO.h"
#include "Delay.h"
#include "misc.h"
#include "math.h"
#include "BTCPP.h"
#include <InitArg.h>
#include "ADCManager.h"
#include "stdio.h"

/* This code follows the following naming conventions.
Const:  UPPER_SNAKE_CASE
Enum:   PascalCase / ABBR_Case / Abbr_Case  {UPPER_SNAKE_CASE}
Class： PascalCase / ABBR_Case / Abbr_Case
Arg:    snake_name              
Var:    camelCase
Func:   PascalCase (bool Func:  isPascalCase / hasPascalCase)
namespace:  NS_PascalCase / NS_ABBR
Bool var:   is_snake_name
*/

const uint8_t DEBUG_LINE = 4;
const uint8_t DEBUG_COW = 1;


namespace NS_DMA {

    enum class AddrMode:uint8_t { MEMORY, PERIPH};

    void ChannelSetAddr(AddrMode addr_mode ,DMA_Channel_TypeDef* DMA_Channel, uint32_t mem_addr, uint32_t data_count) {
        // 禁用全局中断，防止在配置过程中被中断打断
        DMA_ITConfig(DMA_Channel, DMA_IT_TC | DMA_IT_HT, DISABLE);
        
        // 停止DMA通道
        DMA_Cmd(DMA_Channel, DISABLE);
        
        // 设置新的内存地址和传输数据数量
        if (addr_mode == AddrMode::MEMORY) {
            DMA_Channel->CMAR = mem_addr;
        } else if (addr_mode == AddrMode::PERIPH) {
            DMA_Channel->CPAR = mem_addr;
        }
        DMA_Channel->CNDTR = data_count;  // 重置传输数量
        
        // 重新启用DMA通道
        DMA_Cmd(DMA_Channel, ENABLE);
        
        // 恢复全局中断
        DMA_ITConfig(DMA_Channel, DMA_IT_TC | DMA_IT_HT, ENABLE);
    }

}

namespace NS_DAC {

    void DAC_Cmd(DAC_Channel channel, FunctionalState state){
        DAC_Cmd(static_cast<uint32_t>(channel), state);
    }

    void DAC_Controller::Init(DAC_Channel cv_channel){
        this->cv_channel = cv_channel;
        if (this->cv_channel == DAC_Channel::CH1) {
            this->constant_channel = DAC_Channel::CH2;
        };
    }
    DAC_Controller::DAC_Controller(DAC_Channel cv_channel) {
        this->Init(cv_channel);
    }

    DAC_Controller::~DAC_Controller()
    {
        
    }

    void DAC_Controller::SetCV_Channel(DAC_Channel cv_channel){
        if (cv_channel == DAC_Channel::CH1) {
            this->cv_channel = DAC_Channel::CH1;
            this->constant_channel = DAC_Channel::CH2;
        } else if (cv_channel == DAC_Channel::CH2) {
            this->cv_channel = DAC_Channel::CH2;
            this->constant_channel = DAC_Channel::CH1;
        }
    }

    // Class CV_Controller
    // // DAC 每伏特电压对应的步长值（1240.91 steps/V）
    const constexpr float CV_Controller::dacStepsPerVolt = 1240.91f;

    void CV_Controller::UpdateScanStep(void){
        this->cvParams.scanStep12Bit = this->dacStepsPerVolt * this->cvParams.voltScanRate * this->cvParams.scanDuration;
    }

    void CV_Controller::InitCVParams(const CV_VoltParams& volt_params, float scan_duration, float volt_scan_rate, ScanDIR scan_dir){
        float cvMaxVolt = volt_params.highVolt + volt_params.voltOffset;
        float cvMinVolt = volt_params.lowVolt + volt_params.voltOffset;
        float initVolt = volt_params.voltOffset;
        // DAC进行CV时的最大和最小值
        this->cvParams.maxVal = To_uint16(cvMaxVolt * this->dacStepsPerVolt);
        this->cvParams.minVal = To_uint16(cvMinVolt * this->dacStepsPerVolt);
        this->cvParams.initVal = To_uint16(initVolt * this->dacStepsPerVolt);
        this->currentVal = initVolt * this->dacStepsPerVolt;
        this->bufA[0] = To_uint16(this->currentVal);
        this->bufPtr = this->bufA;
    
        this->cvParams.scanDuration = scan_duration;
        this->cvParams.voltScanRate = volt_scan_rate;
        this->cvParams.scanDIR = scan_dir;
        // 初始化扫描步进值
        this->cvParams.scanStep12Bit = this->dacStepsPerVolt * this->cvParams.voltScanRate * this->cvParams.scanDuration;
        if (this->cvParams.scanDIR == ScanDIR::REVERSE) {
            this->cvParams.scanStep12Bit = -this->cvParams.scanStep12Bit;
        }
    }

    CV_Controller::CV_Controller(CV_VoltParams volt_params, float scan_duration, float volt_scan_rate, ScanDIR scan_dir, 
        WaveForm wave_form, DAC_UpdateMode dac_update_mode) : dacUpdateMode(dac_update_mode) {
        OLED_Init();
        this->waveForm = wave_form;
        InitCVParams(volt_params, scan_duration, volt_scan_rate, scan_dir);
    }

    CV_Controller::CV_Controller(const CV_VoltParams& volt_params, const CV_Params& cv_params, WaveForm wave_form, DAC_UpdateMode dac_update_mode) 
        : CV_Controller(volt_params, cv_params.scanDuration, cv_params.voltScanRate, cv_params.scanDIR, wave_form, dac_update_mode) {
    }

    uint16_t CV_Controller::GetValToSend(void){
        uint16_t result = 0;
        if (this->waveForm == WaveForm::CV_FLUCTUATE) {
            result = this->bufPtr[0];
        } else if (this->waveForm == WaveForm::CONSTANT_VOLT) {
            result = this->cvParams.initVal;
        }
        return result;
    }

    void CV_Controller::ResetParam(const CV_VoltParams& volt_params){
        float cvMaxVolt = volt_params.highVolt + volt_params.voltOffset;
        float cvMinVolt = volt_params.lowVolt + volt_params.voltOffset;
        float initVolt = volt_params.voltOffset;
        // DAC进行CV时的最大和最小值
        this->cvParams.maxVal = To_uint16(cvMaxVolt * this->dacStepsPerVolt);
        this->cvParams.minVal = To_uint16(cvMinVolt * this->dacStepsPerVolt);
        this->cvParams.initVal = To_uint16(initVolt * this->dacStepsPerVolt);
        this->currentVal = initVolt * this->dacStepsPerVolt;
        this->bufPtr[0] = To_uint16(this->currentVal);
    }

    void CV_Controller::ResetParam(const CV_VoltParams& volt_params, float scan_duration, float volt_scan_rate, 
        ScanDIR scan_dir){
        ResetParam(volt_params);

        this->cvParams.scanDuration = scan_duration;
        this->cvParams.voltScanRate = volt_scan_rate;
        this->cvParams.scanDIR = scan_dir;
        // DAC进行CV时的步进值
        this->cvParams.scanStep12Bit = this->dacStepsPerVolt * this->cvParams.voltScanRate * this->cvParams.scanDuration;
        if (this->cvParams.scanDIR == ScanDIR::REVERSE) {
            this->cvParams.scanStep12Bit = - this->cvParams.scanStep12Bit;
        }
        
    }

    void CV_Controller::UpdateCurrentVal(void){
        this->currentVal += this->cvParams.scanStep12Bit;
        if (this->currentVal >= this->cvParams.maxVal) {
            this->cvParams.scanStep12Bit = -fabs(this->cvParams.scanStep12Bit);
            this->currentVal = this->cvParams.maxVal;
        } else if (this->currentVal <= this->cvParams.minVal) {
            this->cvParams.scanStep12Bit = fabs(this->cvParams.scanStep12Bit);
            this->currentVal = this->cvParams.minVal;
        }

        this->bufPtr[0] = To_uint16(this->currentVal);
        // this->bufPtr[0] = dacStepsPerVolt * 0.6;
    }

    void CV_Controller::ReStoreDefault(void){
        this->currentVal = this->cvParams.initVal;
        this->bufPtr[0] = To_uint16(this->currentVal);
    }

    void CV_Controller::ShowCv(uint16_t delay_ms){
        OLED_ShowString(4, 1, "CV:");
        OLED_ShowNum(4, 4, To_uint16(currentVal), 4);
    }


    DAC_ParamsTypedef::DAC_ParamsTypedef(DAC_Channel channel, TIM_TypeDef *TIM, DAC_UpdateMode update_mode) {
        AssignParams(channel, TIM);
        AssignUpdateMode(update_mode);
    }

    bool DAC_ParamsTypedef::AssignParams(DAC_Channel channel, TIM_TypeDef * tim){
        bool result = true;

        this->channel = channel;
        this->TIM = tim;
        
        this->pin = ((channel == DAC_Channel::CH1) ? DAC_GPIO_PIN::PIN4 : DAC_GPIO_PIN::PIN5);
        
        AssignDacTimIRQn();

        return result;
    }

    bool DAC_ParamsTypedef::AssignUpdateMode(DAC_UpdateMode update_mode){
        this->updateMode = update_mode;
        bool result = false;
        if (this->updateMode == DAC_UpdateMode::TIM_TRIG_DMA_SEND) {
            for (auto &tim_dac_map : timDacMap){
                if (tim_dac_map.TIMx == this->TIM) {
                    this->trigger = tim_dac_map.trigger;
                    result = true;
                    break;
                }
            }
        } else if (this->updateMode == DAC_UpdateMode::DIRECT_UPDATE || this->updateMode == DAC_UpdateMode::TIM_IT_UPDATE) {
            this->trigger = DAC_Trigger::SOFTWARE;
            result = true;
        } 
        return result;
    }
    // TimDacIRQn Up Params: TIM
    bool DAC_ParamsTypedef::AssignDacTimIRQn(void){
        bool result = true;
        this->timIRQn = TIM_IRQnManage::GetIRQn(this->TIM);
        if (this->timIRQn == IRQN_NONE) result = false; 
        return result;
    }


    // Class DAC_Chan_Controller
    DAC_ChanController::DAC_ChanController(WaveForm wave_form, DAC_Channel dac_channel, TIM_TypeDef * timx_dac, TIM_TypeDef * timx_dma, TIM_DMA_Source tim_dma_source, BufSize wave_buf_size, CV_VoltParams cv_volt_params, CV_Params cv_params, DAC_UpdateMode dac_update_mode) :  cvController(cv_volt_params, cv_params, wave_form, dac_update_mode){
        this->waveForm = wave_form;

        AssignDacParams(dac_channel, timx_dac);
        AssignDmaParams(timx_dma, tim_dma_source);
        this->scanDuration = this->cvController.GetCvParams().scanDuration;

        this->state = State::INITIALIZED;

    }
    DAC_ChanController::DAC_ChanController(WaveForm wave_form, DAC_ParamsTypedef dac_params, DMA_ParamsTypedef dma_params, BufSize wave_buf_size, CV_VoltParams volt_params, CV_Params cv_params, DAC_UpdateMode dac_update_mode) : DAC_ChanController(wave_form, dac_params.channel, dac_params.TIM, dma_params.TIM, dma_params.timDmaSource, wave_buf_size, volt_params, cv_params, dac_update_mode) {

    }


    DAC_ChanController::~DAC_ChanController()
    {
        FunctionalState state = DISABLE;
        
        DAC_Cmd(To_uint32(this->DAC_Params.channel), state);
        TIM_Cmd(this->DAC_Params.TIM, state);
        TIM::TIM_ITConfig(this->DAC_Params.TIM, TIM::IT::UP, state);
        DMA_Cmd(this->DMA_Params.channel, state);
        TIM_Cmd(this->DMA_Params.TIM, state);
        TIM_DMACmd(this->DMA_Params.TIM, To_uint16(this->DMA_Params.timDmaSource), state);
        
    }

    // Private Functions of DAC_Chan_Controller
    bool DAC_ChanController::AssignDacParams(DAC_Channel dac_channel, TIM_TypeDef * timx_dac){
        bool result = true;

        this->DAC_Params.channel = dac_channel;
        this->DAC_Params.TIM = timx_dac;

        // DAC Sub Setup
        this->DAC_Params.pin = (this->DAC_Params.channel == DAC_Channel::CH1 ? DAC_GPIO_PIN::PIN4 : DAC_GPIO_PIN::PIN5);

        // 根据波形/模式分配更新方式与触发源
        result &= AssignDacUpdateMode();

        // 根据TIM分配中断号
        result &= AssignDacTimIRQn();

        return result;
    }
    // DacUpdateMode Up Params: waveForm
    // Will Assign dacUpdateMode dacTrigger 
    bool DAC_ChanController::AssignDacUpdateMode(){
        bool result = true;
        switch (this->waveForm)
        {
        case WaveForm::SINE:
        case WaveForm::TRIANGLE:
            this->DAC_Params.updateMode = DAC_UpdateMode::TIM_TRIG_DMA_SEND;
            break;
        case WaveForm::CV_FLUCTUATE:
            this->DAC_Params.updateMode = this->cvController.GetUpdateMode();
            break;
        case WaveForm::CONSTANT_VOLT:
            this->DAC_Params.updateMode = DAC_UpdateMode::DIRECT_UPDATE;
            break;
        default:
            result = false;
            break;
        }

        // 触发源依赖updateMode与TIM
        result &= this->AssignDacTrigger();

        return result;
    }
    // DacTrigger Up Params: TIM, updateMode
    bool DAC_ChanController::AssignDacTrigger(void) {
        bool result = false;
        if (this->DAC_Params.updateMode == DAC_UpdateMode::TIM_TRIG_DMA_SEND) {
            for (auto &tim_dac_map : timDacMap){
                if (tim_dac_map.TIMx == this->DAC_Params.TIM) {
                    this->DAC_Params.trigger = tim_dac_map.trigger;
                    result = true;
                    break;
                }
            }
        } else if (this->DAC_Params.updateMode == DAC_UpdateMode::DIRECT_UPDATE || this->DAC_Params.updateMode == DAC_UpdateMode::TIM_IT_UPDATE) {
            this->DAC_Params.trigger = DAC_Trigger::SOFTWARE;
            result = true;
        } 
        return result;
    }
    // TimDacIRQn Up Params: TIM
    bool DAC_ChanController::AssignDacTimIRQn(void){
        bool result = true;
        this->DAC_Params.timIRQn = TIM::GetIRQn(this->DAC_Params.TIM);
        if (this->DAC_Params.timIRQn == IRQN_NONE) {
            result = false;
        }
        return result;
    }

    bool DAC_ChanController::AssignDmaParams(TIM_TypeDef * timx_dma, TIM_DMA_Source tim_dma_source){
        bool result = true;

        this->DMA_Params.TIM = timx_dma;
        this->DMA_Params.timDmaSource = tim_dma_source;

        // 根据(TIM, DMA请求源)查表分配DMA通道
        result &= AssignDmaTrigger(tim_dma_source);

        return result;
    }
    // Up Params: TIM
    // Up Params: TIM, timDmaSource
    bool DAC_ChanController::AssignDmaTrigger(void) {
        return AssignDmaTrigger(this->DMA_Params.timDmaSource);
    }
    // DmaTrigger Up Params: TIM
    bool DAC_ChanController::AssignDmaTrigger(TIM_DMA_Source dma_tim_trigger) {
        bool result = false;
        this->DMA_Params.timDmaSource = dma_tim_trigger;
        for (auto& map : timDmaMap) {
            if (this->DMA_Params.TIM == map.TIMx && this->DMA_Params.timDmaSource == map.DMA_Request) {
                this->DMA_Params.channel = map.DMA_Channel;
                result = true;
                break;
            }
        }
        return result;
    }

    // Will Change dacUpdateMode, dacTrigger, scanDuration
    bool DAC_ChanController::AssignValAboutWaveForm(void) {
        bool result = true;
        switch (this->waveForm)
        {
        case WaveForm::CV_FLUCTUATE:
            this->DAC_Params.updateMode = this->cvController.GetUpdateMode();
            this->scanDuration = this->cvController.GetCvParams().scanDuration;
            break;
        case WaveForm::CONSTANT_VOLT:
            this->DAC_Params.updateMode = DAC_UpdateMode::DIRECT_UPDATE;
            break;
        default: result = false;
            break;
        }
        if (!AssignDacTrigger()) {
            OLED_ShowString(4, 1, "Error DAC Trigger");
        }

        return result;
    }

    void DAC_ChanController::SetDacChannelDate(void){
        if (this->DAC_Params.channel == DAC_Channel::CH1) {
            if (this->waveForm == WaveForm::CONSTANT_VOLT) {
                DAC_SetChannel1Data(DAC_Align_12b_R, this->cvController.GetCvParams().initVal);
            } 
        } else if (this->DAC_Params.channel == DAC_Channel::CH2) {
            if (this->waveForm == WaveForm::CONSTANT_VOLT) {
                DAC_SetChannel2Data(DAC_Align_12b_R, this->cvController.GetCvParams().initVal);
            } 
        } 
    }

    void DAC_ChanController::RCC_PeriphSetState(FunctionalState state){
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, state);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, state);
        RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, state);
    }
    void DAC_ChanController::AllPeriph_Cmd(FunctionalState state){
        if (DAC_Params.updateMode  == DAC_UpdateMode::TIM_IT_UPDATE) {
            TIM_ITConfig(this->DAC_Params.TIM, TIM_IT_Update, state);
        } else if (DAC_Params.updateMode == DAC_UpdateMode::TIM_TRIG_DMA_SEND) {
            TIM_DMACmd(this->DMA_Params.TIM, To_uint16(this->DMA_Params.timDmaSource), state);
        }
    }


    // Public Functions
    void DAC_ChanController::ResetWaveForm(WaveForm wave_form){
        this->waveForm = wave_form;
        this->AssignValAboutWaveForm();
    }
    void DAC_ChanController::ResetDacParams(DAC_Channel dac_channel){
        this->DAC_Params.channel = dac_channel;
        this->DAC_Params.pin = (this->DAC_Params.channel == DAC_Channel::CH1? DAC_GPIO_PIN::PIN4 : DAC_GPIO_PIN::PIN5);
    }
    void DAC_ChanController::ResetDacParams(TIM_TypeDef * timx_dac){
        this->DAC_Params.TIM = timx_dac;
        AssignDacTrigger();
        AssignDacTimIRQn();
    }
    void DAC_ChanController::ResetDacParams(DAC_Channel dac_channel, TIM_TypeDef * timx_dac){
        this->ResetDacParams(dac_channel);
        this->ResetDacParams(timx_dac);
    }

    void DAC_ChanController::ResetDmaTrigger(TIM_TypeDef * timx_dma, TIM_DMA_Source dma_tim_trigger){
        this->DMA_Params.TIM = timx_dma;
        this->AssignDmaTrigger(dma_tim_trigger);
    }
    
    void DAC_ChanController::ResetCvUpdateMode(DAC_UpdateMode update_mode){
        if (this->cvController.GetUpdateMode() != update_mode) {
            this->cvController.ResetParam(update_mode);
            AssignDacUpdateMode();
        }
    }

    void DAC_ChanController::ResetCvController(CV_VoltParams volt_params, CV_Params cv_params, DAC_UpdateMode dac_update_mode){
        this->cvController.ResetParam(volt_params, cv_params.scanDuration, cv_params.voltScanRate, cv_params.scanDIR, this->waveForm, dac_update_mode);
        AssignDacUpdateMode();  
    }
    
    void DAC_ChanController::ReStore(void) { 
        RCC_PeriphSetState(ENABLE);
        AllPeriph_Cmd(ENABLE);
    }

    // Important Functions

    void DAC_ChanController::GPIO_Setup(void){
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

        GPIO_InitTypeDef GPIO_InitStructure;
        GPIO_InitStructure.GPIO_Pin = To_uint16(this->DAC_Params.pin);
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
        GPIO_Init(GPIOA, &GPIO_InitStructure);
    }

    void DAC_ChanController::DAC_Setup(void) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

        DAC_InitTypeDef DAC_InitStructure;
        // DAC_StructInit(&DAC_InitStructure);
        DAC_InitStructure.DAC_Trigger = To_uint32(this->DAC_Params.trigger);
        DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
        DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Disable;
        DAC_Init(To_uint32(this->DAC_Params.channel), &DAC_InitStructure);

        DAC_Cmd(To_uint32(this->DAC_Params.channel), ENABLE);
        
        if (this->DAC_Params.updateMode == DAC_UpdateMode::DIRECT_UPDATE) {
            this->SetDacChannelDate();
            DAC_SoftwareTriggerCmd(To_uint32(this->DAC_Params.channel), ENABLE);
        }

        // OLED_ShowString(4, 1, "DAC_Setup");
        
    }
    void DAC_ChanController::TIM_DAC_Setup(void) {
        // OLED_ShowString(1, 1, "TIM_DAC_Setup");
        // Delay_ms(500);
        
        this->DAC_Setup();     
        if (this->DAC_Params.updateMode == DAC_UpdateMode::TIM_IT_UPDATE || this->DAC_Params.updateMode == DAC_UpdateMode::TIM_TRIG_DMA_SEND) {
            // TIM_Clock ENABLE
            TIM::InitTIM(this->DAC_Params.TIM, this->scanDuration);
            TIM_SelectOutputTrigger(this->DAC_Params.TIM, TIM_TRGOSource_Update);
            TIM_Cmd(this->DAC_Params.TIM, ENABLE);
            TIM_ITConfig(this->DAC_Params.TIM, TIM_IT_Update, ENABLE);
        }
    }
    void DAC_ChanController::TIM_DAC_IRQHandler(void) {
        this->cvController.UpdateCurrentVal();
        if (this->DAC_Params.updateMode == DAC_UpdateMode::TIM_IT_UPDATE) {
            if (this->DAC_Params.channel == DAC_Channel::CH1) {
                DAC_SetChannel1Data(DAC_Align_12b_R, this->cvController.GetBufferVal());
            } else if (this->DAC_Params.channel == DAC_Channel::CH2) {
                DAC_SetChannel2Data(DAC_Align_12b_R, this->cvController.GetBufferVal());
            }
            DAC_SoftwareTriggerCmd(To_uint32(this->DAC_Params.channel), ENABLE);
        }
    }

    void DAC_ChanController::DMA_Setup(void) {
        RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
        DMA_InitTypeDef DMA_InitStructure;
        uint32_t arrToSendAddr;
        uint16_t bufSize;

        if (this->DAC_Params.updateMode == DAC_UpdateMode::TIM_TRIG_DMA_SEND) {
            if (this->waveForm == WaveForm::CV_FLUCTUATE) {
                arrToSendAddr = reinterpret_cast<uintptr_t>(this->cvController.GetBufferAPtr());
                bufSize = 1;
            } 
            if (this->DAC_Params.channel == DAC_Channel::CH1) {
                DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) & (DAC->DHR12R1);
            } else if (this->DAC_Params.channel == DAC_Channel::CH2) {
                DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) & (DAC->DHR12R2);
            }
            DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
            DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
            DMA_InitStructure.DMA_MemoryBaseAddr = arrToSendAddr;
            DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
            DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
            DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
            DMA_InitStructure.DMA_Priority = DMA_Priority_High;
            DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
            DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
            DMA_InitStructure.DMA_BufferSize = bufSize;
            DMA_Init(this->DMA_Params.channel, &DMA_InitStructure);
            DMA_Cmd(this->DMA_Params.channel, ENABLE);
        } 
    }
    
    void DAC_ChanController::TIM_DMA_Setup(void) {
        this->DMA_Setup();
        // OLED_ShowString(1, 1, "TIM_DMA_Setup");
        // Delay_ms(500);
        
        if (this->DMA_Params.TIM != this->DAC_Params.TIM) {
            TIM::InitTIM(this->DMA_Params.TIM, this->scanDuration);
            TIM_Cmd(this->DMA_Params.TIM, ENABLE);
        }
        if (this->DAC_Params.updateMode == DAC_UpdateMode::TIM_TRIG_DMA_SEND ) {
            TIM_DMACmd(this->DMA_Params.TIM, To_uint16(this->DMA_Params.timDmaSource), ENABLE);
        }

    }

    void DAC_ChanController::SetState(FunctionalState state) {
        DAC_Cmd(To_uint32(this->DAC_Params.channel), state);
        TIM_Cmd(this->DAC_Params.TIM, state);
        TIM_ITConfig(this->DAC_Params.TIM, TIM_IT_Update, state);
        TIM_Cmd(this->DMA_Params.TIM, state);
        TIM_DMACmd(this->DAC_Params.TIM, To_uint16(this->DMA_Params.timDmaSource), state);
        TIM_DMACmd(this->DMA_Params.TIM, To_uint16(this->DMA_Params.timDmaSource), state);
        DMA_Cmd(this->DMA_Params.channel, state);
        DMA::DMA_ITConfig(this->DMA_Params.channel, DMA::IT::TC, state);
    }

    void DAC_ChanController::Start(){
        if (state == State::INITIALIZED) {
            // 启动操作
            state = State::RUNNING;
            this->GPIO_Setup();
            this->TIM_DAC_Setup();
            this->TIM_DMA_Setup();
        } else if (state == State::STOPPED) {
            // 
            state = State::RUNNING;
            this->SetState(ENABLE);
        }
    }
    
    void NS_DAC::DAC_ChanController::Pause() {
        // 避免重复暂停
        if (pauseState.isPaused || state != State::RUNNING) return;

        // 1. 保存暂停前的硬件使能状态到结构体
        pauseState.preDACState = STM32PeriphFunc::DAC_GetCmdStatus(To_uint32(this->DAC_Params.channel));
        pauseState.preTIMDACState = STM32PeriphFunc::TIM_GetCmdStatus(this->DAC_Params.TIM);
        pauseState.preTIMDMAState = STM32PeriphFunc::TIM_GetCmdStatus(this->DMA_Params.TIM);
        pauseState.preDMAState = STM32PeriphFunc::DMA_GetCmdStatus(this->DMA_Params.channel);
        pauseState.preTIMITState = TIM_GetITStatus(this->DAC_Params.TIM, TIM_IT_Update) == RESET? ENABLE: DISABLE;
        pauseState.preTIMDMACmdState = STM32PeriphFunc::TIM_GetDMACmdStatus(this->DMA_Params.TIM, To_uint16(this->DMA_Params.timDmaSource)) ? ENABLE : DISABLE;

        // 2. 禁用硬件
        DAC_Cmd(To_uint32(this->DAC_Params.channel), DISABLE);
        TIM_Cmd(this->DAC_Params.TIM, DISABLE);
        TIM_Cmd(this->DMA_Params.TIM, DISABLE);
        TIM_ITConfig(this->DAC_Params.TIM, TIM_IT_Update, DISABLE);
        DMA_Cmd(this->DMA_Params.channel, DISABLE);
        TIM_DMACmd(this->DMA_Params.TIM, To_uint16(this->DMA_Params.timDmaSource), DISABLE);
        NVIC_DisableIRQ(DAC_Params.timIRQn);  // 禁用定时器中断的 NVIC 通道

        // 3. 更新状态标记
        pauseState.isPaused = true;
        state = State::PAUSED;
    }

    void NS_DAC::DAC_ChanController::Resume() {
        // 仅在暂停状态下执行恢复
        if (!pauseState.isPaused || state != State::PAUSED) return;

        // 1. 根据结构体保存的状态恢复硬件
        if (pauseState.preDMAState == ENABLE) {
            DMA_Cmd(this->DMA_Params.channel, ENABLE);
        }
        if (pauseState.preTIMDMACmdState == ENABLE) {
            TIM_DMACmd(this->DMA_Params.TIM, To_uint16(this->DMA_Params.timDmaSource), ENABLE);
        }
        if (pauseState.preTIMDACState == ENABLE) {
            TIM_SetCounter(this->DAC_Params.TIM, 0); // 重置计数器
            TIM_Cmd(this->DAC_Params.TIM, ENABLE);
        }
        if (pauseState.preTIMDMAState == ENABLE) {
            TIM_Cmd(this->DMA_Params.TIM, ENABLE);
        }
        if (pauseState.preTIMITState == ENABLE) {
            // 关键：清除残留的中断标志位（必须在启用中断前）
            TIM_ClearITPendingBit(this->DAC_Params.TIM, TIM_IT_Update);
            TIM_ITConfig(this->DAC_Params.TIM, TIM_IT_Update, ENABLE);

            NVIC_EnableIRQ(DAC_Params.timIRQn);  
        }
        if (pauseState.preDACState == ENABLE) {
            DAC_Cmd(To_uint32(this->DAC_Params.channel), ENABLE);
            // 值初始化
            this->Get_CV_Controller().ReStoreDefault();
            // 方案2：若所有波形模式均需恢复输出，直接移除判断（简化逻辑）
            this->SetDacChannelDate(); // 加载最新数据
            DAC_SoftwareTriggerCmd(To_uint32(this->DAC_Params.channel), ENABLE); // 触发输出
        }
        
        // 2. 清除暂停标记
        pauseState.isPaused = false;
        state = State::RUNNING;
    }

    
    // 定义并初始化静态成员
    NS_DAC::DAC_ChanController NS_DAC::DAC_Controller::dacChanCV(
        NS_DAC::WaveForm::CV_FLUCTUATE, 
        DAC_Channel::CH2, 
        TIM2, TIM2, 
        TIM_DMA_Source::UP, 
        BufSize::BUF_32, 
        CV_VoltParams(0.4f, -0.8f, 1.5f), 
        CV_Params(0.05f, 0.05f, NS_DAC::ScanDIR::FORWARD), 
        DAC_UpdateMode::TIM_IT_UPDATE
    );

    NS_DAC::DAC_ChanController NS_DAC::DAC_Controller::dacChanConstant(
        NS_DAC::WaveForm::CONSTANT_VOLT,
        DAC_Channel::CH1,
        TIM3, TIM3,
        TIM_DMA_Source::UP,
        BufSize::BUF_32,
        CV_VoltParams(0.4f, -0.8f, 1.5f), 
        CV_Params(0.05f, 0.05f, NS_DAC::ScanDIR::FORWARD),
        DAC_UpdateMode::TIM_IT_UPDATE
    );

    // 获取DAC即将发送的CV值（12Bit）
    const uint16_t GetCvValToSend(void) {
        return NS_DAC::DAC_Controller::dacChanCV.Get_CV_Controller().GetValToSend();
    }

    // 获取DAC即将发送的CV值指针（12Bit）
    const uint16_t * GetCvValToSendBuf(void) {
        return NS_DAC::DAC_Controller::dacChanCV.Get_CV_Controller().GetValToSendPtr();
    }

    // 获取DAC即将发送的CV值引用（12Bit）
    const uint16_t & GetCvValToSendRef(void) {
        return NS_DAC::DAC_Controller::dacChanCV.Get_CV_Controller().GetValToSendRef();
    }

    NS_ADC::InitParams& CGMParams2AdcInitParams(CGM::Params &params, NS_ADC::InitParams &init_params) {
        init_params.adc = ADC1;
        init_params.mode = NS_ADC::Mode::INDEPENDENT;
        init_params.nbr_of_channels = params.nbr_of_channels;
        init_params.channels = &(params.adChanConfig[0]);
        init_params.scan_mode = ENABLE;
        init_params.cont_mode = ENABLE;
        init_params.clock_prescaler = RCC_PCLK2_Div6;
        return init_params;
    }

    CGM::Params adcParams = CGM::CGM_250901::params;
    NS_ADC::InitParams adcInitParams = CGMParams2AdcInitParams(adcParams, adcInitParams);

    NS_ADC::ADC adc(adcInitParams, NS_ADC::ShowParams(TIM3, 0.05), NS_DAC::DAC_Controller::dacChanCV.Get_CV_Controller());

    // 获取ADC DMA缓冲区头指针
    const uint16_t * GetADCDmaBufHeader(void) {
        return adc.GetDmaBufferHeader();
    }
    // 获取ADC DMA缓冲区引用
    const std::array<uint16_t, 16> & GetADCDmaBufRef(void) {
        return adc.GetDmaBufferRef();
    }

    // 获取ADC DMA缓冲区对应计算所得电流大小缓冲区头指针
    const double * GetADCCurrentBufHeader(void) {
        return adc.GetCurrentBufHeader();
    }
    // 获取ADC DMA缓冲区对应计算所得电流大小缓冲区引用
    const std::array<double, 16> & GetADCCurrentBufRef(void) {
        return adc.GetCurrentBufRef();
    }

    // 直接返回adc引用
    const NS_ADC::ADC & GetADCRef() {
        return adc;
    }

    NS_ADC::ADC & GetADC(void) {
    return adc;
    }


    static void TIM_IRQn_DAC() {
        NS_DAC::DAC_Controller::dacChanCV.TIM_DAC_IRQHandler();
    }

    static void TIM_IRQn_AD() {
        adc.TIM_IRQnHandler();
    }

    static void DMA_IRQn_AD() {
        adc.DMA_IRQnHandler();
    }

    // 定义单例实例的获取方法
    SystemController& SystemController::GetInstance() {
        static SystemController instance;  // 局部静态变量，确保唯一实例且线程安全（C++11及以上）
        return instance;
    }

    void SystemController::Start() { 
        bool isAdd = TIM_IRQnManage::Add(DAC_Controller::dacChanCV.Get_DAC_Params().TIM, TIM::IT::UP, &TIM_IRQn_DAC, 1, 1);
        if (!isAdd) { JustShow("Add 1 Failed"); }
        isAdd = TIM_IRQnManage::Add(adc.GetShowParams().TIMx, adc.GetShowParams().tim_it,  &TIM_IRQn_AD, 1, 2);
        if (!isAdd) { JustShow("Add 2 Failed"); }
        // isAdd = DMA_IRQnManage::Add(adc.GetDmaChannel(), &DMA_IRQn_AD, DMA::DMA_IT::TC);
        // if (!isAdd) { JustShow("Add 3 Failed"); }
        DAC_Controller::dacChanCV.Start();
        DAC_Controller::dacChanConstant.Start();
        adc.StartConversion();
        this->systemState = SystemState::RUNNING;
    }

    // 系统级暂停实现
    void SystemController::Pause() {
        // 暂停ADC（检查状态避免重复操作）
        if (!adc.IsPaused()) {
            adc.Pause();
        }

        // 暂停CV通道DAC（检查状态避免重复操作）
        if (!NS_DAC::DAC_Controller::dacChanCV.IsPaused()) {
            NS_DAC::DAC_Controller::dacChanCV.Pause();
        }

        // 暂停恒定电压通道DAC（检查状态避免重复操作）
        if (!NS_DAC::DAC_Controller::dacChanConstant.IsPaused()) {
            NS_DAC::DAC_Controller::dacChanConstant.Pause();
        }

        this->ClearUpdataTimes();
        this->systemState = SystemState::PAUSED;

        OLED_ShowString(4, 1, "Syetem Paused");

        // 可选：添加其他模块的暂停逻辑
        // 例如：按键扫描暂停、OLED刷新暂停等
        // KeyScanner::GetInstance().Pause();
        // OLED::GetInstance().PauseRefresh();
    }

    // 系统级恢复实现
    void SystemController::Resume() {
        // 恢复ADC（检查状态避免重复操作）
        if (adc.IsPaused()) {
            adc.Resume();
        }

        // 恢复CV通道DAC（检查状态避免重复操作）
        if (NS_DAC::DAC_Controller::dacChanCV.IsPaused()) {
            NS_DAC::DAC_Controller::dacChanCV.Resume();
        }

        // 恢复恒定电压通道DAC（检查状态避免重复操作）
        if (NS_DAC::DAC_Controller::dacChanConstant.IsPaused()) {
            NS_DAC::DAC_Controller::dacChanConstant.Resume();
        }

        OLED_ShowString(4, 1, "System Resumed");
        this->systemState = SystemState::RUNNING;

        // 可选：添加其他模块的恢复逻辑
        // 例如：恢复按键扫描、OLED刷新等
        // KeyScanner::GetInstance().Resume();
        // OLED::GetInstance().ResumeRefresh();
    }

}
