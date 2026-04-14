// Air Quality Monitoring
// Robbie Leslie April 2026

// Imports
#include <Arduino.h>
#include <Wire.h>
#include "lora.h"

#define LED 8

#define SDA_PIN 4  
#define SCL_PIN 5 




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

void setup() {

    // pinMode(LED, OUTPUT);
    // digitalWrite(LED, HIGH);
    Serial.begin(115200);
    delay(500);
    Serial.println("Hello world!");

    Wire.begin(SDA_PIN, SCL_PIN);

    if (setup_lora())
        Serial.println("SPI OK - RFM95 found");
    else
        Serial.println("SPI FAIL - check wiring");
}



void loop() {
    //testPortI2C(Wire);
    digitalWrite(LED, LOW);
    delay(1000);
    digitalWrite(LED, HIGH);
    delay(1000);
}
