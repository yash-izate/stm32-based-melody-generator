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
#include <stdlib.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
#define RED     0
#define YELLOW  1
#define GREEN   2

uint16_t sirenFreq = 700;
int8_t   sirenDir  = 1;

uint8_t  sirenMode = 0;   // 0 = WAIL, 1 = YELP
uint32_t sirenModeTick = 0;

uint32_t lastSirenTick = 0;
uint32_t lastLedTick   = 0;

// ===== SIREN 2 (HI-LO) =====
uint8_t  hiloState = 0;        // 0 = LO, 1 = HI
uint32_t hiloTick  = 0;

uint8_t  adaptiveLevel = 0;      // 0 → 3
uint32_t adaptiveTick  = 0;
uint8_t ledPhase = 0;   // 0 → 5

// ===== SIREN 8 : MISSILE LAUNCH =====
uint8_t  missileStage = 0;     // 0=COUNTDOWN, 1=IGNITION, 2=LAUNCH
uint32_t missileTick  = 0;

uint16_t missileFreq = 800;
uint16_t missileBeepDelay = 600;
uint8_t missileCount = 0;   // 0=T-3, 1=T-2, 2=T-1
uint8_t  missileBeepOn = 0;
uint32_t missileBeepTick = 0;

typedef enum
{
    TIMER_AUDIO = 0,
    TIMER_LED,
    TIMER_BEEP,
    TIMER_MODE,
    TIMER_MISC,
    TIMER_COUNT
} TimerID_t;

uint32_t timerTick[TIMER_COUNT];

/* ===== BUTTON PINS ===== */
#define RED_BTN_PORT     GPIOB
#define RED_BTN_PIN      GPIO_PIN_15

#define BLUE_BTN_PORT    GPIOB
#define BLUE_BTN_PIN     GPIO_PIN_4

#define RED_BTN_PRESSED()   (HAL_GPIO_ReadPin(RED_BTN_PORT, RED_BTN_PIN) == GPIO_PIN_RESET)
#define BLUE_BTN_PRESSED()  (HAL_GPIO_ReadPin(BLUE_BTN_PORT, BLUE_BTN_PIN) == GPIO_PIN_RESET)

#define BTN_DEBOUNCE_MS     30


typedef struct
{
    uint8_t  lastState;
    uint8_t  stableState;
    uint32_t lastChangeTick;

    uint8_t  tap;
    uint8_t  hold;
    uint8_t  release;
} Button_t;



Button_t btnRed;
Button_t btnBlue;

static uint32_t danceTick = 0;



typedef enum
{
    MODE_IDLE = 0,
    MODE_SIREN,
    MODE_LED_DANCE
} SystemMode_t;

SystemMode_t systemMode = MODE_IDLE;

uint8_t sirenIndex = 0;
#define SIREN_COUNT 8

uint8_t ledPatternIndex = 0;
#define LED_PATTERN_COUNT 5


uint8_t sirenEnabled = 0;   // Emergency siren ON/OFF

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
/* ===== LED CONTROL ===== */
void Timer_ResetAll(void);

void LeftLED_AllOff(void);
void RightLED_AllOff(void);
void LeftLED_Set(uint8_t color);
void RightLED_Set(uint8_t color);
void AllLED_Off(void);


/* ===== AUDIO ===== */
void Speaker_PlayFreq(uint16_t freq);
void Buzzer_PlayFreq(uint16_t freq);


/* ===== SIREN 1 : AMERICAN WAIL + YELP ===== */
void Siren1_AmericanWailYelp_Run(void);


/* ===== SIREN 2 : HI-LO ===== */
void Siren2_HiLo_Run(void);
void Siren3_Phaser_Run(void);
/* ===== SIREN 4 : FIRE TRUCK ===== */
void Siren4_FireTruck_Run(void);
/* ===== SIREN 5 : AMBULANCE (EUROPEAN) ===== */
void Siren5_Ambulance_Run(void);
/* ===== SIREN 6 : AIR RAID ===== */
void Siren6_AirRaid_Run(void);


