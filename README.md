# ⚡ ESP32 Smart Energy Meter 🔌

![Platform](https://img.shields.io/badge/platform-ESP32-blue.svg)  
![Status](https://img.shields.io/badge/status-In%20Progress-yellow)  
![License](https://img.shields.io/badge/license-MIT-green.svg)  

An energy monitoring system using **ESP32**, **INA219** current sensor, and a **0.96" OLED display** to measure and visualize power consumption of connected loads in real-time. This project also logs hourly energy usage and estimates monthly billing based on slab rates.

---

## 📸 Preview

> 🔧 Circuit Preview  
![Circuit Diagram](./assets/circuit.png) <!-- Replace with your actual image path -->

---

## 🧰 Components Used

| Component              | Description                         |
|------------------------|-------------------------------------|
| 🧠 ESP32 Dev Board     | Microcontroller with Wi-Fi & BT     |
| ⚡ INA219 Sensor        | Current, Voltage & Power Monitoring |
| 📺 0.96" OLED Display   | SPI/I2C Display for Output          |
| 🔋 Battery & Load       | Power source and load connections   |
| 🔌 Jumper Wires         | Circuit Connections                 |
| 📏 750Ω Resistors       | Current limiting for LEDs           |
| 💡 LEDs                | Simulated Load                      |
| 🧱 Mini Breadboard      | Prototyping Platform                |

---

## 🔗 Circuit Connections

### 🔹 INA219 to ESP32 (I2C)
| INA219 Pin | ESP32 Pin |
|------------|-----------|
| VCC        | 3.3V      |
| GND        | GND       |
| SDA        | GPIO 21   |
| SCL        | GPIO 22   |

### 🔹 OLED Display to ESP32 (SPI)
| OLED Pin | ESP32 Pin |
|----------|-----------|
| VCC      | 3.3V      |
| GND      | GND       |
| SCL      | GPIO 18   |
| SDA      | GPIO 23   |
| RES      | GPIO 16   |
| DC       | GPIO 17   |
| CS       | GPIO 5    |

### 🔹 Load Circuit (LEDs)
- All LEDs are connected in **parallel** with 750Ω resistors.
- Load powered by a separate **battery**.
- INA219’s **Vin+** connected to battery +ve.
- **Vin-** connected to load +ve.

---

## 🧠 Features

- 📈 Real-time Voltage, Current & Power monitoring
- ⏱️ Logs hourly energy consumption in **Watt-hours**
- 🧮 Daily average power usage estimation
- 💰 Predicts **monthly billing** based on slab rates
- 📟 OLED shows real-time values: V, A, W, Wh
- 🌐 (Optional) Send data to Google Sheets / MQTT

---

## 💻 Installation

1. **Clone the Repository**
   ```bash
   git clone https://github.com/yourusername/esp32-energy-meter.git
   cd esp32-energy-meter
