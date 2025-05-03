# ESP32 SinricPro Wake-on-LAN Device

A smart IoT device that lets you remotely power on your PC using Wake-on-LAN technology, with seamless integration for Google Home and Amazon Alexa via SinricPro.

![Version](https://img.shields.io/badge/version-3.6.2-blue)
![License](https://img.shields.io/badge/license-MIT-green)

## 🌟 Features

- **Smart Home Integration**: Control your PC with voice commands through Google Home and Amazon Alexa
- **Remote Power-On**: Wake your PC from anywhere as long as your ESP32 is on the same network as the PC
- **Easy Setup**: Web configuration interface - no code modification needed
- **Physical Control**: Optional physical button to manually wake your PC
- **Status Indicators**: RGB LED effects to indicate system status
- **Audible Feedback**: Optional buzzer notifications
- **Configurable Modes**: Choose between SinricPro control, physical button, or both

## 📋 Requirements

- ESP32 development board
- RGB LED (common anode/cathode)
- Buzzer (optional)
- Momentary push button
- PC with Wake-on-LAN capability (most laptops not supported)
- SinricPro account

## 🔌 Hardware Connections

Default pin configuration:
- **LED_RGB**: GPIO 27 (Red), GPIO 26 (Green), GPIO 25 (Blue)
- **BUZZER**: GPIO 33
- **PHY_BTN**: GPIO 19 (with internal pull-up)

## 🛠️ Installation

### 1. Upload the firmware

Upload the code to your ESP32 using Arduino IDE or PlatformIO.

### 2. Initial Configuration

- Press and hold the button for more than 5 seconds — the device will enter AP mode.
1. Connect to the Wi-Fi network named `ESP32-SinricProWol` with password `12345678`
2. Open a web browser and navigate to http://192.168.4.1
3. Fill in the required configuration:
   - Wi-Fi credentials (2.4GHz networks only)
   - SinricPro details (App Key, App Secret, Device ID)
   - PC MAC address
   - Preferred operation mode and options
4. Click "Save" and the device will restart

### 3. SinricPro Setup

1. Create an account at [SinricPro](https://sinric.pro)
2. Add a new device (type: Switch)
3. Copy the App Key, App Secret, and Device ID to the ESP32 configuration
4. Link your SinricPro account with Google Home or Amazon Alexa

## 💻 PC Configuration

To enable Wake-on-LAN on your PC:
- Google it — it's free.

## 🎮 Usage

### Voice Commands

- "Hey Google, turn on [device name]" or "Alexa, turn on [device name]"

### Physical Control

- **Short press** the button to send a Wake-on-LAN packet
- **Long press** (5+ seconds) to enter configuration mode

## 🔄 Configuration Mode

You can re-enter configuration mode anytime by:
1. Long-pressing the physical button for more than 5 seconds
2. Connecting to `ESP32-SinricProWol` Wi-Fi network
3. Navigating to http://192.168.4.1 in your browser

## 📝 Operation Modes

- **Allow Both**: Control via SinricPro (voice commands) and physical button
- **SinricPro Only**: Control via SinricPro only
- **Physical Button Only**: Control via physical button only

## 🚨 Troubleshooting

### LED Indicators

- **Blinking Red**: Wi-Fi connection issue
- **Solid Red**: Sending WOL packet
- **Solid Green**: Connected successfully
- **Solid Blue**: In configuration mode or performing OTA update
- **Rainbow Effect**: Normal operation (when RGB LED is enabled)

### Common Issues

- **PC won't wake up**: Check BIOS and OS WOL settings, verify MAC address
- **Can't connect to Wi-Fi**: Ensure you're using a 2.4GHz network
- **Voice commands not working**: Check SinricPro account linking with Google Home/Alexa

## 🔄 Updates

The device supports OTA (Over-The-Air) updates through the SinricPro platform, but only use this if you know what you're doing!

## ⚙️ Advanced Configuration

You can customize the following parameters in the code:
- PIN assignments
- Buzzer duration
- Logic code (only do this if you know what you're doing)

## 📜 License

This project is licensed under the MIT License — free to use and modify.  
Please keep the original credit and respect the author's effort. Open source is built on trust!

## 🙏 Credits

- Created by [daongochuy2516](https://github.com/daongochuy2516)
- Uses [SinricPro](https://github.com/sinricpro/esp8266-esp32-sdk) library for IoT integration
- Uses [WakeOnLan](https://github.com/a7md0/WakeOnLan) library

## 📌 Notes

- This project only works with PCs that support Wake-on-LAN functionality
- Most laptops do not support Wake-on-LAN
- The PC must be connected via Ethernet cable with Wake-on-LAN properly configured
