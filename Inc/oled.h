/**
 * @file    oled.h
 * @brief   SSD1306 128×64 OLED display driver (I2C).
 *
 * Provides a minimal framebuffer-based driver for the SSD1306 128×64
 * monochrome OLED.  A full 128×64 / 8-bit-per-page framebuffer is
 * maintained in RAM and flushed to the display with OLED_Update().
 *
 * Typical usage
 * -------------
 *   OLED_Init();
 *   OLED_Clear();
 *   OLED_DrawString(0, 0, "AQI: 42");
 *   OLED_DrawString(0, 2, "Temp: 23.5 C");
 *   OLED_DrawString(0, 4, "RH:   58.0 %");
 *   OLED_DrawString(0, 6, "Fan:  60%");
 *   OLED_Update();
 */

#ifndef __OLED_H
#define __OLED_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ------------------------------------------------------------------ */
/*  Display geometry                                                   */
/* ------------------------------------------------------------------ */
#define OLED_WIDTH   128U  /**< Display width in pixels               */
#define OLED_HEIGHT   64U  /**< Display height in pixels              */
#define OLED_PAGES     8U  /**< Number of 8-pixel-tall pages          */

/* ------------------------------------------------------------------ */
/*  I2C address                                                        */
/* ------------------------------------------------------------------ */
/** 7-bit address of SSD1306 with SA0 pulled low (0x3C << 1 = 0x78). */
#define SSD1306_I2C_ADDR  0x3CU

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

/**
 * @brief  Initialise and power-on the SSD1306 display.
 *
 * Sends the standard initialisation command sequence and clears the
 * display.  Must be called after MX_I2C1_Init().
 */
void OLED_Init(void);

/**
 * @brief  Clear the framebuffer (all pixels off).
 *
 * Does NOT update the physical display – call OLED_Update() afterwards.
 */
void OLED_Clear(void);

/**
 * @brief  Set a single pixel in the framebuffer.
 *
 * @param x      Column index (0–127).
 * @param y      Row index (0–63).
 * @param color  Non-zero = pixel on, 0 = pixel off.
 */
void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t color);

/**
 * @brief  Draw a null-terminated ASCII string at a character cell.
 *
 * Uses a built-in 5×7 font; each character cell is 6×8 pixels.
 *
 * @param col   Character column (0–20, wraps silently).
 * @param page  Character row / page (0–7).
 * @param str   Null-terminated string.
 */
void OLED_DrawString(uint8_t col, uint8_t page, const char *str);

/**
 * @brief  Draw a single character at a character cell.
 *
 * @param col   Character column (0–20).
 * @param page  Character row / page (0–7).
 * @param ch    ASCII character to draw.
 */
void OLED_DrawChar(uint8_t col, uint8_t page, char ch);

/**
 * @brief  Draw a horizontal progress bar.
 *
 * @param x      Left edge column (0–127).
 * @param y      Top edge row (0–63).
 * @param width  Bar width in pixels.
 * @param height Bar height in pixels.
 * @param pct    Fill percentage (0–100).
 */
void OLED_DrawProgressBar(uint8_t x, uint8_t y, uint8_t width,
                          uint8_t height, uint8_t pct);

/**
 * @brief  Flush the framebuffer to the physical display over I2C.
 */
void OLED_Update(void);

#ifdef __cplusplus
}
#endif

#endif /* __OLED_H */
