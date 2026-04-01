/**
 * @file    stm32f4xx_it.c
 * @brief   STM32F4xx interrupt service routines.
 *
 * Only the interrupts actually used by the air purifier firmware are
 * implemented here.  All other exception handlers fall through to
 * their weak default implementations in the startup file.
 */

#include "main.h"
#include "stm32f4xx_it.h"

/* ------------------------------------------------------------------ */
/*  Cortex-M4 exception handlers                                     */
/* ------------------------------------------------------------------ */

/**
 * @brief  NMI handler.
 */
void NMI_Handler(void)
{
    while (1) { /* Trap */ }
}

/**
 * @brief  Hard-fault handler.
 */
void HardFault_Handler(void)
{
    while (1) { /* Trap */ }
}

/**
 * @brief  Memory management fault handler.
 */
void MemManage_Handler(void)
{
    while (1) { /* Trap */ }
}

/**
 * @brief  Bus fault handler.
 */
void BusFault_Handler(void)
{
    while (1) { /* Trap */ }
}

/**
 * @brief  Usage fault handler.
 */
void UsageFault_Handler(void)
{
    while (1) { /* Trap */ }
}

/**
 * @brief  SVC handler.
 */
void SVC_Handler(void)
{
}

/**
 * @brief  Debug monitor handler.
 */
void DebugMon_Handler(void)
{
}

/**
 * @brief  PendSV handler.
 */
void PendSV_Handler(void)
{
}

/**
 * @brief  SysTick handler – drives HAL_GetTick() millisecond counter.
 */
void SysTick_Handler(void)
{
    HAL_IncTick();
}

/* ------------------------------------------------------------------ */
/*  Peripheral interrupt handlers                                     */
/* ------------------------------------------------------------------ */

/**
 * @brief  DMA1 Stream1 global interrupt handler (USART3 RX).
 */
void DMA1_Stream1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart3_rx);
}
