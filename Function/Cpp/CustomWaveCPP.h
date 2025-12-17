#pragma once
#include "stm32f10x.h"
#include <InitArg.h>
#include <IRQnManage.h>

namespace NS_ADC {
    class ADC;
}

namespace NS_DAC {

    enum class WaveForm:uint8_t { SINE, TRIANGLE, DC_CONSTANT, CV_FLUCTUATE, CV_CONSTANT };
    enum class DAC_Channel:uint32_t { CH1 = DAC_Channel_1, CH2 = DAC_Channel_2 };
    enum class DAC_GPIO_PIN:uint16_t { PIN4 = GPIO_Pin_4, PIN5 = GPIO_Pin_5 };
    enum class DAC_Trigger:uint32_t { SOFTWARE = DAC_Trigger_Software, T2_TRGO = DAC_Trigger_T2_TRGO, 
        T3_TRGO = DAC_Trigger_T3_TRGO, T4_TRGO = DAC_Trigger_T4_TRGO, T5_TRGO = DAC_Trigger_T5_TRGO,
        T6_TRGO = DAC_Trigger_T6_TRGO, END };
    enum class TIM_DMA_Source:uint16_t { UP = TIM_DMA_Update, CH1 = TIM_DMA_CC1, CH2 = TIM_DMA_CC2, CH3 = TIM_DMA_CC3, CH4 = TIM_DMA_CC4, END };
    enum class ScanDIR:uint8_t { FORWARD, REVERSE };
    enum class DAC_UpdateMode:uint8_t { TIM_TRIG_DMA_SEND, TIM_IT_UPDATE, DIRECT_UPDATE };
    enum class WaveArrMode:uint8_t { CALCULATION, TABLE_LOOKUP, END };

    void DAC_Cmd(DAC_Channel channel, FunctionalState state);
    

    struct TIM_DMA_Mapping {
        TIM_TypeDef* TIMx;
        TIM_DMA_Source DMA_Request;  // 如TIM_DMA_CC1, TIM_DMA_UP
        DMA_TypeDef* DMAx;     // DMA1或DMA2
        DMA_Channel_TypeDef* DMA_Channel;  // DMA通道号
    }; 

    const TIM_DMA_Mapping timDmaMap[] = {
        {TIM1, TIM_DMA_Source::UP, DMA1, DMA1_Channel5},    // TIM1_UP >> DMA1_Channel5
        {TIM1, TIM_DMA_Source::CH1, DMA1, DMA1_Channel2},   // TIM1_CH1 >> DMA1_Channel2
        {TIM1, TIM_DMA_Source::CH2, DMA1, DMA1_Channel3},   // TIM1_CH2 >> DMA1_Channel3
        {TIM1, TIM_DMA_Source::CH3, DMA1, DMA1_Channel6},   // TIM1_CH3 >> DMA1_Channel6
        {TIM1, TIM_DMA_Source::CH4, DMA1, DMA1_Channel4},   // TIM1_CH4 >> DMA1_Channel4

        {TIM2, TIM_DMA_Source::UP, DMA1, DMA1_Channel2},    // TIM2_UP >> DMA1_Channel2
        {TIM2, TIM_DMA_Source::CH1, DMA1, DMA1_Channel5},   // TIM2_CH1 >> DMA1_Channel5
        {TIM2, TIM_DMA_Source::CH2, DMA1, DMA1_Channel7},   // TIM2_CH2 >> DMA1_Channel7
        {TIM2, TIM_DMA_Source::CH3, DMA1, DMA1_Channel1},   // TIM2_CH3 >> DMA1_Channel1
        {TIM2, TIM_DMA_Source::CH4, DMA1, DMA1_Channel7},   // TIM2_CH4 >> DMA1_Channel7

        {TIM3, TIM_DMA_Source::UP, DMA1, DMA1_Channel3},    // TIM3_UP >> DMA1_Channel3
        {TIM3, TIM_DMA_Source::CH1, DMA1, DMA1_Channel6},   // TIM3_CH1 >> DMA1_Channel6
        {TIM3, TIM_DMA_Source::CH3, DMA1, DMA1_Channel2},   // TIM3_CH3 >> DMA1_Channel2
        {TIM3, TIM_DMA_Source::CH4, DMA1, DMA1_Channel3},   // TIM3_CH4 >> DMA1_Channel3

        {TIM4, TIM_DMA_Source::UP, DMA1, DMA1_Channel7},    // TIM4_UP >> DMA1_Channel7
        {TIM4, TIM_DMA_Source::CH1, DMA1, DMA1_Channel1},   // TIM4_CH1 >> DMA1_Channel1
        {TIM4, TIM_DMA_Source::CH2, DMA1, DMA1_Channel4},   // TIM4_CH2 >> DMA1_Channel4
        {TIM4, TIM_DMA_Source::CH3, DMA1, DMA1_Channel5},   // TIM4_CH3 >> DMA1_Channel5 
    };

