/**
 * @file    startup_stm32f446xx.s
 * @brief   STM32F446RETx startup file (ARM Thumb assembly, GNU syntax).
 *
 * Sets up the vector table, initialises .data and .bss sections, then
 * calls SystemInit() and main().
 *
 * Memory layout for STM32F446RET6:
 *   Flash : 0x08000000 – 0x0807FFFF  (512 KB)
 *   SRAM  : 0x20000000 – 0x2001FFFF  (128 KB)
 */

  .syntax unified
  .cpu cortex-m4
  .fpu fpv4-sp-d16
  .thumb

/* ------------------------------------------------------------------ */
/*  Stack / heap sizes (can be overridden at link time)               */
/* ------------------------------------------------------------------ */
  .equ  Stack_Size, 0x400   /* 1 KB */
  .section .stack, "aw", %nobits
  .align 3
Stack_Mem:
  .space Stack_Size
__StackTop:

  .equ  Heap_Size, 0x200    /* 512 B */
  .section .heap, "aw", %nobits
  .align 3
Heap_Mem:
  .space Heap_Size

/* ------------------------------------------------------------------ */
/*  Vector table                                                       */
/* ------------------------------------------------------------------ */
  .section .isr_vector, "a", %progbits
  .align 2
  .global g_pfnVectors

g_pfnVectors:
  /* Core exception vectors */
  .word  __StackTop
  .word  Reset_Handler
  .word  NMI_Handler
  .word  HardFault_Handler
  .word  MemManage_Handler
  .word  BusFault_Handler
  .word  UsageFault_Handler
  .word  0
  .word  0
  .word  0
  .word  0
  .word  SVC_Handler
  .word  DebugMon_Handler
  .word  0
  .word  PendSV_Handler
  .word  SysTick_Handler
  /* STM32F446-specific IRQ vectors (IRQ0 – IRQ96) */
  .word  WWDG_IRQHandler
  .word  PVD_IRQHandler
  .word  TAMP_STAMP_IRQHandler
  .word  RTC_WKUP_IRQHandler
  .word  FLASH_IRQHandler
  .word  RCC_IRQHandler
  .word  EXTI0_IRQHandler
  .word  EXTI1_IRQHandler
  .word  EXTI2_IRQHandler
  .word  EXTI3_IRQHandler
  .word  EXTI4_IRQHandler
  .word  DMA1_Stream0_IRQHandler
  .word  DMA1_Stream1_IRQHandler       /* USART3 RX DMA */
  .word  DMA1_Stream2_IRQHandler
  .word  DMA1_Stream3_IRQHandler
  .word  DMA1_Stream4_IRQHandler
  .word  DMA1_Stream5_IRQHandler
  .word  DMA1_Stream6_IRQHandler
  .word  ADC_IRQHandler
  .word  CAN1_TX_IRQHandler
  .word  CAN1_RX0_IRQHandler
  .word  CAN1_RX1_IRQHandler
  .word  CAN1_SCE_IRQHandler
  .word  EXTI9_5_IRQHandler
  .word  TIM1_BRK_TIM9_IRQHandler
  .word  TIM1_UP_TIM10_IRQHandler
  .word  TIM1_TRG_COM_TIM11_IRQHandler
  .word  TIM1_CC_IRQHandler
  .word  TIM2_IRQHandler
  .word  TIM3_IRQHandler
  .word  TIM4_IRQHandler
  .word  I2C1_EV_IRQHandler
  .word  I2C1_ER_IRQHandler
  .word  I2C2_EV_IRQHandler
  .word  I2C2_ER_IRQHandler
  .word  SPI1_IRQHandler
  .word  SPI2_IRQHandler
  .word  USART1_IRQHandler
  .word  USART2_IRQHandler
  .word  USART3_IRQHandler
  .word  EXTI15_10_IRQHandler
  .word  RTC_Alarm_IRQHandler
  .word  OTG_FS_WKUP_IRQHandler
  .word  TIM8_BRK_TIM12_IRQHandler
  .word  TIM8_UP_TIM13_IRQHandler
  .word  TIM8_TRG_COM_TIM14_IRQHandler
  .word  TIM8_CC_IRQHandler
  .word  DMA1_Stream7_IRQHandler
  .word  FMC_IRQHandler
  .word  SDIO_IRQHandler
  .word  TIM5_IRQHandler
  .word  SPI3_IRQHandler
  .word  UART4_IRQHandler
  .word  UART5_IRQHandler
  .word  TIM6_DAC_IRQHandler
  .word  TIM7_IRQHandler
  .word  DMA2_Stream0_IRQHandler
  .word  DMA2_Stream1_IRQHandler
  .word  DMA2_Stream2_IRQHandler
  .word  DMA2_Stream3_IRQHandler
  .word  DMA2_Stream4_IRQHandler
  .word  0
  .word  0
  .word  CAN2_TX_IRQHandler
  .word  CAN2_RX0_IRQHandler
  .word  CAN2_RX1_IRQHandler
  .word  CAN2_SCE_IRQHandler
  .word  OTG_FS_IRQHandler
  .word  DMA2_Stream5_IRQHandler
  .word  DMA2_Stream6_IRQHandler
  .word  DMA2_Stream7_IRQHandler
  .word  USART6_IRQHandler
  .word  I2C3_EV_IRQHandler
  .word  I2C3_ER_IRQHandler
  .word  OTG_HS_EP1_OUT_IRQHandler
  .word  OTG_HS_EP1_IN_IRQHandler
  .word  OTG_HS_WKUP_IRQHandler
  .word  OTG_HS_IRQHandler
  .word  DCMI_IRQHandler
  .word  0
  .word  0
  .word  RNG_IRQHandler
  .word  FPU_IRQHandler
  .word  UART7_IRQHandler
  .word  UART8_IRQHandler
  .word  SPI4_IRQHandler
  .word  SPI5_IRQHandler
  .word  0
  .word  SAI1_IRQHandler
  .word  0
  .word  0
  .word  0
  .word  SAI2_IRQHandler
  .word  QUADSPI_IRQHandler
  .word  CEC_IRQHandler
  .word  SPDIF_RX_IRQHandler
  .word  FMPI2C1_EV_IRQHandler
  .word  FMPI2C1_ER_IRQHandler

