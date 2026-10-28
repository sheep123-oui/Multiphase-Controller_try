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
#include "uart_data_handle.h"
typedef struct
{
	float ch_data[2];
	uint8_t  tail[4];
}Vofa_data_m_2;

typedef struct
{
	float ch_data[4];
	uint8_t  tail[4];
} Vofa_data_m_4;

typedef struct
{
	float ch_data[8];
	uint8_t  tail[4];
} Vofa_data_m_8;

/********USER_User_Set_Begin****************** */

#define Data_to_repeatTool 1 //是否使用串口转发工具发送数据 0不使用 1使用
#define Uart_H_for_Vofa &huart1 //串口发送数据，选择使用的串口句柄

#define UART_DMA_USED_CACHE 1

/********USER_User_Set_End****************** */

// RAM_Set_D2SRAM1 Vofa_data_m_2 Vofa_data_2 = { .tail = {0x00,0x00,0x80,0x7f} };
// RAM_Set_D2SRAM1 Vofa_data_m_4 Vofa_data_4 = { .tail = {0x00,0x00,0x80,0x7f} };
// RAM_Set_D2SRAM1 Vofa_data_m_8 Vofa_data_8 = { .tail = {0x00,0x00,0x80,0x7f} };

#if (UART_DMA_USED_CACHE == 0)
#define DMA_UN_Cache_SET  RAM_Set_D2SRAM1
#else
#define DMA_UN_Cache_SET
#endif 
DMA_UN_Cache_SET Vofa_data_m_2 Vofa_data_2 = { 0 };
DMA_UN_Cache_SET Vofa_data_m_4 Vofa_data_4 = { 0 };
DMA_UN_Cache_SET Vofa_data_m_8 Vofa_data_8 = { 0 };

uint8_t Uart_repeatTool_stream_ID = 0;

void Vofa_init(void)
{
	Vofa_data_2.tail[0] = 0x00;
	Vofa_data_2.tail[1] = 0x00;
	Vofa_data_2.tail[2] = 0x80;
	Vofa_data_2.tail[3] = 0x7f;
	Vofa_data_4.tail[0] = 0x00;
	Vofa_data_4.tail[1] = 0x00;
	Vofa_data_4.tail[2] = 0x80;
	Vofa_data_4.tail[3] = 0x7f;
	Vofa_data_8.tail[0] = 0x00;
	Vofa_data_8.tail[1] = 0x00;
	Vofa_data_8.tail[2] = 0x80;
	Vofa_data_8.tail[3] = 0x7f;
	Uart_repeatTool_stream_ID = Uart_Repeat_Get_stream_ID(Uart_H_for_Vofa);

}
void Vofa_Send_Data2(float data1, float data2)
{
	Vofa_data_2.ch_data[0] = data1;
	Vofa_data_2.ch_data[1] = data2;
#if Data_to_repeatTool == 1
	Uart_ext_data_sent((uint8_t*)&Vofa_data_2, sizeof(Vofa_data_2), Uart_repeatTool_stream_ID);
#else
	HAL_UART_Transmit_DMA(Uart_H_for_Vofa, (uint8_t*)&Vofa_data_2, sizeof(Vofa_data_2));
#endif
}

void Vofa_Send_Data4(float data1, float data2, float data3, float data4)
{
	Vofa_data_4.ch_data[0] = data1;
	Vofa_data_4.ch_data[1] = data2;
	Vofa_data_4.ch_data[2] = data3;
	Vofa_data_4.ch_data[3] = data4;

#if Data_to_repeatTool == 1
	Uart_ext_data_sent((uint8_t*)&Vofa_data_4, sizeof(Vofa_data_4), Uart_repeatTool_stream_ID);
#else
	HAL_UART_Transmit_DMA(Uart_H_for_Vofa, (uint8_t*)&Vofa_data_4, sizeof(Vofa_data_4));
#endif
}

void Vofa_Send_Data8(float data1, float data2, float data3, float data4, float data5, float data6, float data7, float data8)
{
	Vofa_data_8.ch_data[0] = data1;
	Vofa_data_8.ch_data[1] = data2;
	Vofa_data_8.ch_data[2] = data3;
	Vofa_data_8.ch_data[3] = data4;
	Vofa_data_8.ch_data[4] = data5;
	Vofa_data_8.ch_data[5] = data6;
	Vofa_data_8.ch_data[6] = data7;
	Vofa_data_8.ch_data[7] = data8;

#if Data_to_repeatTool == 1
	Uart_ext_data_sent((uint8_t*)&Vofa_data_8, sizeof(Vofa_data_8), Uart_repeatTool_stream_ID);
#else
	HAL_UART_Transmit_DMA(Uart_H_for_Vofa, (uint8_t*)&Vofa_data_8, sizeof(Vofa_data_8));
#endif
}

void Vofa_Send_Data2_U3(float data1, float data2)
{
	Vofa_data_2.ch_data[0] = data1;
	Vofa_data_2.ch_data[1] = data2;
	// HAL_UART_Transmit_DMA(&huart3, (uint8_t *)&Vofa_data_2, sizeof(Vofa_data_2));   
}
void Vofa_Send_Data_s(float*data, uint8_t ch_num)
{
	Uart_ext_data_sent((uint8_t*)data, ch_num * sizeof(float), Uart_repeatTool_stream_ID);
	Uart_ext_data_sent(Vofa_data_2.tail, sizeof(Vofa_data_2.tail), Uart_repeatTool_stream_ID);
}