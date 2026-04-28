// Robbie Leslie
// April 2026

#include "sensors.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <bme68xLibrary.h>
#include <HardwareSerial.h>

// SPEC sensor calibration (sensitivity in nA/ppm)
#define H2S_SENSITIVITY_NA_PER_PPM   212.0f   // typical for 110-303, replace per-unit
#define NO2_SENSITIVITY_NA_PER_PPM   -30.0f   // typical for 110-507, NEGATIVE — replace per-unit

// Transimpedance gain resistors (from schematic)
#define H2S_TIA_GAIN_OHMS   100000.0f   // R20
#define NO2_TIA_GAIN_OHMS   6800000.0f  // R19

#define DAC_VOLTS 3.3f

#define CH4_RANGE 100

// ADC reference and bias
#define VREF_VOLTS          1.65f       // half-supply bias from R17/R18 divider

// ADS1015 in default ±2.048V FSR mode: 1 LSB = 1.0 mV (12-bit signed, 11-bit + sign)
#define ADC_LSB_VOLTS       0.001f

HardwareSerial CH4Serial(1);

Adafruit_ADS1015 adc;
Bme68x bme;

bool setup_ch4 = false;

// Latest BME688 readings — updated by update_BME(), read by get_BME_reading()
static float bme_temperature = 0.0f;
static float bme_humidity    = 0.0f;
static float bme_pressure    = 0.0f;
static float bme_raw_gas     = 0.0f;

static bool bme_ready = false;

// --------------------------------------------------------------------------
// Public sensor functions
// --------------------------------------------------------------------------

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

bool setup_BME() {
    bme.begin(0x77, Wire);
    if (bme.checkStatus() != BME68X_OK) {
        Serial.printf("BME688: init failed (status %d)\n", bme.checkStatus());
        return false;
    }

    // Oversampling: temp x2, pressure x16, humidity x1
    bme.setTPH(BME68X_OS_2X, BME68X_OS_16X, BME68X_OS_1X);
    // Heater: 320°C for 150ms — reasonable general-purpose VOC setpoint
    bme.setHeaterProf(320, 150);

    bme_ready = true;
    Serial.println("BME688: ready");
    return true;
}

// Non-blocking forced-mode update. Call every loop() iteration.
// Triggers a measurement, then returns immediately. On subsequent calls,
// reads data once the measurement duration has elapsed (~175ms for this heater profile).
static uint32_t bme_ready_at_us = 0;
static bool     bme_measuring   = false;

void update_BME() {
    if (!bme_ready) return;

    uint32_t now = micros();

    if (!bme_measuring) {
        bme.setOpMode(BME68X_FORCED_MODE);
        bme_ready_at_us = now + bme.getMeasDur();
        bme_measuring = true;
        return;
    }

    if ((int32_t)(now - bme_ready_at_us) < 0) return;

    bme_measuring = false;
    bme68xData data;
    if (bme.fetchData()) {
        bme.getData(data);
        bme_temperature = data.temperature;
        bme_humidity    = data.humidity;
        bme_pressure    = data.pressure / 100.0f;
        bme_raw_gas     = (data.status & BME68X_GASM_VALID_MSK) ? data.gas_resistance : 0.0f;
    }
}

// Snapshot of the latest BME688 readings — call this when building your
// transmission payload.
BMEReading get_BME_reading() {
    return {
        .temperature = bme_temperature,
        .humidity    = bme_humidity,
        .pressure    = bme_pressure,
        .raw_gas     = bme_raw_gas,
    };
}

// Send a command and wait for [AK] or [NA]. Returns true on ACK.
static bool inir_cmd(const char* cmd, uint32_t timeoutMs = 500) {
    while (CH4Serial.available()) CH4Serial.read();  // flush
    
    Serial.printf("INIR2: TX bytes for '%s': ", cmd);
    for (size_t i = 0; cmd[i]; i++) {
        Serial.printf("0x%02X ", (uint8_t)cmd[i]);
    }
    Serial.println();
    
    CH4Serial.print(cmd);

    String response = "";
    uint32_t start = millis();
    Serial.print("INIR2: RX bytes: ");
    while (millis() - start < timeoutMs) {
        if (CH4Serial.available()) {
            char c = (char)CH4Serial.read();
            Serial.printf("0x%02X ", (uint8_t)c);
            response += c;
            if (c == ']') break;
        }
    }
    Serial.printf("\nINIR2: RX as string: '%s'\n", response.c_str());
    return response.indexOf("414b") >= 0;
}

