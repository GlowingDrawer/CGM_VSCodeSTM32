# CPV (Cyclic Pulse Voltammetry / DPV-like) minimal patch notes

## What this adds
- A new `NS_DAC::CPV_Controller` that generates a **staircase baseline** with a **single positive pulse** per step.
- Two sampling instants per step (baseline and pulse), producing one `CPV_Point` (baseCode, pulseCode, iBaseRaw, iPulseRaw, idRaw).
- A lock-free ring buffer (ISR producer, main consumer).

## Where to integrate (your existing files)

### 1) Add new files to your project
- `CPV_Controller.h`
- `CPV_Controller.cpp`

### 2) Extend WaveForm enum
In `CustomWaveCPP.h` (namespace `NS_DAC`):
```cpp
enum class WaveForm:uint8_t {
    SINE, TRIANGLE, DC_CONSTANT, CV_FLUCTUATE, CV_CONSTANT,
    CPV_PULSE   // <-- add
};
```

### 3) Include CPV controller header
In `CustomWaveCPP.h` AFTER `CV_VoltParams` and `CV_Params` are defined (so types are available):
```cpp
#include "CPV_Controller.h"
```

### 4) Add CPV controller member to DAC_ChanController
In `class DAC_ChanController` private members:
```cpp
CPV_Controller cpvController;
```

And add a getter in public:
```cpp
CPV_Controller& Get_CPV_Controller() { return cpvController; }
const CPV_Controller& Get_CPV_Controller() const { return cpvController; }
```

### 5) Initialize CPV parameters once (constructor or runtime)
In `DAC_ChanController` constructor body (or after construction), mirror your CV init style:
```cpp
cpvController.ResetParam(volt_params, cv_params, CPV_MethodParams{});
```

### 6) Update AssignValAboutWaveForm()
In `CustomWave.cpp`:
```cpp
case WaveForm::CPV_PULSE:
    this->DAC_Params.updateMode = DAC_UpdateMode::TIM_IT_UPDATE;
    this->scanDuration = this->cpvController.GetScanDuration_s();
    break;
```

### 7) Update TIM_DAC_IRQHandler()
Replace the current CV-only handler with a waveform switch:
```cpp
void DAC_ChanController::TIM_DAC_IRQHandler(void) {
    if (this->waveForm == WaveForm::CPV_PULSE) {
        const auto& adc = NS_DAC::GetADCDmaBufRef();
        uint16_t adcRaw = adc[this->cpvController.GetAdcDmaIndex()];
        uint16_t code12 = this->cpvController.IsrUpdate(adcRaw);

        if (this->DAC_Params.channel == DAC_Channel::CH1) {
            DAC_SetChannel1Data(DAC_Align_12b_R, code12);
        } else {
            DAC_SetChannel2Data(DAC_Align_12b_R, code12);
        }
        DAC_SoftwareTriggerCmd(To_uint32(this->DAC_Params.channel), ENABLE);
        return;
    }

    // existing CV logic
    this->cvController.UpdateCurrentVal();
    if (this->DAC_Params.updateMode == DAC_UpdateMode::TIM_IT_UPDATE) {
        ...
    }
}
```

### 8) How to use (example)
Before `SystemController::Start()`:
```cpp
NS_DAC::DAC_Controller::dacChanCV.ResetWaveForm(NS_DAC::WaveForm::CPV_PULSE);

NS_DAC::CPV_MethodParams mp;
mp.step_mV = 5.0f;
mp.pulseAmp_mV = 25.0f;
mp.baseHold_ms = 50;
mp.pulseWidth_ms = 50;
mp.adcDmaIndex = 0;
mp.cycles = 0; // infinite

auto& cpv = NS_DAC::DAC_Controller::dacChanCV.Get_CPV_Controller();
cpv.ResetParam(NS_DAC::CV_VoltParams(0.4f, -0.8f, 1.5f),
              NS_DAC::CV_Params(0.001f, 0.0f, NS_DAC::ScanDIR::FORWARD), // tick=1ms
              mp);
```

In main loop, pop points and report:
```cpp
NS_DAC::CPV_Point p;
auto& cpv = NS_DAC::DAC_Controller::dacChanCV.Get_CPV_Controller();
while (cpv.PopPoint(p)) {
    // Send p.baseCode12, p.pulseCode12, p.iBaseAdcRaw, p.iPulseAdcRaw, p.idAdcRaw, p.stepSeq, p.dir
}
```

## Notes / limitations
- CPV in this patch requires `TIM_IT_UPDATE` (interrupt-driven). DMA wave output can be done later by precomputing, but phase-dependent sampling becomes harder.
- `idAdcRaw` is raw ADC delta. Converting to current should be done consistently in one place (recommended: main loop / host).
