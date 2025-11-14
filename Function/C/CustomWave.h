#ifndef __CUSTOMWAVE_H
#define __CUSTOMWAVE_H

#ifdef __cplusplus
extern "C" {
#endif 

int Get_DAC_Value(void);

void CustomWave_DAC_Config(uint32_t DAC_Channel_x, uint32_t DAC_Trigger);
void CustomWave_Init(void);



#ifdef __cplusplus
}
#endif 




#endif
