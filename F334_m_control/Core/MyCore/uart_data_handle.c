/******************************************************************************
*** @File           : uart_data_handle.c
*** @Description    : 串口数据处理及命令识别
*** @Attention      : v2.7.1
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
#include "uart_data_handle.h"
#include "stdarg.h"


#define Uart_Cmd_size 256
static uint8_t Uart_cmd_data_cache[Uart_Cmd_size] = { 0 };
static uint16_t cache_len = 0;
static uint16_t cmd_hear_index = 0;
static uint8_t time_cnt;

/********USER_User_Set_Begin****************** */
//用户配置区域
#define UART_NUM 1
UART_HandleTypeDef* uart_rx_handle[UART_NUM] = { &huart1 };
UART_HandleTypeDef* UART_Repeat_stream_handle[UART_Repeat_stream] = { &huart1 };

UART_HandleTypeDef* printf_UART_handle = &huart1;

#define Printf_ON_repeat_stream 1 //是否选用printf函数串口转发功能 0直接发 1使用通道

#define UART_RX_DMA_USED_CACHE 0
#define Support_for_H7 1 //是否支持H7系列单片机的特殊操作 0不支持 1支持
#define CMD_E_for_float_support 0 //是否支持命令参数中浮点数的科学计数法输入 0不支持 1支持
#define Advanced_Init_support 0
/********USER_User_Set_End****************** */

#if defined(STM32H750xx)
#else
#undef Support_for_H7
#define Support_for_H7  0
#endif

#if (Support_for_H7 == 1)&&(UART_RX_DMA_USED_CACHE == 0)
#define DMA_UN_Cache_SET  RAM_Set_D2SRAM1
#else
#define DMA_UN_Cache_SET
#endif 



int8_t Reg_Num = 0;
DMA_UN_Cache_SET Rx_buffer_t  uart_rx_buffer[UART_NUM];

uint8_t Set_buffer_toCmd_Num = 0;
Rx_buffer_t* buffer_toCmd[UART_NUM] = { NULL };

#if UART_Repeat_tool == 1
DMA_UN_Cache_SET Uart_Repeat_Buffer_t uart_repeat_buffer[UART_Repeat_stream];
#if Printf_ON_repeat_stream == 1
DMA_UN_Cache_SET printf_buffer_t uart_printf_buffer;
#endif
#endif

void Uart_user_set_Init(void)
{
	Uart_cmd_register(&huart1);
	// Uart_Repeat_stream_register(&huart1, &huart1);
}


// ! 该组register并未完善
void Uart_handle_register(UART_HandleTypeDef* huart)
{
	for (uint8_t i = 0; i < UART_NUM; i++)
	{
		if (uart_rx_buffer[i].huart == NULL)
		{
			uart_rx_buffer[i].huart = huart;
			Reg_Num++;
			break;
		}
	}
}

