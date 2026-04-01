/**
 * @file    main.c
 * @brief   STM32F446RET6 Air Purifier – main application.
 *
 * System clock: 180 MHz (HSE 8 MHz, PLL: M=8, N=360, P=2, Q=8)
 *
 * Peripherals configured by this file
 * ─────────────────────────────────────
 *  Peripheral   Config         Purpose
 *  ──────────   ────────────   ─────────────────────────────────
 *  I2C1         PB6/PB7        SSD1306 OLED display (400 kHz)
 *  USART3       PB10/PB11      PMS5003 PM sensor (9600 baud)
 *  TIM2         1 MHz, 32-bit  DHT22 µs delay / pulse measurement
 *  TIM3 CH1     25 kHz PWM     HEPA fan speed control (PA6)
 *  DMA1 Str1    USART3 RX      PMS5003 circular DMA reception
 *  GPIO PA0     Input PU       DHT22 data line
 *  GPIO PC13    Output         Status LED (active-low)
 *
 * Main loop (2-second period)
 * ─────────────────────────────
 *  1. Read AQI from PMS5003 (non-blocking, DMA-backed)
 *  2. Read temperature & humidity from DHT22
 *  3. Adjust HEPA fan speed based on AQI category
 *  4. Update OLED display with all sensor values + fan speed
 *  5. Toggle LED to indicate life-sign
 */

#include "main.h"
#include "aqi.h"
#include "dht22.h"
#include "oled.h"
#include "hepa_fan.h"
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*  HAL peripheral handles (declared extern in main.h)               */
/* ------------------------------------------------------------------ */
I2C_HandleTypeDef  hi2c1;
UART_HandleTypeDef huart3;
TIM_HandleTypeDef  htim2;
TIM_HandleTypeDef  htim3;
DMA_HandleTypeDef  hdma_usart3_rx;

/* ------------------------------------------------------------------ */
/*  Forward declarations                                              */
/* ------------------------------------------------------------------ */
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void Display_Update(const AQI_Data_t *aqi, const DHT22_Data_t *dht,
                           uint8_t fan_pct);

/* ------------------------------------------------------------------ */
/*  main()                                                            */
/* ------------------------------------------------------------------ */
int main(void)
{
    /* -- Core init -------------------------------------------------- */
    HAL_Init();
    SystemClock_Config();

    /* -- Peripheral init -------------------------------------------- */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_I2C1_Init();
    MX_USART3_UART_Init();
    MX_TIM2_Init();
    MX_TIM3_Init();

    /* -- Driver init ------------------------------------------------- */
    OLED_Init();
    DHT22_Init();    /* starts TIM2 + 2 s stabilisation delay */
    AQI_Init();      /* starts UART3 DMA reception            */
    HEPA_Fan_Init(); /* starts TIM3 PWM, fan off              */

    /* Splash screen */
    OLED_Clear();
    OLED_DrawString(1, 2, "Air Purifier");
    OLED_DrawString(2, 4, "STM32F446");
    OLED_Update();
    HAL_Delay(2000);

    /* -- Working data ----------------------------------------------- */
    AQI_Data_t  aqi_data  = { 0, 0, 0, 0, AQI_GOOD, "Good" };
    DHT22_Data_t dht_data = { 25.0f, 50.0f };

    /* ---------------------------------------------------------------- */
    /*  Main loop                                                        */
    /* ---------------------------------------------------------------- */
    while (1) {
        /* 1. Read PM sensor */
        AQI_Read(&aqi_data);   /* keeps last value if no new frame yet */

        /* 2. Read DHT22 */
        DHT22_Read(&dht_data); /* keeps last value on transient error  */

        /* 3. Adjust fan speed */
        HEPA_Fan_SetSpeedFromAQI(aqi_data.aqi);

        /* 4. Update display */
        Display_Update(&aqi_data, &dht_data, HEPA_Fan_GetSpeed());

        /* 5. Toggle status LED */
        HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_PIN);

        /* Loop period: 2 seconds */
        HAL_Delay(2000);
    }
}

/* ------------------------------------------------------------------ */
/*  Display helper                                                     */
/* ------------------------------------------------------------------ */
static void Display_Update(const AQI_Data_t *aqi, const DHT22_Data_t *dht,
                            uint8_t fan_pct)
{
    char line[22];

    OLED_Clear();

    /* Line 0: AQI value + category */
    snprintf(line, sizeof(line), "AQI:%3u %-10s", aqi->aqi,
             aqi->category_str ? aqi->category_str : "");
    OLED_DrawString(0, 0, line);

    /* Line 1: PM2.5 and PM10 */
    snprintf(line, sizeof(line), "PM2.5:%3u PM10:%3u",
             aqi->pm2_5_ugm3, aqi->pm10_ugm3);
    OLED_DrawString(0, 1, line);

    /* Line 2: Temperature */
    snprintf(line, sizeof(line), "Temp: %5.1f C", (double)dht->temperature_c);
    OLED_DrawString(0, 2, line);

    /* Line 3: Humidity */
    snprintf(line, sizeof(line), "RH:   %5.1f %%", (double)dht->humidity_pct);
    OLED_DrawString(0, 3, line);

    /* Line 4: Fan speed text */
    snprintf(line, sizeof(line), "Fan:  %3u %%", fan_pct);
    OLED_DrawString(0, 4, line);

    /* Line 5: Fan speed progress bar (120 px wide, 6 px tall) */
    OLED_DrawProgressBar(4, 42, 120, 6, fan_pct);

    /* Line 7: PM1.0 */
    snprintf(line, sizeof(line), "PM1.0:%3u ug/m3", aqi->pm1_0_ugm3);
    OLED_DrawString(0, 7, line);

    OLED_Update();
}

