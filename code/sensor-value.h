
#ifndef __SENSORVALUE_H__
#define __SENSORVALUE_H__

#include "lib/sensors.h"
#include "dev/sensinode-sensors.h"

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
uint16_t get_battery_voltage() {
	int adc;
	int vdd;
	struct sensors_sensor *sensor;
	float value;
	uint16_t result = 0;
	
	sensor = sensors_find(ADC_SENSOR);
	adc = sensor->value(ADC_SENSOR_TYPE_BATTERY);
	vdd = sensor->value(ADC_SENSOR_TYPE_VDD);
	
	if (adc != -1 & vdd !=-1) {
		/* Instead of hard-coding 3.3 here, use the latest VDD (stored in dec)
		 * (slightly inaccurate still, but better than crude 3.3) */
		value = (11.25 * adc * vdd) / (0x7FE002);
		result = (uint16_t)(value * 100);
	}
	
	return result;
}


#endif
