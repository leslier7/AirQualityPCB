// Air Quality Monitoring
// Robbie Leslie April 2026

// Imports
#include <Arduino.h>
#include "sensors.h"
#include "lora.h"

#define LED 8

pkt_fmt myPkt;

static uint32_t lastTx = 0;

static uint32_t lastRead = 0;

void setup() {

    // pinMode(LED, OUTPUT);
    // digitalWrite(LED, HIGH);
    Serial.begin(115200);
    delay(500);
    Serial.println("Air Quality Monitor starting");

    setup_I2C();
    setup_sensors();

    testPortI2C(Wire);

    myPkt.intNum = 0;
    myPkt.floatNum = 3.141592654;
    myPkt.digitalTemp = 35;
    myPkt.thermistorTemp = 25;

    uint8_t myCString[6] = "Hello";
    memcpy(myPkt.myString, myCString, 6);

    // if (!setup_lora())
    //     Serial.println("LoRaWAN init failed");

    // queue first transmission — library handles join first
    //send_lora(myPkt);
    lastTx = millis();
    lastRead = millis();

}

void loop() {
    //myLoRaWAN.loop();

    uint32_t curTime = millis();

    if(curTime - lastRead > 5000){
        if(setup_ch4){
            int methane = read_methane_adc();
            Serial.printf("Methane value: %d\n", methane);
        }
        
        lastRead = curTime;
    }

    //testPortI2C(Wire);
    // if (curTime - lastTx > 60000) {
    //     if (LMIC.devaddr != 0) {
    //         send_lora(myPkt);
    //         lastTx = curTime;
    //     }
    // }
}
