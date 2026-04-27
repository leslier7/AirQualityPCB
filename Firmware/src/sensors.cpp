// Robbie Leslie
// April 2026

#include "sensors.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <bsec2.h>
#include <Preferences.h>

Adafruit_ADS1015 adc;
Bsec2 bsec;
Preferences prefs;

bool setup_ch4 = false;

// Latest BSEC2 outputs — updated every 3s by update_BME(), read hourly for transmission
static float bme_iaq          = 0.0f;
static float bme_co2_eq       = 0.0f;
static float bme_voc_eq       = 0.0f;
static float bme_temperature  = 0.0f;
static float bme_humidity     = 0.0f;
static float bme_pressure     = 0.0f;
static float bme_raw_gas      = 0.0f;
static uint8_t bme_iaq_accuracy = 0;  // 0=unreliable, 1=low, 2=medium, 3=high

static bool bme_ready = false;

// BSEC2 state is saved to NVS every 1 hour so calibration survives reboots
#define BSEC_STATE_KEY             "bsec_state"
#define BSEC_STATE_SAVE_INTERVAL_MS (60UL * 60UL * 1000UL)  // 1 hour
static uint32_t last_state_save_ms = 0;

// --------------------------------------------------------------------------
// BSEC2 state persistence (NVS)
// --------------------------------------------------------------------------

static void save_bsec_state() {
    uint8_t state[BSEC_MAX_STATE_BLOB_SIZE];
    if (bsec.getState(state) != BSEC_OK) {
        Serial.println("BME688: failed to get BSEC2 state");
        return;
    }
    prefs.begin("bsec", false);
    prefs.putBytes(BSEC_STATE_KEY, state, BSEC_MAX_STATE_BLOB_SIZE);
    prefs.end();
    Serial.println("BME688: BSEC2 state saved to NVS");
}

static void load_bsec_state() {
    prefs.begin("bsec", true);
    size_t len = prefs.getBytesLength(BSEC_STATE_KEY);
    if (len == BSEC_MAX_STATE_BLOB_SIZE) {
        uint8_t state[BSEC_MAX_STATE_BLOB_SIZE];
        prefs.getBytes(BSEC_STATE_KEY, state, len);
        if (bsec.setState(state) == BSEC_OK) {
            Serial.println("BME688: BSEC2 state restored from NVS");
        } else {
            Serial.println("BME688: BSEC2 setState failed, starting fresh");
        }
    } else {
        Serial.println("BME688: no saved BSEC2 state, starting fresh calibration");
    }
    prefs.end();
}

// --------------------------------------------------------------------------
// BSEC2 data-ready callback — fires automatically inside bsec.run()
// --------------------------------------------------------------------------

static void bsec_callback(const bme68xData data, const bsecOutputs outputs, Bsec2 bsec2) {
    Serial.printf("BME688: callback fired, nOutputs=%d\n", outputs.nOutputs);
    if (!outputs.nOutputs) return;

    for (uint8_t i = 0; i < outputs.nOutputs; i++) {
        const bsecData& out = outputs.output[i];
        switch (out.sensor_id) {
            // case BSEC_OUTPUT_IAQ:
            //     bme_iaq          = out.signal;
            //     bme_iaq_accuracy = out.accuracy;
            //     break;
            case BSEC_OUTPUT_CO2_EQUIVALENT:
                bme_co2_eq = out.signal;
                break;
            case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
                bme_voc_eq = out.signal;
                break;
            case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
                bme_temperature = out.signal;
                break;
            case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
                bme_humidity = out.signal;
                break;
            case BSEC_OUTPUT_RAW_PRESSURE:
                bme_pressure = out.signal / 100.0f;  // Pa -> hPa
                break;
            case BSEC_OUTPUT_RAW_GAS:
                bme_raw_gas = out.signal;
                break;
        }
    }
}

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
    if (!bsec.begin(0x77, Wire)) {
        Serial.printf("BME688: BSEC2 init failed (err %d)\n", bsec.status);
        return false;
    }

    // Subscribe to the outputs we care about at the 3-second LP sample rate.
    // IAQ accuracy climbs from 0->3 over ~30 minutes of exposure to varying air.
    // Don't trust IAQ values until accuracy >= 2.
    bsecSensor outputs[] = {
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
        BSEC_OUTPUT_RAW_PRESSURE,
        BSEC_OUTPUT_RAW_GAS,
    };
    if (!bsec.updateSubscription(outputs, sizeof(outputs) / sizeof(outputs[0]), BSEC_SAMPLE_RATE_LP)) {
        Serial.printf("BME688: subscription failed (err %d)\n", bsec.status);
        return false;
    }

    bsec.attachCallback(bsec_callback);
    load_bsec_state();

    bme_ready = true;
    Serial.println("BME688: BSEC2 ready (IAQ accuracy 0/3 — calibrating)");
    return true;
}

// Call this every iteration of loop(). BSEC2 internally gates to its 3s
// schedule so calling it more often is harmless.
void update_BME() {
    if (!bme_ready) return;
    if (!bsec.run()) {
        if (bsec.status != BSEC_OK) {
            Serial.printf("BME688: bsec.run() error %d\n", bsec.status);
        }
        return;
    }
    Serial.println("BME688: bsec.run() returned true");

    // Periodically persist calibration state to NVS
    if (millis() - last_state_save_ms >= BSEC_STATE_SAVE_INTERVAL_MS) {
        save_bsec_state();
        last_state_save_ms = millis();
    }
}

// Snapshot of the latest BSEC2 outputs — call this when building your
// hourly transmission payload.
BMEReading get_BME_reading() {
    return {
        .iaq          = bme_iaq,
        .iaq_accuracy = bme_iaq_accuracy,
        .co2_eq       = bme_co2_eq,
        .voc_eq       = bme_voc_eq,
        .temperature  = bme_temperature,
        .humidity     = bme_humidity,
        .pressure     = bme_pressure,
        .raw_gas      = bme_raw_gas,
    };
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
    return true;
}

bool setup_sensors(){
    setup_methane();
    setup_BME();

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