    struct TIM_DAC_Mapping
    {
        TIM_TypeDef * TIMx;
        DAC_Trigger trigger;
    };

    const TIM_DAC_Mapping timDacMap[] = {
        {TIM2, DAC_Trigger::T2_TRGO},   // TIM2 >> DAC_Trigger::T2_TRGO
        {TIM3, DAC_Trigger::T3_TRGO},   // TIM3 >> DAC_Trigger::T3_TRGO
        {TIM4, DAC_Trigger::T4_TRGO},   // TIM4 >> DAC_Trigger::T4_TRGO
        {TIM5, DAC_Trigger::T5_TRGO},   // TIM5 >> DAC_Trigger::T5_TRGO
        {TIM6, DAC_Trigger::T6_TRGO},   // TIM6 >> DAC_Trigger::T6_TRGO
    };

    struct CV_VoltParams {
        float highVolt = 0.8f;
        float lowVolt = -0.8f;
        float voltOffset = 1.0f;
        float maxVolt = 3.3f;
        float minVolt = -3.3f;
        explicit CV_VoltParams() = default;
        explicit CV_VoltParams(float high_volt, float low_volt, float volt_offset) : highVolt(high_volt), lowVolt(low_volt), voltOffset(volt_offset) {  
            if (high_volt < low_volt) {
                auto temp = high_volt;
                high_volt = low_volt;
                low_volt = temp;
            }
        }
    };

    struct CV_Params {
        uint16_t maxVal = 4095, minVal = 0, initVal = 2048;
        float scanDuration = 0.05f;
        float voltScanRate = 0.05f;
        float scanStep12Bit = 0.0f;
        ScanDIR scanDIR = ScanDIR::FORWARD;
        CV_Params() = default;
        explicit CV_Params(float scan_duration, float volt_scan_rate, ScanDIR scan_dir) : scanDuration(scan_duration), voltScanRate(volt_scan_rate), scanDIR(scan_dir) {}
        explicit CV_Params(uint16_t max_value, uint16_t min_value, uint16_t init_value, float scan_duration, float volt_scan_rate, ScanDIR scan_dir) : maxVal(max_value), minVal(min_value), initVal(init_value), scanDuration(scan_duration), voltScanRate (volt_scan_rate), scanDIR(scan_dir) {}
    } ;

    class CV_Controller
    {
    private:
        bool isInit = false;
        
        WaveForm waveForm = WaveForm::CV_FLUCTUATE;
        DAC_UpdateMode dacUpdateMode = DAC_UpdateMode::TIM_IT_UPDATE;

        // DAC 每伏特电压对应的步长值（1240.91 steps/V）
        static const float dacStepsPerVolt;

        CV_Params cvParams;
        float currentVal = 2047.0f;
        uint16_t bufA[1];
        uint16_t bufB[1];

        uint16_t * bufPtr;

        void UpdateScanStep(void);
        
    public:
        WaveForm GetWaveForm(void) { return this->waveForm; }
        DAC_UpdateMode GetUpdateMode(void) {return this->dacUpdateMode; }
        const CV_Params& GetCvParams(void) const { return this->cvParams; }
        float GetCurrentVal(void) const { return this->currentVal; }

        uint16_t GetValToSend(void);
        // 指向“即将发送到DAC”的12bit值（用于DMA/调试）。
        const uint16_t * GetValToSendPtr(void) const {
            if (this->waveForm == WaveForm::CV_FLUCTUATE) {
                return (this->bufPtr != nullptr) ? this->bufPtr : this->bufA;
            }
            return &(this->cvParams.initVal);
        }
        const uint16_t & GetValToSendRef(void) const {
            return *(this->GetValToSendPtr());
        }

        
        const float * GetCurrentValPtr(void) const { return &(this->currentVal);}
        const float & GetCurrentValRef(void) const { return this->currentVal; }

