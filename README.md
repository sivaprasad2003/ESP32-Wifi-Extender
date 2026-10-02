# 📶 ESP32 Pro NAT Wi-Fi Extender

A lightweight, fully functional Wi-Fi Extender (Repeater) for the ESP32 built entirely in the Arduino IDE. 

Unlike standard "bridges" that break DHCP, this project uses actual **NAT (Network Address Translation)** and IP forwarding to create a dedicated, routed sub-network. It features a beautiful dark-mode web dashboard, NVS memory storage, and automatic subnet conflict resolution.

## ✨ Features
* **True NAT Routing:** Creates a completely separate sub-network and routes traffic securely to the internet.
* **Pro Web Dashboard:** Beautiful, responsive, dark-mode UI to monitor WAN IP, Signal Strength (RSSI), Connected Clients, and Uptime.
* **Zero External Dependencies:** Built using only native ESP32 Arduino libraries. No `ArduinoJson` or external web server libraries required!
* **Auto-Subnet Conflict Prevention:** Automatically broadcasts on `192.168.14.x` to prevent routing loops with common home routers.
* **DNS Auto-Forwarding:** Injects Google DNS (`8.8.8.8`) directly into connected clients via DHCP to save ESP32 RAM.
* **High Stability:** Wi-Fi Power Saving is explicitly disabled to prevent dropped packets, and CPU frequency is locked to 240MHz for maximum routing performance.

## 🛠️ Hardware Requirements
* **ESP32 Development Board** (e.g., ESP32 DevKit V1, NodeMCU-32S)
* Micro-USB / USB-C cable
* *Note: ESP8266 is NOT supported as it lacks hardware NAT capabilities.*

## 🚀 Installation & Setup

1. **Install Arduino IDE** and add the [ESP32 Board Manager URL](https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json).
2. Select your board (e.g., `DOIT ESP32 DEVKIT V1`).
3. Copy the provided `ESP32_NAT_Extender.ino` code into a new sketch.
4. Connect your ESP32 and click **Upload**. *(If the console gets stuck at "Connecting...", hold the `BOOT` button on your ESP32 for 2 seconds).*

## 📱 How to Use

1. **Connect:** Once flashed, use your phone or laptop to connect to the new Wi-Fi network:
   * **SSID:** `ESP32_EXTENDER`
   * **Password:** `SecurePassword123`
2. **Access Dashboard:** Open a web browser and navigate to **`http://192.168.14.1`**.
3. **Configure:** Enter your actual Home Wi-Fi credentials in the "Upstream Router" section. You can also customize the Extender's network name and password here.
4. **Save & Reboot:** Click the button. The ESP32 will reboot, connect to your home internet, activate NAT, and begin routing internet to your devices!

## ⚡ Performance Expectations (The "5 Mbps" Limit)

You may notice that even if you have a 100 Mbps home internet connection, devices connected to the ESP32 will max out around **4 to 8 Mbps**. **This is normal and expected.**

**Why? The "Walkie-Talkie" Effect:**
The ESP32 only has **one** 2.4GHz Wi-Fi radio. To act as a repeater, it must rapidly time-slice its radio: listen to your phone, pause, switch to the router, transmit, wait for the reply, switch back to the phone, and transmit. This halves the bandwidth immediately. Combined with the heavy CPU math required for NAT packet translation, 5 Mbps is the physical hardware limit of the ESP32 chip.

**Best Use Cases:**
Because of this limit, this project is not for 4K video streaming. It is absolutely perfect for:
* Extending Wi-Fi to low-bandwidth **IoT devices** (smart plugs, security cameras, sensors) in garages or gardens.
* Acting as a **Travel Router** to bypass hotel Wi-Fi device limits.
* Providing basic web-browsing and messaging access in household "dead zones".

## 🐛 Troubleshooting

* **No Internet on Phone?** Ensure your home router isn't blocking the ESP32. Check the Web Dashboard to ensure the "WAN IP" has successfully populated. 
* **Dashboard won't load?** Ensure you are connected *only* to the ESP32 network. Turn off mobile cellular data while configuring.
* **Compilation Errors?** This code is designed to auto-detect and compile safely on both ESP32 Arduino Core `v2.x` and the newer `v3.x`. Ensure your ESP32 board definitions are up to date in the Arduino Boards Manager.

## 📄 License
MIT License. Free to use, modify, and distribute for your own amazing DIY networking projects!
