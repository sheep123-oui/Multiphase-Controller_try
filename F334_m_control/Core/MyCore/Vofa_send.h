#ifndef VOFA_SEND_H
#define VOFA_SEND_H
#include "main.h"




// extern Vofa_data_m_2 Vofa_data_2;
// extern Vofa_data_m_4 Vofa_data_4;
// extern Vofa_data_m_8 Vofa_data_8;

void Vofa_init(void);

void Vofa_Send_Data2(float data1, float data2);
void Vofa_Send_Data4(float data1, float data2,float data3, float data4);
void Vofa_Send_Data8(float data1, float data2,float data3, float data4,float data5, float data6,float data7, float data8);
void Vofa_Send_Data2_U3(float data1, float data2);

void Vofa_Send_Data_s(float* data, uint8_t ch_num);

#endif