void Uart_handle_unregister(UART_HandleTypeDef* huart)
{
	for (uint8_t i = 0; i < UART_NUM; i++)
	{
		if (uart_rx_buffer[i].huart == huart)
		{
			uart_rx_buffer[i].huart = NULL;
			Reg_Num--;
			break;
		}
	}
}
void Uart_cmd_register(UART_HandleTypeDef* huart)
{
	Rx_buffer_t* buffer_toCmd_temp = NULL;
	for (uint8_t i = 0; i < UART_NUM; i++)
	{
		if (uart_rx_buffer[i].huart == huart)
		{
			buffer_toCmd_temp = &uart_rx_buffer[i];
		}
	}
	for (uint8_t i = 0; i < UART_NUM; i++)
	{
		if (buffer_toCmd[i] == NULL)
		{
			buffer_toCmd[i] = buffer_toCmd_temp;
			Set_buffer_toCmd_Num++;
			return;
		}
	}
}
void Uart_cmd_unregister(UART_HandleTypeDef* huart)
{
	for (uint8_t i = 0; i < UART_NUM; i++)
	{
		if (buffer_toCmd[i]->huart == huart)
		{
			buffer_toCmd[i] = NULL;
			Set_buffer_toCmd_Num--;
			break;
		}
	}
}
#if UART_Repeat_tool == 1
void Uart_Repeat_stream_register(UART_HandleTypeDef* huart_input, UART_HandleTypeDef* huart_output)
{
	for (uint8_t i = 0; i < UART_Repeat_stream; i++)
	{
		if (uart_repeat_buffer[i].huart_output == huart_output)
		{
			for (uint8_t j = 0; j < UART_NUM; j++)
			{
				if (uart_rx_buffer[j].huart == huart_input)
				{
					if (uart_rx_buffer[j].Repeat_cnt < UART_Repeat_NUM)
					{
						uart_rx_buffer[j].Repeat_index[uart_rx_buffer[j].Repeat_cnt] = i;
						uart_rx_buffer[j].Repeat_cnt++;
						break;
					}
					else
					{
						return;
					}
				}
			}
		}
	}
}
uint8_t Uart_Repeat_Get_stream_ID(UART_HandleTypeDef* huart)
{
	for (uint8_t i = 0; i < UART_Repeat_stream; i++)
	{
		if (uart_repeat_buffer[i].huart_output == huart)
		{
			return i;
		}
	}
	return 0;
}
#if Printf_ON_repeat_stream == 1
void Uart_Repeat_stream_register_from_printf(UART_HandleTypeDef* huart_output)
{
	for (uint8_t i = 0; i < UART_Repeat_stream; i++)
	{
		if (uart_repeat_buffer[i].huart_output == huart_output)
		{
			if (uart_repeat_buffer[i].uart_repeat_fifo.p_start_addr == NULL)
			{
				fifo_s_init(&uart_repeat_buffer[i].uart_repeat_fifo, uart_repeat_buffer[i].uart_repeat_area, UART_Repeat_Buffer_Size);
			}

			uart_printf_buffer.Repeat_index[uart_printf_buffer.Repeat_cnt] = i;
			uart_printf_buffer.Repeat_cnt++;
		}
	}
}
#endif
#endif

void Uart_Init_for_Rx(void)
{
	memset(uart_rx_buffer, 0, sizeof(uart_rx_buffer));

	for (uint8_t i = 0; i < UART_NUM; i++)
	{
		uart_rx_buffer[i].huart = uart_rx_handle[i];
		fifo_s_init(&uart_rx_buffer[i].uart_rx_fifo, uart_rx_buffer[i].uart_rx_area, RX_FIFO_SIZE);
	}
	__disable_irq();
	for (uint8_t i = 0; i < UART_NUM; i++)
	{
		HAL_UARTEx_ReceiveToIdle_DMA(uart_rx_buffer[i].huart, (uint8_t*)&uart_rx_buffer[i].DMA_Buffer, DMA_RXBUFFERSIZE);
	}
	__enable_irq();
}

void Uart_Tx_Init(void)
{
#if UART_Repeat_tool == 1
	memset(uart_repeat_buffer, 0, sizeof(uart_repeat_buffer));
#if Printf_ON_repeat_stream == 1
	memset(&uart_printf_buffer, 0, sizeof(uart_printf_buffer));
#endif
	for (uint8_t i = 0; i < UART_Repeat_stream; i++)
	{
		uart_repeat_buffer[i].huart_output = UART_Repeat_stream_handle[i];
		fifo_s_init(&uart_repeat_buffer[i].uart_repeat_fifo, uart_repeat_buffer[i].uart_repeat_area, UART_Repeat_Buffer_Size);
	}
#if Printf_ON_repeat_stream == 1
	Uart_Repeat_stream_register_from_printf(printf_UART_handle);
#endif
#endif

	/* TX initialization is also usable without enabling command reception. */
}

