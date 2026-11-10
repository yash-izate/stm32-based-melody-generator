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
#include "ssd1306.h"
#include "fonts.h"
#include "dht11.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// ===== LED GROUPS =====
#define G1_LED (GPIO_PIN_9|GPIO_PIN_8|GPIO_PIN_5)
#define G2_LED (GPIO_PIN_14|GPIO_PIN_13|GPIO_PIN_12)

// ===== SENSOR PINS =====
#define SHOCK_PIN GPIO_PIN_4
#define FLAME_PIN GPIO_PIN_6
#define DHT_PIN   GPIO_PIN_7

// ===== SYSTEM SWITCH =====
#define SYS_SW_PIN GPIO_PIN_15

// ===== ADC =====
#define LDR_CHANNEL ADC_CHANNEL_5


// ===== DHT =====
#define DHT_INTERVAL 2000

#define LDR_DARK_THRESHOLD  1500
#define LDR_LIGHT_THRESHOLD 1200

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
uint8_t dark_state = 0;
uint8_t system_state = 0;
uint8_t last_button = 1;

uint8_t dht_temp = 0;
uint8_t dht_hum  = 0;
uint32_t last_dht_time = 0;



uint8_t fire_alarm_active = 0;
uint8_t fire_killed = 0;
uint32_t fireDisplayTick = 0;
uint8_t fireFlash = 0;

uint8_t shock_alarm_active = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */
void SYSTEM_Update(void);
void SYSTEM_Run(void);

uint8_t SHOCK_Read(void);
uint8_t FLAME_Read(void);
uint32_t LDR_Read(void);

void LED_G1_On(void);
void LED_G2_On(void);
void LED_All_Off(void);

void DISPLAY_Off(void);
void DISPLAY_Normal(void);
void DISPLAY_Shock(void);
void DISPLAY_Flame(void);

void DHT_Update(void);

void FIRE_Alert_Run(void);
void FIRE_Display(void);
void FIRE_Check(void);

