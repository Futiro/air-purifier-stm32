# air-purifier-stm32

Embedded air purifier firmware for the **STM32F446RET6** microcontroller with
real-time AQI monitoring, DHT22 temperature/humidity sensing, HEPA filtration
control, and SSD1306 OLED display output.

---

## Features

| Feature | Detail |
|---|---|
| **MCU** | STM32F446RET6 (Cortex-M4, 180 MHz, 512 KB Flash, 128 KB SRAM) |
| **PM sensor** | Plantower PMS5003 via UART3 + DMA (PM1.0 / PM2.5 / PM10 µg/m³) |
| **AQI** | US EPA standard breakpoint interpolation (0-500, six categories) |
| **Temp/humidity** | DHT22 via single-wire bit-bang on PA0 (TIM2 µs timing) |
| **HEPA fan** | 25 kHz PWM on TIM3_CH1 / PA6; speed auto-adjusted per AQI category |
| **Display** | SSD1306 128x64 OLED via I2C1 / PB6-PB7; framebuffer driver with 5x7 font |
| **Status LED** | PC13 toggles every 2 s as heartbeat |

---

## Hardware Wiring

```
STM32F446RET6        Peripheral
-----------------    ------------------------------------------
PB10 (UART3 TX)  ->  PMS5003 RX
PB11 (UART3 RX)  <-  PMS5003 TX
PA0  (GPIO OD)   <-> DHT22 data  (4.7 kOhm pull-up to 3.3 V)
PB6  (I2C1 SCL)  ->  SSD1306 SCL
PB7  (I2C1 SDA)  <-> SSD1306 SDA
PA6  (TIM3 CH1)  ->  HEPA fan PWM input (4-wire fan)
PC13 (GPIO out)  ->  Status LED (active-low, Nucleo-64 on-board)
```

---

## Project Structure

```
air-purifier-stm32/
├── Inc/
│   ├── main.h                  # Pin definitions, peripheral handles
│   ├── aqi.h                   # AQI driver API
│   ├── dht22.h                 # DHT22 driver API
│   ├── oled.h                  # SSD1306 OLED driver API
│   ├── hepa_fan.h              # HEPA fan PWM control API
│   ├── stm32f4xx_hal_conf.h    # HAL module selection
│   └── stm32f4xx_it.h          # ISR prototypes
├── Src/
│   ├── main.c                  # System init, peripheral config, main loop
│   ├── aqi.c                   # PMS5003 frame parser + US EPA AQI calc
│   ├── dht22.c                 # DHT22 single-wire bit-bang driver
│   ├── oled.c                  # SSD1306 framebuffer + I2C driver
│   ├── hepa_fan.c              # TIM3 PWM fan speed control
│   └── stm32f4xx_it.c          # Exception / IRQ handlers
├── STM32F446RETx/
│   ├── startup_stm32f446xx.s   # Cortex-M4 startup (vector table, CRT init)
│   └── STM32F446RETx_FLASH.ld  # Linker script (512 KB Flash / 128 KB SRAM)
├── tests/
│   ├── test_aqi.c              # Host-native unit tests (55 cases)
│   └── Makefile                # Build/run tests with gcc
├── Makefile                    # Cross-compile with arm-none-eabi-gcc
└── README.md
```

---

## OLED Display Layout

```
+------------------------------+
| AQI:  42 Good                |  <- AQI value + category
| PM2.5:  12 PM10:  18         |  <- raw PM concentrations (ug/m3)
| Temp:  23.5 C                |  <- DHT22 temperature
| RH:    58.0 %                |  <- DHT22 relative humidity
| Fan:    40 %                 |  <- current fan duty cycle
| [####....................]   |  <- fan speed progress bar
|                              |
| PM1.0:   8 ug/m3             |  <- PM1.0 concentration
+------------------------------+
```

---

## AQI to Fan Speed Mapping

| AQI Range | Category | Fan Speed |
|-----------|----------|-----------|
| 0 - 50 | Good | 20 % |
| 51 - 100 | Moderate | 40 % |
| 101 - 150 | Unhealthy for Sensitive Groups | 60 % |
| 151 - 200 | Unhealthy | 80 % |
| 201 - 300 | Very Unhealthy | 90 % |
| 301 - 500 | Hazardous | 100 % |

The minimum non-zero duty cycle is **20 %** to prevent motor stall.
Setting speed to **0 %** stops the fan completely.

---

## Building

### Prerequisites

- [GNU Arm Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm) (`arm-none-eabi-gcc`)
- [STM32CubeF4](https://github.com/STMicroelectronics/STM32CubeF4) HAL drivers

### Compile

```bash
# Default: looks for STM32CubeF4 under ~/STM32Cube/Repository/...
make

# Override HAL root if needed
make CUBE_ROOT=/path/to/STM32Cube_FW_F4_V1.28.0
```

The build artifacts are placed in `build/`:

| File | Description |
|------|-------------|
| `air-purifier-stm32.elf` | ELF with debug symbols |
| `air-purifier-stm32.hex` | Intel HEX for flashing |
| `air-purifier-stm32.bin` | Raw binary |

### Flash via OpenOCD and ST-Link

```bash
make flash
```

---

## Running the Unit Tests

The `tests/` directory contains host-native C tests (no hardware required)
that validate the AQI calculation, AQI category mapping, fan speed logic,
PMS5003 frame checksum, and DHT22 data decoding.

```bash
cd tests
make test
# Results: 55 passed, 0 failed
```

---

## System Clock

The firmware configures the STM32F446RET6 for **180 MHz** operation:

```
HSE (8 MHz) -> PLL (M=8, N=360, P=2) -> SYSCLK = 180 MHz
AHB  = 180 MHz  (divider /1)
APB1 =  45 MHz  (divider /4)  -> TIM2/3 clock = 90 MHz
APB2 =  90 MHz  (divider /2)
```