void Uart_CMD_Init(void)
{
	Uart_Init_for_Rx();
	Uart_Tx_Init();
	Uart_user_set_Init();
}

uint8_t Uart_state_EF_check(void)
{
	uint8_t return_val = 0;
	for (uint8_t i = 0; i < UART_NUM; i++)
	{
		if (uart_rx_buffer[i].huart == NULL)
		{
			continue;
		}
		if ((__HAL_UART_GET_FLAG(uart_rx_buffer[i].huart, UART_FLAG_FE) != RESET) || \
			(__HAL_UART_GET_FLAG(uart_rx_buffer[i].huart, UART_FLAG_ORE) != RESET))
		{

			if (uart_rx_buffer[i].without_FE_check == 1)
			{
#if Support_for_H7 == 1
				if ((((DMA_Stream_TypeDef*)(uart_rx_buffer[i].huart->hdmarx)->Instance)->CR & DMA_SxCR_EN) == 0)
#else
				if ((uart_rx_buffer[i].huart->hdmarx)->State != HAL_DMA_STATE_READY)
#endif
				{
					uart_rx_buffer[i].Rx_buf_pos = 0;
					uart_rx_buffer[i].Rx_length = 0;
					HAL_UARTEx_ReceiveToIdle_DMA(uart_rx_buffer[i].huart, (uint8_t*)&uart_rx_buffer[i].DMA_Buffer, DMA_RXBUFFERSIZE);
				}
				continue;
			}

			HAL_UART_DMAStop(uart_rx_buffer[i].huart);

			// 清除错误标志
			__HAL_UART_CLEAR_FEFLAG(uart_rx_buffer[i].huart);
			__HAL_UART_CLEAR_OREFLAG(uart_rx_buffer[i].huart);
			__HAL_UART_CLEAR_FLAG(uart_rx_buffer[i].huart, UART_FLAG_ORE);

			// 恢复错误中断使能
			ATOMIC_SET_BIT(uart_rx_buffer[i].huart->Instance->CR3, USART_CR3_EIE);

			uart_rx_buffer[i].Rx_buf_pos = 0;
			uart_rx_buffer[i].Rx_length = 0;

			fifo_s_flush(&uart_rx_buffer[i].uart_rx_fifo);

			HAL_UARTEx_ReceiveToIdle_DMA(uart_rx_buffer[i].huart, (uint8_t*)&uart_rx_buffer[i].DMA_Buffer, DMA_RXBUFFERSIZE);

			Uart_EF_Reset_Event_CallBack(uart_rx_buffer[i].huart);
			return_val = 1;
		}
	}
	return return_val;
}

__weak void Uart_EF_Reset_Event_CallBack(UART_HandleTypeDef* huart)//call_back
{
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart)
{
	Uart_state_EF_check();
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t Size)
{
	for (uint8_t i = 0; i < UART_NUM; i++)
	{
		if (uart_rx_buffer[i].huart == NULL)
		{
			continue;
		}

		if (huart == uart_rx_buffer[i].huart)
		{
			uart_rx_buffer[i].Rx_length = Size - uart_rx_buffer[i].Rx_buf_pos;

#if (Support_for_H7 == 1) && (UART_RX_DMA_USED_CACHE == 1)
			SCB_InvalidateDCache_by_Addr((uint32_t*)&uart_rx_buffer[i].DMA_Buffer, DMA_RXBUFFERSIZE);
#endif

			fifo_s_puts(&uart_rx_buffer[i].uart_rx_fifo, \
				(char*)&(uart_rx_buffer[i].DMA_Buffer[uart_rx_buffer[i].Rx_buf_pos]), \
				uart_rx_buffer[i].Rx_length); // 数据填入 FIFO
#if UART_Repeat_tool == 1//Debug
			if (uart_rx_buffer[i].Repeat_cnt > 0)
			{
				for (uint8_t t = 0; t < uart_rx_buffer[i].Repeat_cnt; t++)
				{
					fifo_s_puts(&uart_repeat_buffer[uart_rx_buffer[i].Repeat_index[t]].uart_repeat_fifo, \
						(char*)&(uart_rx_buffer[i].DMA_Buffer[uart_rx_buffer[i].Rx_buf_pos]), \
						uart_rx_buffer[i].Rx_length); // 数据填入转发 FIFO
				}
			}
#endif
			uart_rx_buffer[i].Rx_buf_pos += uart_rx_buffer[i].Rx_length;
			if (uart_rx_buffer[i].Rx_buf_pos >= DMA_RXBUFFERSIZE)
				uart_rx_buffer[i].Rx_buf_pos = 0; // 缓冲区用完后，返回 0 处重新开始


		}
	}
}
#undef RXBUFFERSIZE

