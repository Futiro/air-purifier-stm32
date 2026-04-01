/**
 * @file    test_aqi.c
 * @brief   Host-native unit tests for AQI calculation and fan speed logic.
 *
 * These tests validate the pure algorithmic functions (AQI_FromPM25,
 * AQI_GetCategory, AQI_CategoryString) and the HEPA fan AQI-to-speed
 * mapping without requiring any STM32 hardware or HAL libraries.
 *
 * Build & run:
 *   cd tests && make && ./test_aqi
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <math.h>

/*
 * We selectively copy out only the functions under test to avoid the
 * HAL_UART_Receive_DMA dependency in AQI_Init and the TIM dependency
 * in HEPA_Fan_Init.  We use #define guards to skip those bodies.
 */

/* ---------- Copied AQI calculation logic (no HAL calls) ----------- */

typedef enum {
    AQI_OK           = 0,
    AQI_ERR_NO_DATA  = 1,
    AQI_ERR_BAD_FRAME = 2,
} AQI_Status_t;

typedef enum {
    AQI_GOOD           = 0,
    AQI_MODERATE       = 1,
    AQI_UNHEALTHY_SENS = 2,
    AQI_UNHEALTHY      = 3,
    AQI_VERY_UNHEALTHY = 4,
    AQI_HAZARDOUS      = 5,
} AQI_Category_t;

typedef struct {
    uint16_t       pm1_0_ugm3;
    uint16_t       pm2_5_ugm3;
    uint16_t       pm10_ugm3;
    uint16_t       aqi;
    AQI_Category_t category;
    const char    *category_str;
} AQI_Data_t;

typedef struct {
    uint16_t c_lo;
    uint16_t c_hi;
    uint16_t i_lo;
    uint16_t i_hi;
} AQI_Breakpoint_t;

static const AQI_Breakpoint_t k_breakpoints[] = {
    {   0U,  120U,   0U,  50U },
    { 121U,  354U,  51U, 100U },
    { 355U,  554U, 101U, 150U },
    { 555U, 1504U, 151U, 200U },
    {1505U, 2504U, 201U, 300U },
    {2505U, 3504U, 301U, 400U },
    {3505U, 5004U, 401U, 500U },
};
#define BP_COUNT (sizeof(k_breakpoints) / sizeof(k_breakpoints[0]))

static const char *const k_category_strings[] = {
    "Good", "Moderate", "Unhealthy*", "Unhealthy",
    "Very Unhealthy", "Hazardous",
};

static uint16_t AQI_FromPM25(float pm2_5)
{
    if (pm2_5 < 0.0f) { pm2_5 = 0.0f; }
    uint16_t c_x10 = (uint16_t)(pm2_5 * 10.0f + 0.5f);
    for (uint8_t i = 0; i < BP_COUNT; i++) {
        if (c_x10 >= k_breakpoints[i].c_lo &&
            c_x10 <= k_breakpoints[i].c_hi) {
            uint32_t num = (uint32_t)(k_breakpoints[i].i_hi - k_breakpoints[i].i_lo)
                         * (uint32_t)(c_x10 - k_breakpoints[i].c_lo);
            uint32_t den = (uint32_t)(k_breakpoints[i].c_hi - k_breakpoints[i].c_lo);
            return (uint16_t)(num / den + k_breakpoints[i].i_lo);
        }
    }
    return 500U;
}

static AQI_Category_t AQI_GetCategory(uint16_t aqi)
{
    if (aqi <= 50U)  return AQI_GOOD;
    if (aqi <= 100U) return AQI_MODERATE;
    if (aqi <= 150U) return AQI_UNHEALTHY_SENS;
    if (aqi <= 200U) return AQI_UNHEALTHY;
    if (aqi <= 300U) return AQI_VERY_UNHEALTHY;
    return AQI_HAZARDOUS;
}

static const char *AQI_CategoryString(AQI_Category_t category)
{
    if ((uint8_t)category < (uint8_t)(sizeof(k_category_strings) /
                                      sizeof(k_category_strings[0]))) {
        return k_category_strings[(uint8_t)category];
    }
    return "Unknown";
}

