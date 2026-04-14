// Robbie Leslie
// April 2026

#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <Wire.h>

#define SDA_PIN 4
#define SCL_PIN 5

bool setup_I2C();
bool setup_sensors();

void testPortI2C(TwoWire &i2c);

int read_methane();

#endif