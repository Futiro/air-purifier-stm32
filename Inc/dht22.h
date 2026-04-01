/**
 * @file    dht22.h
 * @brief   DHT22 temperature and humidity sensor driver.
 *
 * Uses a single GPIO line for bit-bang communication and a hardware
 * timer (TIM2) configured in free-running µs mode for accurate timing.
 *
 * Usage
 * -----
 *   DHT22_Data_t data;
 *   if (DHT22_Read(&data) == DHT22_OK) {
 *       // data.temperature_c, data.humidity_pct are valid
 *   }
 */

#ifndef __DHT22_H
#define __DHT22_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ------------------------------------------------------------------ */
/*  Return codes                                                       */
/* ------------------------------------------------------------------ */
typedef enum {
    DHT22_OK            = 0,  /**< Successful read                    */
    DHT22_ERR_TIMEOUT   = 1,  /**< Sensor did not respond in time     */
    DHT22_ERR_CHECKSUM  = 2,  /**< Received checksum mismatch         */
} DHT22_Status_t;

/* ------------------------------------------------------------------ */
/*  Sensor reading                                                     */
/* ------------------------------------------------------------------ */
typedef struct {
    float temperature_c;  /**< Temperature in degrees Celsius         */
    float humidity_pct;   /**< Relative humidity in percent           */
} DHT22_Data_t;

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

/**
 * @brief  Initialise the DHT22 driver.
 *
 * Must be called once after MX_TIM2_Init() and GPIO clock enables.
 * Starts TIM2 in free-running mode used for µs-accurate delays.
 */
void DHT22_Init(void);

/**
 * @brief  Read temperature and humidity from the DHT22 sensor.
 *
 * Performs a full single-wire transaction and validates the checksum.
 * The call blocks for approximately 5 ms (sensor response time).
 *
 * @param[out] data  Pointer to a DHT22_Data_t structure to fill.
 * @return DHT22_OK on success, DHT22_ERR_TIMEOUT or DHT22_ERR_CHECKSUM
 *         on failure.
 */
DHT22_Status_t DHT22_Read(DHT22_Data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* __DHT22_H */