void Siren8_MissileLaunch_Run(void);
/* ===== SIREN : INDIAN POLICE ===== */
/* ===== SIREN : INDIAN FILM POLICE ===== */
void Siren_IndianFilmPolice_Run(void);

uint8_t Timer_Expired(TimerID_t id, uint32_t interval);

void Button_Test_Run(void);
void Button_Update(Button_t *btn, GPIO_TypeDef *port, uint16_t pin);
void Button_AdvancedTest_Run(void);


/* ===== SIREN CONTROL ===== */
void Siren_RunCurrent(void);


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void Siren_ResetState(void)
{
    sirenFreq = 700;
    sirenDir  = 1;
    sirenMode = 0;

    for (int i = 0; i < TIMER_COUNT; i++)
        timerTick[i] = HAL_GetTick();
}

void Timer_ResetAll(void)
{
    for (int i = 0; i < TIMER_COUNT; i++)
        timerTick[i] = HAL_GetTick();
}


uint8_t Timer_Expired(TimerID_t id, uint32_t interval)
{
    uint32_t now = HAL_GetTick();

    if (now - timerTick[id] >= interval)
    {
        timerTick[id] = now;
        return 1;
    }
    return 0;
}


void Buzzer_PlayFreq(uint16_t freq)
{
    if (freq == 0)
    {
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0);
        return;
    }

    uint32_t arr = (HAL_RCC_GetPCLK1Freq() / freq) - 1;

    __HAL_TIM_SET_AUTORELOAD(&htim2, arr);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, arr / 2);
}

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

// ===== LED GROUPS =====
void LeftLED_AllOff(void)
{
    HAL_GPIO_WritePin(GPIOB,
        GPIO_PIN_9 | GPIO_PIN_7 | GPIO_PIN_5,
        GPIO_PIN_RESET);
}

void RightLED_AllOff(void)
{
    HAL_GPIO_WritePin(GPIOB,
        GPIO_PIN_14 | GPIO_PIN_13 | GPIO_PIN_12,
        GPIO_PIN_RESET);
}

void AllLED_Off(void)
{
    LeftLED_AllOff();
    RightLED_AllOff();
}

void LeftLED_Set(uint8_t color)
{
    LeftLED_AllOff();
    if (color == RED)    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
    if (color == YELLOW) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
    if (color == GREEN)  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
}

void RightLED_Set(uint8_t color)
{
    RightLED_AllOff();
    if (color == RED)    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
    if (color == YELLOW) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
    if (color == GREEN)  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
}



void Siren1_AmericanWailYelp_Run(void)
{
    /* -------- MODE SWITCH (INTERNAL) -------- */
    if (sirenMode == 0 && Timer_Expired(TIMER_MODE, 3000))
    {
        sirenMode = 1;          // WAIL → YELP
    }
    else if (sirenMode == 1 && Timer_Expired(TIMER_MODE, 2000))
    {
        sirenMode = 0;          // YELP → WAIL
    }

    /* ================= AUDIO ================= */

    if (sirenMode == 0)
    {
        // ---- WAIL ----
        if (Timer_Expired(TIMER_AUDIO, 12))
        {
            sirenFreq += sirenDir * 10;

            if (sirenFreq >= 1600)
            {
                sirenFreq = 1600;
                sirenDir = -1;
            }
            else if (sirenFreq <= 700)
            {
                sirenFreq = 700;
                sirenDir = 1;
            }

            Speaker_PlayFreq(sirenFreq);
        }
    }
    else
    {
        // ---- YELP ----
        if (Timer_Expired(TIMER_AUDIO, 80))
        {
            if (sirenFreq > 1200)
                sirenFreq = 900;
            else
                sirenFreq = 1600;

            Speaker_PlayFreq(sirenFreq);
        }
    }

    /* ================= LED SYNC (REFERENCE STYLE) ================= */

    if (Timer_Expired(TIMER_LED, 80))
    {
        if (sirenFreq > 1200)
        {
            LeftLED_Set(RED);
            RightLED_Set(RED);
        }
        else if (sirenFreq > 900)
        {
            LeftLED_Set(YELLOW);
            RightLED_Set(YELLOW);
        }
        else
        {
            LeftLED_Set(GREEN);
            RightLED_Set(GREEN);
        }
    }
}


