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
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE BEGIN PD */

#define SPEAKER1_PIN  GPIO_PIN_8
#define SPEAKER1_PORT GPIOA   // Mini speaker

#define SPEAKER2_PIN  GPIO_PIN_3
#define SPEAKER2_PORT GPIOB   // Buzzer

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t musicEnable = 0;   // 0 = OFF, 1 = PLAY
uint8_t lastSwitchState = 1;
int length = 149;

char notes[] =
"cdeeeeeeeeedef eeedddbdc ccgggggggggaf fffffeddf eedeedeggaff "
"eeeeeddbdc cdeeeedf efgggfedfe feeeddbdc ccggggeggggga "
"fffffedfe egc bab aga ccddeedef ";

int beats[] = {
1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,
2,1,1,2,1,1,1,1,1,1,
2,2,1,2,1,1,1,1,1,1,1,2,1,1,
2,1,1,2,1,1,1,1,1,1,
2,1,1,2,1,1,1,1,2,1,1,1,1,
2,1,1,1,1,1,1,1,1,2,1,
1,1,1,1,2,2,2,1,1,
1,1,1,1,2,1,1,1,1,1,1,
2,2,1,1,1,1,1,1,1,1,
1,1,1,1,2,1,1,2,1,1,1,1,1,1,
2,1,1,2,1,1,1,1,1,1,
1,1,4,1,1,1,4,1,1,1,4,1,
1,1,1,1,1,1,1,1,4,1
};

int tempo = 200;

uint8_t ledMode = 0;
/*
0 = Pitch Sync (current behavior)
1 = Beat Sync (new mode)
*/


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);

/* USER CODE BEGIN PFP */
/* -------- SOUND -------- */
void delay_us(uint32_t us);
void playTone(uint16_t tone, uint32_t duration);
void playNote(char note, uint32_t duration);

/* -------- MUSIC CONTROL -------- */
void Music_Player_Task(void);
void RedSwitch_Task(void);

/* -------- LED CONTROL -------- */
void LED_AllOff(void);
void LED_Red(void);
void LED_Yellow(void);
void LED_Green(void);

/* -------- NEW LED MODES -------- */

void LED_BeatPulse(uint32_t duration);


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* -------- RED SWITCH TOGGLE -------- */
void RedSwitch_Task(void)
{
    uint8_t currentState = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15);

    // Detect press (HIGH -> LOW transition)
    if (lastSwitchState == 1 && currentState == 0)
    {
        HAL_Delay(50); // debounce

        if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15) == 0)
        {
            musicEnable ^= 1;   // Toggle play/stop
        }
    }

    lastSwitchState = currentState;
}


/* -------- MUSIC TASK -------- */
void Music_Player_Task(void)
{
    for (int i = 0; i < length; i++)
    {
        // Check switch continuously
        RedSwitch_Task();

        if (!musicEnable)
        {
            LED_AllOff();
            HAL_GPIO_WritePin(SPEAKER1_PORT, SPEAKER1_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(SPEAKER2_PORT, SPEAKER2_PIN, GPIO_PIN_RESET);
            return;   // exit immediately
        }

        if (notes[i] == ' ')
        {
            LED_AllOff();
            HAL_Delay(beats[i] * tempo);
        }
        else
        {
            playNote(notes[i], beats[i] * tempo);
        }

        HAL_Delay(tempo / 2);
    }
}


void delay_us(uint32_t us)
{
  uint32_t start = DWT->CYCCNT;
  uint32_t ticks = us * (SystemCoreClock / 1000000);

  while ((DWT->CYCCNT - start) < ticks);
}

/* -------- LED CONTROL -------- */
void LED_AllOff(void)
{
    HAL_GPIO_WritePin(GPIOB,
        GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|
        GPIO_PIN_5|GPIO_PIN_8|GPIO_PIN_9,
        GPIO_PIN_RESET);
}

void LED_Red(void)
{
    LED_AllOff();
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9|GPIO_PIN_14, GPIO_PIN_SET);
}

void LED_Yellow(void)
{
    LED_AllOff();
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8|GPIO_PIN_13, GPIO_PIN_SET);
}

void LED_Green(void)
{
    LED_AllOff();
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5|GPIO_PIN_12, GPIO_PIN_SET);
}


void playTone(uint16_t tone, uint32_t duration)
{
  for (uint32_t i = 0; i < duration * 1000; i += tone * 2)
  {
    RedSwitch_Task();
    if (!musicEnable) return;

    HAL_GPIO_WritePin(SPEAKER1_PORT, SPEAKER1_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SPEAKER2_PORT, SPEAKER2_PIN, GPIO_PIN_SET);

    delay_us(tone);

    HAL_GPIO_WritePin(SPEAKER1_PORT, SPEAKER1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SPEAKER2_PORT, SPEAKER2_PIN, GPIO_PIN_RESET);

    delay_us(tone);
  }
}

void playNote(char note, uint32_t duration)
{
    char names[] = { 'c', 'd', 'e', 'f', 'g', 'a', 'b', 'C' };
    uint16_t tones[] = { 956, 851, 758, 716, 638, 568, 1014, 478 };

    for (int i = 0; i < 8; i++)
    {
        if (names[i] == note)
        {
            if (ledMode == 0)
            {
                // Pitch mode
                if (tones[i] > 800)        LED_Green();
                else if (tones[i] > 650)   LED_Yellow();
                else                        LED_Red();

                playTone(tones[i], duration);
                LED_AllOff();
            }
            else
            {
                // Beat mode (LED ON at start of note)
                HAL_GPIO_WritePin(GPIOB,
                    GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_9 |
                    GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14,
                    GPIO_PIN_SET);

                playTone(tones[i], duration);

                LED_AllOff();
            }

        }
    }
}


void LED_BeatPulse(uint32_t duration)
{
    uint32_t onTime = (duration * 4) / 10;

    HAL_GPIO_WritePin(GPIOB,
        GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_9 |
        GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14,
        GPIO_PIN_SET);

    HAL_Delay(onTime);

    LED_AllOff();
}



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */
  /* USER CODE BEGIN 2 */

  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	      RedSwitch_Task();

	      if (musicEnable)
	      {
	          Music_Player_Task();   // plays full song once
	      }
	      else
	      {
	          LED_AllOff();
	          HAL_GPIO_WritePin(SPEAKER1_PORT, SPEAKER1_PIN, GPIO_PIN_RESET);
	          HAL_GPIO_WritePin(SPEAKER2_PORT, SPEAKER2_PIN, GPIO_PIN_RESET);
	      }


  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_3
                          |GPIO_PIN_5|GPIO_PIN_8|GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);

  /*Configure GPIO pins : PB12 PB13 PB14 PB3
                           PB5 PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_3
                          |GPIO_PIN_5|GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB15 PB4 */
  GPIO_InitStruct.Pin = GPIO_PIN_15|GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
