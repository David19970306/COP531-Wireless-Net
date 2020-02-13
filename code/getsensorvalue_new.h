#pragma once

#ifndef __SENSORVALUE_H__
#define __SENSORVALUE_H__


#include "lib/sensors.h"

/* Sensor Values */
 int rv;
 float sane = 0;
 int dec;
 float frac;
 int VDD;

uint16_t results;

/********************************************/
/*Function name: getSupplyVol
*Description:This function is to get the supply voltage from sensor in sensinode.
* Using 1.25V ref. voltage.
* AD Conversion on VDD/3
*
* Thus, at 12bit resolution:
*
*          ADC x 1.25 x 3
* Supply = -------------- V
*               2047
*Parameters:
*		None.
*Return :
*		$SupplyVol: the voltage of supply.
*********************************************/
uint16_t getSupplyVol(struct sensors_sensor * sensor) {

	sensor = sensors_find(ADC_SENSOR);
	rv = sensor->value(ADC_SENSOR_TYPE_VDD);
	if (rv != -1) {
		sane = rv * 3.75 / 2047;
		dec = sane;
		frac = sane - dec;
		//printf("  Supply=%d.%02uV (%d)\n\r", dec, (unsigned int)(frac * 100), rv);
		/* Store rv temporarily in dec so we can use it for the battery */
		dec = rv;
		results = (uint16_t)(sane*100);

	}
	return results;
}


/********************************************/
/*Function name: getBattVol
*Description:This function is to get the Battery voltage from sensor in sensinode.
* Battery Voltage - Only 2/3 of the actual voltage reach the ADC input
* Using 1.25V ref. voltage would result in 2047 AD conversions all the
* time since ADC-in would be gt 1.25. We thus use AVDD_SOC as ref.
*
* Thus, at 12bit resolution (assuming VDD is 3.3V):
*
*           ADC x 3.3 x 3   ADC x 4.95
* Battery = ------------- = ---------- V
*             2047 x 2         2047
*
* Replacing the 3.3V with an ADC reading of the actual VDD would yield
* better accuracy. See monitor-node.c for an example.
*
*           3 x ADC x VDD x 3.75   ADC x VDD x 11.25
* Battery = -------------------- = ----------------- V
*             2 x 2047 x 2047           0x7FE002
*
*Parameters:
*		None.
*Return :
*		$SupplyVol: the voltage of supply.
*********************************************/
uint16_t getBattVol(struct sensors_sensor * sensor) {

	sensor = sensors_find(ADC_SENSOR);
	rv = sensor->value(ADC_SENSOR_TYPE_BATTERY);
	VDD = sensor->value(ADC_SENSOR_TYPE_VDD);
	if (rv != -1 & VDD !=-1) {
		/* Instead of hard-coding 3.3 here, use the latest VDD (stored in dec)
		 * (slightly inaccurate still, but better than crude 3.3) */
		sane = (11.25 * rv * VDD) / (0x7FE002);
		dec = sane;
		frac = sane - dec;
		//printf("  Batt.=%d.%02uV (%d)\n\r", dec, (unsigned int)(frac * 100), rv);
		dec = rv;
		results = (uint16_t)(sane*100);
	}

	return results;
}

/********************************************/
/*Function name: getLight
*Description:This function is to get the value of light from sensor in sensinode.
* Light: Vishay Semiconductors TEPT4400
* Using 1.25V ref. voltage.
* For 600 Lux illuminance, the sensor outputs 1mA current (0.9V ADC In)
* 600 lux = 1mA output => 1473 ADC value at 12 bit resolution)
*
* Thus, at 12bit resolution:
*       600 x 1.25 x ADC
* Lux = ---------------- ~= ADC * 0.4071
*          2047 x 0.9
*Parameters:
*		None.
*Return :
*		$LightVal: the value of light.
*********************************************/
uint16_t getLight(struct sensors_sensor * sensor) {

	rv = sensor->value(ADC_SENSOR_TYPE_LIGHT);
	if (rv != -1) {
		sane = (float)(rv * 0.4071);
		dec = sane;
		frac = sane - dec;
		//printf("  Light=%d.%02ulux (%d)\n\r", dec, (unsigned int)(frac * 100), rv);
		dec = rv;
		results = (uint16_t)sane;
	}

	return results;
}

/********************************************/
/*Function name: getTemp
*Description:This function is to get the value of temperature from sensor in sensinode.
* Using 1.25V ref. voltage (1250mV).
* Typical Voltage at 0°C : 743 mV
* Typical Co-efficient   : 2.45 mV/°C
* Offset at 25°C         : 30 (this varies and needs calibration)
*
* Thus, at 12bit resolution:
*
*     ADC x 1250 / 2047 - (743 + 30)    0.61065 x ADC - 773
* T = ------------------------------ ~= ------------------- °C
*                 2.45                         2.45
*
*Parameters:
*		None.
*Return :
*		$TempVal: the value of temperature.
*********************************************/
uint16_t getTemp(struct sensors_sensor * sensor) {

	sensor = sensors_find(ADC_SENSOR);
	rv = sensor->value(ADC_SENSOR_TYPE_TEMP);
	if (rv != -1) {
		sane = ((rv * 0.61065 - 773) / 2.45);
		dec = sane;
		frac = sane - dec;
		//printf("  Temp=%d.%02u C (%d)\n\r", dec, (unsigned int)(frac * 100), rv);
		dec = rv;
		results = (uint16_t)sane;

	}
	return results;
}


#endif
