#include "lib/sensors.h"
#include "dev/sensinode-sensors.h"
#include "sensor-value.h"

/********************************************/
/*Function name: get_battery_voltage
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
uint16_t get_light()
{
	struct sensors_sensor *sensor;
	int rv;
	float value;
	uint16_t result = 0;
	
	sensor = sensors_find(ADC_SENSOR);
	rv = sensor->value(ADC_SENSOR_TYPE_LIGHT);
	if (rv != -1) {
		value = rv;
		value *= 0.4071;
		result = (uint16_t)(value * 100);
	}
	
	return result;
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
uint16_t get_temperature() {
	struct sensors_sensor *sensor;
	int rv;
	float value;
	uint16_t result = 0;

	sensor = sensors_find(ADC_SENSOR);
	if (sensor == NULL) {
		return 0;
	}
	rv = sensor->value(ADC_SENSOR_TYPE_TEMP);
	if (rv != -1) {
		value = ((rv * 0.61065 - 773) / 2.45);
		result = (uint16_t)value;
		return result;
	}
	else
		return 0;
}