        void ScanDurationReset(float scan_duration) { this->cvParams.scanDuration = scan_duration; this->UpdateScanStep(); };
        void VoltScanRateReset(float volt_scan_rate) { this->cvParams.voltScanRate = volt_scan_rate; this->UpdateScanStep(); }
        // ResetParam Overload
        void ResetParam(WaveForm wave_form) { this->waveForm = wave_form; }
        void ResetParam(DAC_UpdateMode update_mode) { this->dacUpdateMode = update_mode; }
        void ResetParam(WaveForm wave_form, DAC_UpdateMode update_mode) { ResetParam(wave_form); ResetParam(update_mode); }
        void ResetParam(const CV_VoltParams& volt_params);
        void ResetParam(const CV_VoltParams& volt_params, float scan_duration, float volt_scan_rate, ScanDIR scan_dir);
        void ResetParam(const CV_VoltParams& volt_params, float scan_duration, float volt_scan_rate, ScanDIR scan_dir, WaveForm wave_form, 
            DAC_UpdateMode update_mode) { ResetParam(volt_params, scan_duration, volt_scan_rate, scan_dir); 
                this->waveForm = wave_form; this->dacUpdateMode = update_mode; }

        void ResetCurrentVal(float val_update) { this->currentVal = val_update; bufPtr[0] = To_uint16(val_update); } 
        void UpdateCurrentVal(void);
        void ReStoreDefault(void);
        const uint16_t * GetBufferAPtr(void) const { return bufA; }
        const uint16_t * GetBufferBPtr(void) const { return bufB; }
        const uint16_t * GetBufferPtr(void) const { return bufPtr; }
        const uint16_t GetBufferVal(void) const { return *bufPtr; }
        
        // For DMA ping-pong
        void ExchangeBuffer(void) { this->bufPtr = (this->bufPtr == this->bufA? this->bufB: this->bufA); };

        void ShowCv(uint16_t delay_ms);
        
    public:
        void InitCVParams(const CV_VoltParams& volt_params, float scan_duration, float volt_scan_rate, ScanDIR scan_dir);
        
        explicit CV_Controller(CV_VoltParams volt_params = CV_VoltParams(0.8f, -0.8f, 1.0f), float scan_duration = 0.05f, float volt_scan_rate = 0.05f, ScanDIR scan_dir = ScanDIR::FORWARD, WaveForm wave_form = WaveForm::CV_FLUCTUATE, DAC_UpdateMode dac_update_mode = DAC_UpdateMode::TIM_IT_UPDATE);
        explicit CV_Controller(const CV_VoltParams& volt_params, const CV_Params& cv_params, WaveForm wave_form, DAC_UpdateMode dac_update_mode);

        ~CV_Controller() = default;

    };

    struct WaveStaticParams {
        uint16_t maxVal = 4095, minVal = 0;
        static const std::array<uint16_t, To_uint16(BufSize::BUF_END) - 1> s_TriangleArr;
        static const std::array<uint16_t, To_uint16(BufSize::BUF_END) - 1> s_SineArr;
        uint16_t bufMaxSize = To_uint16(BufSize::BUF_END) - 1;
        WaveStaticParams() = default;
        WaveStaticParams(uint16_t max_value, uint16_t min_value) : 
            maxVal(max_value), minVal(min_value){}
    } ;

    struct WaveDynParams {
        uint16_t maxVal = 4095, minVal = 0;
        float scanDuration = 0.05f;
        float period = 16.0f;
        uint16_t bufSize = To_uint16(BufSize::BUF_32);
        uint16_t buffer[To_uint16(BufSize::BUF_END) - 1] {};
        uint16_t constVal = 2048;
        WaveDynParams() = default;
        WaveDynParams(uint16_t max_value, uint16_t min_value, BufSize buf_size) : 
            maxVal(max_value), minVal(min_value), bufSize(To_uint16(buf_size)){}
    } ;

    class WaveArr
    {
    private:
        static bool isInit;

        static WaveStaticParams s_Params;
        

