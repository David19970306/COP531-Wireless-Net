#pragma once
#ifndef SENSOR_VALUE_H
#define SENSOR_VALUE_H

//Get the Supply Voltage from sensor.
uint16_t getSupplyVol();

//Get the Battery Voltage from sensor.
uint16_t getBattVol();

//Get the value of light from sensor.
uint16_t getLight();

//Get the value of temperature from sensor.
uint16_t getTemp();

#endif