void uart_New_baudrate(UART_HandleTypeDef* huart, uint32_t baudrate)
{
	HAL_UART_DeInit(huart);
	huart->Init.BaudRate = baudrate;
	HAL_UART_Init(huart);
	for (uint8_t i = 0; i < UART_NUM; i++)
	{
		if (huart == uart_rx_buffer[i].huart)
		{
			uart_rx_buffer[i].Rx_buf_pos = 0;
			uart_rx_buffer[i].Rx_length = 0;
			HAL_UARTEx_ReceiveToIdle_DMA(uart_rx_buffer[i].huart, (uint8_t*)&uart_rx_buffer[i].DMA_Buffer, DMA_RXBUFFERSIZE);
		}
	}
}

void Uart_Force_Receive_init(UART_HandleTypeDef* huart)
{
#if Advanced_Init_support == 1
	for (uint8_t i = 0; i < UART_NUM; i++)
	{
		if (uart_rx_buffer[i].huart == huart)
		{
			huart->AdvancedInit.AdvFeatureInit &= ~(UART_ADVFEATURE_RXOVERRUNDISABLE_INIT | UART_ADVFEATURE_DMADISABLEONERROR_INIT);
			huart->AdvancedInit.OverrunDisable = UART_ADVFEATURE_OVERRUN_DISABLE;
			huart->AdvancedInit.DMADisableonRxError = UART_ADVFEATURE_DMA_DISABLEONRXERROR;
			uart_rx_buffer[i].without_FE_check = 1;
		}
	}
#endif
}

/// @brief //* 强制接收数据，去除错误标志，插入HAL的中断处理前
/// @param huart 
void Uart_Force_Receive(UART_HandleTypeDef* huart)
{
	__HAL_UART_CLEAR_PEFLAG(huart);
	__HAL_UART_CLEAR_FEFLAG(huart);
	__HAL_UART_CLEAR_NEFLAG(huart);
	__HAL_UART_CLEAR_OREFLAG(huart);
	huart->ErrorCode = HAL_UART_ERROR_NONE;
}

void Uart_Reset_Cache(void)
{
	cache_len = 0;
	cmd_hear_index = 0;
	memset(Uart_cmd_data_cache, 0, Uart_Cmd_size);
}

