/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <math.h>
#include "ssd1306.h"
#include "fonts.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
TIM_HandleTypeDef htim1;

/* USER CODE BEGIN PV */
uint8_t  song_playing = 0;
uint8_t  note_idx = 0;
uint8_t  lyric_idx = 0;
uint32_t note_tick = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM1_Init(void);

/* USER CODE BEGIN PFP */
void OLED_ShowText(const char* txt);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* ===== MUSICAL NOTES (CORRECT OCTAVE) ===== */
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880

   // <--- SPEED CONTROL (0.5 = fast, 1.0 = slow)


void Speaker_PlayFreq(uint16_t freq)
{
    if (freq == 0)
    {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
        return;
    }

    uint32_t arr = (HAL_RCC_GetPCLK2Freq() / freq) - 1;
    __HAL_TIM_SET_AUTORELOAD(&htim1, arr);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, arr / 2);
}

/* ===== OLED ===== */
void OLED_ShowText(const char* txt)
{
    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 22);
    ssd1306_WriteString(txt, Font_7x10, White);
    ssd1306_UpdateScreen(&hi2c1);
}

void OLED_ShowTwoLine(const char* l1, const char* l2)
{
    ssd1306_Fill(Black);

    ssd1306_SetCursor(0, 10);
    ssd1306_WriteString(l1, Font_7x10, White);

    ssd1306_SetCursor(0, 26);
    ssd1306_WriteString(l2, Font_7x10, White);

    ssd1306_UpdateScreen(&hi2c1);
}

/* ===== LED ===== */
void LED_Music(uint16_t freq)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_10, GPIO_PIN_RESET);

    if (freq <= NOTE_E4)
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);   // Green
    else
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);  // Yellow
}
// ===== CORRECT TWINKLE MELODY =====
uint16_t twinkleNotes[] =
{
    NOTE_C4, NOTE_C4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_A4, NOTE_G4,
    NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_D4, NOTE_C4,
    NOTE_G4, NOTE_G4, NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4,
    NOTE_G4, NOTE_G4, NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4,
    NOTE_C4, NOTE_C4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_A4, NOTE_G4,
    NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_D4, NOTE_C4
};

uint16_t twinkleDurations[] =
{
    400,400,400,400,400,400,800,
    400,400,400,400,400,400,800,
    400,400,400,400,400,400,800,
    400,400,400,400,400,400,800,
    400,400,400,400,400,400,800,
    400,400,400,400,400,400,800
};



/* ===== LYRICS ===== */
const char* twinkleLyrics[][2] =
{
    {"Twinkle twinkle", "little star"},
    {"How I wonder", "what you are"},
    {"Up above the world", "so high"},
    {"Like a diamond", "in the sky"}
};

/* which note index changes lyrics */
uint8_t lyricMap[] = {0, 7, 14, 21};


void Twinkle_Update(void)
{
    uint32_t now = HAL_GetTick();

    if (!song_playing)
    {
        song_playing = 1;
        note_idx = 0;
        lyric_idx = 0;
        note_tick = now;

        Speaker_PlayFreq(twinkleNotes[0]);
        LED_Music(twinkleNotes[0]);
        OLED_ShowTwoLine(
            twinkleLyrics[0][0],
            twinkleLyrics[0][1]
        );
        return;
    }

    if (now - note_tick >= twinkleDurations[note_idx])


    {
        note_tick = now;
        note_idx++;

        if (note_idx >= sizeof(twinkleNotes)/sizeof(twinkleNotes[0]))
        {
            Speaker_PlayFreq(0);
            OLED_ShowText("The End");
            song_playing = 0;
            return;
        }

        Speaker_PlayFreq(twinkleNotes[note_idx]);
        LED_Music(twinkleNotes[note_idx]);

        if (lyric_idx + 1 < 4 && note_idx == lyricMap[lyric_idx + 1])
        {
            lyric_idx++;
            OLED_ShowTwoLine(
                twinkleLyrics[lyric_idx][0],
                twinkleLyrics[lyric_idx][1]
            );
        }

        }
    }


void Play_TwinkleTwinkle_Synced(void)
{
    /* ===== NOTES (C4 octave – correct pitch) ===== */
    uint16_t notes[] =
    {
        NOTE_C4, NOTE_C4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_A4, NOTE_G4,
        NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_D4, NOTE_C4,

        NOTE_G4, NOTE_G4, NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4,
        NOTE_G4, NOTE_G4, NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4,

        NOTE_C4, NOTE_C4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_A4, NOTE_G4,
        NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_D4, NOTE_C4
    };

    /* ===== DURATIONS (EXACT FROM WORKING CODE) ===== */
    uint16_t duration[] =
    {
        400,400,400,400,400,400,800,
        400,400,400,400,400,400,800,
        400,400,400,400,400,400,800,
        400,400,400,400,400,400,800,
        400,400,400,400,400,400,800,
        400,400,400,400,400,400,800
    };

    /* ===== LYRICS ===== */
    const char* lyrics[][2] =
    {
        {"Twinkle twinkle", "little star"},
        {"How I wonder", "what you are"},
        {"Up above the world", "so high"},
        {"Like a diamond", "in the sky"}
    };

    /* ===== PHRASE START NOTE INDEX ===== */
    uint8_t lyricMap[] = {0, 7, 14, 21};

    int noteCount = sizeof(notes) / sizeof(notes[0]);
    uint8_t lyricIdx = 0;

    /* ----- SHOW FIRST LYRIC ----- */
    OLED_ShowTwoLine(lyrics[0][0], lyrics[0][1]);

    for (int i = 0; i < noteCount; i++)
    {
        /* ----- CHANGE LYRIC AT PHRASE START ----- */
        if (lyricIdx + 1 < 4 && i == lyricMap[lyricIdx + 1])
        {
            lyricIdx++;
            OLED_ShowTwoLine(
                lyrics[lyricIdx][0],
                lyrics[lyricIdx][1]
            );
        }

        /* ----- PLAY NOTE ----- */
        Speaker_PlayFreq(notes[i]);
        LED_Music(notes[i]);

        HAL_Delay(duration[i]);

        /* ----- SHORT GAP (ARTICULATION) ----- */
        Speaker_PlayFreq(0);
        HAL_Delay(60);
    }

    /* ----- END ----- */
    OLED_ShowTwoLine("The End", "");
    Speaker_PlayFreq(0);
}



/* USER CODE END 0 */

/* ================= MAIN ================= */
int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_TIM1_Init();

  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  ssd1306_Init(&hi2c1);

  while (1)
  {
	  while (1)
	  {
	      Play_TwinkleTwinkle_Synced();
	      HAL_Delay(3000);
	  }

  }
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
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

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
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_10, GPIO_PIN_RESET);

  /*Configure GPIO pins : PB0 PB10 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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
