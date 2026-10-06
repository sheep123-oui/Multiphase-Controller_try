/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED0_Pin GPIO_PIN_13
#define LED0_GPIO_Port GPIOC
#define LED1_Pin GPIO_PIN_14
#define LED1_GPIO_Port GPIOC
#define LED2_Pin GPIO_PIN_15
#define LED2_GPIO_Port GPIOC
#define TIM2_CH1_EC1_Pin GPIO_PIN_0
#define TIM2_CH1_EC1_GPIO_Port GPIOA
#define TIM2_CH2_EC2_Pin GPIO_PIN_1
#define TIM2_CH2_EC2_GPIO_Port GPIOA
#define DACH_Pin GPIO_PIN_4
#define DACH_GPIO_Port GPIOA
#define DACL_Pin GPIO_PIN_5
#define DACL_GPIO_Port GPIOA
#define DAC_COMP_Pin GPIO_PIN_6
#define DAC_COMP_GPIO_Port GPIOA
#define ADC2_4_I_CAP_B_Pin GPIO_PIN_7
#define ADC2_4_I_CAP_B_GPIO_Port GPIOA
#define ADC1_11_V_CAP_Pin GPIO_PIN_0
#define ADC1_11_V_CAP_GPIO_Port GPIOB
#define ADC1_12_V_24V_Pin GPIO_PIN_1
#define ADC1_12_V_24V_GPIO_Port GPIOB
#define ADC2_12_I_CHASSIS_Pin GPIO_PIN_2
#define ADC2_12_I_CHASSIS_GPIO_Port GPIOB
#define ADC1_13_I_BAT_Pin GPIO_PIN_13
#define ADC1_13_I_BAT_GPIO_Port GPIOB
#define ADC2_14_I_CAP_A_Pin GPIO_PIN_14
#define ADC2_14_I_CAP_A_GPIO_Port GPIOB
#define EN_MOS_Pin GPIO_PIN_15
#define EN_MOS_GPIO_Port GPIOB
#define CAP_EN_Pin GPIO_PIN_12
#define CAP_EN_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
