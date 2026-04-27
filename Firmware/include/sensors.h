// Robbie Leslie
// April 2026

#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <Wire.h>

#define SDA_PIN 4
#define SCL_PIN 5

#define CH4_RX 0 //Rx is the sensor recieving, so should be set to TX on the board
#define CH4_TX 1 

struct BMEReading {
    float   iaq;            // Transmit
    uint8_t iaq_accuracy;   // 0=unreliable -> 3=high; don't trust IAQ until >= 2
    float   co2_eq;         // ppm equivalent
    float   voc_eq;         // ppm equivalent
    float   temperature;    // °C
    float   humidity;       // %RH
    float   pressure;       // hPa
    float   raw_gas;        // Ohms — this is your most direct VOC signal for landfill use
};



extern bool setup_ch4;

void update_BME();
BMEReading get_BME_reading();

bool setup_I2C();
bool setup_sensors();

void testPortI2C(TwoWire &i2c);

int read_methane_adc();
int read_methane_uart();

#endif