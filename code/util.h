#ifndef __UTIL_H__
#define __UTIL_H__

#include "contiki.h"

#define ABS(a) ((a)>0)?(a):(-a)

int get_decimal(float value);
uint16_t get_fraction(float value);

#endif /* __UTIL_H__ */
