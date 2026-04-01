# Air Purifier with Real-Time AQI Monitoring
**Platform:** STM32F446RET6 | **Language:** Embedded C

## Overview
A standalone embedded air purifier system that continuously monitors air quality, temperature, and humidity — and automatically controls filtration based on real-time sensor data. Built entirely from scratch including hardware assembly and firmware.

## Features
- Real-time AQI monitoring via MQ135 gas sensor (CO2, smoke, VOCs)
- Temperature and humidity sensing via DHT22
- Automatic dual-fan speed control based on air quality thresholds
- HEPA filter integration for particulate removal
- OLED display showing live AQI, temperature, and humidity readings
- ADC-based analog-to-digital conversion for sensor data processing

## Hardware Components
| Component | Role |
|---|---|
| STM32F446RET6 | Main microcontroller |
| MQ135 | Air quality sensor (gas detection) |
| DHT22 | Temperature & humidity sensor |
| HEPA Filter | Particulate matter filtration |
| Dual DC Fans | Airflow and filtration control |
| OLED Display | Real-time data output |

## How It Works
The STM32 reads analog output from the MQ135 via its onboard ADC and digital data from the DHT22. Based on AQI thresholds, it adjusts fan speed using PWM. All live readings are rendered on the OLED display. The system runs autonomously with no external host required.

## Status
Hardware build complete. Firmware functional. Photos coming soon.