/* ------------------------------------------------------------------ */
/*  Reset handler                                                      */
/* ------------------------------------------------------------------ */
  .section .text.Reset_Handler
  .weak    Reset_Handler
  .type    Reset_Handler, %function

Reset_Handler:
  /* Copy .data section from Flash to SRAM */
  ldr  r0, =_sdata
  ldr  r1, =_edata
  ldr  r2, =_sidata
  movs r3, #0
  b    LoopCopyDataInit

CopyDataInit:
  ldr  r4, [r2, r3]
  str  r4, [r0, r3]
  adds r3, r3, #4

LoopCopyDataInit:
  adds r4, r0, r3
  cmp  r4, r1
  bcc  CopyDataInit

  /* Zero-fill .bss section */
  ldr  r2, =_sbss
  ldr  r4, =_ebss
  movs r3, #0
  b    LoopFillZerobss

FillZerobss:
  str  r3, [r2]
  adds r2, r2, #4

LoopFillZerobss:
  cmp  r2, r4
  bcc  FillZerobss

  /* Call system init and then main */
  bl   SystemInit
  bl   main
  bx   lr

  .size Reset_Handler, .-Reset_Handler

/* ------------------------------------------------------------------ */
/*  Default weak IRQ handlers (spin loops; override in application)  */
/* ------------------------------------------------------------------ */
  .macro DEFAULT_HANDLER handler_name
  .weak  \handler_name
  .thumb_set \handler_name, Default_Handler
  .endm

  .section .text.Default_Handler, "ax", %progbits