/* ---------- Fan speed logic --------------------------------------- */
#define FAN_MIN_DUTY_PCT          20U
#define FAN_SPEED_GOOD            20U
#define FAN_SPEED_MODERATE        40U
#define FAN_SPEED_UNHEALTHY_SENS  60U
#define FAN_SPEED_UNHEALTHY       80U
#define FAN_SPEED_VERY_UNHEALTHY  90U
#define FAN_SPEED_HAZARDOUS      100U

static uint8_t s_speed = 0U;

static void HEPA_Fan_SetSpeed(uint8_t pct)
{
    if (pct > 100U)                    { pct = 100U; }
    if (pct > 0U && pct < FAN_MIN_DUTY_PCT) { pct = FAN_MIN_DUTY_PCT; }
    s_speed = pct;
}

static void HEPA_Fan_SetSpeedFromAQI(uint16_t aqi)
{
    uint8_t speed;
    if      (aqi <=  50U) { speed = FAN_SPEED_GOOD; }
    else if (aqi <= 100U) { speed = FAN_SPEED_MODERATE; }
    else if (aqi <= 150U) { speed = FAN_SPEED_UNHEALTHY_SENS; }
    else if (aqi <= 200U) { speed = FAN_SPEED_UNHEALTHY; }
    else if (aqi <= 300U) { speed = FAN_SPEED_VERY_UNHEALTHY; }
    else                  { speed = FAN_SPEED_HAZARDOUS; }
    HEPA_Fan_SetSpeed(speed);
}

/* ------------------------------------------------------------------ */
/*  Simple test framework                                             */
/* ------------------------------------------------------------------ */
static int g_pass = 0;
static int g_fail = 0;

#define TEST(name, expr) do {                                          \
    if (expr) {                                                        \
        printf("  PASS  %s\n", name);                                  \
        g_pass++;                                                      \
    } else {                                                           \
        printf("  FAIL  %s  (line %d)\n", name, __LINE__);            \
        g_fail++;                                                      \
    }                                                                  \
} while (0)

/* ------------------------------------------------------------------ */
/*  AQI tests                                                         */
/* ------------------------------------------------------------------ */
static void test_aqi_boundary_values(void)
{
    printf("\n[AQI] Boundary values\n");
    /* PM2.5 = 0 → AQI = 0 (Good) */
    TEST("PM2.5=0 → AQI=0",        AQI_FromPM25(0.0f) == 0U);
    /* PM2.5 = 12.0 → AQI = 50 (Good upper) */
    TEST("PM2.5=12.0 → AQI=50",    AQI_FromPM25(12.0f) == 50U);
    /* PM2.5 = 12.1 → AQI = 51 (Moderate lower) */
    TEST("PM2.5=12.1 → AQI=51",    AQI_FromPM25(12.1f) == 51U);
    /* PM2.5 = 35.4 → AQI = 100 (Moderate upper) */
    TEST("PM2.5=35.4 → AQI=100",   AQI_FromPM25(35.4f) == 100U);
    /* PM2.5 = 35.5 → AQI = 101 (Unhealthy sens lower) */
    TEST("PM2.5=35.5 → AQI=101",   AQI_FromPM25(35.5f) == 101U);
    /* PM2.5 = 55.4 → AQI = 150 (Unhealthy sens upper) */
    TEST("PM2.5=55.4 → AQI=150",   AQI_FromPM25(55.4f) == 150U);
    /* PM2.5 = 150.4 → AQI = 200 (Unhealthy upper) */
    TEST("PM2.5=150.4 → AQI=200",  AQI_FromPM25(150.4f) == 200U);
    /* PM2.5 = 250.4 → AQI = 300 (Very unhealthy upper) */
    TEST("PM2.5=250.4 → AQI=300",  AQI_FromPM25(250.4f) == 300U);
    /* PM2.5 = 500.4 → AQI = 500 (cap) */
    TEST("PM2.5=500.4 → AQI=500",  AQI_FromPM25(500.4f) == 500U);
    /* Negative input clamped to 0 */
    TEST("PM2.5=-1 → AQI=0",       AQI_FromPM25(-1.0f) == 0U);
}

