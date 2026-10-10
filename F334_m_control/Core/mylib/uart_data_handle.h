/******************************************************************************
*** @File           : uart_data_handle.h
*** @Description    : 串口数据处理及命令识别
*** @Attention      : v2.7
*** @Author         : TJL
*** @Date           : 2026/5/3
*** @Email          : 2766008685@qq.com
*** @release_address: https://github.com/wuhuzzk/MyCore_Embedded_LIBrary
*** @版权归属:
*       _____                    _____                    _____
*      /\    \                  /\    \                  /\    \
*     /::\    \                /::\    \                /::\____\
*     \:::\    \               \:::\    \              /:::/    /
*      \:::\    \               \:::\    \            /:::/    /
*       \:::\    \               \:::\    \          /:::/    /
*        \:::\    \               \:::\    \        /:::/    /
*        /::::\    \              /::::\    \      /:::/    /
*       /::::::\    \    _____   /::::::\    \    /:::/    /
*      /:::/\:::\    \  /\    \ /:::/\:::\    \  /:::/    /
*     /:::/  \:::\____\/::\    /:::/  \:::\____\/:::/____/
*    /:::/    \::/    /\:::\  /:::/    \::/    /\:::\    \
*   /:::/    / \/____/  \:::\/:::/    / \/____/  \:::\    \
*  /:::/    /            \::::::/    /            \:::\    \
* /:::/    /              \::::/    /              \:::\    \
* \::/    /                \::/    /                \:::\    \
*  \/____/                  \/____/                  \:::\    \
*                                                     \:::\    \
*                                                      \:::\____\
*                                                       \::/    /
*                                                        \/____/
*
******************************************************************************/
#ifndef _uart_data_handle
#define _uart_data_handle

#pragma once

#include "main.h"
#include "usart.h"
#include "string.h"
#include "FIFO.h"
#include "map_set.h"
/********USER_User_Set_Begin****************** */
//用户配置区域
#define UART_Repeat_tool 1  //* 串口转发器 0关闭 1开启

#define DMA_RXBUFFERSIZE 32	
#define RX_FIFO_SIZE 256

#define UART_Repeat_Buffer_Size 512
#define UART_Repeat_stream 1 //串口转发器通道数量
#define UART_Repeat_NUM 3  //单个串口转发器支持的最大再发送通道数量

/********USER_User_Set_End****************** */
typedef struct
{
	UART_HandleTypeDef* huart; //串口寄存器指针
	fifo_s_t uart_rx_fifo;
	RAM_Align_32Byte uint8_t DMA_Buffer[DMA_RXBUFFERSIZE]; // DMA接收数据
	RAM_Align_32Byte uint8_t uart_rx_area[RX_FIFO_SIZE];//FIFO
	uint8_t Rx_buf_pos; // 本次回调接收的数据在缓冲区的起点
	uint8_t Rx_length;  // 本次回调接收数据的长度
	uint8_t without_FE_check; // 不检测错误状态

#if UART_Repeat_tool == 1 

	uint8_t Repeat_cnt; // 串口转发使能计数
	uint8_t Repeat_index[UART_Repeat_NUM]; // 串口转发索引

#endif

}Rx_buffer_t;




#if UART_Repeat_tool == 1 
typedef struct
{
	uint8_t Repeat_cnt; // 串口转发使能计数
	uint8_t Repeat_index[UART_Repeat_NUM]; // 串口转发索引

}printf_buffer_t;

typedef struct
{
	UART_HandleTypeDef* huart_output;
	fifo_s_t uart_repeat_fifo;
	RAM_Align_32Byte uint8_t uart_repeat_area[UART_Repeat_Buffer_Size];//FIFO
	RAM_Align_32Byte uint8_t repeat_tx_DMA_buffer[UART_Repeat_Buffer_Size];
	uint8_t is_used_for_continued_stream;
}Uart_Repeat_Buffer_t;//串口转发器输出通道结构体
#endif

void Uart_handle_register(UART_HandleTypeDef* huart);

void Uart_handle_unregister(UART_HandleTypeDef* huart);

void Uart_cmd_register(UART_HandleTypeDef* huart);

void Uart_cmd_unregister(UART_HandleTypeDef* huart);

uint8_t Uart_Repeat_Get_stream_ID(UART_HandleTypeDef* huart);

void Uart_CMD_Init(void);

void uart_New_baudrate(UART_HandleTypeDef* huart, uint32_t baudrate);

void Uart_Force_Receive_init(UART_HandleTypeDef* huart);

void Uart_Force_Receive(UART_HandleTypeDef* huart);

void Uart_Reset_Cache(void);

void Uart_Cmd_find(void); //main task

void Uart_data_handle_CallBack(uint8_t* data_start, uint8_t* data_end);
//用户回调，已剔除包头包尾，包末为和检验字节
void Uart_private_cmd_CallBack(uint8_t* data_start, uint8_t* data_end);

uint8_t Uart_get_Rx_stream_ID(UART_HandleTypeDef* huart);

uint16_t Uart_get_Rx_stream_len(uint8_t stream_id);

uint16_t Uart_get_Rx_stream_data(uint8_t* data, uint16_t len, uint8_t stream_id);

void Uart_EF_Reset_Event_CallBack(UART_HandleTypeDef* huart);

void private_cmd_handle(uint16_t CMD_index, uint16_t CMD_end_index);

void Uart_ext_data_sent(uint8_t* data, uint16_t len, uint8_t stream_id);

uint16_t Uart_ext_get_Tx_fifo_free_size(uint8_t stream_id);

uint8_t* char_get_int32data(uint8_t* data, int32_t* out_data);

uint8_t* char_get_floatdata(uint8_t* data, float* out_data);

uint8_t* char_get_HexAscii(uint8_t* data, uint8_t* out_data, uint8_t len_of_byte);

void char_to_ascii_Hex(uint8_t* data, uint8_t* out_data, uint16_t len_of_byte);

void Serial_Printf(char* format, ...);

float str2float(uint8_t* s);

#endif