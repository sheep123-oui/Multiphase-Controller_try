#include "status_led_langue.h"
#include "queue_pool.h"

Queue_uint8_ LED_status_cmd_Queue;
#define LED_queue_data_size 4
#define LED_queue_list_size 8
uint8_t LED_status_cmd_Queue_ment[LED_queue_list_size][LED_queue_data_size]={0};

static uint8_t led_state[2][5]={0};//[][$]0-3 LED		4-together	//[$][] 0:常驻状态 id  1:短时状态 id
uint16_t led_time_cnt[2][5]={0};//[$][] : 0:切换计数器    1:短时持续时间
uint8_t led_frame_index[5]={0};
uint8_t led_output_cache;


uint8_t led_occupy_mode;//0:alone >0:occupy  //仅在有短时状态时有效

// uint8_t led_list_level; //bit[0]:是否存在长时动效 bit[1]:是否存在短时单灯 bit[2]:是否存在短时组合动效

//优先级：短时组合动效》短时单灯（带occupy，则屏蔽长时，带帧背景）》长时动效》长时各灯独立

uint8_t cmd_list[8][4]={0};//[0]:cmd [1]:led_id [2]: state_id [3]: time O:常驻 1 对应时长,时基：*100ms

#define LED_NUM 2
#define LED_FRAME_NUM 10
#define LED_FRAME_LEN 10
#define LED_FRAME_END 0b10000
#define LED_FRAME_TIME_BASE 33 // *10ms
const uint8_t led_frame_dynamic[LED_FRAME_NUM][LED_FRAME_LEN]=//末尾标志：0b10000
{
	{0b0000,0b0001,0b0010,0b0100,0b1000,0b10000},//流水灯 0x01
	{0b0000,0b0000,0b0001,0b0011,0b0111,0b11111,0b10000},//加载灯0x02
};
const uint8_t led_alone_time[6][6]=//时序 0亮 1暗 2亮 3暗 4亮 5暗
{//时基：10ms
	{0,100,0,0,0,0},//常暗
	{100,0,0,0,0,0},//常亮
	{40,40,0,0,0,0},//0.4秒变换，50%占空比
	{10,90,0,0,0,0},//一秒一闪
	{4,25,4,90,0,0},//一秒两闪
	{4,25,4,25,4,90},//一秒三闪
};


GPIO_TypeDef * led_port[LED_NUM]={LED1_GPIO_Port,LED2_GPIO_Port,LED3_GPIO_Port,LED1_GPIO_Port};
uint16_t led_pin[LED_NUM]={LED1_Pin,LED2_Pin,LED3_Pin,LED1_Pin};

void status_LED_flash (void)
{
	/* code */

	// led_output_cache=0;
	
	led_cmd_handle();

	if (led_dynamic_time_flash()==STATUS_LIST_IDLE)
	{
		led_alone_time_flash();
	}
	LED_flash(led_output_cache);
}

void status_init (void)
{
	Queue_Init(&LED_status_cmd_Queue,(uint8_t *)LED_status_cmd_Queue_ment,LED_queue_list_size,LED_queue_data_size);

	memset(led_state,0,sizeof(led_state));
	memset(led_time_cnt,0,sizeof(led_time_cnt));
	memset(led_frame_index,0,sizeof(led_frame_index));

	led_output_cache=0;
	led_occupy_mode=0;
	
	led_state[0][0]=3;
	led_state[0][1]=1;
	led_state[0][2]=1;
}

uint8_t led_dynamic_time_flash (void)
{
	if (led_state[0][4]==0&&led_state[1][4]==0)//快速退出
	{
		return STATUS_LIST_IDLE;
	}
	uint8_t long_short=0;
	if (led_time_cnt[1][4] > 0)
	{
		long_short = 1; // 短时
	}

	if (long_short == 1)
	{
		if (led_time_cnt[1][4] > 0)
		{
			led_time_cnt[1][4]--;
		}
		else
		{
			led_time_cnt[1][4] = 0;
			led_state[1][4] = 0;
			long_short=0;
		}
	}
	if (led_state[long_short][4]==0)
	{
		led_time_cnt[0][4]=0;
		led_frame_index[4]=0;
		return STATUS_LIST_IDLE;
	}
	
	if (led_time_cnt[0][4] > 0)
	{
		led_time_cnt[0][4]--;
		led_output_cache=led_frame_dynamic[led_state[long_short][4]-1][led_frame_index[4]];
	}
	else
	{
		for (uint8_t i = 0; i < LED_FRAME_LEN; i++)
		{
			led_frame_index[4]++;
			if (led_frame_index[4] >= LED_FRAME_LEN)
			{
				led_frame_index[4] = 0;
			}
			
			uint8_t frame_cache = led_frame_dynamic[led_state[long_short][4]-1][led_frame_index[4]];
			if (frame_cache >= LED_FRAME_END)
			{
				led_frame_index[4] = 0;
				continue;
			}
			else
			{
				led_output_cache = frame_cache;
				break;
			}
		}
		led_time_cnt[0][4] = LED_FRAME_TIME_BASE+10;
	}
	return (long_short == 1) ? STATUS_LIST_RUNNING : STATUS_LIST_IDLE;;
}