void Siren2_HiLo_Run(void)
{
    static uint8_t ledStep = 0;
    uint32_t now = HAL_GetTick();

    // -------- HI / LO toggle --------
    if (now - hiloTick >= 450)
    {
        hiloTick = now;
        hiloState ^= 1;
        ledStep = 0;
    }

    if (hiloState)
    {
        // ========== HI ==========
        Speaker_PlayFreq(1500);

        if (now - lastLedTick >= 80)
        {
            lastLedTick = now;
            ledStep ^= 1;
        }

        if (ledStep == 0)
        {
            LeftLED_Set(RED);
            RightLED_Set(RED);
        }
        else
        {
            LeftLED_Set(YELLOW);
            RightLED_Set(YELLOW);
        }
    }
    else
    {
        // ========== LO ==========
        Speaker_PlayFreq(800);

        if (now - lastLedTick >= 200)
        {
            lastLedTick = now;
            ledStep ^= 1;
        }

        if (ledStep)
        {
            LeftLED_Set(GREEN);
            RightLED_Set(GREEN);
        }
        else
        {
            AllLED_Off();
        }
    }
}

void Siren3_Phaser_Run(void)
{
    static uint16_t freq = 900;
    static int8_t dir = 1;
    static uint8_t step = 0;

    uint32_t now = HAL_GetTick();

    /* ===== AUDIO ===== */
    if (now - lastSirenTick >= 8)
    {
        lastSirenTick = now;

        freq += dir * 25;

        if (freq >= 1400)
        {
            freq = 1400;
            dir = -1;
        }
        else if (freq <= 900)
        {
            freq = 900;
            dir = 1;
        }

        Speaker_PlayFreq(freq);
    }

    /* ===== LED PHASER CHASE ===== */
    if (now - lastLedTick >= 60)
    {
        lastLedTick = now;
        step = (step + 1) % 3;

        // LEFT: GREEN → YELLOW → RED
        if (step == 0) LeftLED_Set(GREEN);
        else if (step == 1) LeftLED_Set(YELLOW);
        else LeftLED_Set(RED);

        // RIGHT: RED → YELLOW → GREEN (reverse)
        if (step == 0) RightLED_Set(RED);
        else if (step == 1) RightLED_Set(YELLOW);
        else RightLED_Set(GREEN);
    }
}

void Siren4_FireTruck_Run(void)
{
    uint32_t now = HAL_GetTick();

    /* ===== AUDIO : SLOW HEAVY SWEEP ===== */
    if (now - lastSirenTick >= 18)
    {
        lastSirenTick = now;

        sirenFreq += sirenDir * 6;

        if (sirenFreq >= 1200)
        {
            sirenFreq = 1200;
            sirenDir = -1;
        }
        else if (sirenFreq <= 600)
        {
            sirenFreq = 600;
            sirenDir = 1;
        }

        Speaker_PlayFreq(sirenFreq);
    }

    /* ===== LED : FOLLOW RISE / FALL ===== */
    if (now - lastLedTick >= 100)
    {
        lastLedTick = now;

        if (sirenDir > 0)
        {
            // RISING tone → build up
            if (sirenFreq < 800)
            {
                LeftLED_Set(GREEN);
                RightLED_Set(GREEN);
            }
            else if (sirenFreq < 1000)
            {
                LeftLED_Set(YELLOW);
                RightLED_Set(YELLOW);
            }
            else
            {
                LeftLED_Set(RED);
                RightLED_Set(RED);
            }
        }
        else
        {
            // FALLING tone → release
            if (sirenFreq > 1000)
            {
                LeftLED_Set(RED);
                RightLED_Set(RED);
            }
            else if (sirenFreq > 800)
            {
                LeftLED_Set(YELLOW);
                RightLED_Set(YELLOW);
            }
            else
            {
                LeftLED_Set(GREEN);
                RightLED_Set(GREEN);
            }
        }
    }

}