void Uart_move_hear_index(void)
{
	if (cmd_hear_index == 0)
	{
		if (cache_len >= Uart_Cmd_size)
		{
			Uart_Reset_Cache();
		}
		return;
	}

	if (cache_len < cmd_hear_index)
	{
		Uart_Reset_Cache();
	}
	memmove(Uart_cmd_data_cache, Uart_cmd_data_cache + cmd_hear_index, cache_len - cmd_hear_index);
	cache_len -= cmd_hear_index;
	cmd_hear_index = 0;
}
uint16_t Uart_GetData(void)
{
	if (Set_buffer_toCmd_Num == 0)
	{
		return 0;
	}
	uint8_t return_value = 0;
	for (uint8_t i = 0; i < UART_NUM; i++)
	{
		uint16_t len = 0;
		if (buffer_toCmd[i] != NULL)
		{
			len = fifo_s_used(&buffer_toCmd[i]->uart_rx_fifo);
			if (len != 0)
			{
				if (cache_len + len >= Uart_Cmd_size)
				{
					len = Uart_Cmd_size - cache_len;
				}
				if (len == 0)
				{
					time_cnt = 50;
					break;
				}
				fifo_s_gets(&buffer_toCmd[i]->uart_rx_fifo, (char*)(&Uart_cmd_data_cache[cache_len]), len); // 从 FIFO 取数据
				cache_len += len;
				time_cnt = 0;
				return_value = 1;
			}
		}
	}
	return return_value;

}
void Uart_Tx_task(void)
{
#if UART_Repeat_tool == 1//Debug
	for (uint8_t i = 0; i < UART_Repeat_stream; i++)
	{
		Uart_Repeat_Buffer_t* stream = &uart_repeat_buffer[i];
		if (stream->huart_output == NULL || stream->huart_output->gState != HAL_UART_STATE_READY)
		{
			continue;
		}
		/* Never overwrite the buffer while DMA is still transmitting it. */
		if (stream->tx_pending_size == 0)
		{
			uint16_t len = fifo_s_used(&stream->uart_repeat_fifo);
			if (len == 0)
			{
				continue;
			}
			stream->tx_pending_size = fifo_s_gets(&stream->uart_repeat_fifo,
				(char*)stream->repeat_tx_DMA_buffer, len);
		}
		if (stream->tx_pending_size != 0)
		{
#if (Support_for_H7 == 1) && (UART_RX_DMA_USED_CACHE == 1)
			SCB_CleanDCache_by_Addr((uint32_t*)stream->repeat_tx_DMA_buffer, sizeof(stream->repeat_tx_DMA_buffer));
#endif
			if (HAL_UART_Transmit_DMA(stream->huart_output, stream->repeat_tx_DMA_buffer,
				stream->tx_pending_size) == HAL_OK)
			{
				stream->tx_pending_size = 0;
			}
		}
	}
#endif
}

__attribute__((noinline)) void Uart_Cmd_find(void)
{
	Uart_Tx_task();

	uint8_t read_state = Uart_GetData(); // 待发送数据长度
	if (read_state == 0)
	{
		time_cnt++;
		if (time_cnt < 50)
		{
			return;
		}
		else
		{
			if (Uart_state_EF_check() == 1)
			{
				Uart_Reset_Cache();
				return;
			}
			time_cnt = 0;
			Uart_Reset_Cache();
		}
		return;
	}
	for (uint16_t i = cmd_hear_index; i < cache_len; i++)
	{
		if (Uart_cmd_data_cache[i] == 'B' && Uart_cmd_data_cache[i + 1] == 'T' && Uart_cmd_data_cache[i + 2] == '+')
		{
			cmd_hear_index = i;
			for (uint16_t t = i; t < Uart_Cmd_size; t++)
			{
				if (Uart_cmd_data_cache[t] == '\0')
				{
					cmd_hear_index = t + 1;
					break;
				}
				if (Uart_cmd_data_cache[t] == '*')
				{
					Uart_data_handle_CallBack(&Uart_cmd_data_cache[cmd_hear_index], &Uart_cmd_data_cache[t]);
					cmd_hear_index = t + 1;
					if (cmd_hear_index > (Uart_Cmd_size - 16))
					{
						Uart_Reset_Cache();
					}
					i = t;
					break;
				}
			}
		}
		else if (Uart_cmd_data_cache[i] == 0xA5)
		{
			cmd_hear_index = i;
			for (uint16_t t = i; t < Uart_Cmd_size; t++)
			{
				if (Uart_cmd_data_cache[t] == 0x5A)
				{
					private_cmd_handle(cmd_hear_index, t);
					cmd_hear_index = t + 1;
					if (cmd_hear_index > (Uart_Cmd_size - 16))
					{
						Uart_Reset_Cache();
					}
					i = t;
					break;
				}
			}
		}
	}
	Uart_move_hear_index();
}
__weak void Uart_data_handle_CallBack(uint8_t* data_start, uint8_t* data_end)//call_back
{
}
__weak void Uart_private_cmd_CallBack(uint8_t* data_start, uint8_t* data_end)//call_back
{
}


