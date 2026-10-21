// #pragma once
// #ifndef _STATUS_LED_LANGER_H_
// #define _STATUS_LED_LANGER_H_
// #include "main.h"

// // #include "stdint.h"
// #include "string.h"


// enum
// {
// 	LED0 = 0b0001,
// 	LED1 = 0b0010,
// 	LED2 = 0b0100,
// 	LED3 = 0b1000,
// };
// enum
// {
// 	CMD_NODATA = 0x00,
// 	CMD_LED0 = 0x01,
// 	CMD_LED1 = 0x02,
// 	CMD_LED2 = 0x03,
// 	CMD_LED3 = 0x04,
// 	CMD_LED0_clear = 0b10001,
// 	CMD_LED1_clear = 0b10010,
// 	CMD_LED2_clear = 0b10011,
// 	CMD_LED3_clear = 0b10100,
	
// 	CMD_LED_dynamic = 0x05,
// 	CMD_LED_dynamic_clear = 0b10101,
	
// 	CMD_divider = 0x40,
// 	CMD_Re_Init =0xC1,

// };

// enum
// {
// 	STATUS_LIST_IDLE = 0,
// 	STATUS_LIST_RUNNING = 1,
// };

// void status_LED_flash(void);

// void status_init(void);

// uint8_t led_dynamic_time_flash(void);

// void led_alone_time_flash(void);

// void led_cmd_handle(void);

// void LED_Set_CMD_toQueue(uint8_t cmd, uint8_t state_ID, uint8_t time, uint8_t occupy);

// void LED_flash(uint8_t led);

// #endif
