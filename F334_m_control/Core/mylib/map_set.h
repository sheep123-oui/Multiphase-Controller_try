#pragma once
#if defined(STM32H750xx)
#define RAM_Set_DTCM __attribute__((section(".RAM_DTCM"))) //MDMA 400M
#define RAM_Set_D1 __attribute__((section(".RAM_D1"))) //MDMA,DMA1,2 200M
#define RAM_Set_D2 __attribute__((section(".RAM_D2"))) //MDMA,DMA1,2 200M
#define RAM_Set_D3 __attribute__((section(".RAM_D3"))) //B M DMA 1 2 200M

#define RAM_Set_D2SRAM1 __attribute__((section(".RAM_SRAM1"))) //D2域	128k
#define RAM_Set_D2SRAM2 __attribute__((section(".RAM_SRAM2"))) //D2域	128k
#define RAM_Set_D2SRAM3 __attribute__((section(".RAM_SRAM3"))) //D2域	64k

#endif

#define RAM_Align_32Byte __attribute__((aligned(32)))