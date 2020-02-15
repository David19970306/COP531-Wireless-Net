
#ifndef __SENSOR_VALUE_H__
#define __SENSOR_VALUE_H__

#include "lib/sensors.h"
#include "dev/sensinode-sensors.h"

uint16_t get_battery_voltage();
uint16_t get_light();
uint16_t get_temp();

#endif
