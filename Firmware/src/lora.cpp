// Robbie Leslie
// April 2026

#include <Arduino.h>
#include <SPI.h>
#include "lora.h"

const lmic_pinmap lmic_pins = {
    .nss  = PIN_CS,               // IO10
    .rxtx = LMIC_UNUSED_PIN,      // not used for RFM95
    .rst  = LMIC_UNUSED_PIN,              // your reset pin, or LMIC_UNUSED_PIN
    .dio  = { PIN_DIO0, PIN_DIO1, LMIC_UNUSED_PIN },
};

static uint8_t readRegister(uint8_t address) {
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(PIN_CS, LOW);
    SPI.transfer(address & 0x7F);  // MSB=0 means read
    uint8_t value = SPI.transfer(0x00);
    digitalWrite(PIN_CS, HIGH);
    SPI.endTransaction();
    return value;
}

bool setup_lora(){
    pinMode(PIN_CS, OUTPUT);
    digitalWrite(PIN_CS, HIGH);

    SPI.begin(PIN_SCLK, PIN_MISO, PIN_MOSI, PIN_CS);

    uint8_t version = readRegister(0x42);
    Serial.printf("RFM95 version register: 0x%02X (expect 0x12)\n", version);

    return version == 0x12;
}