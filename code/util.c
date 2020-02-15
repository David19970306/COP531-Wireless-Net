#include "util.h"

int get_decimal(float value)
{
	return value;
}
uint16_t get_fraction(float value)
{
	float fraction = (value - get_decimal(value)) * 100;
	return ABS(fraction);
}



