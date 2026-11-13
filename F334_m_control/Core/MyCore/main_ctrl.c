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
#include "current_calibration.h"
#include <math.h>

/* The comparator receives (I_A + I_B) / 2; software power uses total current. */
#define CAP_TOTAL_TO_AVERAGE_CURRENT 0.5f
#define CAP_MIN_CONTROL_VOLTAGE 1.0f
/* Assumes positive current charges the capacitor. Calibrate for the hardware. */
#define CAP_VOLTAGE_COMPENSATION_RESISTANCE 0.115f
#define POWER_MONITOR_SAMPLE_COUNT 1000U
#define BAT_POWER_CORRECTION_LIMIT 10.0f

float hysteresis_width = 0.2f; // Half-width in average-current amperes.
float current_save = 0;

uint8_t ctrl_enable_flag = 0;
uint8_t Cap_enable_flag = 0;

float Bat_power_set = 40;
float Cap_current_set = 0; // Total capacitor-current target, A + B.
Power_monitor_data_t Power_monitor_data = { 0 };
static volatile uint32_t power_monitor_sequence = 0;
static volatile float battery_power_correction = 0.0f; // W; main task to TIM15.


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
	/* Initialize all control state before enabling the 100 kHz interrupt. */
	PID_struct_init(&Bat_power_loop, PID_DELTA,
		BAT_POWER_CORRECTION_LIMIT, -BAT_POWER_CORRECTION_LIMIT,
		BAT_POWER_CORRECTION_LIMIT, -BAT_POWER_CORRECTION_LIMIT,
		0.05f, 0.001f, 0.0f);
	Pid_clear_i(&Bat_power_loop);
	battery_power_correction = 0.0f;
	Ctrl_Set_hysteresis_width(0.3f);
	Ctrl_hysteresis(0.3f);
	HRtim_start();
	DAC_start();
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

static void power_monitor_update(void)
{
	static float current_sum = 0.0f;
	static float cap_power_sum = 0.0f;
	static float bat_power_sum = 0.0f;
	static uint16_t sample_count = 0;

	/* Diagnostic statistics only: never feed these averages back to the DAC. */
	current_sum += User_ADC_out_data.Current_CAP;
	cap_power_sum += User_ADC_out_data.Cap_power;
	bat_power_sum += User_ADC_out_data.Bat_power;
	if (++sample_count >= POWER_MONITOR_SAMPLE_COUNT)
	{
		const float inverse_count = 1.0f / POWER_MONITOR_SAMPLE_COUNT;
		Power_monitor_data.Current_CAP_mean = current_sum * inverse_count;
		Power_monitor_data.Cap_power_mean = cap_power_sum * inverse_count;
		Power_monitor_data.Bat_power_mean = bat_power_sum * inverse_count;
		++power_monitor_sequence;
		current_sum = 0.0f;
		cap_power_sum = 0.0f;
		bat_power_sum = 0.0f;
		sample_count = 0;
	}
}

void main_ctrl_task(void)
{
	ADC_handle_task(&User_ADC_out_data);
	User_ADC_out_data.Cap_power = User_ADC_out_data.Current_CAP * User_ADC_out_data.V_CAP;
	User_ADC_out_data.Bat_power = User_ADC_out_data.Current_BAT * User_ADC_out_data.V_BAT;
	User_ADC_out_data.Chassis_power = User_ADC_out_data.Current_Chassis * User_ADC_out_data.V_BAT;
	power_monitor_update();

	float power_delta = Bat_power_set - User_ADC_out_data.Chassis_power;
	/* Keep the empty-load isolation test; apply the slow loop's correction in W. */
	// float power_delta = Bat_power_set + battery_power_correction;
	if (isfinite(power_delta) && isfinite(User_ADC_out_data.V_CAP) &&
		User_ADC_out_data.V_CAP >= CAP_MIN_CONTROL_VOLTAGE)
	{
		Cap_current_set = power_delta / User_ADC_out_data.V_CAP;
		Clamp_de_(-36.0f, 32.0f, Cap_current_set);
	}
	else
	{
		/* Do not turn an invalid/near-zero voltage into a full-current request. */
		Cap_current_set = 0.0f;
	}
	Ctrl_hysteresis(Cap_current_set * CAP_TOTAL_TO_AVERAGE_CURRENT);
	// Ctrl_hysteresis(5.0f);

	// ctrl_check();

	// static uint32_t count = 0;
	// count++;
	// if (count >= 50)
	// {
	// 	Vofa_Send_Data_s((float*)&User_ADC_out_data, sizeof(User_ADC_out_data) / sizeof(float));
	// 	count = 0;
	// }
}

