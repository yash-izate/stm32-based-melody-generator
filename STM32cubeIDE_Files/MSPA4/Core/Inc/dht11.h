/* dht11.h — Bit-bang DHT11 driver for STM32 HAL
 * Uses a free-running TIM2 (1 MHz tick) for µs timing.
 * The data pin must be configured as GPIO_MODE_OUTPUT_OD.
 */

#ifndef DHT11_H
#define DHT11_H

#include "stm32f1xx_hal.h"

/* Return codes */
#define DHT11_OK        0
#define DHT11_ERR_NO_RESPONSE  1
#define DHT11_ERR_CHECKSUM     2
#define DHT11_ERR_TIMEOUT      3

/**
 * @brief  Read temperature and humidity from DHT11.
 * @param  port     GPIO port of data pin  (e.g. GPIOA)
 * @param  pin      GPIO pin mask          (e.g. GPIO_PIN_7)
 * @param  htim     Pointer to free-running 1 MHz TIM handle (TIM2)
 * @param  temp     Pointer to temperature result (°C, integer part)
 * @param  hum      Pointer to humidity result (%, integer part)
 * @retval DHT11_OK on success, error code on failure
 */
uint8_t DHT11_Read(GPIO_TypeDef *port, uint16_t pin,
                   TIM_HandleTypeDef *htim,
                   uint8_t *temp, uint8_t *hum);

#endif /* DHT11_H */