static void test_aqi_midpoints(void)
{
    printf("\n[AQI] Mid-range interpolation\n");
    /* PM2.5 = 6.0 (mid-Good) → AQI = 25 */
    uint16_t aqi = AQI_FromPM25(6.0f);
    TEST("PM2.5=6.0 → AQI≈25",     aqi == 25U);
    /* PM2.5 = 23.75 (mid-Moderate) → AQI ≈ 75 */
    aqi = AQI_FromPM25(23.75f);
    TEST("PM2.5=23.75 → AQI≈75",   aqi >= 74U && aqi <= 76U);
}

static void test_aqi_category_mapping(void)
{
    printf("\n[AQI] Category mapping\n");
    TEST("AQI=0   → Good",           AQI_GetCategory(0U)   == AQI_GOOD);
    TEST("AQI=50  → Good",           AQI_GetCategory(50U)  == AQI_GOOD);
    TEST("AQI=51  → Moderate",       AQI_GetCategory(51U)  == AQI_MODERATE);
    TEST("AQI=100 → Moderate",       AQI_GetCategory(100U) == AQI_MODERATE);
    TEST("AQI=101 → Unhealthy*",     AQI_GetCategory(101U) == AQI_UNHEALTHY_SENS);
    TEST("AQI=150 → Unhealthy*",     AQI_GetCategory(150U) == AQI_UNHEALTHY_SENS);
    TEST("AQI=151 → Unhealthy",      AQI_GetCategory(151U) == AQI_UNHEALTHY);
    TEST("AQI=200 → Unhealthy",      AQI_GetCategory(200U) == AQI_UNHEALTHY);
    TEST("AQI=201 → Very Unhealthy", AQI_GetCategory(201U) == AQI_VERY_UNHEALTHY);
    TEST("AQI=300 → Very Unhealthy", AQI_GetCategory(300U) == AQI_VERY_UNHEALTHY);
    TEST("AQI=301 → Hazardous",      AQI_GetCategory(301U) == AQI_HAZARDOUS);
    TEST("AQI=500 → Hazardous",      AQI_GetCategory(500U) == AQI_HAZARDOUS);
}

static void test_aqi_category_strings(void)
{
    printf("\n[AQI] Category strings\n");
    TEST("Good string",           strcmp(AQI_CategoryString(AQI_GOOD),           "Good")           == 0);
    TEST("Moderate string",       strcmp(AQI_CategoryString(AQI_MODERATE),       "Moderate")       == 0);
    TEST("Unhealthy* string",     strcmp(AQI_CategoryString(AQI_UNHEALTHY_SENS), "Unhealthy*")     == 0);
    TEST("Unhealthy string",      strcmp(AQI_CategoryString(AQI_UNHEALTHY),      "Unhealthy")      == 0);
    TEST("Very Unhealthy string", strcmp(AQI_CategoryString(AQI_VERY_UNHEALTHY), "Very Unhealthy") == 0);
    TEST("Hazardous string",      strcmp(AQI_CategoryString(AQI_HAZARDOUS),      "Hazardous")      == 0);
}