void private_cmd_handle(uint16_t CMD_index, uint16_t CMD_end_index)
{
	uint8_t sum_check = 0;
	for (uint16_t i = CMD_index + 1; i < CMD_end_index - 1; i++)
	{
		sum_check += Uart_cmd_data_cache[i];
	}
	if (sum_check != Uart_cmd_data_cache[CMD_end_index - 1])
	{
		return;
	}
	/********USER_DATA_handle_Begin****************** */
	Uart_private_cmd_CallBack(&Uart_cmd_data_cache[CMD_index + 1], &Uart_cmd_data_cache[CMD_end_index - 1]);
	/********USER_DATA_handle_End****************** */
}

uint8_t Uart_get_Rx_stream_ID(UART_HandleTypeDef* huart)
{
	for (uint8_t i = 0; i < UART_Repeat_stream; i++)
	{
		if (uart_repeat_buffer[i].huart_output == huart)
		{
			uart_repeat_buffer[i].is_used_for_continued_stream = 1;
			return i;
		}
	}
	return 0;
}
uint16_t Uart_get_Rx_stream_len(uint8_t stream_id)
{
	if (stream_id >= UART_Repeat_stream)
	{
		return 0;
	}
	return fifo_s_used(&uart_repeat_buffer[stream_id].uart_repeat_fifo);
}

uint16_t Uart_get_Rx_stream_data(uint8_t* data, uint16_t len, uint8_t stream_id)
{
	if (stream_id >= UART_Repeat_stream)
	{
		return 0;
	}
	uint16_t used = fifo_s_used(&uart_repeat_buffer[stream_id].uart_repeat_fifo);
	if (len > used)
	{
		len = used;
	}
	fifo_s_gets(&uart_repeat_buffer[stream_id].uart_repeat_fifo, (char*)data, len);
	return len;
}



#if UART_Repeat_tool == 1 
void Uart_ext_data_sent(uint8_t* data, uint16_t len, uint8_t stream_id)
{
	if (stream_id >= UART_Repeat_stream || data == NULL || len == 0)
	{
		return;
	}
	uint32_t irq_state = __get_PRIMASK();
	__disable_irq();
	if (len <= fifo_s_free(&uart_repeat_buffer[stream_id].uart_repeat_fifo))
	{
		fifo_s_puts_noprotect(&uart_repeat_buffer[stream_id].uart_repeat_fifo, (char*)data, len);
	}
	uart_repeat_buffer[stream_id].is_used_for_continued_stream = 1;
	__set_PRIMASK(irq_state);
}

uint16_t Uart_ext_get_Tx_fifo_free_size(uint8_t stream_id)
{
	if (stream_id >= UART_Repeat_stream)
	{
		return 0;
	}
	return fifo_s_free(&uart_repeat_buffer[stream_id].uart_repeat_fifo);
}
#endif
__attribute__((noinline))  uint8_t* char_get_int32data(uint8_t* data, int32_t* out_data)
{
	uint8_t data_cache[16] = { 0 };
	uint8_t k = 0;
	uint8_t* p = data;
	if (data == NULL)
	{
		return NULL;
	}

	for (uint8_t i = 0; i < 15; i++)
	{
		if (data[i] == '+')
		{
			p = (&(data[i + 1])); // 返回+后数据的地址
			break;
		}
		if (data[i] == '*')
		{
			p = NULL; // 结束该命令的读取
			break;
		}
		else if (data[i] <= '9' && data[i] >= '0' || data[i] == '-' || data[i] == '.')
		{
			data_cache[k] = data[i];
			k++;
		}
	}
	*out_data = strtol((char*)data_cache, NULL, 10);
	return p;
}