bool setup_methane(){
    CH4Serial.end();
    delay(50);
    CH4Serial.begin(38400, SERIAL_8N1, CH4_TX, CH4_RX);

    // Datasheet Table 1: 38400 baud, 8 data bits, no parity, 1 stop bits
    //CH4Serial.begin(38400, SERIAL_8N1, CH4_RX, CH4_TX);
    delay(2000);

    // Listen for any spontaneous output
    // Serial.println("INIR2: listening for 3 seconds...");
    // uint32_t listen_start = millis();
    // while (millis() - listen_start < 3000) {
    //     if (CH4Serial.available()) {
    //         char c = (char)CH4Serial.read();
    //         Serial.printf("0x%02X ", (uint8_t)c);
    //     }
    // }
    // Serial.println("\nINIR2: done listening");

    // Initialization procedure (datasheet section 1.1.5):
    // Step 1: enter Configuration Mode
    Serial.println("INIR2: [C] config mode...");
    if (!inir_cmd("[C]")) {
        Serial.println("INIR2: no ACK to [C]");
        return false;
    }

    // Step 2: read back settings for operation check, then flush response
    CH4Serial.print("[I]");
    delay(300);
    while (CH4Serial.available()) CH4Serial.read();

    // Step 3: enter Engineering Mode — sensor starts streaming every 1s
    Serial.println("INIR2: [B] engineering mode...");
    if (!inir_cmd("[B]")) {
        Serial.println("INIR2: no ACK to [B]");
        return false;
    }
    Serial.println("INIR2: streaming started (45s warmup)");
    return true;
}

bool setup_sensors(){
    bool methane_setup = setup_methane();
    Serial.printf("Methane setup: %d\n", methane_setup);
    setup_BME();

    return adc.begin(0x49);
}

// Returns %vol
float read_methane_adc(){
    int raw = adc.readADC_SingleEnded(0);
    float ch4 = ((raw/1.25)-1)*CH4_RANGE;
    return ch4;
}

// Read one 8-char hex token terminated by \r\n. Returns "" on timeout.
static String inir_read_token(uint32_t timeoutMs = 500) {
    String token = "";
    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        if (CH4Serial.available()) {
            char c = (char)CH4Serial.read();
            if (c == '\n' || c == '\r') {
                if (token.length() == 8) return token;
                token = "";  // skip blank/partial lines
            } else {
                token += c;
                if (token.length() > 8) token = "";  // malformed, reset
            }
        }
    }
    return "";
}

int read_methane_uart() {
    if (!CH4Serial.available()) return -1;

    uint32_t deadline = millis() + 2000;
    while (millis() < deadline) {
        String tok = inir_read_token(500);
        if (tok == "0000005b") {
            String conc = inir_read_token(500);
            String faults_tok = inir_read_token(500);
            if (conc.length() == 8) {
                uint32_t faults = (uint32_t)strtoul(faults_tok.c_str(), nullptr, 16);
                if ((faults & 0x0F000000) == 0x03000000) return -1; // warm-up
                return (int)strtol(conc.c_str(), nullptr, 16);
            }
            return -1;
        }
    }
    return -1;
}

float read_h2s_ppm() {
    int16_t raw = adc.readADC_SingleEnded(1);   // VH2S on AIN1
    float v_out = raw * ADC_LSB_VOLTS;
    //Serial.printf("H2S raw=%d v_out=%.3fV (Vref expected 1.65V)\n", raw, v_out);
    float v_signal = v_out - VREF_VOLTS;        // remove bias
    float current_amps = v_signal / H2S_TIA_GAIN_OHMS;
    float current_na = current_amps * 1e9f;
    return current_na / H2S_SENSITIVITY_NA_PER_PPM;
}

float read_no2_ppm() {
    int16_t raw = adc.readADC_SingleEnded(2);   // VNO2 on AIN2
    float v_out = raw * ADC_LSB_VOLTS;
    //Serial.printf("NOx raw=%d v_out=%.3fV (Vref expected 1.65V)\n", raw, v_out);
    float v_signal = v_out - VREF_VOLTS;
    float current_amps = v_signal / NO2_TIA_GAIN_OHMS;
    float current_na = current_amps * 1e9f;
    return current_na / NO2_SENSITIVITY_NA_PER_PPM;  // sensitivity is negative for NO2
}