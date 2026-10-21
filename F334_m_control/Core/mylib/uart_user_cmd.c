#include "uart_user_cmd.h"
#include "status_led_langue.h"
#include "main_ctrl.h"
__attribute__((noinline))  void Uart_data_handle_CallBack(uint8_t* data_start, uint8_t* data_end)
{
	uint8_t cmd[16] = { 0 };
	uint16_t i = 0;

	if (data_start[i] == 'B' && data_start[i + 1] == 'T' && data_start[i + 2] == '+')
	{
		uint8_t t;
		memset(cmd, 0, 16);
		for (t = 0; t < 15 && (data_start[i + 3 + t] != '=') && (t + i) < 60; t++) // BT+cmd=1+2*
		{
			cmd[t] = data_start[i + 3 + t];
		}
		if (t >= 15)
		{
			return;
		}

		cmd[t++] = '=';
		uint8_t* p_data = &data_start[i + 3 + t];

		if (strcmp((char*)cmd, "RST=") == 0) // cmd:BT+RST=*
		{
			__set_FAULTMASK(1);//禁止所有的可屏蔽中断
			NVIC_SystemReset();//软件复位
			// goto *(void**)(*(int*)0x08000004);
			// Scheduler_stop(wdg_flash);
		}
		if (strcmp((char*)cmd, "Start=") == 0) // cmd:BT+Start=0*
		{
			int32_t data1;
			p_data = char_get_int32data(p_data, &data1);
			HAL_GPIO_WritePin(EN_MOS_GPIO_Port, EN_MOS_Pin, 1);
		}
		if (strcmp((char*)cmd, "Stop=") == 0) // cmd:BT+Stop=*
		{
			HAL_GPIO_WritePin(EN_MOS_GPIO_Port, EN_MOS_Pin, 0);
		}
		if (strcmp((char*)cmd, "SetVal=") == 0) // cmd:BT+SetVal=*
		{
			float data1, data2;
			p_data = char_get_floatdata(p_data, &data1);
			p_data = char_get_floatdata(p_data, &data2);
		}
		if (strcmp((char*)cmd, "testCMD=") == 0) // cmd:BT+testCMD=1*
		{
			int32_t data1;
			p_data = char_get_int32data(p_data, &data1);
			switch (data1)
			{
			case 1: break;
			case 2: break;
			case 3: break;
			default:
				break;
			}
		}
		if (strcmp((char*)cmd, "hexCmd=") == 0) // cmd:BT+hexCmd=3D*
		{
			uint8_t hexdata[2] = { 0 };
			uint8_t hexdata1[4] = { 0 };
			p_data = char_get_HexAscii(p_data, (uint8_t*)&hexdata, 2);
			p_data = char_get_HexAscii(p_data, (uint8_t*)&hexdata1, 4);

			// HAL_UART_Transmit(&huart1, (uint8_t*)&hexdata, 32, 10);

		}
		if (strcmp((char*)cmd, "period=") == 0) // cmd:BT+period=54400*
		{
		}
		if (strcmp((char*)cmd, "ratio=") == 0) // cmd:BT+ratio=1*
		{
			float data1;
			p_data = char_get_floatdata(p_data, &data1);
			// Serial_Printf("ratio:%f\r\n", data1);
		}
		if (strcmp((char*)cmd, "armi=") == 0) // cmd:BT+armi=1*
		{
			float data1;
			p_data = char_get_floatdata(p_data, &data1);
			Ctrl_hysteresis(data1);
		}
		if (strcmp((char*)cmd, "CVfb=") == 0) // cmd:BT+CVfb=1*
		{
			float data1;
			p_data = char_get_floatdata(p_data, &data1);
			Ctrl_Set_hysteresis_width(data1);
		}
		if (strcmp((char*)cmd, "limit=") == 0) // cmd:BT+limit=111*
		{
			int32_t data1;
			p_data = char_get_int32data(p_data, &data1);
			ctrl_set_power(data1);
		}
		if (strcmp((char*)cmd, "pid=") == 0) // cmd:BT+pid=p+i+d+cc
		{
			float p = 0, i = 0, d = 0;
			int32_t c = 0;
			p_data = char_get_floatdata(p_data, &p);
			p_data = char_get_floatdata(p_data, &i);
			p_data = char_get_floatdata(p_data, &d);
			p_data = char_get_int32data(p_data, &c);
			// PID_struct_updata(Pid_arrry[c], p, i, d);
			// Pid_arrry[cc - 1]->f = ff;
			ctrl_Updata_pid(p, i, d);
		}
		if (strcmp((char*)cmd, "buffermode=") == 0) // cmd:BT+buffermode=a*
		{
		}
		if (strcmp((char*)cmd, "bufferval=") == 0) // cmd:BT+bufferval=a*
		{
		}
		if (strcmp((char*)cmd, "bufferper=") == 0) // cmd:BT+bufferper=a*
		{
		}
		// if (p_data == NULL)
		{
			// LED_Set_CMD_toQueue(CMD_LED1, 0x04, 1, 0);
		}
	}
	return;
}

__attribute__((noinline))  void Uart_private_cmd_CallBack(uint8_t* data_start, uint8_t* data_end)
{
	// LED_Set_CMD_toQueue(CMD_LED1, 0x04, 10, 0);
}
void Uart_EF_Reset_Event_CallBack(UART_HandleTypeDef* huart)
{
	// LED_Set_CMD_toQueue(CMD_LED1, 0x06, 20, 0b10000);
}