
#include "contiki.h"
#include "contiki-conf.h"
#include "dev/sensinode-sensors.h"
#include "net/rime.h"
#include "dev/watchdog.h"
#include "lib/random.h"

#include "sensor_value.h"//reference the header file from ownself



#include<stdio.h>
#define DEBUG 1
#if DEBUG
#include "sensinode-debug.h"
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
/* We overwrite (read as annihilate) all output functions here */
#define PRINTF(...)
#define putstring(...)
#define putchar(...)
#endif


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
float getSupplyVol() {
	float SupplyVol = 0.00;
	
	rv = sensor->value(ADC_SENSOR_TYPE_VDD);
	if (rv != -1) {
		sane = rv * 3.75 / 2047;
		dec = sane;
		frac = sane - dec;
		PRINTF("  Supply=%d.%02uV (%d)\n\r", dec, (unsigned int)(frac * 100), rv);
		/* Store rv temporarily in dec so we can use it for the battery */
		dec = rv;
		SupplyVol = sane;
	}
	return SupplhyVol;
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
float getBattVol() {
		float BattVol = 0.00;
		rv = sensor->value(ADC_SENSOR_TYPE_BATTERY);
		if (rv != -1) {
			/* Instead of hard-coding 3.3 here, use the latest VDD (stored in dec)
			 * (slightly inaccurate still, but better than crude 3.3) */
			sane = (11.25 * rv * dec) / (0x7FE002);
			dec = sane;
			frac = sane - dec;
			PRINTF("  Batt.=%d.%02uV (%d)\n\r", dec, (unsigned int)(frac * 100), rv);
			dec = rv;
			BattVol = sane;
		}
		
		return BattVol;
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
float getLight() {
	float LightVal = 0.00;
	rv = sensor->value(ADC_SENSOR_TYPE_LIGHT);
	if (rv != -1) {
		sane = (float)(rv * 0.4071);
		dec = sane;
		frac = sane - dec;
		PRINTF("  Light=%d.%02ulux (%d)\n\r", dec, (unsigned int)(frac * 100), rv);
		dec = rv;
		LightVal = sane;
	}
	
	return LightVal;
}

/********************************************/
/*Function name: getTemp
*Description:This function is to get the value of temperature from sensor in sensinode.
* Using 1.25V ref. voltage (1250mV).
* Typical Voltage at 0�C : 743 mV
* Typical Co-efficient   : 2.45 mV/�C
* Offset at 25�C         : 30 (this varies and needs calibration)
*
* Thus, at 12bit resolution:
*
*     ADC x 1250 / 2047 - (743 + 30)    0.61065 x ADC - 773
* T = ------------------------------ ~= ------------------- �C
*                 2.45                         2.45
*
*Parameters:
*		None.
*Return :
*		$TempVal: the value of temperature.
*********************************************/
float getTemp() {
	float TempVal = 0.00;
	rv = sensor->value(ADC_SENSOR_TYPE_TEMP);
	if (rv != -1) {
		sane = ((rv * 0.61065 - 773) / 2.45);
		dec = sane;
		frac = sane - dec;
		PRINTF("  Temp=%d.%02u C (%d)\n\r", dec, (unsigned int)(frac * 100), rv);
		dec = rv;
		TempVal = sane;
	}
	
	return TempVal;
}