void power_loop_task(void)
{
	static uint32_t last_sequence = 0;
	uint32_t irq_state = __get_PRIMASK();
	__disable_irq();
	uint32_t sequence = power_monitor_sequence;
	float measured_power = Power_monitor_data.Bat_power_mean;
	float cap_voltage = User_ADC_out_data.V_CAP;
	float target_power = Bat_power_set;
	uint8_t enabled = ctrl_enable_flag;
	__set_PRIMASK(irq_state);

	if (!enabled || !isfinite(measured_power) || !isfinite(target_power) ||
		!isfinite(cap_voltage) || cap_voltage < CAP_MIN_CONTROL_VOLTAGE)
	{
		Pid_clear_i(&Bat_power_loop);
		battery_power_correction = 0.0f;
		last_sequence = sequence;
		return;
	}
	/* Never integrate the same 10 ms measurement window twice. */
	if (sequence == last_sequence)
	{
		return;
	}
	last_sequence = sequence;
	float correction = pid_calc(&Bat_power_loop, measured_power, target_power);
	if (!isfinite(correction))
	{
		Pid_clear_i(&Bat_power_loop);
		correction = 0.0f;
	}
	Clamp_de_(-BAT_POWER_CORRECTION_LIMIT, BAT_POWER_CORRECTION_LIMIT, correction);
	battery_power_correction = correction;
}

void HRtim_start(void)
{
	HAL_HRTIM_WaveformCountStart(&hhrtim1, HRTIM_TIMERID_TIMER_A);
	HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2);
}
void DAC_start(void)
{
	HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
	HAL_DAC_Start(&hdac2, DAC_CHANNEL_1);
	HAL_TIM_Base_Start_IT(&htim15);
}
void DAC_set_value(float current_H, float current_L)
{
	uint16_t value1, value2;
	/* Thresholds use the measured average-output zero and actual DAC reference. */
	value1 = (current_H * CAP_CURRENT_VOLTS_PER_AMP + CAP_AVG_FEEDBACK_ZERO_VOLTAGE) * CAP_DAC_CODES_PER_VOLT;
	value2 = (current_L * CAP_CURRENT_VOLTS_PER_AMP + CAP_AVG_FEEDBACK_ZERO_VOLTAGE) * CAP_DAC_CODES_PER_VOLT;
	HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, value1);
	HAL_DAC_SetValue(&hdac2, DAC_CHANNEL_1, DAC_ALIGN_12B_R, value2);
}

void Ctrl_Set_hysteresis_width(float width)
{
	hysteresis_width = width;
	Ctrl_hysteresis(current_save);
}

void Ctrl_hysteresis(float average_current)
{
	float current_H, current_L;
	current_save = average_current;
	current_H = average_current + hysteresis_width;
	current_L = average_current - hysteresis_width;
	DAC_set_value(current_H, current_L);
}

/* Keep 30 W below 23.8 V, then taper to 2 W at 24.3 V.
 * Reaching the last voltage point must not introduce a power discontinuity. */
#ifndef POWER_STEP_COUNT
#define POWER_STEP_COUNT 6
#endif

#ifndef POWER_END_POWER
#define POWER_END_POWER 2.0f
#endif

const float POWER_VOLTAGE_POINTS[POWER_STEP_COUNT] = { 0.0f, 12.0f, 17.0f, 20.8f, 23.8f, 24.3f };
// const float POWER_VALUES[POWER_STEP_COUNT] = { 30.0f, 30.0f, 30.0f, 30.0f, 30.0f, POWER_END_POWER };
const float POWER_VALUES[POWER_STEP_COUNT] = { 45.0f, 200.0f, 150.0f, 60.0f, 30.0f, POWER_END_POWER };

void PowerlimitUpdata(void)
{
	/* Read voltage and total current from the same control-interrupt update. */
	uint32_t irq_state = __get_PRIMASK();
	__disable_irq();
	float cap_voltage = User_ADC_out_data.V_CAP;
	float cap_current = User_ADC_out_data.Current_CAP;
	__set_PRIMASK(irq_state);
	float V_real_measure = cap_voltage - cap_current * CAP_VOLTAGE_COMPENSATION_RESISTANCE;
	if (!isfinite(V_real_measure))
	{
		Bat_power_set = POWER_END_POWER;
		return;
	}
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
		/* Match the last point's power continuously above the full-voltage limit. */
		if (V_real_measure > POWER_VOLTAGE_POINTS[POWER_STEP_COUNT - 1])
		{
			battery_power_limit_temp = POWER_END_POWER;
		}
	}

	Bat_power_set = battery_power_limit_temp;
	Clamp_de_(POWER_END_POWER, 200.0f, Bat_power_set);
}
