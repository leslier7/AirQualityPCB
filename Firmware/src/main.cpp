// Air Quality Monitoring
// Robbie Leslie April 2026

#include <Arduino.h>
#include "sensors.h"
#include "scheduler.h"

#ifdef LORA
#include "lora.h"
#elif defined(WIFI)
#include "wifi.h"
#endif

#define LED 2

#ifdef LORA
pkt_fmt myPkt;
#endif

// --- Averaging accumulators -------------------------------------------------
 
struct {
    float    voc_load;
    float    temp;
    float    humidity;
    float    pressure;
    uint32_t n;
} bme_acc = {};
 
struct {
    float    ch4;
    uint32_t ch4_n;
    float    h2s;
    uint32_t h2s_n;
    float    nox;
    uint32_t nox_n;
} gas_acc = {};
 
static void reset_accumulators() {
    bme_acc = {};
    gas_acc = {};
}


// --- Scheduled tasks --------------------------------------------------------

void task_read_bme() {
    update_BME();
    BMEReading bme = get_BME_reading();
    if (bme.raw_gas > 0.0f) {
        myPkt.voc_load = (uint16_t)constrain((1.0f / bme.raw_gas) * 1e6f * 10.0f, 0, 65535);
    } else {
        myPkt.voc_load = 0;
    }
    myPkt.temp        = (int16_t)(bme.temperature * 100.0f);
    myPkt.humidity    = (uint16_t)(bme.humidity * 100.0f);
    myPkt.pressure    = (uint16_t)(bme.pressure * 10.0f);
    Serial.printf("Voc load: %d  Temp: %.2f  Humidity: %.2f Pressure: %.2f\n", myPkt.voc_load, bme.temperature, bme.humidity, bme.pressure);
}

void task_read_gases() {
    uint16_t ch4 = read_methane_uart();
    float h2s = read_h2s_ppm();
    float no2 = read_no2_ppm();

    if (ch4 != 65535){
        myPkt.ch4 = ch4;
    }
    myPkt.h2s = (uint16_t)constrain(h2s * 100.0f, 0, 5000);
    myPkt.nox = (uint16_t)constrain(no2 * 100.0f, 0, 30000);

    Serial.printf("CH4: %d ppm H2S: %.2f ppm  NO2: %.2f ppm\n", ch4, h2s, no2);
}

void task_send_lora() {
    if (!g_joined) return;
    if (LMIC.devaddr != 0) {

        #ifdef TEST
        myPkt.ch4 = 0;
        myPkt.h2s = 0;
        myPkt.nox = 0;
        myPkt.humidity = 0;
        myPkt.temp = 0;
        myPkt.voc_load = 0;

        #else
        // Flush BME averages into packet
        if (bme_acc.n > 0) {
            myPkt.voc_load = (uint16_t)(bme_acc.voc_load / bme_acc.n);
            myPkt.temp     = (int16_t)((bme_acc.temp     / bme_acc.n) * 100.0f);
            myPkt.humidity = (uint16_t)((bme_acc.humidity / bme_acc.n) * 100.0f);
            myPkt.pressure = (uint16_t)((bme_acc.pressure / bme_acc.n) * 10.0f);
        }
        // Flush gas averages into packet
        if (gas_acc.ch4_n > 0)
            myPkt.ch4 = (uint16_t)(gas_acc.ch4 / gas_acc.ch4_n);
        if (gas_acc.h2s_n > 0)
            myPkt.h2s = (uint16_t)constrain((gas_acc.h2s / gas_acc.h2s_n) * 100.0f, 0, 5000);
        if (gas_acc.nox_n > 0)
            myPkt.nox = (uint16_t)constrain((gas_acc.nox / gas_acc.nox_n) * 100.0f, 0, 30000);
        
        #endif

        Serial.printf("TX averages — CH4: %d  H2S: %d  NOx: %d  Temp: %d  Hum: %d  VOC: %d\n",
            myPkt.ch4, myPkt.h2s, myPkt.nox, myPkt.temp, myPkt.humidity, myPkt.voc_load);
        
        uint8_t *raw = (uint8_t*)&myPkt;
        Serial.print("RAW PKT: ");
        for (int i = 0; i < (int)sizeof(myPkt); i++)
            Serial.printf("%02X ", raw[i]);
        Serial.println();
        
        send_lora(myPkt);
        reset_accumulators();
 
        digitalWrite(LED, HIGH);
        delay(50);
        digitalWrite(LED, LOW);
    }
}


Task tasks[] = {
    #ifndef TEST
    { task_read_bme,   1500,  0 },
    { task_read_gases, 3000,  0 },
    #endif
    { task_send_lora,  TX_INTERVAL_MS, 0 },
};

Scheduler scheduler(tasks, sizeof(tasks) / sizeof(tasks[0]));

// ----------------------------------------------------------------------------

// void setup() {
//     Serial.begin(115200);
//     Serial0.begin(38400, SERIAL_8N1, CH4_RX, CH4_TX);
//     delay(2000);
// }

// void loop() {
//     Serial0.print("[C]");
//     delay(10);  // 100 commands per second
// }

void setup() {
    pinMode(LED, OUTPUT);
    digitalWrite(LED, HIGH);
    delay(1000);
    digitalWrite(LED, LOW);
    Serial.begin(115200);
    delay(500);
    Serial.println("Air Quality Monitor starting");

    setup_I2C();
    setup_sensors();
    testPortI2C(Wire);

    myPkt.ch4 = 40;
    myPkt.h2s = 50;
    myPkt.nox = 60;
    myPkt.humidity = 70;
    myPkt.temp = 80;
    myPkt.voc_load = 90;

    #ifndef TEST
    if (!setup_lora())
        Serial.println("LoRaWAN init failed");
    #endif

    

    #ifndef TEST
    send_lora(myPkt);   // initial transmission before scheduler takes over
    #endif
    scheduler.begin();  // start all task clocks from now
}

void loop() {
    #ifndef TEST
    myLoRaWAN.loop();
    #endif
    scheduler.run();
}
