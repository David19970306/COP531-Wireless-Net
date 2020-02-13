#ifndef __UTIL_H__
#define __UTIL_H__
#define ABS(a) ((a)>0)?(a):(-a)

int get_decimal(float value)
{
	return value;
}
uint16_t get_fraction(float value)
{
	float fraction = (value - get_decimal(value)) * 100;
	return ABS(fraction);
}

#endif /* __UTIL_H__ */
