/**
 * @file    dht22.c
 * @brief   DHT22 temperature / humidity sensor driver.
 *
 * Protocol (single-wire, open-drain with pull-up):
 *  1. Host pulls line LOW for ≥ 1 ms (start signal).
 *  2. Host releases line; sensor pulls LOW 80 µs then HIGH 80 µs.
 *  3. Sensor sends 40 bits: each bit is preceded by a 50 µs LOW pulse
 *     followed by a HIGH pulse whose width encodes the bit value:
 *       – 26–28 µs → '0'
 *       – 70 µs    → '1'
 *  4. Checksum = sum of first four bytes (lower 8 bits).
 *
 * Hardware:
 *  – PA0 – open-drain GPIO (external 4.7 kΩ pull-up to 3.3 V)
 *  – TIM2 – free-running 1 MHz (1 µs tick) for timing measurements
 */

#include "dht22.h"
#include "main.h"
#include "stm32f4xx_hal.h"

/* ------------------------------------------------------------------ */
/*  Constants                                                          */
/* ------------------------------------------------------------------ */
#define DHT22_START_LOW_MS    2U     /**< Host LOW time (ms) ≥ 1 ms   */
#define DHT22_RESPONSE_WAIT  100U    /**< Max wait for sensor response (µs) */
#define DHT22_BIT_THRESHOLD   50U    /**< HIGH pulse > this → bit '1' (µs) */
#define DHT22_MAX_WAIT       200U    /**< Per-edge timeout (µs)        */
#define DHT22_BIT_COUNT       40U    /**< Total bits per frame         */

/* ------------------------------------------------------------------ */
/*  Helpers: set pin as output / input                                */
/* ------------------------------------------------------------------ */
static void pin_output(void)
{
    GPIO_InitTypeDef cfg = {0};
    cfg.Pin   = DHT22_PIN;
    cfg.Mode  = GPIO_MODE_OUTPUT_OD;
    cfg.Pull  = GPIO_NOPULL;
    cfg.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT22_GPIO_PORT, &cfg);
}

static void pin_input(void)
{
    GPIO_InitTypeDef cfg = {0};
    cfg.Pin  = DHT22_PIN;
    cfg.Mode = GPIO_MODE_INPUT;
    cfg.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT22_GPIO_PORT, &cfg);
}

/* ------------------------------------------------------------------ */
/*  µs timer helpers (TIM2 free-running at 1 MHz)                    */
/* ------------------------------------------------------------------ */
static inline void delay_us(uint32_t us)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (__HAL_TIM_GET_COUNTER(&htim2) < us) { /* spin */ }
}

/**
 * Wait for the pin level to match @p level, return elapsed µs.
 * Returns DHT22_MAX_WAIT + 1 on timeout.
 */
static uint32_t wait_for_level(GPIO_PinState level)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (HAL_GPIO_ReadPin(DHT22_GPIO_PORT, DHT22_PIN) != level) {
        if (__HAL_TIM_GET_COUNTER(&htim2) > DHT22_MAX_WAIT) {
            return DHT22_MAX_WAIT + 1U;
        }
    }
    return __HAL_TIM_GET_COUNTER(&htim2);
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */
void DHT22_Init(void)
{
    DHT22_GPIO_CLK_EN();
    pin_input();
    /* Start TIM2 in free-running mode (configured by MX_TIM2_Init) */
    HAL_TIM_Base_Start(&htim2);
    /* Allow sensor to stabilise after power-on */
    HAL_Delay(2000);
}

DHT22_Status_t DHT22_Read(DHT22_Data_t *data)
{
    uint8_t  raw[5] = {0};
    uint32_t elapsed;

    /* ---- 1. Send start signal ---- */
    pin_output();
    HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_PIN, GPIO_PIN_RESET);
    HAL_Delay(DHT22_START_LOW_MS);
    HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_PIN, GPIO_PIN_SET);
    delay_us(40);
    pin_input();

    /* ---- 2. Wait for sensor response (LOW ~80 µs) ---- */
    elapsed = wait_for_level(GPIO_PIN_RESET);
    if (elapsed > DHT22_RESPONSE_WAIT) {
        return DHT22_ERR_TIMEOUT;
    }
    /* Wait for the HIGH part of the response (~80 µs) */
    elapsed = wait_for_level(GPIO_PIN_SET);
    if (elapsed > DHT22_RESPONSE_WAIT) {
        return DHT22_ERR_TIMEOUT;
    }
    elapsed = wait_for_level(GPIO_PIN_RESET);
    if (elapsed > DHT22_RESPONSE_WAIT) {
        return DHT22_ERR_TIMEOUT;
    }

    /* ---- 3. Read 40 data bits ---- */
    for (uint8_t i = 0; i < DHT22_BIT_COUNT; i++) {
        /* Each bit starts with a ~50 µs LOW pulse */
        elapsed = wait_for_level(GPIO_PIN_SET);
        if (elapsed > DHT22_MAX_WAIT) {
            return DHT22_ERR_TIMEOUT;
        }
        /* Measure the HIGH pulse duration to determine bit value */
        elapsed = wait_for_level(GPIO_PIN_RESET);
        if (elapsed > DHT22_MAX_WAIT) {
            return DHT22_ERR_TIMEOUT;
        }
        /* Shift bit into appropriate byte */
        raw[i / 8U] <<= 1U;
        if (elapsed > DHT22_BIT_THRESHOLD) {
            raw[i / 8U] |= 1U;
        }
    }

    /* ---- 4. Validate checksum ---- */
    uint8_t checksum = (uint8_t)(raw[0] + raw[1] + raw[2] + raw[3]);
    if (checksum != raw[4]) {
        return DHT22_ERR_CHECKSUM;
    }

    /* ---- 5. Decode 16-bit humidity and temperature fields ---- */
    /* Humidity: raw[0]<<8 | raw[1], scale = 0.1 % */
    uint16_t raw_hum = ((uint16_t)raw[0] << 8U) | raw[1];
    data->humidity_pct = (float)raw_hum * 0.1f;

    /* Temperature: bits[14:0] of raw[2]<<8|raw[3], MSB = sign, scale = 0.1 °C */
    uint16_t raw_temp = ((uint16_t)(raw[2] & 0x7FU) << 8U) | raw[3];
    data->temperature_c = (float)raw_temp * 0.1f;
    if (raw[2] & 0x80U) {
        data->temperature_c = -data->temperature_c;
    }

    return DHT22_OK;
}
