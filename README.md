# Self-Balancing Robot V1

<img width="300" alt="SELF BALANCING NEWEST NOW v42" src="https://github.com/user-attachments/assets/31e3edfb-8ad2-4ecb-965e-198cb3b56d62" />
<img width="300" alt="photo_2025-09-23_17-21-45" src="https://github.com/user-attachments/assets/d308d0ab-ab1d-402c-942a-c00db6d4f908" />
<img width="300" alt="photo_2025-09-23_17-21-36" src="https://github.com/user-attachments/assets/f1dfc15c-3fa4-4129-be33-5d158e9775c9" />

CAD files and PCB design for a two-wheeled self-balancing robot, built and competition-tested within a **two-week timeframe**. **1st Place** — Robotics Club Self-Balancing Robot Competition 2024.

---

## Overview

A modular self-balancing robot designed in Fusion 360, featuring a custom PCB and a PID control loop for real-time dynamic balancing. The full build pipeline — mechanical design, electronics integration, PCB layout, and firmware tuning — was completed independently.

---

## ⚙️ Hardware

| Component | Details |
|---|---|
| Microcontroller | ESP32 |
| IMU Sensor | MPU6050 (accelerometer + gyroscope) |
| Motors | Cytron SPG30E-GR30 12V 37mm DC Geared Motors with Encoders |
| Encoder Resolution | 16× GR |
| Motor Driver | 24V Dual Motor Driver |
| Power | 7.4V LiPo (2S) |
| Chassis | Modular, fully 3D printed — designed in Fusion 360 |

---

## Firmware

- **PID control loop** for real-time balancing using Kalman-filtered IMU data
- **Encoder feedback** on both motors (D13/D14 left, D15/D27 right) for closed-loop speed control
- PID interval tuned to **5ms** for responsive balancing
- WiFi-capable via ESP32 for remote monitoring and OTA updates

---

## Result

**1st Place** — Robotics Club Self-Balancing Robot Competition, 2024
Designed, built, and tuned independently within a two-week timeframe.

---

*Part of an ongoing robotics portfolio alongside SAFMC drone competition builds.*