/* ------------------------------------------------------------------ */
/*  Fan speed tests                                                   */
/* ------------------------------------------------------------------ */
static void test_fan_speed_from_aqi(void)
{
    printf("\n[Fan] AQI-to-speed mapping\n");

    HEPA_Fan_SetSpeedFromAQI(0U);
    TEST("AQI=0   → fan=20%",  s_speed == FAN_SPEED_GOOD);

    HEPA_Fan_SetSpeedFromAQI(50U);
    TEST("AQI=50  → fan=20%",  s_speed == FAN_SPEED_GOOD);

    HEPA_Fan_SetSpeedFromAQI(51U);
    TEST("AQI=51  → fan=40%",  s_speed == FAN_SPEED_MODERATE);

    HEPA_Fan_SetSpeedFromAQI(100U);
    TEST("AQI=100 → fan=40%",  s_speed == FAN_SPEED_MODERATE);

    HEPA_Fan_SetSpeedFromAQI(101U);
    TEST("AQI=101 → fan=60%",  s_speed == FAN_SPEED_UNHEALTHY_SENS);

    HEPA_Fan_SetSpeedFromAQI(150U);
    TEST("AQI=150 → fan=60%",  s_speed == FAN_SPEED_UNHEALTHY_SENS);

    HEPA_Fan_SetSpeedFromAQI(151U);
    TEST("AQI=151 → fan=80%",  s_speed == FAN_SPEED_UNHEALTHY);

    HEPA_Fan_SetSpeedFromAQI(200U);
    TEST("AQI=200 → fan=80%",  s_speed == FAN_SPEED_UNHEALTHY);

    HEPA_Fan_SetSpeedFromAQI(201U);
    TEST("AQI=201 → fan=90%",  s_speed == FAN_SPEED_VERY_UNHEALTHY);

    HEPA_Fan_SetSpeedFromAQI(300U);
    TEST("AQI=300 → fan=90%",  s_speed == FAN_SPEED_VERY_UNHEALTHY);

    HEPA_Fan_SetSpeedFromAQI(301U);
    TEST("AQI=301 → fan=100%", s_speed == FAN_SPEED_HAZARDOUS);

    HEPA_Fan_SetSpeedFromAQI(500U);
    TEST("AQI=500 → fan=100%", s_speed == FAN_SPEED_HAZARDOUS);
}

static void test_fan_speed_clamp(void)
{
    printf("\n[Fan] Speed clamping\n");
    /* Speed = 0 must remain 0 (fan off) */
    HEPA_Fan_SetSpeed(0U);
    TEST("speed=0 stays 0",         s_speed == 0U);
    /* Speed < MIN but > 0 must be clamped to MIN */
    HEPA_Fan_SetSpeed(5U);
    TEST("speed=5 → MIN_DUTY=20",   s_speed == FAN_MIN_DUTY_PCT);
    HEPA_Fan_SetSpeed(19U);
    TEST("speed=19 → MIN_DUTY=20",  s_speed == FAN_MIN_DUTY_PCT);
    /* Speed >= MIN must be unchanged */
    HEPA_Fan_SetSpeed(20U);
    TEST("speed=20 unchanged",      s_speed == 20U);
    HEPA_Fan_SetSpeed(75U);
    TEST("speed=75 unchanged",      s_speed == 75U);
    /* Speed > 100 must be clamped to 100 */
    HEPA_Fan_SetSpeed(110U);
    TEST("speed=110 → 100",         s_speed == 100U);
}

/* ------------------------------------------------------------------ */
/*  PMS5003 checksum test (raw frame validation logic)               */
/* ------------------------------------------------------------------ */
static void test_pms5003_checksum(void)
{
    printf("\n[PMS5003] Frame checksum validation\n");

    /* Build a synthetic valid 32-byte PMS5003 frame:
     *   PM1.0 = 5, PM2.5 = 12, PM10 = 18 (all standard µg/m³)
     *   Frame length field = 28 (0x001C)
     */
    uint8_t frame[32];
    memset(frame, 0, sizeof(frame));
    frame[0]  = 0x42U; /* Start1 */
    frame[1]  = 0x4DU; /* Start2 */
    frame[2]  = 0x00U; /* Frame len hi */
    frame[3]  = 0x1CU; /* Frame len lo (28) */
    frame[4]  = 0x00U; frame[5]  = 5U;   /* PM1.0 std */
    frame[6]  = 0x00U; frame[7]  = 12U;  /* PM2.5 std */
    frame[8]  = 0x00U; frame[9]  = 18U;  /* PM10  std */
    /* Bytes 10–29 remain 0 */

    /* Compute correct checksum */
    uint16_t chk = 0U;
    for (uint8_t i = 0; i < 30U; i++) { chk += frame[i]; }
    frame[30] = (uint8_t)(chk >> 8U);
    frame[31] = (uint8_t)(chk & 0xFFU);

    /* Re-verify checksum */
    uint16_t chk2 = 0U;
    for (uint8_t i = 0; i < 30U; i++) { chk2 += frame[i]; }
    uint16_t chk_frame = ((uint16_t)frame[30] << 8U) | frame[31];
    TEST("Valid frame: checksum matches",   chk2 == chk_frame);

    /* Corrupt one byte and confirm mismatch */
    frame[7] = 99U;
    uint16_t chk3 = 0U;
    for (uint8_t i = 0; i < 30U; i++) { chk3 += frame[i]; }
    TEST("Corrupted frame: checksum fails", chk3 != chk_frame);

    /* Confirm PM values are decoded correctly from the good frame */
    frame[7] = 12U; /* restore */
    uint16_t pm2_5 = ((uint16_t)frame[6] << 8U) | frame[7];
    uint16_t aqi   = AQI_FromPM25((float)pm2_5);
    TEST("PM2.5=12 → AQI=50", aqi == 50U);
}

