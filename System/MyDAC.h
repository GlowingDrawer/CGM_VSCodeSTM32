#ifndef __MYDAC_H
#define __MYDAC_H

#ifdef __cplusplus
 extern "C" {
#endif 

extern const uint16_t Sine12bit[32];
extern uint32_t DualSine12bit[32];

void MyDAC_Init(void);
void DAC_SetChannelVolt(float voltage);
void DAC_SetChanne2Volt(float voltage);

#ifdef __cplusplus
 }
#endif 


#endif