/* ------------------------------------------------------------------ */
/*  System Clock Configuration                                        */
/*  180 MHz from HSE 8 MHz: PLL M=8, N=360, P=2, Q=8                 */
/* ------------------------------------------------------------------ */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Enable power controller clock; set voltage scale 1 for 180 MHz */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /* HSE oscillator → PLL */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 8U;
    RCC_OscInitStruct.PLL.PLLN       = 360U;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ       = 8U;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /* Enable over-drive to reach 180 MHz */
    if (HAL_PWREx_EnableOverDrive() != HAL_OK) {
        Error_Handler();
    }

    /* AHB=180 MHz, APB1=45 MHz, APB2=90 MHz */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK   |
                                   RCC_CLOCKTYPE_SYSCLK |
                                   RCC_CLOCKTYPE_PCLK1  |
                                   RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

/* ------------------------------------------------------------------ */
/*  GPIO Initialisation                                               */
/* ------------------------------------------------------------------ */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* PA0 – DHT22 data (open-drain, pull-up) – set to input initially */
    GPIO_InitStruct.Pin  = DHT22_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT22_GPIO_PORT, &GPIO_InitStruct);

    /* PA6 – TIM3_CH1 PWM (AF2) */
    GPIO_InitStruct.Pin       = FAN_PWM_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(FAN_PWM_GPIO_PORT, &GPIO_InitStruct);

    /* PC13 – Status LED (output, push-pull) */
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_GPIO_PORT, LED_PIN, GPIO_PIN_SET); /* LED off */
}

/* ------------------------------------------------------------------ */
/*  DMA Initialisation                                                */
/* ------------------------------------------------------------------ */
static void MX_DMA_Init(void)
{
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* DMA1 Stream1 Ch4 – USART3 RX */
    hdma_usart3_rx.Instance                 = DMA1_Stream1;
    hdma_usart3_rx.Init.Channel             = DMA_CHANNEL_4;
    hdma_usart3_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_usart3_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_usart3_rx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_usart3_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart3_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_usart3_rx.Init.Mode                = DMA_CIRCULAR;
    hdma_usart3_rx.Init.Priority            = DMA_PRIORITY_LOW;
    hdma_usart3_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart3_rx) != HAL_OK) {
        Error_Handler();
    }
    __HAL_LINKDMA(&huart3, hdmarx, hdma_usart3_rx);

    HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
}

/* ------------------------------------------------------------------ */
/*  I2C1 Initialisation (SSD1306 OLED, 400 kHz)                      */
/* ------------------------------------------------------------------ */
static void MX_I2C1_Init(void)
{
    hi2c1.Instance             = I2C1;
    hi2c1.Init.ClockSpeed      = 400000U;
    hi2c1.Init.DutyCycle       = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1     = 0U;
    hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2     = 0U;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }
}

/* ------------------------------------------------------------------ */
/*  USART3 Initialisation (PMS5003, 9600 baud)                       */
/* ------------------------------------------------------------------ */
static void MX_USART3_UART_Init(void)
{
    huart3.Instance          = USART3;
    huart3.Init.BaudRate     = 9600U;
    huart3.Init.WordLength   = UART_WORDLENGTH_8B;
    huart3.Init.StopBits     = UART_STOPBITS_1;
    huart3.Init.Parity       = UART_PARITY_NONE;
    huart3.Init.Mode         = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart3) != HAL_OK) {
        Error_Handler();
    }
}

/* ------------------------------------------------------------------ */
/*  TIM2 Initialisation (1 MHz free-running, DHT22 timing)           */
/* ------------------------------------------------------------------ */
static void MX_TIM2_Init(void)
{
    /*
     * APB1 timer clock = 90 MHz (APB1 = 45 MHz, × 2 due to prescaler ≠ 1)
     * Prescaler = 89 → 90 MHz / 90 = 1 MHz (1 µs resolution)
     * Period = 0xFFFFFFFF (32-bit free-running)
     */
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};

    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = 89U;
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = 0xFFFFFFFFU;
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }
}

/* ------------------------------------------------------------------ */
/*  TIM3 Initialisation (25 kHz PWM, HEPA fan)                       */
/* ------------------------------------------------------------------ */
static void MX_TIM3_Init(void)
{
    /*
     * APB1 timer clock = 90 MHz
     * Prescaler = 3 → 90 MHz / 4 = 22.5 MHz timer clock
     * Period (ARR) = 899 → 22.5 MHz / 900 = 25 000 Hz
     */
    TIM_OC_InitTypeDef sConfigOC = {0};

    htim3.Instance               = TIM3;
    htim3.Init.Prescaler         = 3U;
    htim3.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim3.Init.Period            = 899U;
    htim3.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK) {
        Error_Handler();
    }

    sConfigOC.OCMode     = TIM_OCMODE_PWM1;
    sConfigOC.Pulse      = 0U;   /* 0 % duty cycle initially */
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }
}

/* ------------------------------------------------------------------ */
/*  Error handler                                                     */
/* ------------------------------------------------------------------ */
void Error_Handler(void)
{
    __disable_irq();
    /* Rapid LED flash to indicate fault */
    while (1) {
        HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_PIN);
        for (volatile uint32_t i = 0; i < 500000UL; i++) { /* spin */ }
    }
}

/* ------------------------------------------------------------------ */
/*  Assert callback (only compiled when USE_FULL_ASSERT is defined)  */
/* ------------------------------------------------------------------ */
#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
    Error_Handler();
}
#endif
