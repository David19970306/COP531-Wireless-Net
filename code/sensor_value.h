#pragma once
#ifndef SENSOR_VALUE_H
#define SENSOR_VALUE_H

//Get the Supply Voltage from sensor.
float getSupplyVol();

//Get the Battery Voltage from sensor.
float getBattVol();

//Get the value of light from sensor.
float getLight();

//Get the value of temperature from sensor.
float getTemp();

#endif