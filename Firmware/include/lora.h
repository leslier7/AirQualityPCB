// Robbie Leslie
// April 2026

#ifndef LORA_H
#define LORA_H

#define PIN_SCLK  3
#define PIN_MOSI  6
#define PIN_MISO  7
#define PIN_CS    10
#define PIN_DIO0  21
#define PIN_DIO1  20  

#include <arduino_lmic.h>
#include <hal/hal.h>

extern const lmic_pinmap lmic_pins;

bool setup_lora();

#endif