void Siren5_Ambulance_Run(void)
{
    static uint8_t tone = 0;   // 0 = LOW, 1 = HIGH
    uint32_t now = HAL_GetTick();

    if (now - lastSirenTick >= 350)
    {
        lastSirenTick = now;
        tone ^= 1;

        if (tone == 0)
        {
            // LOW tone
            Speaker_PlayFreq(650);

            LeftLED_Set(GREEN);
            RightLED_Set(GREEN);
        }
        else
        {
            // HIGH tone
            Speaker_PlayFreq(950);

            LeftLED_Set(RED);
            RightLED_Set(RED);
        }
    }

    /* Yellow accent (always on during ambulance) */
    if (now - lastLedTick >= 120)
    {
        lastLedTick = now;

        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_7);   // Left YELLOW
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_13);  // Right YELLOW
    }
}

void Siren6_AirRaid_Run(void)
{
    static uint16_t freq = 400;
    static int8_t dir = 1;

    uint32_t now = HAL_GetTick();

    /* ===== AUDIO : SLOW WIDE SWEEP ===== */
    if (now - lastSirenTick >= 25)
    {
        lastSirenTick = now;

        freq += dir * 4;

        if (freq >= 1200)
        {
            freq = 1200;
            dir = -1;
        }
        else if (freq <= 400)
        {
            freq = 400;
            dir = 1;
        }

        Speaker_PlayFreq(freq);
    }

    /* ===== LED : WARNING LEVEL ===== */
    if (now - lastLedTick >= 120)
    {
        lastLedTick = now;

        if (freq > 950)
        {
            LeftLED_Set(RED);
            RightLED_Set(RED);
        }
        else if (freq > 650)
        {
            LeftLED_Set(YELLOW);
            RightLED_Set(YELLOW);
        }
        else
        {
            LeftLED_Set(GREEN);
            RightLED_Set(GREEN);
        }
    }
}

void Siren8_MissileLaunch_Run(void)
{
    uint32_t now = HAL_GetTick();

    /* ===== STAGE TRANSITION ===== */
    if (missileStage == 0 && now - missileTick > 6000)
    {
        missileStage = 1;   // IGNITION
        missileTick = now;
        missileFreq = 800;
    }
    else if (missileStage == 1 && now - missileTick > 3000)
    {
        missileStage = 2;   // LAUNCH
        missileTick = now;
    }

    /* ================= STAGE 0 : COUNTDOWN ================= */
    if (missileStage == 0)
    {
        /* ---- BEEP TIMING ---- */
        if (now - missileBeepTick >= missileBeepDelay)
        {
            missileBeepTick = now;
            missileBeepOn ^= 1;

            if (missileBeepOn)
                Speaker_PlayFreq(1000);
            else
                Speaker_PlayFreq(0);

            if (missileBeepDelay > 150)
                missileBeepDelay -= 40;
        }

        /* ---- LED COUNTDOWN ---- */
        if (now - lastLedTick >= 300)
        {
            lastLedTick = now;
            static uint8_t side = 0;
            side ^= 1;

            if (side)
            {
                LeftLED_Set(YELLOW);
                RightLED_AllOff();
            }
            else
            {
                RightLED_Set(YELLOW);
                LeftLED_AllOff();
            }
        }
    }

    /* ================= STAGE 1 : IGNITION ================= */
    else if (missileStage == 1)
    {
        if (now - lastSirenTick >= 20)
        {
            lastSirenTick = now;
            missileFreq += 12;
            if (missileFreq > 1600) missileFreq = 1600;
            Speaker_PlayFreq(missileFreq);
        }

        if (now - lastLedTick >= 80)
        {
            lastLedTick = now;
            LeftLED_Set(RED);
            RightLED_Set(RED);
        }
    }

    /* ================= STAGE 2 : LAUNCH ================= */
    else
    {
        Speaker_PlayFreq(1800);

        if (now - lastLedTick >= 100)
        {
            lastLedTick = now;
            static uint8_t pulse = 0;
            pulse ^= 1;

            if (pulse)
            {
                LeftLED_Set(RED);
                RightLED_Set(RED);
            }
            else
            {
                AllLED_Off();
            }
        }
    }
}

