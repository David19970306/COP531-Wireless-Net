#pragma once
#ifndef SENSOR_VALUE_H
#define SENSOR_VALUE_H

//Get the Supply Voltage from sensor.
uint16_t getSupplyVol(struct sensors_sensor * sensor);

//Get the Battery Voltage from sensor.
uint16_t getBattVol(struct sensors_sensor * sensor);

//Get the value of light from sensor.
uint16_t getLight(struct sensors_sensor * sensor);

//Get the value of temperature from sensor.
uint16_t getTemp(struct sensors_sensor * sensor);

#endif