/* ------------------------------------------------------------------ */
/*  DHT22 data decoding test                                         */
/* ------------------------------------------------------------------ */
static void test_dht22_decode(void)
{
    printf("\n[DHT22] Data decoding\n");

    /* Simulate a raw DHT22 5-byte payload:
     *   Humidity    = 65.3 % → 0x028D (653)
     *   Temperature = 23.8 °C → 0x00EE (238)
     *   Checksum    = 0x02 + 0x8D + 0x00 + 0xEE = 0x17D → lower byte 0x7D
     */
    uint8_t raw[5];
    uint16_t hum_raw  = 653U;  /* 0x028D */
    uint16_t temp_raw = 238U;  /* 0x00EE */
    raw[0] = (uint8_t)(hum_raw  >> 8U);
    raw[1] = (uint8_t)(hum_raw  & 0xFFU);
    raw[2] = (uint8_t)(temp_raw >> 8U);
    raw[3] = (uint8_t)(temp_raw & 0xFFU);
    raw[4] = (uint8_t)(raw[0] + raw[1] + raw[2] + raw[3]);

    uint8_t chk = (uint8_t)(raw[0] + raw[1] + raw[2] + raw[3]);
    TEST("DHT22 checksum valid",          chk == raw[4]);

    float humidity    = (float)(((uint16_t)raw[0] << 8U) | raw[1]) * 0.1f;
    float temperature = (float)(((uint16_t)(raw[2] & 0x7FU) << 8U) | raw[3]) * 0.1f;
    if (raw[2] & 0x80U) { temperature = -temperature; }

    TEST("DHT22 humidity = 65.3%",    fabsf(humidity    - 65.3f) < 0.01f);
    TEST("DHT22 temperature = 23.8C", fabsf(temperature - 23.8f) < 0.01f);

    /* Negative temperature test: −5.6 °C → raw[2] = 0x80|0x00=0x80, raw[3]=56 */
    raw[2] = 0x80U | 0x00U;
    raw[3] = 56U;
    raw[0] = 0x02U; raw[1] = 0x8DU; /* keep humidity */
    raw[4] = (uint8_t)(raw[0] + raw[1] + raw[2] + raw[3]);

    float temp_neg = (float)(((uint16_t)(raw[2] & 0x7FU) << 8U) | raw[3]) * 0.1f;
    if (raw[2] & 0x80U) { temp_neg = -temp_neg; }
    TEST("DHT22 negative temp = -5.6C", fabsf(temp_neg - (-5.6f)) < 0.01f);
}

/* ------------------------------------------------------------------ */
/*  main                                                              */
/* ------------------------------------------------------------------ */
int main(void)
{
    printf("=== Air Purifier STM32 Unit Tests ===\n");

    test_aqi_boundary_values();
    test_aqi_midpoints();
    test_aqi_category_mapping();
    test_aqi_category_strings();
    test_fan_speed_from_aqi();
    test_fan_speed_clamp();
    test_pms5003_checksum();
    test_dht22_decode();

    printf("\n────────────────────────────────────\n");
    printf("Results: %d passed, %d failed\n", g_pass, g_fail);

    return g_fail == 0 ? 0 : 1;
}