__attribute__((noinline))  uint8_t* char_get_floatdata(uint8_t* data, float* out_data)
{
	uint8_t data_cache[32] = { 0 };
	uint8_t k = 0;
	uint8_t* p = data;
	if (data == NULL)
	{
		return NULL;
	}
	for (uint8_t i = 0; i < 31; i++)//[31]保留为结束符
	{
		if (data[i] == '+')
		{
			p = (&(data[i + 1])); // 返回+后数据的地址
			break;
		}
		if (data[i] == '*')
		{
			p = NULL; // 结束该命令的读取
			break;
		}
		else if (data[i] <= '9' && data[i] >= '0' || data[i] == '-' || data[i] == '.' || data[i] == 'e' || data[i] == 'E')
		{
			data_cache[k] = data[i];
			k++;
		}
	}
	// *out_data = strtod((char*)data_cache, NULL); //标准库，占用大
	*out_data = str2float((uint8_t*)data_cache);
	return p;
}

/// @brief 
/// @param data 
/// @param out_data 
/// @param len_of_byte 输出数据的长
/// @return 
__attribute__((noinline))  uint8_t* char_get_HexAscii(uint8_t* data, uint8_t* out_data, uint8_t len_of_byte)
{
	uint8_t data_cache[16] = { 0 };
	uint8_t k = 0;
	uint8_t cnt = 0;
	uint8_t* p = data;
	int int_temp = 0;
	if (data == NULL)
	{
		return NULL;
	}
	uint8_t i = 0;
	for (i = 0; i < 2 * len_of_byte; i++)
	{
		if (data[i] == '+')
		{
			p = (&(data[i + 1])); // 返回+后数据的地址
			break;
		}
		if (data[i] == '*')
		{
			p = NULL; // 结束该命令的读取
			break;
		}
		else if ((data[i] <= '9' && data[i] >= '0') || (data[i] <= 'Z' && data[i] >= 'A') || (data[i] <= 'z' && data[i] >= 'a'))
		{
			data_cache[k] = data[i];
			k++;
		}
		if (i % 8 == 7)
		{
			int_temp = strtol((char*)data_cache, NULL, 16);
			out_data[cnt + 3] = (int_temp & 0xFF);
			out_data[cnt + 2] = (int_temp & 0xFF00) >> 8;
			out_data[cnt + 1] = (int_temp & 0xFF0000) >> 16;
			out_data[cnt] = (int_temp & 0xFF000000) >> 24;
			memset((char*)data_cache, 0, 16);
			k = 0;
			cnt += 4;
		}
	}

	if (k > 0)
	{
		uint8_t short_data[4] = { 0 };
		uint8_t l = 0;
		for (uint8_t j = 0; j < k; )
		{
			short_data[j] = data_cache[j];
			short_data[j + 1] = data_cache[j + 1];
			int_temp = strtol((char*)short_data, NULL, 16);
			out_data[cnt + l] = (int_temp & 0xFF);
			j += 2;
			l++;
		}
	}
	if (i >= 2 * len_of_byte)
	{
		p = data + 2 * len_of_byte;
	}
	return p;
}

