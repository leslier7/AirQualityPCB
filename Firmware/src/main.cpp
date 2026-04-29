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
    float voc = (bme.raw_gas > 0.0f)
        ? constrain((1.0f / bme.raw_gas) * 1e6f * 10.0f, 0, 65535)
        : 0.0f;
 
    bme_acc.voc_load += voc;
    bme_acc.temp     += bme.temperature;
    bme_acc.humidity += bme.humidity;
    bme_acc.pressure += bme.pressure;
    bme_acc.n++;
 
    Serial.printf("Voc load (raw): %.1f  [%lu samples]\n", voc, bme_acc.n);

}

void task_read_gases() {
    uint16_t ch4 = read_methane_uart();
    float h2s = read_h2s_ppm();
    float no2 = read_no2_ppm();

    if (ch4 != 65535) {
        gas_acc.ch4 += ch4;
        gas_acc.ch4_n++;
    }
    gas_acc.h2s += h2s;
    gas_acc.h2s_n++;
    gas_acc.nox += no2;
    gas_acc.nox_n++;
 
    Serial.printf("CH4: %d ppm H2S: %.2f ppm  NO2: %.2f ppm\n", ch4, h2s, no2);

}

void task_send_lora() {
    if (!g_joined) return;
    if (LMIC.devaddr != 0) {
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
 
        Serial.printf("TX averages — CH4: %d  H2S: %d  NOx: %d  Temp: %d  Hum: %d  VOC: %d\n",
            myPkt.ch4, myPkt.h2s, myPkt.nox, myPkt.temp, myPkt.humidity, myPkt.voc_load);
        
        
        send_lora(myPkt);
        reset_accumulators();
 
        digitalWrite(LED, HIGH);
        delay(50);
        digitalWrite(LED, LOW);
    }
}


Task tasks[] = {
    { task_read_bme,   1500,  0 },
    { task_read_gases, 3000,  0 },
    { task_send_lora,  360000, 0 }, //TODO transmit this 10 times per hour (360 seconds)
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

    if (!setup_lora())
        Serial.println("LoRaWAN init failed");

    send_lora(myPkt);   // initial transmission before scheduler takes over
    scheduler.begin();  // start all task clocks from now
}

void loop() {
    myLoRaWAN.loop();
    scheduler.run();
}