        WaveDynParams dynParams;
        WaveForm waveForm = WaveForm::SINE;
        void InitBuffer(const uint16_t* sourceArr, float gain, int offset, uint16_t interval);
        void InitArr() { this->InitArr(this->waveForm); }
        bool InitArr(WaveForm wave_form);

    public:
        // Get an pointer point to TriangleArr[Index] 
        // CAUTION: Return array head When out-of-range
        static const uint16_t * TriangleArr(uint16_t index = 0);
        // Get an pointer point to SineArr[Index] 
        // CAUTION: Return array head When out-of-range
        static const uint16_t * SineArr(uint16_t index = 0);

        //  Set wave_form
        void ResetWaveForm(WaveForm wave_form);
        const uint16_t* GetBufferAddrPtr(uint16_t index = 0);
        const uint16_t GetBufferVal(uint16_t index = 0);
                
    public:
        WaveArr(WaveForm wave_form = WaveForm::SINE, BufSize buf_size  = BufSize::BUF_32);
        // ~WaveArr();

        static void ResetStaticArr(const uint16_t max_value, const uint16_t min_value, const BufSize buf_max_size = BufSize::BUF_END);

        static const WaveStaticParams& GetStaticParams(void) { return WaveArr::s_Params; }
        const WaveDynParams& GetDynParams(void) const { return this->dynParams; }

        void ResetStepDuration(const float step_duration) { this->dynParams.scanDuration = step_duration;
            this->dynParams.period = this->dynParams.scanDuration * this->dynParams.bufSize; };
        void ResetPeriod(const float period) { this->dynParams.period = period;
            this->dynParams.scanDuration = this->dynParams.period / (float)this->dynParams.bufSize; };
        void ResetBufSize(const BufSize buf_size) { this->dynParams.bufSize = To_uint16(buf_size); 
            this->dynParams.period = this->dynParams.bufSize * this->dynParams.scanDuration;
            this->InitArr();} 
        void ResetConstVal(const uint16_t const_val) { this->dynParams.constVal = const_val; }
        // Show the wave array (delay_time: x (ms))
        static void ShowStatic(uint8_t line = 4, uint16_t delay_time = 500);
        void ShowDynamic(uint8_t line = 4, uint16_t delay_time = 500) const;

        
    };

    struct DAC_ParamsTypedef {
        // Sub Params pin
        DAC_Channel channel = DAC_Channel::CH1;
        DAC_GPIO_PIN pin = DAC_GPIO_PIN::PIN4;
        // Sub Params dacTrigger, timIRQn
        TIM_TypeDef *TIM = TIM2;
        DAC_Trigger trigger = DAC_Trigger::T2_TRGO;
        // Sub Params dacTrigger
        DAC_UpdateMode updateMode = DAC_UpdateMode::TIM_IT_UPDATE;
        IRQn timIRQn = IRQn::TIM2_IRQn;
        
        DAC_ParamsTypedef() = default;
        DAC_ParamsTypedef(DAC_Channel channel, TIM_TypeDef *TIM, DAC_UpdateMode updateMode);
        DAC_ParamsTypedef(DAC_Channel channel, DAC_GPIO_PIN pin, TIM_TypeDef *TIM, DAC_Trigger trigger, DAC_UpdateMode updateMode, IRQn timIRQn) :
            channel(channel), pin(pin), TIM(TIM), trigger(trigger), updateMode(updateMode), timIRQn(timIRQn) {}

        public:
            bool AssignParams(DAC_Channel dac_channel, TIM_TypeDef * timx_dac);
            bool AssignUpdateMode(DAC_UpdateMode update_mode);
            bool AssignDacTimIRQn(void);
            
    };

    struct DMA_ParamsTypedef {
        // Up Params: TIM, timDmaSource
        DMA_Channel_TypeDef *channel;
        // Sub Params: channel
        TIM_TypeDef *TIM = TIM3;
        // Sub Params: channel
        TIM_DMA_Source timDmaSource = TIM_DMA_Source::CH1;

        DMA_ParamsTypedef() = default;
        DMA_ParamsTypedef(DMA_Channel_TypeDef *channel, TIM_TypeDef *TIM, TIM_DMA_Source timDmaSource) :
            channel(channel), TIM(TIM), timDmaSource(timDmaSource) {}
    };

