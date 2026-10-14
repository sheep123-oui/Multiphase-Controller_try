#include "scheduler.h"
// #include "status_led_langue.h"
#include "Vofa_send.h"
#include "adc_handle.h"
#include "main_ctrl.h"

uint8_t task_num;

#define Debug_State 1
#define FORCE_CAP_ENABLE_DEBUG 1
#define FORCE_EN_MOS_ENABLE_DEBUG 1

#if Debug_State == 1 // Debug

#elif Debug_State == 0 // Release
#include "iwdg.h"
#endif

extern scheduler_task_t scheduler_task[];

void ALL_Init_Task(void)
{
    Scheduler_init();
    ADC_handle_Init();
    Vofa_Send_Init();
    main_ctrl_Init();

    // status_init();

#if Debug_State == 0 // Release
    HAL_IWDG_Init(&hiwdg);
#endif
}

void led_flash(void)
{ // 10ms
  // status_LED_flash();
}
void wdg_flash(void)
{

#if Debug_State == 0 // Release
    HAL_IWDG_Refresh(&hiwdg);
#endif
}
void sys_data_sent(void)
{
    // Vofa_Send_Data2_U3(cnt_on_P, cnt_on_I);
    // {CoreDebug->DHCSR,SCB->CPUID,CoreDebug->DCRSR,CoreDebug->DCRDR,CoreDebug->DEMCR};
}

// void adc_data_update(void)
// {
//     ADC_handle_task(&User_ADC_out_data);
// }

void Start_task(void)
{
    //start_uvlo();

#if FORCE_CAP_ENABLE_DEBUG
    /* Debug only: keep the capacitor enable asserted for EN_MOS testing. */
    Cap_Enable_set(1);
#endif

#if FORCE_EN_MOS_ENABLE_DEBUG
    /* Debug only: bypass UVLO shutdown and force EN_MOS high. */
    ctrl_Enable_set(1);
#endif
}

void uart_cmd_find(void)
{
    // Uart_Cmd_find();
    Vofa_Send_Data8(User_ADC_out_data.V_BAT,
                    User_ADC_out_data.V_CAP,
                    User_ADC_out_data.Current_BAT,
                    User_ADC_out_data.Current_CAP_A,
                    User_ADC_out_data.Current_CAP_B,
                    User_ADC_out_data.Current_Chassis, 1, 1);
	
}

//************************************//
scheduler_task_t scheduler_task[] =
    {
        //@毫秒

        // {led_flash, 10, 0},
        //{adc_data_update, 1, 0},

        {uart_cmd_find, 10, 0},
        {PowerlimitUpdata, 1, 0},
        {wdg_flash, 200, 0},
        {Start_task, 1, 0},

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

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM15)
    {
       main_ctrl_task();
    }
}
