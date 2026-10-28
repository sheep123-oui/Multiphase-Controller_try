#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "main.h"

typedef struct
{
	void(*task_func)(void);
	uint16_t rate_ms;
	int32_t volatile last_run;
}scheduler_task_t;

extern scheduler_task_t scheduler_task[];



void Scheduler_init(void);
void Scheduler_ALL_Init_Task(void);
void Scheduler_run(void);
void Scheduler_stop(void (*task_func_P)(void));
void Scheduler_continue(void (*task_func_P)(void));
void Scheduler_set_RateTime(void (*task_func_P)(void), uint16_t time);
void Scheduler_Delay(void (*task_func_P)(void));

void Scheduler_up_task(void(*task_func_P)(void), uint8_t next_time);


void average1S(void);

void can_sent(void);

void free_Uart_sent(void);

void Cap_V_Limit(void);

void Start_task(void);

void wdg_flash(void);

#endif


