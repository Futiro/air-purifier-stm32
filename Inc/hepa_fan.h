/**
 * @file    hepa_fan.h
 * @brief   HEPA fan speed control via PWM (TIM3_CH1, PA6).
 *
 * Generates a 25 kHz PWM signal suitable for 4-wire PWM-controlled fans.
 * Fan speed is set as a percentage (0–100 %) and mapped to a configurable
 * minimum–maximum duty cycle to account for motor start-up torque.
 *
 * Usage
 * -----
 *   HEPA_Fan_Init();
 *   HEPA_Fan_SetSpeed(75);   // run at 75 %
 *   HEPA_Fan_SetSpeed(0);    // stop fan
 */

#ifndef __HEPA_FAN_H
#define __HEPA_FAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ------------------------------------------------------------------ */
/*  Fan speed limits (percent)                                        */
/* ------------------------------------------------------------------ */

/**
 * Minimum PWM duty cycle below which the fan stalls.
 * Adjust to match the specific fan motor characteristics.
 */
#define FAN_MIN_DUTY_PCT   20U

/** Maximum PWM duty cycle (full speed). */
#define FAN_MAX_DUTY_PCT  100U

/* ------------------------------------------------------------------ */
/*  AQI → fan speed thresholds                                        */
/* ------------------------------------------------------------------ */
/** Fan speed mappings for each AQI category (percent). */
#define FAN_SPEED_GOOD            20U  /**< AQI   0 –  50 */
#define FAN_SPEED_MODERATE        40U  /**< AQI  51 – 100 */
#define FAN_SPEED_UNHEALTHY_SENS  60U  /**< AQI 101 – 150 */
#define FAN_SPEED_UNHEALTHY       80U  /**< AQI 151 – 200 */
#define FAN_SPEED_VERY_UNHEALTHY  90U  /**< AQI 201 – 300 */
#define FAN_SPEED_HAZARDOUS      100U  /**< AQI 301 – 500 */

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

/**
 * @brief  Initialise the fan PWM output and start the timer.
 *
 * Must be called after MX_TIM3_Init().  The fan is left at 0 % speed.
 */
void HEPA_Fan_Init(void);

/**
 * @brief  Set the fan speed.
 *
 * @param speed_pct  Desired speed in percent (0 = off, 100 = full).
 *                   Values between 1 and FAN_MIN_DUTY_PCT – 1 are
 *                   clamped to FAN_MIN_DUTY_PCT to prevent stalling.
 */
void HEPA_Fan_SetSpeed(uint8_t speed_pct);

/**
 * @brief  Return the current fan speed in percent.
 */
uint8_t HEPA_Fan_GetSpeed(void);

/**
 * @brief  Automatically select fan speed based on an AQI value.
 *
 * Maps the AQI into one of six pre-defined speed steps corresponding
 * to the standard AQI categories.
 *
 * @param aqi  US-EPA AQI value (0–500).
 */
void HEPA_Fan_SetSpeedFromAQI(uint16_t aqi);

#ifdef __cplusplus
}
#endif

#endif /* __HEPA_FAN_H */
