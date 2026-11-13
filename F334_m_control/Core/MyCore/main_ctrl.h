#pragma once
#include "main.h"

/* Total capacitor-current target (A + B), before the average-feedback scaling. */
extern float Cap_current_set;

/* Completed 1000-sample windows at the 100 kHz control-update rate (10 ms). */
typedef struct
{
	float Current_CAP_mean;
	float Cap_power_mean;
	float Bat_power_mean;
} Power_monitor_data_t;

extern Power_monitor_data_t Power_monitor_data;

void ctrl_set_power(float power);

void ctrl_Enable_set(uint8_t enable);

void Cap_Enable_set(uint8_t enable);

void start_uvlo(void);

void main_ctrl_Init(void);

void ctrl_Updata_pid(float p, float i, float d);

void main_ctrl_task(void);

/* Slow battery-power PID: call from the scheduler at 10 ms, never from TIM15. */
void power_loop_task(void);

void HRtim_start(void);

void DAC_start(void);

void DAC_set_value(float current_H, float current_L);

void Ctrl_Set_hysteresis_width(float width);

/* Comparator feedback is the two-phase average; pass average current in A. */
void Ctrl_hysteresis(float average_current);

void PowerlimitUpdata(void);

