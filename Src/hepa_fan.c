/**
 * @file    hepa_fan.c
 * @brief   HEPA fan speed control via TIM3_CH1 PWM (PA6).
 *
 * Timer configuration (set up by MX_TIM3_Init in main.c):
 *  – APB1 timer clock : 90 MHz (SystemCoreClock / 2, × 2 APB1 prescaler)
 *  – Prescaler        : 3  → 90 MHz / (3+1)  = 22.5 MHz timer clock
 *  – Period (ARR)     : 899 → 22.5 MHz / 900 = 25 000 Hz (25 kHz)
 *  – Duty cycle range : 0 – 900 counts
 *
 * The 25 kHz PWM frequency is the standard for 4-wire PC/industrial fans.
 */

#include "hepa_fan.h"
#include "main.h"
#include "stm32f4xx_hal.h"

/* ------------------------------------------------------------------ */
/*  Internal state                                                     */
/* ------------------------------------------------------------------ */
static uint8_t s_current_speed_pct = 0U;

/* ------------------------------------------------------------------ */
/*  Helpers                                                           */
/* ------------------------------------------------------------------ */

/**
 * Convert a percentage to a TIM3 compare register value.
 * ARR = 899, so 100 % → CCR = 900, 0 % → CCR = 0.
 */
static uint32_t pct_to_ccr(uint8_t pct)
{
    return (uint32_t)(__HAL_TIM_GET_AUTORELOAD(&htim3) + 1U) * pct / 100U;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */
void HEPA_Fan_Init(void)
{
    /* Start PWM output; fan is off until HEPA_Fan_SetSpeed() is called */
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0U);
    s_current_speed_pct = 0U;
}

void HEPA_Fan_SetSpeed(uint8_t speed_pct)
{
    if (speed_pct > 100U) {
        speed_pct = 100U;
    }

    /* Prevent stalling: anything between 1 % and FAN_MIN_DUTY_PCT is
     * clamped to FAN_MIN_DUTY_PCT so the motor always has enough torque. */
    if (speed_pct > 0U && speed_pct < FAN_MIN_DUTY_PCT) {
        speed_pct = FAN_MIN_DUTY_PCT;
    }

    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pct_to_ccr(speed_pct));
    s_current_speed_pct = speed_pct;
}

uint8_t HEPA_Fan_GetSpeed(void)
{
    return s_current_speed_pct;
}

void HEPA_Fan_SetSpeedFromAQI(uint16_t aqi)
{
    uint8_t speed;

    if (aqi <= 50U) {
        speed = FAN_SPEED_GOOD;
    } else if (aqi <= 100U) {
        speed = FAN_SPEED_MODERATE;
    } else if (aqi <= 150U) {
        speed = FAN_SPEED_UNHEALTHY_SENS;
    } else if (aqi <= 200U) {
        speed = FAN_SPEED_UNHEALTHY;
    } else if (aqi <= 300U) {
        speed = FAN_SPEED_VERY_UNHEALTHY;
    } else {
        speed = FAN_SPEED_HAZARDOUS;
    }

    HEPA_Fan_SetSpeed(speed);
}
