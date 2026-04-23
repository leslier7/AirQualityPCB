// Robbie Leslie
// April 2026

#include <Arduino.h>

//Firmware version
String FIRMWARE_VERSION = "1-0-0";
//TODO fix this for double digit values
const int FIRMWARE_VERSION_MAJOR = FIRMWARE_VERSION.substring(0, 1).toInt();
const int FIRMWARE_VERSION_MINOR = FIRMWARE_VERSION.substring(2, 3).toInt();
const int FIRMWARE_VERSION_PATCH = FIRMWARE_VERSION.substring(4, 5).toInt();

uint8_t device_id = 0;

String server_name = "";