void Speaker_PlayFreq(uint16_t freq);
void DISPLAY_Night(void);
/* USER CODE BEGIN PFP */
// ... existing prototypes ...
void DISPLAY_Splash(void);   // ← ADD THIS
/* USER CODE END PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void Speaker_PlayFreq(uint16_t freq)
{
    if(freq == 0)
    {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
        return;
    }

    uint32_t prescaler = 72 - 1;     // 1 MHz timer
    uint32_t arr = (1000000 / freq) - 1;

    __HAL_TIM_SET_PRESCALER(&htim1, prescaler);
    __HAL_TIM_SET_AUTORELOAD(&htim1, arr);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, arr / 2);
}

void SYSTEM_Update(void)
{
    uint8_t current = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15);

    if(current == GPIO_PIN_RESET && last_button == GPIO_PIN_SET)
    {
        HAL_Delay(50);  // debounce
        if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15) == GPIO_PIN_RESET)
        {
            system_state = !system_state;
            // Clear all alarms on toggle
            fire_alarm_active = 0;
            fire_killed = 0;
            shock_alarm_active = 0;
            Speaker_PlayFreq(0);
            LED_All_Off();
        }
    }
    last_button = current;
}


uint8_t SHOCK_Read(void)
{
    return HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4);
}

uint8_t FLAME_Read(void)
{
    return HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6);
}

uint32_t LDR_Read(void)
{
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    uint32_t val = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return val;
}



void LED_G1_On(void)
{
    HAL_GPIO_WritePin(GPIOB, G1_LED, GPIO_PIN_SET);
}

void LED_G2_On(void)
{
    HAL_GPIO_WritePin(GPIOB, G2_LED, GPIO_PIN_SET);
}

void LED_All_Off(void)
{
    HAL_GPIO_WritePin(GPIOB, G1_LED|G2_LED, GPIO_PIN_RESET);
}



void DHT_Update(void)
{
    if(HAL_GetTick() - last_dht_time > DHT_INTERVAL)
    {
        last_dht_time = HAL_GetTick();

        uint8_t t, h;
        if(DHT11_Read(GPIOA, GPIO_PIN_7, &htim2, &t, &h) == 0)
        {
            dht_temp = t;
            dht_hum  = h;
        }
    }
}

// display functions
void DISPLAY_Off(void)
{
    ssd1306_Fill(Black);

    // Thin single border
    for(int x = 0; x < 128; x++)
    {
        ssd1306_DrawPixel(x, 0,  White);
        ssd1306_DrawPixel(x, 63, White);
    }
    for(int y = 0; y < 64; y++)
    {
        ssd1306_DrawPixel(0,   y, White);
        ssd1306_DrawPixel(127, y, White);
    }

    // Large lock icon (pixel art, centered ~x=55..73, y=8..38)
    // Shackle (arch top)
    ssd1306_DrawPixel(60,8,White);  ssd1306_DrawPixel(61,8,White);
    ssd1306_DrawPixel(62,8,White);  ssd1306_DrawPixel(63,8,White);
    ssd1306_DrawPixel(64,8,White);  ssd1306_DrawPixel(65,8,White);
    ssd1306_DrawPixel(59,9,White);  ssd1306_DrawPixel(66,9,White);
    ssd1306_DrawPixel(58,10,White); ssd1306_DrawPixel(67,10,White);
    ssd1306_DrawPixel(58,11,White); ssd1306_DrawPixel(67,11,White);
    ssd1306_DrawPixel(58,12,White); ssd1306_DrawPixel(67,12,White);
    ssd1306_DrawPixel(58,13,White); ssd1306_DrawPixel(67,13,White);
    ssd1306_DrawPixel(58,14,White); ssd1306_DrawPixel(67,14,White);
    // Lock body
    for(int x = 55; x <= 70; x++)
    {
        ssd1306_DrawPixel(x, 15, White);
        ssd1306_DrawPixel(x, 32, White);
    }
    for(int y = 15; y <= 32; y++)
    {
        ssd1306_DrawPixel(55, y, White);
        ssd1306_DrawPixel(70, y, White);
    }
    // Keyhole
    ssd1306_DrawPixel(62,21,White); ssd1306_DrawPixel(63,21,White);
    ssd1306_DrawPixel(61,22,White); ssd1306_DrawPixel(64,22,White);
    ssd1306_DrawPixel(61,23,White); ssd1306_DrawPixel(64,23,White);
    ssd1306_DrawPixel(62,24,White); ssd1306_DrawPixel(63,24,White);
    ssd1306_DrawPixel(62,25,White); ssd1306_DrawPixel(63,25,White);
    ssd1306_DrawPixel(61,26,White); ssd1306_DrawPixel(64,26,White);
    ssd1306_DrawPixel(62,27,White); ssd1306_DrawPixel(63,27,White);

    ssd1306_SetCursor(22, 40);
    ssd1306_WriteString("-- STANDBY --", Font_7x10, White);

    ssd1306_SetCursor(28, 52);
    ssd1306_WriteString("PRESS TO ARM", Font_7x10, White);

    ssd1306_UpdateScreen(&hi2c1);
}

void DISPLAY_Normal(void)
{
    static uint32_t last = 0;
    if(HAL_GetTick() - last < 150) return;
    last = HAL_GetTick();

    char buf[20];
    ssd1306_Fill(Black);

    // ===== TOP BAR =====
    for(int i = 0; i < 128; i++) ssd1306_DrawPixel(i, 11, White);

    ssd1306_SetCursor(2, 1);
    ssd1306_WriteString("SMART HOME", Font_7x10, White);

    // ===== DAY/NIGHT icon (top right) =====
    if(dark_state)
    {
        // Crescent moon (right side of top bar)
        // Outer arc (left side of moon body)
        for(int y = 0; y <= 8; y++)
        {
            ssd1306_DrawPixel(112, y+1, White);
            ssd1306_DrawPixel(113, y+1, White);
        }
        ssd1306_DrawPixel(114, 2, White); ssd1306_DrawPixel(114, 8, White);
        ssd1306_DrawPixel(115, 3, White); ssd1306_DrawPixel(115, 7, White);
        // Inner cutout (creates crescent)
        ssd1306_DrawPixel(113, 3, Black); ssd1306_DrawPixel(113, 4, Black);
        ssd1306_DrawPixel(113, 5, Black); ssd1306_DrawPixel(113, 6, Black);
        ssd1306_DrawPixel(113, 7, Black);
        // Stars
        ssd1306_DrawPixel(120, 2, White);
        ssd1306_DrawPixel(123, 5, White);
        ssd1306_DrawPixel(119, 7, White);
    }
    else
    {
        // Sun: center + rays
        // Center dot
        ssd1306_DrawPixel(117, 5, White);
        ssd1306_DrawPixel(118, 5, White);
        ssd1306_DrawPixel(117, 6, White);
        ssd1306_DrawPixel(118, 6, White);
        // Rays: N S E W
        ssd1306_DrawPixel(117, 2, White);
        ssd1306_DrawPixel(117, 9, White);
        ssd1306_DrawPixel(113, 5, White);
        ssd1306_DrawPixel(122, 5, White);
        // Rays: diagonals
        ssd1306_DrawPixel(114, 3, White);
        ssd1306_DrawPixel(121, 3, White);
        ssd1306_DrawPixel(114, 8, White);
        ssd1306_DrawPixel(121, 8, White);
    }

    // ===== TEMP =====
    sprintf(buf, "%d", dht_temp);
    ssd1306_SetCursor(2, 14);
    ssd1306_WriteString("TEMP", Font_7x10, White);
    ssd1306_SetCursor(35, 14);
    ssd1306_WriteString(buf, Font_7x10, White);
    // Degree symbol (pixel art)
    int tx = 35 + (dht_temp >= 100 ? 21 : (dht_temp >= 10 ? 14 : 7));
    ssd1306_DrawPixel(tx,   14, White);
    ssd1306_DrawPixel(tx+1, 14, White);
    ssd1306_DrawPixel(tx,   15, White);
    ssd1306_DrawPixel(tx+1, 15, White);
    ssd1306_SetCursor(tx+3, 14);
    ssd1306_WriteString("C", Font_7x10, White);

    // Temp progress bar (0–50°C range)
    // Temp progress bar (0-50°C range)
        {
            uint8_t bar_w = (dht_temp > 50) ? 60 : ((uint16_t)dht_temp * 60 / 50);
            ssd1306_DrawPixel(2, 26, White);
            for(int i = 0; i < 60; i++) ssd1306_DrawPixel(3+i, 26, (i < bar_w) ? White : Black);
            ssd1306_DrawPixel(63, 26, White);
            ssd1306_DrawPixel(33, 27, White);
            ssd1306_DrawPixel(63, 27, White);
        }

    // ===== HUM =====
    sprintf(buf, "%d%%", dht_hum);
    ssd1306_SetCursor(2, 30);
    ssd1306_WriteString("HUM ", Font_7x10, White);
    ssd1306_SetCursor(35, 30);
    ssd1306_WriteString(buf, Font_7x10, White);

    // Humidity bar
    {
        uint8_t bar_w = (dht_hum > 100) ? 60 : (dht_hum * 60 / 100);
        ssd1306_DrawPixel(2, 42, White);
        for(int i = 0; i < 60; i++) ssd1306_DrawPixel(3+i, 42, (i < bar_w) ? White : Black);
        ssd1306_DrawPixel(63, 42, White);
        ssd1306_DrawPixel(33, 43, White);
        ssd1306_DrawPixel(63, 43, White);
    }

    // ===== VERTICAL DIVIDER =====
    for(int y = 13; y < 63; y++) ssd1306_DrawPixel(76, y, White);

    // ===== RIGHT PANEL: TIME-OF-DAY LABEL =====
    ssd1306_SetCursor(80, 14);
    ssd1306_WriteString(dark_state ? "NIGHT" : "DAY  ", Font_7x10, White);

    // ===== RIGHT PANEL: STATUS ICONS =====
    // Thermometer icon (pixel art) next to temp
    ssd1306_DrawPixel(80, 28, White); ssd1306_DrawPixel(81, 28, White);
    ssd1306_DrawPixel(80, 30, White); ssd1306_DrawPixel(81, 30, White);
    ssd1306_DrawPixel(80, 32, White); ssd1306_DrawPixel(81, 32, White);
    ssd1306_DrawPixel(79, 34, White); ssd1306_DrawPixel(82, 34, White);
    ssd1306_DrawPixel(78, 36, White); ssd1306_DrawPixel(83, 36, White);
    ssd1306_DrawPixel(79, 38, White); ssd1306_DrawPixel(82, 38, White);
    ssd1306_DrawPixel(80, 39, White); ssd1306_DrawPixel(81, 39, White);

    // Droplet icon (humidity)
    ssd1306_DrawPixel(91, 28, White);
    ssd1306_DrawPixel(90, 30, White); ssd1306_DrawPixel(92, 30, White);
    ssd1306_DrawPixel(89, 32, White); ssd1306_DrawPixel(93, 32, White);
    ssd1306_DrawPixel(89, 34, White); ssd1306_DrawPixel(93, 34, White);
    ssd1306_DrawPixel(90, 36, White); ssd1306_DrawPixel(92, 36, White);
    ssd1306_DrawPixel(91, 37, White);

    // ===== BOTTOM STATUS BAR =====
    for(int i = 0; i < 128; i++) ssd1306_DrawPixel(i, 53, White);

    ssd1306_SetCursor(2, 55);
    ssd1306_WriteString("STATUS: OK", Font_7x10, White);

    // Animated scrolling dot
    static uint8_t pos = 100;
    ssd1306_DrawPixel(pos, 58, White);
    ssd1306_DrawPixel(pos+1, 58, White); // 2-pixel wide for visibility
    pos += 2;
    if(pos >= 126) pos = 100;

    ssd1306_UpdateScreen(&hi2c1);
}

void DISPLAY_Shock(void)
{
    static uint32_t last  = 0;
    static uint8_t  phase = 0;
    if(HAL_GetTick() - last < 100) return;
    last = HAL_GetTick();
    phase++;

    ssd1306_Fill(Black);

    // Flashing inverted header
    if(phase % 2 == 0)
    {
        for(int x = 0; x < 128; x++)
            for(int y = 0; y < 13; y++)
                ssd1306_DrawPixel(x, y, White);
        ssd1306_SetCursor(10, 2);
        ssd1306_WriteString("!! EARTHQUAKE !!", Font_7x10, Black);
    }
    else
    {
        ssd1306_SetCursor(10, 2);
        ssd1306_WriteString("!! EARTHQUAKE !!", Font_7x10, White);
    }

    // Expanding diamond rings from center
    uint8_t cx = 64, cy = 34;
    for(int r = 0; r < 3; r++)
    {
        uint8_t rad = (phase * 4 + r * 8) % 22;
        if(rad < 2) continue;
        for(int i = 0; i <= rad; i++)
        {
            int j = rad - i;
            if(cx+i < 127 && cy+j < 52 && cy+j > 13) ssd1306_DrawPixel(cx+i, cy+j, White);
            if(cx-i > 0   && cy+j < 52 && cy+j > 13) ssd1306_DrawPixel(cx-i, cy+j, White);
            if(cx+i < 127 && cy-j > 13 && cy-j < 52) ssd1306_DrawPixel(cx+i, cy-j, White);
            if(cx-i > 0   && cy-j > 13 && cy-j < 52) ssd1306_DrawPixel(cx-i, cy-j, White);
        }
    }

    // Pulsing center cross
    ssd1306_DrawPixel(cx, cy, White);
    if(phase % 2 == 0)
    {
        ssd1306_DrawPixel(cx-1, cy,   White);
        ssd1306_DrawPixel(cx+1, cy,   White);
        ssd1306_DrawPixel(cx,   cy-1, White);
        ssd1306_DrawPixel(cx,   cy+1, White);
    }

    // Bottom bar
    for(int x = 0; x < 128; x++) ssd1306_DrawPixel(x, 53, White);
    ssd1306_SetCursor(12, 55);
    ssd1306_WriteString("DROP-COVER-HOLD!", Font_7x10, White);

    ssd1306_UpdateScreen(&hi2c1);
}

void DISPLAY_Flame(void)
{
    ssd1306_Fill(Black);
    ssd1306_SetCursor(0,20);
    ssd1306_WriteString("FIRE ALERT", Font_7x10, White);
    ssd1306_UpdateScreen(&hi2c1);
}

void FIRE_Alert_Run(void)
{
    if(!fire_alarm_active)
        return;

    // 🔊 simple wail-yelp (from your friend logic)
    static uint16_t freq = 700;
    static int8_t dir = 1;
    static uint32_t lastTick = 0;

    if(HAL_GetTick() - lastTick > 15)
    {
        lastTick = HAL_GetTick();

        freq += dir * 10;

        if(freq >= 1600)
        {
            freq = 1600;
            dir = -1;
        }
        else if(freq <= 700)
        {
            freq = 700;
            dir = 1;
        }

        Speaker_PlayFreq(freq);
    }

    // LEDs ON
    static uint8_t led_state = 0;

    if(HAL_GetTick() % 200 < 100)
        led_state = 1;
    else
        led_state = 0;

    HAL_GPIO_WritePin(GPIOB,
        G1_LED|G2_LED,
        led_state ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // Display
    FIRE_Display();
}

void SYSTEM_Run(void)
{
    FIRE_Check();

    // === FIRE: highest priority ===
    if(fire_alarm_active)
    {
        FIRE_Alert_Run();
        return;
    }

    SYSTEM_Update();

    if(!system_state)
    {
        LED_All_Off();
        Speaker_PlayFreq(0);
        DISPLAY_Off();
        fire_killed = 0;
        return;
    }

    DHT_Update();

    uint32_t ldr = LDR_Read();
    if(ldr > LDR_DARK_THRESHOLD)        dark_state = 1;
    else if(ldr < LDR_LIGHT_THRESHOLD)  dark_state = 0;

    // === SHOCK: second priority ===
    if(shock_alarm_active)
    {
        // Wail buzzer for shock
        static uint32_t shockBuzzTick = 0;
        static uint8_t  shockBuzzState = 0;
        if(HAL_GetTick() - shockBuzzTick > 300)
        {
            shockBuzzTick = HAL_GetTick();
            shockBuzzState = !shockBuzzState;
            Speaker_PlayFreq(shockBuzzState ? 1200 : 800);
        }
        LED_G1_On();
        DISPLAY_Shock();
        return;
    }

    // Normal state: silence speaker
    Speaker_PlayFreq(0);

    // === NIGHT LEDs ===
    if(dark_state) LED_G2_On();
    else HAL_GPIO_WritePin(GPIOB, G2_LED, GPIO_PIN_RESET);

    DISPLAY_Normal();
}

void DISPLAY_Night(void)
{
    ssd1306_Fill(Black);

    ssd1306_SetCursor(20,10);
    ssd1306_WriteString("NIGHT MODE", Font_11x18, White);

    ssd1306_SetCursor(10,40);
    ssd1306_WriteString("LOW LIGHT", Font_7x10, White);

    ssd1306_UpdateScreen(&hi2c1);
}

void FIRE_Check(void)
{
    if(FLAME_Read() == GPIO_PIN_RESET && !fire_killed)
        fire_alarm_active = 1;

    if(SHOCK_Read() == GPIO_PIN_RESET)
        shock_alarm_active = 1;

    // Blue button (PB4) = RESET EVERYTHING
    if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4) == GPIO_PIN_RESET)
    {
        HAL_Delay(50); // debounce
        if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4) == GPIO_PIN_RESET)
        {
            fire_alarm_active  = 0;
            fire_killed        = 1;
            shock_alarm_active = 0;
            Speaker_PlayFreq(0);
            LED_All_Off();
        }
    }
}

void FIRE_Display(void)
{
    if(HAL_GetTick() - fireDisplayTick < 220) return;
    fireDisplayTick = HAL_GetTick();
    fireFlash ^= 1;

    ssd1306_Fill(Black);

    if(fireFlash)
    {
        // Inverted top warning band
        for(int x = 0; x < 128; x++)
            for(int y = 0; y < 14; y++)
                ssd1306_DrawPixel(x, y, White);

        ssd1306_SetCursor(20, 3);
        ssd1306_WriteString("FIRE DETECTED", Font_7x10, Black);

        // Pixel flame left side (3 layered triangles)
        // Base
        for(int x = 8; x <= 24; x++) ssd1306_DrawPixel(x, 50, White);
        // Layer 1
        for(int x = 10; x <= 22; x++) ssd1306_DrawPixel(x, 46, White);
        ssd1306_DrawPixel(9,47,White); ssd1306_DrawPixel(23,47,White);
        // Layer 2
        for(int x = 12; x <= 20; x++) ssd1306_DrawPixel(x, 42, White);
        ssd1306_DrawPixel(11,43,White); ssd1306_DrawPixel(21,43,White);
        ssd1306_DrawPixel(11,44,White); ssd1306_DrawPixel(21,44,White);
        // Tip
        ssd1306_DrawPixel(15,38,White); ssd1306_DrawPixel(16,38,White);
        ssd1306_DrawPixel(17,38,White);
        ssd1306_DrawPixel(15,37,White); ssd1306_DrawPixel(16,36,White);
        ssd1306_DrawPixel(17,37,White);

        // Pixel flame right side
        for(int x = 104; x <= 120; x++) ssd1306_DrawPixel(x, 50, White);
        for(int x = 106; x <= 118; x++) ssd1306_DrawPixel(x, 46, White);
        ssd1306_DrawPixel(105,47,White); ssd1306_DrawPixel(119,47,White);
        for(int x = 108; x <= 116; x++) ssd1306_DrawPixel(x, 42, White);
        ssd1306_DrawPixel(107,43,White); ssd1306_DrawPixel(117,43,White);
        ssd1306_DrawPixel(107,44,White); ssd1306_DrawPixel(117,44,White);
        ssd1306_DrawPixel(111,38,White); ssd1306_DrawPixel(112,38,White);
        ssd1306_DrawPixel(113,38,White);
        ssd1306_DrawPixel(111,37,White); ssd1306_DrawPixel(112,36,White);
        ssd1306_DrawPixel(113,37,White);

        // Center text
        ssd1306_SetCursor(36, 20);
        ssd1306_WriteString("EVACUATE", Font_7x10, White);
        ssd1306_SetCursor(46, 32);
        ssd1306_WriteString("NOW!", Font_7x10, White);

        // Ground line
        for(int x = 0; x < 128; x++) ssd1306_DrawPixel(x, 52, White);

        ssd1306_SetCursor(28, 55);
        ssd1306_WriteString("CALL 101", Font_7x10, White);
    }
    else
    {
        // Frame 2: double border + centered alert
        for(int x = 2; x < 126; x++)
        {
            ssd1306_DrawPixel(x, 2,  White);
            ssd1306_DrawPixel(x, 61, White);
        }
        for(int y = 2; y < 62; y++)
        {
            ssd1306_DrawPixel(2,   y, White);
            ssd1306_DrawPixel(125, y, White);
        }
        for(int x = 5; x < 123; x++)
        {
            ssd1306_DrawPixel(x, 5,  White);
            ssd1306_DrawPixel(x, 58, White);
        }
        for(int y = 5; y < 59; y++)
        {
            ssd1306_DrawPixel(5,   y, White);
            ssd1306_DrawPixel(122, y, White);
        }

        // Warning triangle (pixel art, centered)
        for(int x = 57; x <= 70; x++) ssd1306_DrawPixel(x, 36, White);
        ssd1306_DrawPixel(56,35,White); ssd1306_DrawPixel(71,35,White);
        ssd1306_DrawPixel(55,34,White); ssd1306_DrawPixel(72,34,White);
        ssd1306_DrawPixel(54,33,White); ssd1306_DrawPixel(73,33,White);
        ssd1306_DrawPixel(53,32,White); ssd1306_DrawPixel(74,32,White);
        for(int x = 52; x <= 75; x++) ssd1306_DrawPixel(x, 31, White);
        // Exclamation inside
        ssd1306_DrawPixel(63,25,White); ssd1306_DrawPixel(64,25,White);
        ssd1306_DrawPixel(63,26,White); ssd1306_DrawPixel(64,26,White);
        ssd1306_DrawPixel(63,27,White); ssd1306_DrawPixel(64,27,White);
        ssd1306_DrawPixel(63,29,White); ssd1306_DrawPixel(64,29,White);

        ssd1306_SetCursor(30, 42);
        ssd1306_WriteString("FIRE ALARM!", Font_7x10, White);

        ssd1306_SetCursor(22, 10);
        ssd1306_WriteString("STAY LOW/EXIT", Font_7x10, White);
    }

    ssd1306_UpdateScreen(&hi2c1);
}
void DISPLAY_Splash(void)
{
    ssd1306_Fill(Black);

    // Corners only (no full border)
    // Top-left
    for(int i = 0; i < 12; i++) ssd1306_DrawPixel(i, 0, White);
    for(int i = 0; i < 8;  i++) ssd1306_DrawPixel(0, i, White);
    // Top-right
    for(int i = 116; i < 128; i++) ssd1306_DrawPixel(i, 0, White);
    for(int i = 0;   i < 8;  i++) ssd1306_DrawPixel(127, i, White);
    // Bottom-left
    for(int i = 0; i < 12; i++) ssd1306_DrawPixel(i, 63, White);
    for(int i = 56; i < 64; i++) ssd1306_DrawPixel(0, i, White);
    // Bottom-right
    for(int i = 116; i < 128; i++) ssd1306_DrawPixel(i, 63, White);
    for(int i = 56;  i < 64; i++) ssd1306_DrawPixel(127, i, White);

    ssd1306_SetCursor(14, 6);
    ssd1306_WriteString("SECURITY SYS", Font_7x10, White);

    // Center divider line
    for(int x = 10; x < 118; x += 3)
        ssd1306_DrawPixel(x, 19, White);

    ssd1306_SetCursor(32, 23);
    ssd1306_WriteString("STM32 v2.0", Font_7x10, White);

    // Shield icon (pixel art, centered)
    // Outer shield
    ssd1306_DrawPixel(58,34,White); ssd1306_DrawPixel(59,34,White);
    ssd1306_DrawPixel(60,34,White); ssd1306_DrawPixel(61,34,White);
    ssd1306_DrawPixel(62,34,White); ssd1306_DrawPixel(63,34,White);
    ssd1306_DrawPixel(64,34,White); ssd1306_DrawPixel(65,34,White);
    ssd1306_DrawPixel(66,34,White); ssd1306_DrawPixel(67,34,White);
    ssd1306_DrawPixel(57,35,White); ssd1306_DrawPixel(68,35,White);
    ssd1306_DrawPixel(57,36,White); ssd1306_DrawPixel(68,36,White);
    ssd1306_DrawPixel(57,37,White); ssd1306_DrawPixel(68,37,White);
    ssd1306_DrawPixel(57,38,White); ssd1306_DrawPixel(68,38,White);
    ssd1306_DrawPixel(58,39,White); ssd1306_DrawPixel(67,39,White);
    ssd1306_DrawPixel(59,40,White); ssd1306_DrawPixel(66,40,White);
    ssd1306_DrawPixel(60,41,White); ssd1306_DrawPixel(65,41,White);
    ssd1306_DrawPixel(61,42,White); ssd1306_DrawPixel(64,42,White);
    ssd1306_DrawPixel(62,43,White); ssd1306_DrawPixel(63,43,White);
    // Tick inside shield
    ssd1306_DrawPixel(60,38,White); ssd1306_DrawPixel(61,39,White);
    ssd1306_DrawPixel(62,40,White); ssd1306_DrawPixel(63,39,White);
    ssd1306_DrawPixel(64,38,White); ssd1306_DrawPixel(65,37,White);

    ssd1306_UpdateScreen(&hi2c1);
    HAL_Delay(600);

    // Slide-in loading bar (bracket style)
    ssd1306_DrawPixel(14, 52, White); ssd1306_DrawPixel(14, 53, White);
    ssd1306_DrawPixel(14, 54, White);
    ssd1306_DrawPixel(113,52, White); ssd1306_DrawPixel(113,53, White);
    ssd1306_DrawPixel(113,54, White);

    for(int i = 0; i <= 97; i++)
    {
        ssd1306_DrawPixel(15 + i, 53, White);
        if(i % 8 == 0) ssd1306_UpdateScreen(&hi2c1);
        HAL_Delay(12);
    }
    ssd1306_UpdateScreen(&hi2c1);
    HAL_Delay(300);
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
  MX_I2C1_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start(&htim2);
  __HAL_TIM_SET_AUTORELOAD(&htim2, 0xFFFFFFFF);
  __HAL_TIM_SET_COUNTER(&htim2, 0);
  ssd1306_Init(&hi2c1);
  DISPLAY_Splash();   // <-- add this line
  HAL_Delay(100);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
	  SYSTEM_Run();
    /* USER CODE BEGIN 3 */
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */
  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

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
  htim1.Init.Period = 0xffff;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV2;
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
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 71;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_3
                          |GPIO_PIN_5|GPIO_PIN_8|GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA4 PA6 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB12 PB13 PB14 PB3
                           PB5 PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_3
                          |GPIO_PIN_5|GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB15 PB4 */
  GPIO_InitStruct.Pin = GPIO_PIN_15|GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
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
