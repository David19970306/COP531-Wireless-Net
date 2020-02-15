#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "contiki.h"

/* Route Discovery */
#define ROUTE_DISCOVERY_CHANNEL 146
#define ROUTE_DISCOVERY_TIMEOUT 2 * CLOCK_SECOND

/* Multihop forward and receive. */
#define MULTIHOP_CHANNEL 128
#define MULTIHOP_MAX_HOPS 8

#define SOURCE_PERIOD 2 * CLOCK_SECOND
#define SOURCE_ROUTE_LIFETIME 10

#define DESTINATION_PERIOD 10 * CLOCK_SECOND

#define GROUP_NUMBER 0x33


#endif