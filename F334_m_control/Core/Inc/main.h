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
#define LED1_Pin GPIO_PIN_13
#define LED1_GPIO_Port GPIOC
#define LED2_Pin GPIO_PIN_14
#define LED2_GPIO_Port GPIOC
#define LED3_Pin GPIO_PIN_15
#define LED3_GPIO_Port GPIOC
#define DAC_H_Pin GPIO_PIN_4
#define DAC_H_GPIO_Port GPIOA
#define DAC_L_Pin GPIO_PIN_6
#define DAC_L_GPIO_Port GPIOA
#define ADC2_4_I_CAP_B_Pin GPIO_PIN_7
#define ADC2_4_I_CAP_B_GPIO_Port GPIOA
#define ADC1_11_Vcap_Pin GPIO_PIN_0
#define ADC1_11_Vcap_GPIO_Port GPIOB
#define ADC1_12_V24_Pin GPIO_PIN_1
#define ADC1_12_V24_GPIO_Port GPIOB
#define ADC2_12_I_chas_Pin GPIO_PIN_2
#define ADC2_12_I_chas_GPIO_Port GPIOB
#define ADC1_13_I_BAT_Pin GPIO_PIN_13
#define ADC1_13_I_BAT_GPIO_Port GPIOB
#define ADC2_14_I_CAP_A_Pin GPIO_PIN_14
#define ADC2_14_I_CAP_A_GPIO_Port GPIOB
#define EN_MOS_Pin GPIO_PIN_15
#define EN_MOS_GPIO_Port GPIOB
#define Cap_En_Pin GPIO_PIN_12
#define Cap_En_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
