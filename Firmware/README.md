[![PlatformIO CI](https://github.com/Spaceona/Washr/actions/workflows/PlatformIO_CI.yaml/badge.svg)](https://github.com/Spaceona/Washr/actions/workflows/PlatformIO_CI.yaml)

# About
Firmware for the Air Quality Monitoring board. Runs on an ESP32-C3 and reads the following sensors every 1.5–3 seconds, averaging the results before transmission:

| Sensor | Measurements |
|--------|-------------|
| Bosch BME688 | Temperature, Humidity, Pressure, VOC gas resistance |
| CH4 sensor (UART) | Methane (ppm) |
| H2S sensor (I2C via ADS1X15 ADC) | Hydrogen sulfide (ppm) |
| NO2 sensor (I2C via ADS1X15 ADC) | Nitrogen dioxide (ppm) |

Data is transmitted over LoRaWAN to [The Things Network](https://www.thethingsnetwork.org/) every 15 minutes. Uplinks are unconfirmed by default; a confirmed uplink (which solicits an ACK downlink from TTN) is sent at most every 10th transmission to stay within TTN's fair use policy of 10 downlink messages per 24 hours.

### Packet Format
All fields are little-endian integers transmitted over LoRaWAN port 1.

| Field | Type | Scale | Range | Unit |
|-------|------|-------|-------|------|
| `ch4` | `uint16` | ×1 | 0–65535 | ppm |
| `h2s` | `uint16` | ×100 | 0–5000 → 0.00–50.00 | ppm |
| `nox` | `uint16` | ×100 | 0–30000 → 0.00–300.00 | ppm |
| `voc_load` | `uint16` | ×1 | 0–65535 | (1/Ω)×10⁶×10 |
| `temp` | `int16` | ×100 | -4000–8500 → -40.00–85.00 | °C |
| `humidity` | `uint16` | ×100 | 0–10000 → 0.00–100.00 | %RH |
| `pressure` | `uint16` | ×10 | — | hPa |

---

# Contributing Guidelines

### Development Environment Setup
1. Install a C/C++ IDE which supports PlatformIO (e.g. [Visual Studio Code](https://code.visualstudio.com/), [CLion](https://www.jetbrains.com/clion/)).
2. Install [PlatformIO](https://platformio.org/install/) as an extension in your IDE.
3. Open the `Firmware` folder in your IDE.
4. Open the PlatformIO Home from the PlatformIO extension in your IDE.
5. Make sure to select the environment which corresponds to the board you are using (see **Build Environments** below).
6. Build the project using the PlatformIO build button.
7. Upload the project to the ESP32-C3 board using the PlatformIO upload button.
8. Open the serial monitor to view the output. (If using CLion, use the CLion Serial Monitor Plugin rather than the built-in upload-and-monitor button.)
9. Make changes to the code and test them on the board.
10. If you are adding new libraries, add them to `platformio.ini`.
11. Develop your changes in a separate Git branch.

### Build Environments

| Environment | Description |
|-------------|-------------|
| `lora` | Standard LoRaWAN build — use this for production |
| `test` | LoRaWAN build with zeroed sensor values (safe for RF testing without real sensors) |
| `long-range` | LoRaWAN build using SF10 for extended range |
| `long-range-debug` | SF10 build with confirmed uplinks on every TX (exceeds TTN fair use — bench only) |
| `wifi` | WiFi build (development/alternative transport) |

Select the environment from the PlatformIO toolbar before building and uploading.

### Code Style
TODO

### Pull Request Guidelines
Please write a brief description of what your code changed and why. Set yourself as the assignee and assign Robbie (leslier7) as the reviewer. Do not merge until Robbie has reviewed and approved.


# Configuration

### LoRa Configuration
To connect the board to The Things Network via LoRaWAN, you need a `keys.h` file containing your device credentials. **This file must never be committed to version control.**

1. Register your device in the [TTN Console](https://console.cloud.thethings.network/) using OTAA activation.
2. Create a new file named `keys.h` in the `include` directory.
3. Copy and paste the following template into `keys.h`:

```c
#include "stdint.h"
#ifndef KEYS_H
#define KEYS_H

// App EUI — 8 bytes, LSB first (from TTN console, "JoinEUI")
const uint8_t PROGMEM APPEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// Dev EUI — 8 bytes, LSB first (from TTN console, copy as "lsb" byte order)
const uint8_t PROGMEM DEVEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// App Key — 16 bytes, MSB first (from TTN console, copy as "msb" byte order)
const uint8_t PROGMEM APPKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

#endif
```

4. Replace each byte array with your actual device credentials from the TTN console.
   - **APPEUI / JoinEUI**: copy in **LSB** (least-significant byte first) order.
   - **DEVEUI**: copy in **LSB** order.
   - **APPKEY**: copy in **MSB** (most-significant byte first) order.

The firmware is configured for the **US915** region. To change region, edit the `CFG_*` build flag in `platformio.ini`.

TTN fair use limits confirmed uplinks to at most 10 per 24 hours. The firmware enforces this automatically — do not use the `long-range-debug` environment in the field.

### WiFi Configuration
In order to connect the board to WiFi, please follow the steps below:

1. Create a new file named `wifi_secrets.h` in the `include` directory.
2. Copy and paste the following code into the `wifi_secrets.h` file:

```c
#ifndef WIFI_SECRETS_H
#define WIFI_SECRETS_H

#define WIFI_SSID "your_ssid"
#define WIFI_PASSWORD "your_password"
#define CLIENT_NAME "your_client_name"
#define CLIENT_KEY "your_client_key"

#endif //WIFI_SECRETS_H
```

3. Replace `your_ssid` with your actual Wi-Fi network name (SSID).
4. Replace `your_password` with your actual Wi-Fi password (with the quotation marks still).
5. Replace `your_client_name` with your client name from the backend.
6. Replace `your_client_key` with your client key that was generated by the backend during onboarding.

Please ensure that you do not share your `wifi_secrets.h` file publicly or commit it to version control, as it contains sensitive information.
