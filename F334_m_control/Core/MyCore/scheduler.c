#include "scheduler.h"
#include "status_led_langue.h"
#include "uart_data_handle.h"
#include "Anotc.h"
#include "Vofa_send.h"
#include "adc_handle.h"
#include "main_ctrl.h"
uint8_t task_num;

#define Debug_State 1

#if Debug_State == 1//Debug

#elif Debug_State == 0//Release
#include "iwdg.h"
#endif

extern scheduler_task_t scheduler_task[];

void Scheduler_ALL_Init_Task(void)
{
	Scheduler_init();
	status_init();
	Uart_Tx_Init();
	Vofa_init();
	ADC_handle_Init();
	main_ctrl_Init();
#if Debug_State == 0//Release
	MX_IWDG_Init();
	hiwdg.Init.Prescaler = IWDG_PRESCALER_16;
	hiwdg.Init.Reload = 600;
	HAL_IWDG_Init(&hiwdg);
#endif

}

void led_flash(void)
{ // 10ms
	status_LED_flash();
}
void wdg_flash(void)
{

#if Debug_State == 0//Release
	HAL_IWDG_Refresh(&hiwdg);
#endif

}
void sys_data_sent(void)
{
	/* TIM15 updates these values; copy one consistent sample before sending. */
	User_ADC_out_data_t sample;
	Power_monitor_data_t monitor;
	float cap_current_target;
	uint32_t irq_state = __get_PRIMASK();
	__disable_irq();
	sample = User_ADC_out_data;
	monitor = Power_monitor_data;
	cap_current_target = Cap_current_set;
	__set_PRIMASK(irq_state);

	/* Preserve the original nine channels; append phase currents and window means. */
	float data[] = {
		sample.V_BAT,
		sample.V_CAP,
		sample.Current_BAT,
		sample.Current_CAP,
		sample.Current_Chassis,
		sample.Cap_power,
		sample.Bat_power,
		sample.Chassis_power,
		cap_current_target,
		sample.Current_CAP_A,
		sample.Current_CAP_B,
		monitor.Current_CAP_mean,
		monitor.Cap_power_mean,
		monitor.Bat_power_mean,
	};
	Vofa_Send_Data_s(data, sizeof(data) / sizeof(data[0]));
}
void Start_task(void)
{
	start_uvlo();
}


//************************************//
scheduler_task_t scheduler_task[] =
{
	//@毫秒
	{led_flash, 10, 0},
	/* 14 float32 values + JustFloat tail = 60 bytes at 100 Hz. */
	{sys_data_sent,10,0},
	{Uart_Tx_task,1,0},
	{PowerlimitUpdata,1,0},
	{power_loop_task,10,0},
	{Start_task, 1, 0},
#if Debug_State == 0
	{wdg_flash, 100, 0}
#endif
};

//************************************// 

void Scheduler_init(void)
{
	task_num = sizeof(scheduler_task) / sizeof(scheduler_task_t);
}

void Scheduler_run(void)
{
	for (uint8_t i = 0; i < task_num; i++)
	{
		int32_t now_time = HAL_GetTick();
		if (now_time - scheduler_task[i].last_run >= scheduler_task[i].rate_ms)
		{
			scheduler_task[i].last_run = now_time;
			scheduler_task[i].task_func();
		}
	}
}

void Scheduler_stop(void (*task_func_P)(void))
{
	for (size_t i = 0; i < task_num; i++)
	{
		if (scheduler_task[i].task_func == task_func_P)
			scheduler_task[i].last_run = 0x7fffffff;
	}
}

void Scheduler_continue(void (*task_func_P)(void))
{
	for (size_t i = 0; i < task_num; i++)
	{
		if (scheduler_task[i].task_func == task_func_P)
			scheduler_task[i].last_run = 0;
	}
}
void Scheduler_set_RateTime(void (*task_func_P)(void), uint16_t time)
{
	for (size_t i = 0; i < task_num; i++)
	{
		if (scheduler_task[i].task_func == task_func_P)
			scheduler_task[i].rate_ms = time;
	}
}
void Scheduler_Delay(void (*task_func_P)(void))
{
	for (size_t i = 0; i < task_num; i++)
	{
		if (scheduler_task[i].task_func == task_func_P)
		{
			scheduler_task[i].last_run = HAL_GetTick();
		}
	}
}
void Scheduler_up_task(void (*task_func_P)(void), uint8_t next_time)
{
	for (size_t i = 0; i < task_num; i++)
	{
		if (scheduler_task[i].task_func == task_func_P)
		{
			if (scheduler_task[i].rate_ms + scheduler_task[i].last_run - HAL_GetTick() > next_time)
			{
				scheduler_task[i].last_run = HAL_GetTick() - scheduler_task[i].rate_ms + next_time;
			}
		}
	}
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
	if (htim->Instance == TIM1)
	{
	}
}
