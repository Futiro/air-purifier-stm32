/**
 * @file    aqi.c
 * @brief   PMS5003 PM sensor driver and US EPA AQI calculator.
 *
 * PMS5003 frame format (32 bytes total):
 *  Byte  0–1  : Start characters 0x42, 0x4D
 *  Byte  2–3  : Frame length (28)
 *  Byte  4–5  : PM1.0 standard (µg/m³)
 *  Byte  6–7  : PM2.5 standard (µg/m³)
 *  Byte  8–9  : PM10  standard (µg/m³)
 *  Byte 10–11 : PM1.0 atmospheric (µg/m³)
 *  Byte 12–13 : PM2.5 atmospheric (µg/m³)
 *  Byte 14–15 : PM10  atmospheric (µg/m³)
 *  Byte 16–29 : Particle counts / reserved
 *  Byte 30–31 : Checksum (sum of bytes 0–29)
 *
 * Reception strategy:
 *  UART3 DMA circular mode fills a 32-byte ring buffer.  AQI_Read()
 *  scans for the start bytes and validates the checksum in software.
 */

#include "aqi.h"
#include "main.h"
#include "stm32f4xx_hal.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/*  PMS5003 protocol constants                                        */
/* ------------------------------------------------------------------ */
#define PMS5003_FRAME_LEN   32U
#define PMS5003_START1      0x42U
#define PMS5003_START2      0x4DU

/* DMA receive buffer – two full frames to guarantee frame alignment */
#define RX_BUF_LEN          (2U * PMS5003_FRAME_LEN)

/* ------------------------------------------------------------------ */
/*  Internal state                                                     */
/* ------------------------------------------------------------------ */
static uint8_t s_rx_buf[RX_BUF_LEN];

/* ------------------------------------------------------------------ */
/*  US EPA AQI breakpoint table (PM2.5, µg/m³ × 10 → AQI)           */
/* ------------------------------------------------------------------ */
typedef struct {
    uint16_t c_lo;   /**< Lower concentration breakpoint (×10 µg/m³) */
    uint16_t c_hi;   /**< Upper concentration breakpoint (×10 µg/m³) */
    uint16_t i_lo;   /**< Lower AQI breakpoint                        */
    uint16_t i_hi;   /**< Upper AQI breakpoint                        */
} AQI_Breakpoint_t;

/* NowCast breakpoints per EPA Table 6 (PM2.5 24-h) */
static const AQI_Breakpoint_t k_breakpoints[] = {
    {   0U,  120U,   0U,  50U },   /* Good             */
    { 121U,  354U,  51U, 100U },   /* Moderate         */
    { 355U,  554U, 101U, 150U },   /* Unhealthy (sens) */
    { 555U, 1504U, 151U, 200U },   /* Unhealthy        */
    {1505U, 2504U, 201U, 300U },   /* Very unhealthy   */
    {2505U, 3504U, 301U, 400U },   /* Hazardous        */
    {3505U, 5004U, 401U, 500U },   /* Hazardous+       */
};
#define BP_COUNT (sizeof(k_breakpoints) / sizeof(k_breakpoints[0]))

static const char *const k_category_strings[] = {
    "Good",
    "Moderate",
    "Unhealthy*",
    "Unhealthy",
    "Very Unhealthy",
    "Hazardous",
};

/* ------------------------------------------------------------------ */
/*  Public: AQI calculation helpers                                   */
/* ------------------------------------------------------------------ */
uint16_t AQI_FromPM25(float pm2_5)
{
    if (pm2_5 < 0.0f) { pm2_5 = 0.0f; }

    /* Work in units of 0.1 µg/m³ to stay with integer arithmetic */
    uint16_t c_x10 = (uint16_t)(pm2_5 * 10.0f + 0.5f);

    for (uint8_t i = 0; i < BP_COUNT; i++) {
        if (c_x10 >= k_breakpoints[i].c_lo &&
            c_x10 <= k_breakpoints[i].c_hi) {
            /* Linear interpolation:
             * I = (I_hi - I_lo) / (C_hi - C_lo) * (C_p - C_lo) + I_lo */
            uint32_t num = (uint32_t)(k_breakpoints[i].i_hi - k_breakpoints[i].i_lo)
                         * (uint32_t)(c_x10 - k_breakpoints[i].c_lo);
            uint32_t den = (uint32_t)(k_breakpoints[i].c_hi - k_breakpoints[i].c_lo);
            return (uint16_t)(num / den + k_breakpoints[i].i_lo);
        }
    }
    return 500U; /* Cap at 500 */
}

AQI_Category_t AQI_GetCategory(uint16_t aqi)
{
    if (aqi <= 50U)  return AQI_GOOD;
    if (aqi <= 100U) return AQI_MODERATE;
    if (aqi <= 150U) return AQI_UNHEALTHY_SENS;
    if (aqi <= 200U) return AQI_UNHEALTHY;
    if (aqi <= 300U) return AQI_VERY_UNHEALTHY;
    return AQI_HAZARDOUS;
}

const char *AQI_CategoryString(AQI_Category_t category)
{
    if ((uint8_t)category < (uint8_t)(sizeof(k_category_strings) /
                                      sizeof(k_category_strings[0]))) {
        return k_category_strings[(uint8_t)category];
    }
    return "Unknown";
}

/* ------------------------------------------------------------------ */
/*  UART / DMA initialisation                                         */
/* ------------------------------------------------------------------ */
void AQI_Init(void)
{
    memset(s_rx_buf, 0, sizeof(s_rx_buf));
    /* Start circular DMA reception; fills s_rx_buf continuously */
    HAL_UART_Receive_DMA(&huart3, s_rx_buf, RX_BUF_LEN);
}

/* ------------------------------------------------------------------ */
/*  Frame parser                                                       */
/* ------------------------------------------------------------------ */
AQI_Status_t AQI_Read(AQI_Data_t *data)
{
    /* Search for start-of-frame marker within the DMA buffer */
    for (uint8_t i = 0; i <= RX_BUF_LEN - PMS5003_FRAME_LEN; i++) {
        if (s_rx_buf[i]      != PMS5003_START1) { continue; }
        if (s_rx_buf[i + 1U] != PMS5003_START2) { continue; }

        const uint8_t *frame = &s_rx_buf[i];

        /* Verify frame-length field (bytes 2–3 should be 0x00, 0x1C = 28) */
        uint16_t frame_len = ((uint16_t)frame[2] << 8U) | frame[3];
        if (frame_len != 28U) { continue; }

        /* Verify checksum: sum of bytes 0–29 == bytes 30–31 */
        uint16_t chk = 0U;
        for (uint8_t j = 0; j < PMS5003_FRAME_LEN - 2U; j++) {
            chk += frame[j];
        }
        uint16_t chk_frame = ((uint16_t)frame[30] << 8U) | frame[31];
        if (chk != chk_frame) {
            return AQI_ERR_BAD_FRAME;
        }

        /* Extract PM concentrations (standard, µg/m³) */
        data->pm1_0_ugm3 = ((uint16_t)frame[4]  << 8U) | frame[5];
        data->pm2_5_ugm3 = ((uint16_t)frame[6]  << 8U) | frame[7];
        data->pm10_ugm3  = ((uint16_t)frame[8]  << 8U) | frame[9];

        /* Compute AQI from PM2.5 */
        data->aqi         = AQI_FromPM25((float)data->pm2_5_ugm3);
        data->category    = AQI_GetCategory(data->aqi);
        data->category_str = AQI_CategoryString(data->category);

        return AQI_OK;
    }

    return AQI_ERR_NO_DATA;
}
