/* dht11.c — Bit-bang DHT11 driver for STM32 HAL
 * Timing based on DHT11 datasheet:
 *   Host start: pull LOW ≥18 ms, then HIGH ≥40 µs
 *   Sensor response: LOW 80 µs, HIGH 80 µs
 *   Bit 0: HIGH ~26–28 µs;  Bit 1: HIGH ~70 µs
 *   Threshold: 50 µs
 */

#include "dht11.h"

/* ── Internal µs delay using free-running TIM counter ─────── */
static void delay_us(TIM_HandleTypeDef *htim, uint32_t us)
{
  __HAL_TIM_SET_COUNTER(htim, 0);
  while (__HAL_TIM_GET_COUNTER(htim) < us);
}

/* ── Switch data pin direction ─────────────────────────────── */
static void pin_output(GPIO_TypeDef *port, uint16_t pin)
{
  GPIO_InitTypeDef g = {0};
  g.Pin   = pin;
  g.Mode  = GPIO_MODE_OUTPUT_OD;
  g.Speed = GPIO_SPEED_FREQ_HIGH;
  g.Pull  = GPIO_NOPULL;
  HAL_GPIO_Init(port, &g);
}

static void pin_input(GPIO_TypeDef *port, uint16_t pin)
{
  GPIO_InitTypeDef g = {0};
  g.Pin  = pin;
  g.Mode = GPIO_MODE_INPUT;
  g.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(port, &g);
}

/* ── Main read function ─────────────────────────────────────── */
uint8_t DHT11_Read(GPIO_TypeDef *port, uint16_t pin,
                   TIM_HandleTypeDef *htim,
                   uint8_t *temp, uint8_t *hum)
{
  uint8_t  data[5]  = {0};
  uint32_t timeout  = 0;

  /* ── 1. Host start signal: pull LOW 18 ms ──────────────── */
  pin_output(port, pin);
  HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
  HAL_Delay(18);                          /* 18 ms */
  HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
  delay_us(htim, 40);                     /* pull HIGH 40 µs */

  /* ── 2. Switch to input, wait for sensor response LOW ──── */
  pin_input(port, pin);

  timeout = 0;
  while (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET)
  {
    delay_us(htim, 1);
    if (++timeout > 100) return DHT11_ERR_NO_RESPONSE;
  }

  /* wait out sensor LOW ~80 µs */
  timeout = 0;
  while (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET)
  {
    delay_us(htim, 1);
    if (++timeout > 100) return DHT11_ERR_NO_RESPONSE;
  }

  /* wait out sensor HIGH ~80 µs */
  timeout = 0;
  while (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET)
  {
    delay_us(htim, 1);
    if (++timeout > 100) return DHT11_ERR_NO_RESPONSE;
  }

  /* ── 3. Read 40 bits (5 bytes) ──────────────────────────── */
  for (uint8_t byte_i = 0; byte_i < 5; byte_i++)
  {
    for (int8_t bit_i = 7; bit_i >= 0; bit_i--)
    {
      /* Wait for start-of-bit LOW */
      timeout = 0;
      while (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET)
      {
        delay_us(htim, 1);
        if (++timeout > 60) return DHT11_ERR_TIMEOUT;
      }

      /* Measure HIGH pulse length to distinguish 0 vs 1 */
      delay_us(htim, 50);   /* sample after 50 µs */

      if (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET)
      {
        data[byte_i] |= (1 << bit_i);   /* bit = 1 */
        /* wait for pin to go LOW */
        timeout = 0;
        while (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET)
        {
          delay_us(htim, 1);
          if (++timeout > 80) return DHT11_ERR_TIMEOUT;
        }
      }
      /* else bit = 0, pin already LOW or goes LOW soon */
    }
  }

  /* ── 4. Verify checksum ─────────────────────────────────── */
  if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF))
    return DHT11_ERR_CHECKSUM;

  *hum  = data[0];
  *temp = data[2];

  /* Return pin as output HIGH (idle state) */
  pin_output(port, pin);
  HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);

  return DHT11_OK;
}