    class DAC_ChanController
    {
    private:
        //新增暂停恢复功能
        struct PauseState {
            bool isPaused = false;                     // 是否处于暂停状态
            FunctionalState preDACState = DISABLE;     // 暂停前DAC使能状态
            FunctionalState preTIMDACState = DISABLE;  // 暂停前DAC定时器使能状态
            FunctionalState preTIMDMAState = DISABLE;  // 暂停前DMA定时器使能状态
            FunctionalState preDMAState = DISABLE;     // 暂停前DMA通道使能状态
            FunctionalState preTIMITState = DISABLE;   // 暂停前定时器中断使能状态
            FunctionalState preTIMDMACmdState = DISABLE; // 暂停前定时器DMA命令使能状态
        } pauseState;  // 暂停状态实例


        enum class State {
            UNINITIALIZED,
            INITIALIZED,
            PAUSED,
            RUNNING,
            STOPPED,
        };
        State state = State::UNINITIALIZED;

        // Base Methods
        // Assign DAC Params
        bool AssignDacParams(DAC_Channel dac_channel, TIM_TypeDef * timx_dac);
        bool AssignDacUpdateMode();
        bool AssignDacTrigger(void);        // Sub to DacUpdateMode
        bool AssignDacTimIRQn(void);
        // Assign DMA Params
        bool AssignDmaParams(TIM_TypeDef * timx_dma, TIM_DMA_Source tim_dma_source);
        // 实现了 TIM1_UP, TIM2_UP, TIM3_UP, TIM4_UP 的DMA1触发自动分配
        bool AssignDmaTrigger(void);
        bool AssignDmaTrigger(TIM_DMA_Source dma_tim_trigger);

        // After WaveForm Reset
        bool AssignValAboutWaveForm(void);

        void SetDacChannelDate();
        void RCC_PeriphSetState(FunctionalState state);
        void AllPeriph_Cmd(FunctionalState state);
    // 在DAC_ChanController类中添加静态成员

    protected:
        // Important Variables
        // Sub Params dacUpdateMode, dacTrigger, scanDuration
        WaveForm waveForm;

        DAC_ParamsTypedef DAC_Params;
        DMA_ParamsTypedef DMA_Params;

        WaveArr waveArr;
        CV_Controller cvController;

        float scanDuration = 0.05f;
        
    public:
    // TIMx_DAC Must be TIM2/TIM3/TIM4
        explicit DAC_ChanController(WaveForm wave_form = WaveForm::CV_FLUCTUATE, DAC_Channel dac_channel = DAC_Channel::CH1, TIM_TypeDef * timx_dac = TIM2, TIM_TypeDef * timx_dma = TIM2, TIM_DMA_Source dma_tim_trigger = TIM_DMA_Source::UP, BufSize wave_buf_size = BufSize::BUF_32, CV_VoltParams volt_params = CV_VoltParams(0.8f, -0.8f, 1.0f), CV_Params cv_params = CV_Params(0.05f, 0.05f, ScanDIR::FORWARD), DAC_UpdateMode dac_update_mode = DAC_UpdateMode::TIM_IT_UPDATE);
        explicit DAC_ChanController(WaveForm wave_form, DAC_ParamsTypedef dac_params,  DMA_ParamsTypedef dma_params, BufSize wave_buf_size, CV_VoltParams volt_params, CV_Params cv_params = CV_Params(0.05f, 0.05f, ScanDIR::FORWARD), DAC_UpdateMode dac_update_mode = DAC_UpdateMode::TIM_IT_UPDATE);

        ~DAC_ChanController();

        void Pause();   // 暂停功能
        void Resume();  // 重新开始功能
        bool IsPaused() const { return pauseState.isPaused; }  // 获取暂停状态
        
        // TODO:
        void ResetWaveForm(WaveForm wave_form);
        void ResetDacParams(DAC_Channel dac_channel);
        void ResetDacParams(TIM_TypeDef * timx_dac);
        void ResetDacParams(DAC_Channel dac_channel, TIM_TypeDef * timx_dac);
        void ResetDmaTrigger(TIM_TypeDef * timx_dma, const TIM_DMA_Source dma_tim_trigger = TIM_DMA_Source::UP);
        void ResetWaveArr(BufSize wave_buf_size);
        void ResetCvUpdateMode(DAC_UpdateMode update_mode);
        void ResetCvController(CV_VoltParams volt_params, CV_Params cv_params = CV_Params(0.05f, 0.05f, ScanDIR::FORWARD), DAC_UpdateMode dac_update_mode = DAC_UpdateMode::TIM_IT_UPDATE);

