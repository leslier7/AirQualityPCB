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

extern bool setup_ch4;

bool setup_I2C();
bool setup_sensors();

void testPortI2C(TwoWire &i2c);

int read_methane_adc();
int read_methane_uart();

#endif