void led_alone_time_flash (void)//短时各灯独立
{

	uint8_t short_cnt=0;
	if (led_state[1][0]+led_state[1][1]+led_state[1][2]+led_state[1][3]==0)
	{
		led_occupy_mode=0;
	}
	if (led_occupy_mode==0b10000)//代替0b0000,[4:0]都为0
	{
		led_output_cache=0;
	}
	else if (led_occupy_mode>0)
	{
		led_output_cache= led_occupy_mode & 0b1111;
	}
	
	
	for (uint8_t i = 0; i < 4; i++)
	{
		uint8_t long_short=0;
		if (led_state[1][i]>0)
		{
			long_short=1;//短时
			short_cnt++;
		}
		else
		{
			led_state[1][i]=0;
		}

		if (led_time_cnt[1][i]>0)
		{
			led_time_cnt[1][i]--;
//			continue;
		}
		if(long_short==1&&led_time_cnt[1][i]==0)//关闭短时灯效
		{
			led_time_cnt[1][i]=0;
			led_state[1][i]=0;
			led_frame_index[i]=0;
			long_short=0;
			led_output_cache &= ~(1 << i);
		}

		if (led_state[0][4]>0&&long_short==0)//长时组合动效，屏蔽长时各灯
		{
			continue;//下一灯
		}
		if ((led_occupy_mode>0)&&(long_short==0))//占用模式，屏蔽长时各灯
		{
			continue;
		}
		if (led_state[long_short][i]==0)
		{
			led_time_cnt[0][i]=0;
			led_frame_index[i]=0;
			continue;//下一灯
		}

		if (led_time_cnt[0][i]>0)
		{
			led_time_cnt[0][i]--;
			if (led_frame_index[i]%2==0)
			{
				led_output_cache|=1<<i;
			}
			else
			{
				led_output_cache&=~(1<<i);
			}
			continue;
		}
		else
		{
			for (uint8_t t = 0; t < 6; t++)
			{
				
				if ((++led_frame_index[i])>=6)
				{
					led_frame_index[i]=0;
				}
				uint8_t cnt_temp=led_alone_time[led_state[long_short][i]-1][led_frame_index[i]];
				if (cnt_temp==0)
				{
					continue;
				}
				else
				{
					led_time_cnt[0][i]=cnt_temp;
					if (led_frame_index[i]%2==0)
					{
						led_output_cache|=1<<i;
						break;
					}
					else
					{
						led_output_cache&=~(1<<i);
						break;
					}
				}
			}
		}
	}
}


void led_cmd_handle(void)
{
	uint8_t cmd_num=Queue_used_uint8(&LED_status_cmd_Queue);
	uint8_t cmd_list_ok=0;
	if (cmd_num == 0)
	{
		return;
	}
	uint8_t k=0;
	for (uint8_t i = 0; i < cmd_num;)
	{
		uint8_t state=Queue_get_uint8(&LED_status_cmd_Queue,&cmd_list[i][0],4);
		if(state==QUEUE_ERROR_BUSY)
		{
			k++;
			if (k==10)//重复尝试10次,若消息队列仍满，则放弃
			{
				break;
			}
			continue;
		}
		else if (state==QUEUE_OK)
		{
			cmd_list_ok++;
		}
		if(cmd_list_ok==cmd_num)
		{
			break;
		}
		i++;
	}
	for (uint8_t i = 0; i < cmd_list_ok; i++)
	{
		if (cmd_list[i][0] > 0x40)
		{
			if (cmd_list[i][0] == CMD_Re_Init) // r
			{
				status_init();
			}
		}
		else if (cmd_list[i][0] == CMD_NODATA)
		{
			continue; // 无效指令
		}
		else
		{
			uint8_t clear_cmd = (cmd_list[i][0] & 0b10000)>>4;
			uint8_t led_id = (cmd_list[i][0]&0b111)-1;//下标
			uint8_t led_state_id = cmd_list[i][1];//状态id下标
			uint8_t led_time_long = cmd_list[i][2];//时间id下标
			uint8_t led_occupy_id = cmd_list[i][3];//占用背景

			uint8_t led_time_type = 0;
			if (clear_cmd == 1)
			{
				led_state[led_time_type][led_id]=0;
			}
			if (led_time_long > 0)
			{
				led_time_type = 1; // 短时
				led_time_cnt[1][led_id] = led_time_long*10;
				if ((led_occupy_mode &0b1111 ) > 0)
				{
					led_occupy_mode =0b10000;//作为0b0000的代替值,[4:0]都为0
				}
				else
				{
					led_occupy_mode = led_occupy_id&0b11111;
				}
			}
			led_state[led_time_type][led_id] = led_state_id;
			led_time_cnt[0][led_id] = LED_FRAME_TIME_BASE;
			led_frame_index[led_id] = 0;

		}
	}
}

/// @brief 
/// @param cmd 
/// @param state_ID //0x01:常暗  0x02 常亮 0x03 变换 0x04 一秒一闪 0x05 一秒两闪 0x06 一秒三闪 
/// @param time 时基 100ms
/// @param occupy 
void LED_Set_CMD_toQueue(uint8_t cmd,uint8_t state_ID,uint8_t time,uint8_t occupy)
{
	uint8_t cmd_data[4]={cmd,state_ID,time,occupy};
	for (uint8_t i = 0; i < 4; i++)
	{
		uint8_t state=Queue_put_uint8(&LED_status_cmd_Queue,cmd_data,4);
		if (state==QUEUE_OK)
		{
			break;
		}
		else if (state!=QUEUE_ERROR_BUSY)
		{
			break;
		}
	}
}

void LED_flash(uint8_t led)
{
	for (uint8_t i = 0; i < LED_NUM; i++)
	{
		if (((led>>i)&0b0001)==1)
		{
			HAL_GPIO_WritePin(led_port[i],led_pin[i],0);
		}
		else
		{
			HAL_GPIO_WritePin(led_port[i],led_pin[i],1);
		}
	}
}

// void LED_OFF(uint8_t led)
// {
// 	for (uint8_t i = 0; i < 4; i++)
// 	{
// 		if (led ==( 1<<i))
// 		{
// 			HAL_GPIO_WritePin(led_port[i],led_pin[i],1);
// 		}
// 	}
// }




