/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include "ssd1306.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

/* USER CODE BEGIN PV */

/* ECG waveform (single, realistic line) */
const int8_t ecgWave[] = {
    0,0,1,2,3,2,1,0,
    -1,-3,18,-6,-2,0,
    1,2,1,0,
    0,0,0,0,0
};

#define ECG_LEN (sizeof(ecgWave)/sizeof(ecgWave[0]))

static int baseline = 32;
static int offset   = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);

/* USER CODE BEGIN PFP */
void ECG_Init(void);
void ECG_DrawFrame(void);
void ECG_Update(void);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* Initialize ECG display */
void ECG_Init(void)
{
    ssd1306_Init(&hi2c1);
    srand(HAL_GetTick());
}

/* Draw one ECG frame */
void ECG_DrawFrame(void)
{
    int prevY = baseline;

    for (int x = 0; x < 128; x++)
    {
        int idx = (x + offset) % ECG_LEN;

        /* small controlled noise */
        int noise = (rand() % 3) - 1;

        int y = baseline - ecgWave[idx] + noise;

        if (y < 0)  y = 0;
        if (y > 63) y = 63;

        /* smooth line (no steep jumps) */
        if (y > prevY)
        {
            for (int py = prevY; py <= y; py++)
                ssd1306_DrawPixel(x, py, White);
        }
        else
        {
            for (int py = y; py <= prevY; py++)
                ssd1306_DrawPixel(x, py, White);
        }

        prevY = y;
    }
}

/* Update ECG animation */
void ECG_Update(void)
{
    ssd1306_Fill(Black);
    ECG_DrawFrame();
    ssd1306_UpdateScreen(&hi2c1);

    offset++;
    HAL_Delay(35);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  */
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C1_Init();

  /* USER CODE BEGIN 2 */
  ECG_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  while (1)
  {
      ECG_Update();
  }
}

/* ================= STM32 AUTO-GENERATED CODE BELOW ================= */

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}

static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  HAL_I2C_Init(&hi2c1);
}

static void MX_GPIO_Init(void)
{
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif
