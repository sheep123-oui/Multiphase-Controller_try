#ifndef __RAMP_H
#define __RAMP_H
#include "main.h"
#include "math.h"

typedef struct
{
	float input;
	float out;
	float num[1];
	float frame_period;
} first_order_filter_type_t;

typedef struct
{
	float x_last;
	float p_last;
	float out;
} one_kalman_filter_init_t;


void FirstOrderFilterInit(first_order_filter_type_t* first_order_filter_type, float frame_period, const float num[1]);

void FirstOrderFilterCali(first_order_filter_type_t* first_order_filter_type, float input);

void SlopeSlow(float* rec, float target, float slow_Inc);

float  KalmanFilter(one_kalman_filter_init_t* data, float ResrcData, float ProcessNoise_Q, float MeasureNoise_R);

float RC_filter(float last_out, float input, float alpha);

void FloatToChar(float fNum, unsigned char* strBuf, int nLen);
void CharToFloat(float* fNum, unsigned char* strBuf, int nLen);
void Uint16ToChar(uint16_t fNum, unsigned char* strBuf, int nLen);
void CharToUint16(uint16_t* fNum, unsigned char* strBuf, int nLen);
void int16ToChar(int16_t fNum, unsigned char* strBuf, int nLen);
void CharToint16(int16_t* fNum, unsigned char* strBuf, int nLen);

#endif
