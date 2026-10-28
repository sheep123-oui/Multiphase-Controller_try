#include "main_ctrl.h"
#include "uart_data_handle.h"
#include "hrtim.h"
#include "dac.h"
#include "tim.h"
#include "adc_handle.h"
#include "Pid.h"
#include "My_Math.h"
#include "Vofa_send.h"
#include "status_led_langue.h"


float hysteresis_width = 0.2f;
float current_save = 0;

uint8_t ctrl_enable_flag = 0;
uint8_t Cap_enable_flag = 0;

float Bat_power_set = 40;
float Cap_current_set = 0;



Pid_t Bat_power_loop = { 0 };

void ctrl_set_power(float power)
{
	Bat_power_set = power;
}

void ctrl_Enable_set(uint8_t enable)
{
	if (Cap_enable_flag == 0 || enable == 0)
	{
		HAL_GPIO_WritePin(EN_MOS_GPIO_Port, EN_MOS_Pin, 0);
		HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, 1);
		ctrl_enable_flag = 0;
	}
	else if (ctrl_enable_flag == 0 && enable == 1)
	{
		HAL_GPIO_WritePin(EN_MOS_GPIO_Port, EN_MOS_Pin, 1);
		HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, 0);
		ctrl_enable_flag = 1;
	}
}

void Cap_Enable_set(uint8_t enable)
{
	if (enable == 1)
	{
		HAL_GPIO_WritePin(Cap_En_GPIO_Port, Cap_En_Pin, 1);
		Cap_enable_flag = 1;
	}
	else
	{
		HAL_GPIO_WritePin(Cap_En_GPIO_Port, Cap_En_Pin, 0);
		Cap_enable_flag = 0;
	}
}

void start_uvlo(void)
{
	static uint8_t start_flag = 0;

	/* ---- UVLO: battery voltage too low, orderly shutdown ---- */
	if (User_ADC_out_data.V_BAT < 20.71f)
	{
		if (start_flag < 100)
		{
			start_flag = 100;				/* enter UVLO counting state */
		}
		else if (start_flag < 150)
		{
			start_flag++;					/* 50-unit delay before shutdown */
		}
		else if (start_flag == 150)
		{
			ctrl_Enable_set(0);				/* step 1: disable control first */
			start_flag++;
		}
		else if (start_flag < 160)
		{
			start_flag++;					/* 10-unit interval before disabling cap */
		}
		else /* start_flag == 160 */
		{
			Cap_Enable_set(0);				/* step 2: then disable capacitor */
			start_flag = 0;					/* fully reset for next startup */
		}
		return;
	}

	/* ---- Startup: battery voltage OK, sequenced enable ---- */
	if (User_ADC_out_data.V_BAT > 21.0f)
	{
		/* If recovering from an interrupted UVLO sequence, restart cleanly */
		if (start_flag >= 100)
		{
			start_flag = 0;
		}

		if (start_flag < 5)
		{
			start_flag++;					/* debounce delay */
		}
		else if (start_flag == 5)
		{
			Cap_Enable_set(1);				/* step 1: enable capacitor pre-charge first */
			start_flag++;
		}
		else if (start_flag < 50)
		{
			start_flag++;					/* wait for capacitor to stabilize */
		}
		else if (start_flag == 50)
		{
			ctrl_Enable_set(1);				/* step 2: then enable main control */
			start_flag = 51;				/* normal operation */
		}
	}
}

void ctrl_check(void)
{
	if (User_ADC_out_data.V_BAT > 30.0f)
	{
		ctrl_Enable_set(0);
	}
}

void main_ctrl_Init(void)
{
	DAC_start();
	HRtim_start();
	// DAC_set_value(1, -1);
	Ctrl_Set_hysteresis_width(0.1f);
	Ctrl_hysteresis(0.3f);
	//! 电池功率环路PID还没细调
	PID_struct_init(&Bat_power_loop, PID_DELTA, 10, -10, 10, -10, 0.05f, 0.001f, 0.0f);
}
void ctrl_Updata_pid(float p, float i, float d)
{
	Bat_power_loop.p = p;
	Bat_power_loop.i = i;
	Bat_power_loop.d = d;
}
void TIM1_BRK_TIM15_IRQHandler(void)
{
	__HAL_TIM_CLEAR_FLAG(&htim15, TIM_FLAG_UPDATE);
	main_ctrl_task();
}
void main_ctrl_task(void)
{
	ADC_handle_task(&User_ADC_out_data);
	User_ADC_out_data.Cap_power = User_ADC_out_data.Current_CAP * User_ADC_out_data.V_CAP;
	User_ADC_out_data.Bat_power = User_ADC_out_data.Current_BAT * User_ADC_out_data.V_BAT;
	User_ADC_out_data.Chassis_power = User_ADC_out_data.Current_Chassis * User_ADC_out_data.V_BAT;

	float power_delta = Bat_power_set - User_ADC_out_data.Chassis_power;
	// pid_calc(&Bat_power_loop, User_ADC_out_data.Bat_power, Bat_power_set);
	Cap_current_set = (power_delta ) / User_ADC_out_data.V_CAP;
	Clamp_de_(-36.0f, 32.0f, Cap_current_set);
	Ctrl_hysteresis(Cap_current_set);

	// ctrl_check();

	// static uint32_t count = 0;
	// count++;
	// if (count >= 50)
	// {
	// 	Vofa_Send_Data_s((float*)&User_ADC_out_data, sizeof(User_ADC_out_data) / sizeof(float));
	// 	count = 0;
	// }
}