void Siren_IndianFilmPolice_Run(void)
{
    static uint16_t freq = 500;
    static int8_t dir = 1;
    uint32_t now = HAL_GetTick();

    /* ===== AUDIO SWEEP ===== */
    if (now - lastSirenTick >= 20)
    {
        lastSirenTick = now;

        freq += dir * 15;

        if (freq >= 1100)
        {
            freq = 1100;
            dir = -1;
        }
        else if (freq <= 500)
        {
            freq = 500;
            dir = 1;
        }

        Speaker_PlayFreq(freq);
    }

    /* ===== LED SYNC ===== */
    if (now - lastLedTick >= 80)
    {
        lastLedTick = now;

        if (freq > 900)
        {
            LeftLED_Set(RED);
            RightLED_Set(RED);
        }
        else if (freq > 700)
        {
            LeftLED_Set(YELLOW);
            RightLED_Set(YELLOW);
        }
        else
        {
            LeftLED_Set(GREEN);
            RightLED_Set(GREEN);
        }
    }
}

void Siren_AlertOverride_Run(void)
{
    static uint16_t spkFreq = 600;
    static int8_t spkDir = 1;

    static uint8_t buzzerState = 0;
    static uint32_t buzzerTick = 0;

    uint32_t now = HAL_GetTick();

    /* ========= SPEAKER : MAIN SWEEP ========= */
    if (now - lastSirenTick >= 15)
    {
        lastSirenTick = now;

        spkFreq += spkDir * 12;

        if (spkFreq >= 1400)
        {
            spkFreq = 1400;
            spkDir = -1;
        }
        else if (spkFreq <= 600)
        {
            spkFreq = 600;
            spkDir = 1;
        }

        Speaker_PlayFreq(spkFreq);
    }

    /* ========= BUZZER : WARNING PULSE ========= */
    if (now - buzzerTick >= (buzzerState ? 80 : 120))
    {
        buzzerTick = now;
        buzzerState ^= 1;

        if (buzzerState)
            Buzzer_PlayFreq(2800);   // sharp alert
        else
            Buzzer_PlayFreq(0);
    }

    /* ========= LED SYNC ========= */
    if (now - lastLedTick >= 70)
    {
        lastLedTick = now;

        if (buzzerState)
        {
            // OVERRIDE warning
            LeftLED_Set(RED);
            RightLED_Set(RED);
        }
        else if (spkFreq > 1100)
        {
            LeftLED_Set(RED);
            RightLED_Set(RED);
        }
        else if (spkFreq > 850)
        {
            LeftLED_Set(YELLOW);
            RightLED_Set(YELLOW);
        }
        else
        {
            LeftLED_Set(GREEN);
            RightLED_Set(GREEN);
        }
    }
}

void Button_Test_Run(void)
{
    static uint8_t redLast  = 1;
    static uint8_t blueLast = 1;

    uint8_t redNow  = RED_BTN_PRESSED();
    uint8_t blueNow = BLUE_BTN_PRESSED();

    /* ===== RED BUTTON TEST ===== */
    if (redNow && !redLast)
    {
        // Red button pressed → solid RED LEDs
        LeftLED_Set(RED);
        RightLED_Set(RED);
    }

    /* ===== BLUE BUTTON TEST ===== */
    if (blueNow && !blueLast)
    {
        // Blue button pressed → solid BLUE substitute (YELLOW used here)
        LeftLED_Set(YELLOW);
        RightLED_Set(YELLOW);
    }

    redLast  = redNow;
    blueLast = blueNow;
}

