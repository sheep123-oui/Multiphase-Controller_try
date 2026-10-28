#pragma once
#include "main.h"

void ctrl_set_power(float power);

void ctrl_Enable_set(uint8_t enable);

void Cap_Enable_set(uint8_t enable);

void start_uvlo(void);

void main_ctrl_Init(void);

void ctrl_Updata_pid(float p, float i, float d);

void main_ctrl_task(void);

void HRtim_start(void);

void DAC_start(void);

void DAC_set_value(float current_H, float current_L);

void Ctrl_Set_hysteresis_width(float width);

void Ctrl_hysteresis(float current);

void PowerlimitUpdata(void);

