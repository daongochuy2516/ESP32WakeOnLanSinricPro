# ESP32 SinricPro Wake-on-LAN Device (Haven't done yet.)

A smart Wake-on-LAN (WOL) device using ESP32 with SinricPro integration for remote control or physical button activation.

![Version](https://img.shields.io/badge/version-3.4.0-blue)
![License](https://img.shields.io/badge/license-MIT-green)

## Features

- **SinricPro Integration**: Control your PC power state remotely through the SinricPro platform
- **Physical Button Support**: Send WOL packets with a simple button press
- **Web Configuration Portal**: Easy setup via WiFi access point
- **Visual/Audio Feedback**: Customizable RGB LED indicators and buzzer notifications
- **OTA Updates**: Seamless firmware upgrades over-the-air
- **Flexible WOL Modes**: Choose between SinricPro control, physical button, or both
- **Persistent Settings**: Configuration stored in non-volatile memory

## Hardware Requirements

- ESP32 development board
- RGB LED (connected to pins 27, 26, 25)
- Buzzer (connected to pin 33)
- Push button (connected to pin 19)
- Relay (optional, connected to pin 2)

## Installation

1. Clone this repository:
   ```
   git clone https://github.com/yourusername/esp32-sinricpro-wol.git
   ```
2. Open the project in Arduino IDE or PlatformIO
3. Install the required libraries:
   - [SinricPro](https://github.com/sinricpro/esp8266-esp32-sdk)
   - [Arduino-WakeOnLan](https://github.com/a7md0/WakeOnLan)
   - Preferences (built-in ESP32 library)
4. Modify the configuration parameters in the code if needed
5. Upload the firmware to your ESP32

## Setup Instructions

### Initial Configuration

1. Power on your ESP32 device
2. Press and hold the BOOT button for 5+ seconds to enter AP Mode
3. Connect to the "ESP32-SinricProWol" WiFi network (password: 12345678)
4. Open a web browser and navigate to the configuration page (typically 192.168.4.1)
5. Enter your:
   - WiFi credentials (2.4GHz networks only)
   - SinricPro APP_KEY and APP_SECRET (from SinricPro console)
   - SinricPro DEVICE_ID (from SinricPro console)
   - PC MAC address to wake (format: AA:BB:CC:DD:EE:FF)
   - Configure WOL mode (SinricPro only, button only, or both)
   - Toggle LED and buzzer effects

### SinricPro Setup

1. Create an account at [SinricPro](https://sinric.pro)
2. Add a new device (type: Switch)
3. Copy the generated APP_KEY, APP_SECRET, and DEVICE_ID to the device configuration

## Usage

- **SinricPro Control**: Use the SinricPro app or integrations to turn on your device
- **Button Control**: Short press the button to send a WOL packet
- **Configuration Mode**: Long press (5+ seconds) to enter AP Mode

### Visual Feedback

| LED Status | Meaning |
|------------|---------|
| Blue | AP Mode active |
| Red Blinking | WiFi connection issue |
| Red Solid | Sending WOL packet |
| Green | Operation successful |
| Rainbow Effect | Device idle (when enabled) |

## Troubleshooting

- If device doesn't connect to WiFi, check your credentials in AP Mode
- Ensure your SinricPro keys are correct
- Verify the MAC address format is correct (AA:BB:CC:DD:EE:FF)
- Check that your PC has Wake-on-LAN enabled in BIOS and network adapter settings

## Screenshots

<div align="center">
  <img src="assets/config-page.png" alt="Configuration Page" width="400"/>
</div>

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is open source under the MIT license - see the [LICENSE](LICENSE) file for details.

## Credits

Developed by Dao Ngoc Huy