void LED_Dance_Pattern1(void)
{
    static uint8_t step = 0;

    if (HAL_GetTick() - danceTick < 250) return;
    danceTick = HAL_GetTick();

    step = (step + 1) % 3;

    if (step == 0)
    {
        LeftLED_Set(RED);
        RightLED_Set(GREEN);
    }
    else if (step == 1)
    {
        LeftLED_Set(YELLOW);
        RightLED_Set(RED);
    }
    else
    {
        LeftLED_Set(GREEN);
        RightLED_Set(YELLOW);
    }
}

void LED_Dance_Pattern2(void)
{
    static uint8_t state = 0;

    if (HAL_GetTick() - danceTick < 180) return;
    danceTick = HAL_GetTick();

    state ^= 1;

    if (state)
    {
        LeftLED_Set(RED);
        RightLED_AllOff();
    }
    else
    {
        RightLED_Set(RED);
        LeftLED_AllOff();
    }
}

void LED_Dance_Pattern3(void)
{
    if (HAL_GetTick() - danceTick < 120) return;
    danceTick = HAL_GetTick();

    uint8_t left  = rand() % 3;
    uint8_t right = rand() % 3;

    LeftLED_Set(left);
    RightLED_Set(right);
}

void LED_Dance_Pattern4(void)
{
    static uint8_t phase = 0;

    if (HAL_GetTick() - danceTick < 200) return;
    danceTick = HAL_GetTick();

    phase = (phase + 1) % 3;

    if (phase == 0)
    {
        LeftLED_Set(GREEN);
        RightLED_AllOff();
    }
    else if (phase == 1)
    {
        LeftLED_Set(YELLOW);
        RightLED_Set(GREEN);
    }
    else
    {
        LeftLED_Set(RED);
        RightLED_Set(YELLOW);
    }
}

void LED_Dance_Pattern5(void)
{
    static uint8_t on = 0;

    if (HAL_GetTick() - danceTick < 300) return;
    danceTick = HAL_GetTick();

    on ^= 1;

    if (on)
    {
        LeftLED_Set(RED);
        RightLED_Set(RED);
    }
    else
    {
        AllLED_Off();
    }
}

void LED_Dance_Run(uint8_t pattern)
{
    switch (pattern)
    {
        case 0: LED_Dance_Pattern1(); break;
        case 1: LED_Dance_Pattern2(); break;
        case 2: LED_Dance_Pattern3(); break;
        case 3: LED_Dance_Pattern4(); break;
        case 4: LED_Dance_Pattern5(); break;
        default: AllLED_Off(); break;
    }
}


void Button_Update(Button_t *btn, GPIO_TypeDef *port, uint16_t pin)
{
    uint32_t now = HAL_GetTick();
    uint8_t raw = (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET); // active LOW

    /* Clear one-shot events */
    btn->tap     = 0;
    btn->release = 0;
    btn->hold    = 0;

    /* Debounce */
    if (raw != btn->lastState)
    {
        btn->lastState = raw;
        btn->lastChangeTick = now;
    }

    if ((now - btn->lastChangeTick) < BTN_DEBOUNCE_MS)
        return;

    /* Stable edge detected */
    if (raw != btn->stableState)
    {
        btn->stableState = raw;

        if (raw)
        {
            /* TAP = press edge */
            btn->tap = 1;
        }
        else
        {
            /* RELEASE = release edge */
            btn->release = 1;
        }
    }

    /* HOLD = continuously pressed */
    if (btn->stableState)
    {
        btn->hold = 1;
    }
}



void Button_AdvancedTest_Run(void)
{
    Button_Update(&btnRed,  GPIOB, GPIO_PIN_15);
    Button_Update(&btnBlue, GPIOB, GPIO_PIN_4);

    /* ===== RELEASE (highest priority) ===== */
    if (btnRed.release || btnBlue.release)
    {
        AllLED_Off();
        return;
    }

    /* ===== HOLD ===== */
    if (btnRed.hold)
    {
        LeftLED_Set(RED);
        RightLED_Set(RED);
        return;
    }

    if (btnBlue.hold)
    {
        LeftLED_Set(YELLOW);
        RightLED_Set(YELLOW);
        return;
    }

    /* ===== TAP ===== */
    if (btnRed.tap)
    {
        LeftLED_Set(RED);
        RightLED_Set(GREEN);
        return;
    }

    if (btnBlue.tap)
    {
        LeftLED_Set(GREEN);
        RightLED_Set(GREEN);
        return;
    }
}

