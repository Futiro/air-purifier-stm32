/**
 * @file    aqi.h
 * @brief   AQI (Air Quality Index) monitoring via PMS5003 PM sensor.
 *
 * The PMS5003 outputs 32-byte frames over UART at 9600 baud.
 * This driver parses PM1.0 / PM2.5 / PM10 concentrations and converts
 * PM2.5 to a US EPA AQI value using the standard breakpoint table.
 *
 * Usage
 * -----
 *   AQI_Init();
 *   // In main loop:
 *   AQI_Data_t data;
 *   if (AQI_Read(&data) == AQI_OK) {
 *       // data.aqi, data.pm2_5_ugm3, data.pm10_ugm3, data.pm1_0_ugm3
 *   }
 */

#ifndef __AQI_H
#define __AQI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ------------------------------------------------------------------ */
/*  Return codes                                                       */
/* ------------------------------------------------------------------ */
typedef enum {
    AQI_OK              = 0,  /**< Valid frame received and parsed     */
    AQI_ERR_NO_DATA     = 1,  /**< No frame available yet             */
    AQI_ERR_BAD_FRAME   = 2,  /**< Frame header or checksum mismatch  */
} AQI_Status_t;

/* ------------------------------------------------------------------ */
/*  AQI category descriptors                                          */
/* ------------------------------------------------------------------ */
typedef enum {
    AQI_GOOD             = 0,   /**< 0  – 50                          */
    AQI_MODERATE         = 1,   /**< 51 – 100                         */
    AQI_UNHEALTHY_SENS   = 2,   /**< 101 – 150                        */
    AQI_UNHEALTHY        = 3,   /**< 151 – 200                        */
    AQI_VERY_UNHEALTHY   = 4,   /**< 201 – 300                        */
    AQI_HAZARDOUS        = 5,   /**< 301 – 500                        */
} AQI_Category_t;

/* ------------------------------------------------------------------ */
/*  Sensor reading                                                     */
/* ------------------------------------------------------------------ */
typedef struct {
    uint16_t       pm1_0_ugm3;   /**< PM1.0 concentration (µg/m³)    */
    uint16_t       pm2_5_ugm3;   /**< PM2.5 concentration (µg/m³)    */
    uint16_t       pm10_ugm3;    /**< PM10  concentration (µg/m³)    */
    uint16_t       aqi;          /**< Computed US-EPA AQI (PM2.5)    */
    AQI_Category_t category;     /**< AQI category enum              */
    const char    *category_str; /**< Human-readable category label  */
} AQI_Data_t;

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

/**
 * @brief  Initialise the AQI driver and start UART DMA reception.
 *
 * Must be called once after MX_USART3_UART_Init().
 */
void AQI_Init(void);

/**
 * @brief  Attempt to parse the latest PMS5003 frame.
 *
 * Non-blocking: returns AQI_ERR_NO_DATA if a complete frame has not
 * arrived yet.
 *
 * @param[out] data  Pointer to an AQI_Data_t structure to fill.
 * @return AQI_OK, AQI_ERR_NO_DATA, or AQI_ERR_BAD_FRAME.
 */
AQI_Status_t AQI_Read(AQI_Data_t *data);

/**
 * @brief  Convert a PM2.5 concentration (µg/m³) to US EPA AQI.
 *
 * Uses the standard NowCast linear interpolation between breakpoints.
 *
 * @param  pm2_5  PM2.5 value in µg/m³.
 * @return Computed AQI (0 – 500).
 */
uint16_t AQI_FromPM25(float pm2_5);

/**
 * @brief  Return the AQI category for a given AQI value.
 */
AQI_Category_t AQI_GetCategory(uint16_t aqi);

/**
 * @brief  Return a short human-readable label for an AQI category.
 */
const char *AQI_CategoryString(AQI_Category_t category);

#ifdef __cplusplus
}
#endif

#endif /* __AQI_H */
