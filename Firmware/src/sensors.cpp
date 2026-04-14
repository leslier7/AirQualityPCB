// Robbie Leslie
// April 2026

#include "sensors.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1015 adc;

bool setup_ch4 = false;

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

// Send a command and wait for [AK] or [NA]. Returns true on ACK.
static bool inir_cmd(const char* cmd, uint32_t timeoutMs = 500) {
    while (Serial1.available()) Serial1.read();  // flush
    Serial1.print(cmd);

    String response = "";
    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        if (Serial1.available()) {
            char c = (char)Serial1.read();
            response += c;
            if (c == ']') break;
        }
    }
    return response.indexOf("AK") >= 0;
}

bool setup_methane(){
    // Datasheet Table 1: 38400 baud, 8 data bits, no parity, 2 stop bits
    Serial1.begin(38400, SERIAL_8N2, CH4_TX, CH4_RX);
    delay(100);

    // Initialization procedure (datasheet section 1.1.5):
    // Step 1: enter Configuration Mode
    Serial.println("INIR2: [C] config mode...");
    if (!inir_cmd("[C]")) {
        Serial.println("INIR2: no ACK to [C]");
        return false;
    }

    // Step 2: read back settings for operation check, then flush response
    Serial1.print("[I]");
    delay(300);
    while (Serial1.available()) Serial1.read();

    // Step 3: enter Engineering Mode — sensor starts streaming every 1s
    Serial.println("INIR2: [B] engineering mode...");
    if (!inir_cmd("[B]")) {
        Serial.println("INIR2: no ACK to [B]");
        return false;
    }
    Serial.println("INIR2: streaming started (45s warmup)");
}

bool setup_sensors(){
    setup_methane;

    return adc.begin(0x49);
}

int read_methane_adc(){
    return adc.readADC_SingleEnded(0);
}

int read_methane_uart() {
    // Sensor streams: [0xGGGGGGGG 0xFFFFFFFF 0xTTTTTTTT 0xCCCCCCCC 0xXXXXXXXX]
    // First hex value is concentration in PPM (e.g. 500 PPM = 0x000001F4)

    if (!Serial1.available()) return -1;

    // Find '[' packet start
    uint32_t start = millis();
    while (millis() - start < 1500) {
        if (Serial1.available() && Serial1.read() == '[') break;
    }
    if (millis() - start >= 1500) return -1;

    // Read until ']'
    String packet = "";
    start = millis();
    while (millis() - start < 1500) {
        if (Serial1.available()) {
            char c = (char)Serial1.read();
            if (c == ']') break;
            packet += c;
        }
    }

    // Parse first "0x..." value — concentration in PPM
    int idx = packet.indexOf("0x");
    if (idx < 0) return -1;
    return (int)strtol(packet.c_str() + idx + 2, nullptr, 16);
}