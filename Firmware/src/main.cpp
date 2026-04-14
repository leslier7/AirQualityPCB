// Air Quality Monitoring
// Robbie Leslie April 2026

// Imports
#include <Arduino.h>
#include <Wire.h>
#include "lora.h"

#define LED 8

#define SDA_PIN 4
#define SCL_PIN 5


pkt_fmt myPkt;

static uint32_t lastTx = 0;

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

    myPkt.intNum = 0;
    myPkt.floatNum = 3.141592654;
    myPkt.digitalTemp = 35;
    myPkt.thermistorTemp = 25;

    uint8_t myCString[6] = "Hello";
    memcpy(myPkt.myString, myCString, 6);
}

void setup() {

    // pinMode(LED, OUTPUT);
    // digitalWrite(LED, HIGH);
    Serial.begin(115200);
    delay(500);
    Serial.println("Air Quality Monitor starting");

    Wire.begin(SDA_PIN, SCL_PIN);

    testPortI2C(Wire);

    if (!setup_lora())
        Serial.println("LoRaWAN init failed");

    // queue first transmission — library handles join first
    send_lora(myPkt);
    lastTx = millis();
}

void loop() {
    myLoRaWAN.loop();

    //testPortI2C(Wire);
    if (millis() - lastTx > 60000) {
        if (LMIC.devaddr != 0) {
            send_lora(myPkt);
            lastTx = millis();
        }
    }
}
