// Robbie Leslie
// April 2026

#include "sensors.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1015 adc;

void testPortI2C(TwoWire &i2c) {
    Serial.printf("Scanning... time (ms): %d\n", millis());
    uint8_t detected = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        i2c.beginTransmission(addr);
        uint8_t retval = i2c.endTransmission();
        if (retval == 0) {
            Serial.printf("\t0x%02X detected\n", addr);
            detected++;
        }
    }
    if (!detected) {
        Serial.printf("\tNo device detected!\n");
    }
    Serial.println();
}

bool setup_I2C(){
    return Wire.begin(SDA_PIN, SCL_PIN);
}

bool setup_sensors(){
    return adc.begin(0x49);
}

int read_methane(){
    return adc.readADC_SingleEnded(0);
}