        // TODO:
        void ReStore();

        // Important Functions
        void GPIO_Setup(void);
        void DAC_Setup(void);
        void TIM_DAC_Setup(void);
        void TIM_DAC_IRQHandler(void);
        void DMA_Setup(void);
        void TIM_DMA_Setup(void);
        
        // Get Functions
        const DAC_ParamsTypedef & Get_DAC_Params(void) {return this->DAC_Params;}
        const DMA_ParamsTypedef & Get_DMA_Params(void) {return this->DMA_Params;}
        const WaveArr & Get_WaveArr(void) const {return this->waveArr;}
        CV_Controller & Get_CV_Controller(void) {return this->cvController;}

        DAC_ParamsTypedef & Modifiable_DAC_Params(void) {return this->DAC_Params;}
        DMA_ParamsTypedef & Modifiable_DMA_Params(void) {return this->DMA_Params;}
        
        // Control Functions
        void SetState(FunctionalState state);
        void Start();
        void Stop();
    };

    class DAC_Controller
    {
    private:

    protected:
        DAC_Channel cv_channel, constant_channel;

        void Init(DAC_Channel cv_channel = DAC_Channel::CH1);

    public:
        DAC_Controller(DAC_Channel cv_channel = DAC_Channel::CH1);
        ~DAC_Controller();

        static DAC_ChanController dacChanCV;    // 声明CV波动控制器
        static DAC_ChanController dacChanConstant; // 声明恒定电压控制器

        const DAC_ChanController & GetDACChanCV() { return dacChanCV; }
        const DAC_ChanController & GetDACChanConstant() { return dacChanConstant;  }

        void SetCV_Channel(DAC_Channel cv_channel = DAC_Channel::CH1);
    };

    
    const uint16_t GetCvValToSend(void);
    const uint16_t * GetCvValToSendBuf(void);
    const uint16_t & GetCvValToSendRef(void);

    // 获取ADC DMA缓冲区头指针
    const uint16_t * GetADCDmaBufHeader(void);
    // 获取ADC DMA缓冲区引用
    const std::array<uint16_t, 16> & GetADCDmaBufRef(void);
    // 获取ADC DMA缓冲区对应计算所得电流大小缓冲区头指针
    const double * GetADCCurrentBufHeader(void);
    // 获取ADC DMA缓冲区对应计算所得电流大小缓冲区引用
    const std::array<double, 16> & GetADCCurrentBufRef(void);

    const NS_ADC::ADC & GetADCRef(void);


    // 直接返回 adc 的可写引用（用于 adc.Service()）
    NS_ADC::ADC & GetADC(void);


    // 系统控制器类，采用单例模式实现系统级暂停与恢复
    class SystemController {

    private:
        enum class SystemState {
            RUNNING,
            PAUSED,
            DEFAULT
        };


        uint32_t updataTimes = 0;
        SystemState systemState = SystemState::DEFAULT;
        // 私有构造函数（禁止外部实例化）
        SystemController() = default;

        // 私有析构函数（禁止外部销毁）
        ~SystemController() = default;

        // 清零系统计数器
        void ClearUpdataTimes() { SystemController::updataTimes = 0; }
    public:
        // 获取单例实例（唯一入口）
        static SystemController& GetInstance();

        // 系统启动
        void Start();

        // 系统级暂停：暂停ADC和所有DAC通道
        void Pause();

        // 系统级恢复：恢复ADC和所有DAC通道
        void Resume();

        // 返回系统计数器值
        const uint32_t & GetUpdataTimes() const { return updataTimes; }
        // 更新系统计数器
        void UpdateTimes() { if (systemState == SystemState::RUNNING) ++updataTimes; }

        bool IsRunning() const { return systemState == SystemState::RUNNING; }
        bool IsPaused() const { return systemState == SystemState::PAUSED; }

        // 禁止拷贝构造和赋值操作（确保单例唯一性）
        SystemController(const SystemController&) = delete;
        SystemController& operator=(const SystemController&) = delete;

    
    };


} // namespace NS_DAC


