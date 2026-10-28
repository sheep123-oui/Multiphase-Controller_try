#pragma once

// #include "stdint.h"
#include "main.h"

#define MAX_QUEUE_LIST 10

typedef struct __queue_uint8_
{
	uint8_t *pdata;
	uint8_t mutex[MAX_QUEUE_LIST]; // 0:free 1:lock
	// uint8_t group_id[4];
	/// @arg  group_id_list
	uint8_t front; // 队头
	uint8_t rear;  // 队尾
	uint8_t state_lock;
	uint8_t list_size; // 最大为MAX_QUEUE_LIST
	uint8_t data_size;
	uint8_t used_size;
} Queue_uint8_;

enum
{
	QUEUE_UNLOCK = 0,
	QUEUE_LOCK = 1,
};

enum
{
	QUEUE_OK = 1,
	QUEUE_ERROR_FULL = 2,
	QUEUE_ERROR_DATA_OVERFLOW = 3,
	QUEUE_ERROR_BUSY = 4,
	QUEUE_ERROR_EMPTY = 5,
};

void Queue_Init(Queue_uint8_ *queue, uint8_t *pdata, uint8_t queue_listsize, uint8_t queue_data_size);

uint8_t Queue_put_uint8(Queue_uint8_ *queue, uint8_t *data, uint8_t len);
uint8_t Queue_get_uint8(Queue_uint8_ *queue, uint8_t *data, uint8_t len);

uint8_t Queue_used_uint8(Queue_uint8_ *queue);

void Queue_data_Lock(Queue_uint8_ *queue, uint8_t index);
void Queue_data_unLock(Queue_uint8_ *queue, uint8_t index);
void Queue_state_Lock(Queue_uint8_ *queue);
void Queue_state_unLock(Queue_uint8_ *queue);

/** @defgroup group_id_list
 *
 *
 */