static uint8_t Hex_table[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' }; // 16进制转换表
__attribute__((noinline))  void char_to_ascii_Hex(uint8_t* data, uint8_t* out_data, uint16_t len_of_byte)
{
	uint16_t t = 0;
	for (uint16_t i = 0; i < len_of_byte; i++)
	{
		out_data[t] = Hex_table[(data[i] & 0xF0) >> 4];
		out_data[t + 1] = Hex_table[data[i] & 0x0F];
		t += 2;
	}
}

void Serial_Printf(char* format, ...)
{
#define size_of_string 128
	DMA_UN_Cache_SET static char String[size_of_string] = { 0 };
	memset(String, 0, size_of_string);
	va_list arg;
	va_start(arg, format);
	vsprintf(String, format, arg);
	va_end(arg);

#if Printf_ON_repeat_stream == 1
	for (uint8_t i = 0; i < uart_printf_buffer.Repeat_cnt; i++)
	{
		if (uart_repeat_buffer[uart_printf_buffer.Repeat_index[i]].is_used_for_continued_stream == 1)
		{
			uart_repeat_buffer[uart_printf_buffer.Repeat_index[i]].is_used_for_continued_stream = 0;
			continue;
		}
		fifo_s_puts(&uart_repeat_buffer[uart_printf_buffer.Repeat_index[i]].uart_repeat_fifo, (char*)&String, strlen(String));
	}
#else
	HAL_UART_Transmit(&printf_UART_handle, (uint8_t*)&String, strlen(String), 1);
#endif
}

#undef DMA_UN_Cache_SET

const uint32_t ten_ZOOM[8] = { 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000 };
float str2float(uint8_t* s)
{
	uint8_t* p = s;

	/* 符号 */
	uint8_t neg = 0;
	if (*p == '+')
	{
		p++;
	}
	else if (*p == '-')
	{
		neg = 1;
		p++;
	}

	/* 整数部分 */
	uint32_t intv = 0;
	int32_t digits = 0;
	int8_t move_int = -1;
	while (*p >= '0' && *p <= '9' && digits < 9)
	{
		intv = intv * 10u + (*p++ - '0');
		digits++;
	}
	// 跳过多余整数位（防溢出）
	while (*p >= '0' && *p <= '9')
	{
		p++;
		move_int++;
	}

	float val = (float)intv;//输出值

	if (move_int >= 0)
	{
		if (move_int >= 8)
		{
			val = val * ten_ZOOM[7];
			move_int -= 8;
		}
		if (move_int < 8)
		{
			if (move_int >= 0)
			{
				val = val * ten_ZOOM[move_int];
			}
		}

	}

	/* 小数部分 */
	uint32_t frac = 0;
	int8_t frac_digits = 0;
	int8_t move_frac = 0;
	if (*p == '.')
	{
		p++;
		while (*p == '0')
		{
			p++;
			move_frac++;
		}
		while (*p >= '0' && *p <= '9' && frac_digits < (9 - digits))
		{
			frac = frac * 10u + (*p++ - '0');
			frac_digits++;
		}
		// 跳过多余小数位
		while (*p >= '0' && *p <= '9')
		{
			p++;
		}
	}

	frac_digits += move_frac;
	if (frac_digits > 0)
	{
		float divisor = 1;
		frac_digits--;
		if (frac_digits >= 8)
		{
			divisor = divisor * ten_ZOOM[7];
			frac_digits -= 8;
		}
		if (frac_digits < 8)
		{
			if (frac_digits >= 0)
			{
				divisor = divisor * ten_ZOOM[frac_digits];
			}
		}
		val += (float)frac / (float)divisor;
	}
#if CMD_E_for_float_support==1
	if (*p == 'E' || *p == 'e')
	{
		p++;
		int16_t move = strtol((char*)p, NULL, 10);
		if (move > 0)
		{
			move--;
			for (uint8_t i = 0; i < 4; i++)
			{
				if (move >= 8)
				{
					val = val * ten_ZOOM[7];
					move -= 8;
					continue;
				}
				if (move < 8)
				{
					if (move >= 0)
					{
						val = val * ten_ZOOM[move];
					}
					break;
				}
			}
		}
		else if (move < 0)
		{
			move = -move;
			move--;
			for (uint8_t i = 0; i < 4; i++)
			{
				if (move >= 8)
				{
					val = val / ten_ZOOM[7];
					move -= 8;
					continue;
				}
				if (move < 8)
				{
					if (move >= 0)
					{
						val = val / ten_ZOOM[move];
					}
				}
			}
		}
	}
#endif
	return neg ? -val : val;
}
