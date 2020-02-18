#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "contiki.h"

/* Route Discovery */
#define ROUTE_DISCOVERY_CHANNEL 146
#define ROUTE_DISCOVERY_TIMEOUT CLOCK_SECOND/5

/* Multihop forward and receive. */
#define MULTIHOP_CHANNEL 128
#define MULTIHOP_MAX_HOPS 8

#define SOURCE_PERIOD 2 * CLOCK_SECOND
#define SOURCE_ACK_WAIT_TIME CLOCK_SECOND/5

#define DESTINATION_PERIOD 6 * CLOCK_SECOND

#define GROUP_NUMBER 0x33

#define PRESSURE_UNIQ_NUMBER 9
#define PRESSURE_MODE 0

#define ACKNOWLEDGEMENT 1
#define ROUTE_LIFETIME 10


#endif