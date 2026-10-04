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
#include "stm32f1xx_hal.h"

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
#define TFT_CS_Pin GPIO_PIN_0
#define TFT_CS_GPIO_Port GPIOB
#define TFT_DC_Pin GPIO_PIN_1
#define TFT_DC_GPIO_Port GPIOB
#define TFT_RST_Pin GPIO_PIN_10
#define TFT_RST_GPIO_Port GPIOB
#define TOUCH_CS_Pin GPIO_PIN_11
#define TOUCH_CS_GPIO_Port GPIOB
#define TFT_LED_Pin GPIO_PIN_12
#define TFT_LED_GPIO_Port GPIOB
#define T_IRQ_Pin GPIO_PIN_13
#define T_IRQ_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define ST7789_CS_Pin        TFT_CS_Pin
#define ST7789_CS_GPIO_Port  TFT_CS_GPIO_Port

#define ST7789_DC_Pin        TFT_DC_Pin
#define ST7789_DC_GPIO_Port  TFT_DC_GPIO_Port

#define ST7789_RST_Pin       TFT_RST_Pin
#define ST7789_RST_GPIO_Port TFT_RST_GPIO_Port
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
