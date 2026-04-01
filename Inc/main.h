/**
 * @file    main.h
 * @brief   Main application header for STM32F446RET6 air purifier.
 *
 * Pin assignments
 * ───────────────
 *  Peripheral        Pin(s)          Function
 *  ─────────────     ──────────      ─────────────────────────────
 *  PMS5003 UART      PB10 / PB11     UART3 TX / RX  (PM2.5 sensor)
 *  DHT22             PA0             GPIO bit-bang + TIM2 µs timer
 *  SSD1306 OLED      PB6  / PB7      I2C1 SCL / SDA
 *  HEPA fan PWM      PA6             TIM3_CH1  (25 kHz PWM)
 *  Status LED        PC13            GPIO output
 */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

/* ------------------------------------------------------------------ */
/*  GPIO pin definitions                                               */
/* ------------------------------------------------------------------ */

/** DHT22 data line – PA0 */
#define DHT22_PIN           GPIO_PIN_0
#define DHT22_GPIO_PORT     GPIOA
#define DHT22_GPIO_CLK_EN() __HAL_RCC_GPIOA_CLK_ENABLE()

/** HEPA fan PWM – PA6 (TIM3_CH1, AF2) */
#define FAN_PWM_PIN         GPIO_PIN_6
#define FAN_PWM_GPIO_PORT   GPIOA

/** Status LED – PC13 (active-low on Nucleo-64 board) */
#define LED_PIN             GPIO_PIN_13
#define LED_GPIO_PORT       GPIOC
#define LED_GPIO_CLK_EN()   __HAL_RCC_GPIOC_CLK_ENABLE()

/* ------------------------------------------------------------------ */
/*  I2C / UART handles (defined in main.c, declared extern here)      */
/* ------------------------------------------------------------------ */
extern I2C_HandleTypeDef  hi2c1;   /**< SSD1306 OLED */
extern UART_HandleTypeDef huart3;  /**< PMS5003 PM sensor */
extern TIM_HandleTypeDef  htim2;   /**< DHT22 µs delay timer */
extern TIM_HandleTypeDef  htim3;   /**< HEPA fan PWM */

/* ------------------------------------------------------------------ */
/*  Application error handler                                         */
/* ------------------------------------------------------------------ */
void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