void HRtim_start(void)
{
	HAL_HRTIM_WaveformCountStart(&hhrtim1, HRTIM_TIMERID_TIMER_A);
	HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2);
}
void DAC_start(void)
{
	HAL_TIM_Base_Start_IT(&htim15);
	HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
	HAL_DAC_Start(&hdac2, DAC_CHANNEL_1);
}
void DAC_set_value(float current_H, float current_L)
{
	//Current_CAP = (adc2_value[0] * 3.3f / 4095 - 1.65f) / 20 / 0.002f;
	uint16_t value1, value2;
	value1 = (current_H * 0.002f * 20 + 1.65f) / 3.3f * 4095;
	value2 = (current_L * 0.002f * 20 + 1.65f) / 3.3f * 4095;
	HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, value1);
	HAL_DAC_SetValue(&hdac2, DAC_CHANNEL_1, DAC_ALIGN_12B_R, value2);
}

void Ctrl_Set_hysteresis_width(float width)
{
	hysteresis_width = width;
	Ctrl_hysteresis(current_save);
}

void Ctrl_hysteresis(float current)
{
	float current_H, current_L;
	current_save = current;
	current_H = current + hysteresis_width;
	current_L = current - hysteresis_width;
	DAC_set_value(current_H, current_L);
}

/* Power step arrays (voltage breakpoints and target power values).
   Edit these arrays to change the multi-stage power curve.
   The length must match POWER_STEP_COUNT (default 5).
   Example default points replicate prior behavior roughly.
*/
#ifndef POWER_STEP_COUNT
#define POWER_STEP_COUNT 5
#endif

const float POWER_VOLTAGE_POINTS[POWER_STEP_COUNT] = { 0.0f, 12.0f, 17.0f, 20.8f, 24.3f };
const float POWER_VALUES[POWER_STEP_COUNT] = { 45.0f, 200.0f, 150.0f, 60.0f, 30.0f };

/* final minimum power when approaching Cap_full_voltage_value (can be changed) */
#ifndef POWER_END_POWER
#define POWER_END_POWER 2.0f
#endif

void PowerlimitUpdata(void)
{

	float V_real_measure = User_ADC_out_data.V_CAP - User_ADC_out_data.Current_CAP * 0.115f;
	Clamp_de_(0.01f, 30.0f, V_real_measure);

	float battery_power_limit_temp = POWER_VALUES[0];

	/* find interval and do linear interpolation between configured points */
	if (V_real_measure <= POWER_VOLTAGE_POINTS[0])
	{
		battery_power_limit_temp = POWER_VALUES[0];
	}
	else
	{
		uint8_t i;
		for (i = 0; i < POWER_STEP_COUNT - 1; ++i)
		{
			float x0 = POWER_VOLTAGE_POINTS[i];
			float x1 = POWER_VOLTAGE_POINTS[i + 1];
			if (V_real_measure >= x0 && V_real_measure <= x1)
			{
				float y0 = POWER_VALUES[i];
				float y1 = POWER_VALUES[i + 1];
				float t = (V_real_measure - x0) / (x1 - x0);
				battery_power_limit_temp = y0 + t * (y1 - y0);
				break;
			}
		}
		/*如果高于最后一个配置点，则夹紧到最后一个值；如果Cap_full_voltage_value更高，
		可选地插值到Cap_full_voltage_value附近的最小值*/
		if (V_real_measure > POWER_VOLTAGE_POINTS[POWER_STEP_COUNT - 1])
		{
			battery_power_limit_temp = POWER_END_POWER;
		}
	}

	Bat_power_set = battery_power_limit_temp;
	Clamp_de_(POWER_END_POWER, 200.0f, Bat_power_set);
}