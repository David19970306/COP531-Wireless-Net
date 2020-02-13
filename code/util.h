#ifndef __UTIL_H__
#define __UTIL_H__
#define ABS(a) ((a)>0)?(a):(-a)

int get_decimal(float value)
{
	return value;
}
int get_fraction(float value)
{
	return ABS(value - get_decimal(value)) * 100;
}

#endif /* __UTIL_H__ */