Default_Handler:
  b  Default_Handler
  .size Default_Handler, .-Default_Handler

  DEFAULT_HANDLER WWDG_IRQHandler
  DEFAULT_HANDLER PVD_IRQHandler
  DEFAULT_HANDLER TAMP_STAMP_IRQHandler
  DEFAULT_HANDLER RTC_WKUP_IRQHandler
  DEFAULT_HANDLER FLASH_IRQHandler
  DEFAULT_HANDLER RCC_IRQHandler
  DEFAULT_HANDLER EXTI0_IRQHandler
  DEFAULT_HANDLER EXTI1_IRQHandler
  DEFAULT_HANDLER EXTI2_IRQHandler
  DEFAULT_HANDLER EXTI3_IRQHandler
  DEFAULT_HANDLER EXTI4_IRQHandler
  DEFAULT_HANDLER DMA1_Stream0_IRQHandler
  DEFAULT_HANDLER DMA1_Stream1_IRQHandler
  DEFAULT_HANDLER DMA1_Stream2_IRQHandler
  DEFAULT_HANDLER DMA1_Stream3_IRQHandler
  DEFAULT_HANDLER DMA1_Stream4_IRQHandler
  DEFAULT_HANDLER DMA1_Stream5_IRQHandler
  DEFAULT_HANDLER DMA1_Stream6_IRQHandler
  DEFAULT_HANDLER ADC_IRQHandler
  DEFAULT_HANDLER CAN1_TX_IRQHandler
  DEFAULT_HANDLER CAN1_RX0_IRQHandler
  DEFAULT_HANDLER CAN1_RX1_IRQHandler
  DEFAULT_HANDLER CAN1_SCE_IRQHandler
  DEFAULT_HANDLER EXTI9_5_IRQHandler
  DEFAULT_HANDLER TIM1_BRK_TIM9_IRQHandler
  DEFAULT_HANDLER TIM1_UP_TIM10_IRQHandler
  DEFAULT_HANDLER TIM1_TRG_COM_TIM11_IRQHandler
  DEFAULT_HANDLER TIM1_CC_IRQHandler
  DEFAULT_HANDLER TIM2_IRQHandler
  DEFAULT_HANDLER TIM3_IRQHandler
  DEFAULT_HANDLER TIM4_IRQHandler
  DEFAULT_HANDLER I2C1_EV_IRQHandler
  DEFAULT_HANDLER I2C1_ER_IRQHandler
  DEFAULT_HANDLER I2C2_EV_IRQHandler
  DEFAULT_HANDLER I2C2_ER_IRQHandler
  DEFAULT_HANDLER SPI1_IRQHandler
  DEFAULT_HANDLER SPI2_IRQHandler
  DEFAULT_HANDLER USART1_IRQHandler
  DEFAULT_HANDLER USART2_IRQHandler
  DEFAULT_HANDLER USART3_IRQHandler
  DEFAULT_HANDLER EXTI15_10_IRQHandler
  DEFAULT_HANDLER RTC_Alarm_IRQHandler
  DEFAULT_HANDLER OTG_FS_WKUP_IRQHandler
  DEFAULT_HANDLER TIM8_BRK_TIM12_IRQHandler
  DEFAULT_HANDLER TIM8_UP_TIM13_IRQHandler
  DEFAULT_HANDLER TIM8_TRG_COM_TIM14_IRQHandler
  DEFAULT_HANDLER TIM8_CC_IRQHandler
  DEFAULT_HANDLER DMA1_Stream7_IRQHandler
  DEFAULT_HANDLER FMC_IRQHandler
  DEFAULT_HANDLER SDIO_IRQHandler
  DEFAULT_HANDLER TIM5_IRQHandler
  DEFAULT_HANDLER SPI3_IRQHandler
  DEFAULT_HANDLER UART4_IRQHandler
  DEFAULT_HANDLER UART5_IRQHandler
  DEFAULT_HANDLER TIM6_DAC_IRQHandler
  DEFAULT_HANDLER TIM7_IRQHandler
  DEFAULT_HANDLER DMA2_Stream0_IRQHandler
  DEFAULT_HANDLER DMA2_Stream1_IRQHandler
  DEFAULT_HANDLER DMA2_Stream2_IRQHandler
  DEFAULT_HANDLER DMA2_Stream3_IRQHandler
  DEFAULT_HANDLER DMA2_Stream4_IRQHandler
  DEFAULT_HANDLER CAN2_TX_IRQHandler
  DEFAULT_HANDLER CAN2_RX0_IRQHandler
  DEFAULT_HANDLER CAN2_RX1_IRQHandler
  DEFAULT_HANDLER CAN2_SCE_IRQHandler
  DEFAULT_HANDLER OTG_FS_IRQHandler
  DEFAULT_HANDLER DMA2_Stream5_IRQHandler
  DEFAULT_HANDLER DMA2_Stream6_IRQHandler
  DEFAULT_HANDLER DMA2_Stream7_IRQHandler
  DEFAULT_HANDLER USART6_IRQHandler
  DEFAULT_HANDLER I2C3_EV_IRQHandler
  DEFAULT_HANDLER I2C3_ER_IRQHandler
  DEFAULT_HANDLER OTG_HS_EP1_OUT_IRQHandler
  DEFAULT_HANDLER OTG_HS_EP1_IN_IRQHandler
  DEFAULT_HANDLER OTG_HS_WKUP_IRQHandler
  DEFAULT_HANDLER OTG_HS_IRQHandler
  DEFAULT_HANDLER DCMI_IRQHandler
  DEFAULT_HANDLER RNG_IRQHandler
  DEFAULT_HANDLER FPU_IRQHandler
  DEFAULT_HANDLER UART7_IRQHandler
  DEFAULT_HANDLER UART8_IRQHandler
  DEFAULT_HANDLER SPI4_IRQHandler
  DEFAULT_HANDLER SPI5_IRQHandler
  DEFAULT_HANDLER SAI1_IRQHandler
  DEFAULT_HANDLER SAI2_IRQHandler
  DEFAULT_HANDLER QUADSPI_IRQHandler
  DEFAULT_HANDLER CEC_IRQHandler
  DEFAULT_HANDLER SPDIF_RX_IRQHandler
  DEFAULT_HANDLER FMPI2C1_EV_IRQHandler
  DEFAULT_HANDLER FMPI2C1_ER_IRQHandler

  .end
