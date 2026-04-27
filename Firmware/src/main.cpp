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

// --- Scheduled tasks --------------------------------------------------------

void task_read_bme() {
    BMEReading bme = get_BME_reading();
    if (bme.raw_gas > 0.0f) {
        myPkt.voc_load = (uint16_t)constrain((1.0f / bme.raw_gas) * 1e6f * 10.0f, 0, 65535);
    } else {
        myPkt.voc_load = 0;
    }
    myPkt.temp        = (int16_t)(bme.temperature * 100.0f);
    myPkt.humidity    = (uint16_t)(bme.humidity * 100.0f);
    myPkt.pressure    = (uint16_t)(bme.pressure * 10.0f);
    Serial.printf("Voc load: %d\n", myPkt.voc_load);
}

void task_read_gases() {
    float ch4 = read_methane_adc();
    float h2s = read_h2s_ppm();
    float no2 = read_no2_ppm();

    //myPkt.ch4 = 
    myPkt.h2s = (uint16_t)constrain(h2s * 100.0f, 0, 5000);
    myPkt.nox = (uint16_t)constrain(no2 * 100.0f, 0, 30000);

    Serial.printf("CH4 %%vol: %.2f H2S: %.2f ppm  NO2: %.2f ppm\n", ch4, h2s, no2);
}

void task_send_lora() {
    if (LMIC.devaddr != 0) {
        send_lora(myPkt);
        digitalWrite(LED, HIGH);
        delay(50);
        digitalWrite(LED, LOW);
    }
}

Task tasks[] = {
    { task_read_bme,   3000,  0 },
    { task_read_gases, 3000,  0 },
    //{ task_send_lora,  30000, 0 },
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

    // if (!setup_lora())
    //     Serial.println("LoRaWAN init failed");

    //send_lora(myPkt);   // initial transmission before scheduler takes over
    scheduler.begin();  // start all task clocks from now
}

void loop() {
    //myLoRaWAN.loop();
    update_BME();
    scheduler.run();
}
