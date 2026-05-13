# Air Quality PCB

A custom air quality monitoring device with an ESP32-C3 microcontroller, designed to measure environmental sensor data and transmit it over WiFi and LoRa.

## Repository Structure

```
AirQualityPCB/
├── PCB/        # KiCad schematic, layout, BOM, and fabrication outputs
└── Firmware/   # PlatformIO firmware for the ESP32-C3
```

## PCB

The PCB was designed in KiCad and lives in the [PCB/](PCB/) folder. This folder contains the schematic, board layout, and a custom component library (3D models, footprints, and symbols).

The BOM, STEP file, and Gerbers can be found on the [GitHub Releases](https://github.com/leslier7/AirQualityPCB/releases) page. They can also be extracted from the KiCAD project.

## Firmware

The firmware is written in C++ using the PlatformIO build system and targets the ESP32-C3.

For instructions on setting up the development environment, building, and flashing the firmware, see [Firmware/README.md](Firmware/README.md).