void System_Control_Run(void)
{
    /* ================= BUTTON UPDATES ================= */
    Button_Update(&btnRed,  GPIOB, GPIO_PIN_15);
    Button_Update(&btnBlue, GPIOB, GPIO_PIN_4);

    /* ================= RED BUTTON : EMERGENCY TOGGLE ================= */
    if (btnRed.tap && !btnBlue.hold)
    {
        sirenEnabled ^= 1;

        if (sirenEnabled)
            Siren_ResetState();
        else
        {
            Speaker_PlayFreq(0);
            Buzzer_PlayFreq(0);
            AllLED_Off();
        }
    }

    /* ================= MODE SELECTION ================= */
    /* LED dance ONLY if BLUE is held AND RED is NOT held */
    if (btnBlue.hold && !btnRed.hold)
    {
        systemMode = MODE_LED_DANCE;
    }
    else if (sirenEnabled)
    {
        systemMode = MODE_SIREN;
    }
    else
    {
        systemMode = MODE_IDLE;
    }

    static SystemMode_t lastMode = MODE_IDLE;

    if (systemMode != lastMode)
    {
        if (systemMode == MODE_SIREN)
            Siren_ResetState();
        else
        {
            Speaker_PlayFreq(0);
            Buzzer_PlayFreq(0);
        }

        lastMode = systemMode;
    }

    /* ================= MODE EXECUTION ================= */

    switch (systemMode)
    {
        /* ===== EMERGENCY SIREN MODE ===== */
        case MODE_SIREN:

            /* 🔥 CHANGE SIREN: RED held + BLUE tap */
            if (btnRed.hold && btnBlue.tap)
            {
                sirenIndex++;
                if (sirenIndex >= SIREN_COUNT)
                    sirenIndex = 0;

                Siren_ResetState();
            }

            Siren_RunCurrent();
            break;

        /* ===== LED DANCE MODE ===== */
        case MODE_LED_DANCE:

            /* Change LED pattern using RED tap */
            if (btnRed.tap)
            {
                ledPatternIndex++;
                if (ledPatternIndex >= LED_PATTERN_COUNT)
                    ledPatternIndex = 0;
            }

            LED_Dance_Run(ledPatternIndex);
            break;

        /* ===== IDLE MODE ===== */
        default:
            Speaker_PlayFreq(0);
            Buzzer_PlayFreq(0);
            AllLED_Off();
            break;
    }
}

void Siren_RunCurrent(void)
{
    switch (sirenIndex)
    {
        case 0: Siren1_AmericanWailYelp_Run(); break;
        case 1: Siren2_HiLo_Run(); break;
        case 2: Siren3_Phaser_Run(); break;
        case 3: Siren4_FireTruck_Run(); break;
        case 4: Siren5_Ambulance_Run(); break;
        case 5: Siren6_AirRaid_Run(); break;
        case 6: Siren8_MissileLaunch_Run(); break;
        case 7: Siren_IndianFilmPolice_Run(); break;
        default: break;
    }
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
  srand(HAL_GetTick());

  // Enable AFIO clock
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_AFIO_REMAP_SWJ_NOJTAG();


  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);


  // REMOVE GPIO_PIN_3 from this call
  HAL_GPIO_WritePin(GPIOB,
      GPIO_PIN_9  | GPIO_PIN_7  | GPIO_PIN_5 |
      GPIO_PIN_14 | GPIO_PIN_13 | GPIO_PIN_12,
      GPIO_PIN_RESET);


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	      System_Control_Run();


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
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
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
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

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
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_5
                          |GPIO_PIN_7|GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pins : PB12 PB13 PB14 PB5
                           PB7 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_5
                          |GPIO_PIN_7|GPIO_PIN_9;
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
