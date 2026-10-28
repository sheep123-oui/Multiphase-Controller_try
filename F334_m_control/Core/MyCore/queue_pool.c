#include "queue_pool.h"

// #include <stdlib.h>
// #include <string.h>
#include "string.h"
extern Queue_uint8_ LED_status_cmd_Queue;

void Queue_Init(Queue_uint8_ *queue, uint8_t *pdata, uint8_t queue_listsize, uint8_t queue_data_size)
{
	queue->pdata= pdata;
	queue->data_size = queue_data_size;
	queue->list_size = queue_listsize;
	if (queue->list_size > sizeof(queue->mutex))
	{
		Error_Handler();
	}
	
}

uint8_t Queue_put_uint8(Queue_uint8_ *queue, uint8_t *data, uint8_t len)
{
	if (queue->used_size >= queue->list_size)
	{
		return QUEUE_ERROR_FULL;
	}
	if (len > queue->data_size)
	{
		return QUEUE_ERROR_DATA_OVERFLOW;
	}
	if (queue->state_lock == QUEUE_LOCK)
	{
		return QUEUE_ERROR_BUSY;
	}
	Queue_state_Lock(queue);
	uint8_t index_temp = queue->front;
	uint8_t index_next = queue->front + 1;
	if ((index_next) == queue->list_size)
	{
		index_next = 0; // 回环
	}

	if (queue->mutex[index_temp] == QUEUE_LOCK)
	{
		Queue_state_unLock(queue);
		return QUEUE_ERROR_BUSY;
	}
	queue->front = index_next;
	queue->used_size++;

	Queue_state_unLock(queue);

	Queue_data_Lock(queue, index_temp);
	memcpy(&queue->pdata[index_temp*queue->data_size], data, len);
	Queue_data_unLock(queue, index_temp);

	return QUEUE_OK;
}


uint8_t Queue_get_uint8(Queue_uint8_ *queue, uint8_t *data, uint8_t len)
{
	if (queue->used_size == 0)
	{
		return QUEUE_ERROR_EMPTY;
	}
	if (queue->state_lock == QUEUE_LOCK)
	{
		return QUEUE_ERROR_BUSY;
	}
	Queue_state_Lock(queue);
	uint8_t index_temp = queue->rear;
	if (queue->mutex[index_temp] == QUEUE_LOCK)
	{
		Queue_state_unLock(queue);
		return QUEUE_ERROR_BUSY;
	}
	
	if (index_temp+1 >= queue->list_size)
	{
		queue->rear= 0; // 回环
	}
	else
	{
		queue->rear = index_temp + 1;
	}
	queue->used_size--;
	Queue_state_unLock(queue);
	if (len > queue->data_size)
	{
		len = queue->data_size;
	}
	Queue_data_Lock(queue, index_temp);
	memcpy(data, &queue->pdata[index_temp*queue->data_size], len);
	memset(&queue->pdata[index_temp*queue->data_size], 0, queue->data_size);
	Queue_data_unLock(queue, index_temp);

	return QUEUE_OK;
}

uint8_t Queue_used_uint8(Queue_uint8_ *queue)
{
	return queue->used_size;
}

void Queue_clear_uint8(Queue_uint8_ *queue)
{
}

void Queue_data_Lock(Queue_uint8_ *queue, uint8_t index)
{
	queue->mutex[index] = QUEUE_LOCK;
}
void Queue_data_unLock(Queue_uint8_ *queue, uint8_t index)
{
	queue->mutex[index] = QUEUE_UNLOCK;
}
void Queue_state_Lock(Queue_uint8_ *queue)
{
	queue->state_lock = QUEUE_LOCK;
}
void Queue_state_unLock(Queue_uint8_ *queue)
{
	queue->state_lock = QUEUE_UNLOCK;
}
