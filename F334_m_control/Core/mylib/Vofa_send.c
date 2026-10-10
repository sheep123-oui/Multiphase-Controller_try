/**
  **********************************2022 CKYF***********************************
  * @file    Vofa_send.c
  * @brief   ��Vofa+��λ��ͨѶ����
	* @author  �������� - ����
  ******************************************************************************
  * @attention
  *	
  * ��һ������λ������2/4/8��float���ݡ�
  *
  **********************************2022 CKYF***********************************
  */
	
#include "Vofa_send.h"
#include "string.h"
#include "usart.h"
#include "FIFO.h"
Vofa_data_m_2 Vofa_data_2={.tail={0x00,0x00,0x80,0x7f}};
Vofa_data_m_4 Vofa_data_4={.tail={0x00,0x00,0x80,0x7f}};
Vofa_data_m_8 Vofa_data_8={.tail={0x00,0x00,0x80,0x7f}};

#define VOFA_TX_FIFO_SIZE 1024
#define VOFA_MAX_FRAME_SIZE sizeof(Vofa_data_m_8)
#define VOFA_FIFO_RECORD_SIZE (sizeof(uint16_t) + VOFA_MAX_FRAME_SIZE)

static char vofa_tx_fifo_area[VOFA_TX_FIFO_SIZE];
static uint8_t vofa_tx_fifo_record[VOFA_FIFO_RECORD_SIZE];
static uint8_t vofa_tx_dma_buffer[VOFA_MAX_FRAME_SIZE];
static volatile uint8_t vofa_tx_dma_busy;
fifo_s_t uart_tx_fifo;

static void Vofa_QueueFrame(const void *frame, uint16_t frame_size)
{
	if (frame_size > VOFA_MAX_FRAME_SIZE ||
		VOFA_FIFO_RECORD_SIZE > (uint16_t)fifo_s_free(&uart_tx_fifo))
	{
		return;
	}

	memset(vofa_tx_fifo_record, 0, sizeof(vofa_tx_fifo_record));
	memcpy(vofa_tx_fifo_record, &frame_size, sizeof(frame_size));
	memcpy(vofa_tx_fifo_record + sizeof(frame_size), frame, frame_size);

	if (fifo_s_puts(&uart_tx_fifo, (char *)vofa_tx_fifo_record,
					VOFA_FIFO_RECORD_SIZE) != VOFA_FIFO_RECORD_SIZE)
	{
		return;
	}

	Vofa_Send_Process();
}

void Vofa_Send_Init(void)
{
	fifo_s_init(&uart_tx_fifo, vofa_tx_fifo_area, VOFA_TX_FIFO_SIZE);
	vofa_tx_dma_busy = 0;
}

void Vofa_Send_Process(void)
{
	uint16_t frame_size;

	if (vofa_tx_dma_busy ||
		fifo_s_used(&uart_tx_fifo) < VOFA_FIFO_RECORD_SIZE)
	{
		return;
	}

	if (fifo_s_gets(&uart_tx_fifo, (char *)vofa_tx_fifo_record,
					VOFA_FIFO_RECORD_SIZE) != VOFA_FIFO_RECORD_SIZE)
	{
		return;
	}

	memcpy(&frame_size, vofa_tx_fifo_record, sizeof(frame_size));
	if (frame_size > VOFA_MAX_FRAME_SIZE)
	{
		return;
	}

	memcpy(vofa_tx_dma_buffer,
		   vofa_tx_fifo_record + sizeof(frame_size),
		   frame_size);

	if (HAL_UART_Transmit_DMA(&huart1, vofa_tx_dma_buffer, frame_size) == HAL_OK)
	{
		vofa_tx_dma_busy = 1;
	}
}

void Vofa_Send_Data2(float data1, float data2)
{
	Vofa_data_2.ch_data[0] = data1;
	Vofa_data_2.ch_data[1] = data2;
	Vofa_QueueFrame(&Vofa_data_2, sizeof(Vofa_data_2));
}

void Vofa_Send_Data4(float data1, float data2,float data3, float data4)
{
	Vofa_data_4.ch_data[0] = data1;
	Vofa_data_4.ch_data[1] = data2;
	Vofa_data_4.ch_data[2] = data3;
	Vofa_data_4.ch_data[3] = data4;
	Vofa_QueueFrame(&Vofa_data_4, sizeof(Vofa_data_4));
}

void Vofa_Send_Data8(float data1, float data2,float data3, float data4,float data5, float data6,float data7, float data8)
{
	Vofa_data_8.ch_data[0] = data1;
	Vofa_data_8.ch_data[1] = data2;
	Vofa_data_8.ch_data[2] = data3;
	Vofa_data_8.ch_data[3] = data4;
	Vofa_data_8.ch_data[4] = data5;
	Vofa_data_8.ch_data[5] = data6;
	Vofa_data_8.ch_data[6] = data7;
	Vofa_data_8.ch_data[7] = data8;
	Vofa_QueueFrame(&Vofa_data_8, sizeof(Vofa_data_8));
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart == &huart1)
	{
		vofa_tx_dma_busy = 0;
	}
}
