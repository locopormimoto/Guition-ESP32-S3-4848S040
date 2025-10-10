# Guition-ESP32-S3-4848S040

<p align="center">
    <img alt="Static Badge" src="https://img.shields.io/badge/version-v1.0%20Beta-green">
    <img alt="Static Badge" src="https://img.shields.io/badge/ESP IDF min version-4.1.0-red">
</p>

## 📋 Overview
This repo contains sample projects for using the Guition ESP32-S3-4848S040 smart display with the ESP-IDF framework.

### 📟 Core Specifications
- Microcontroller: ESP32-S3 dual-core (Xtensa LX7, Wi-Fi + Bluetooth 5.0)
- CPU Frequency: up to 240 MHz
- Internal Memory: 512 KiB SRAM + 384 KiB ROM
- External Flash: 16 MB (QIO mode)
- External PSRAM: 8 MB (OPI mode)
- USB Interface: USB-C port (with CH340 USB-to-UART bridge)

### 🖥️ Display and Touch
- Display Size: 4.0-inch IPS TFT panel
- Resolution: 480 × 480 pixels
- Color Depth: 16-bit color (65 K colors)
- Display Driver: ST7701 (ST7701S controller)
- Touch Controller: GT911 (capacitive touch)
- Touch Interface: I²C
- Viewing Angle: Wide viewing IPS screen

## </> How to Build
The project can be built by Visual Studio Code. ESP-IDF plugin must be installed on Visual Studio Code. Follow the [Installation Guide](https://github.com/espressif/vscode-esp-idf-extension/#quick-installation-guide) to install the ESP-IDF plugin.
