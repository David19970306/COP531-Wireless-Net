#include <stdio.h>
#include "contiki.h"
#include "dev/button-sensor.h"
#include "dev/sensinode-sensors.h"
#include "dev/leds.h"

#include "route-discovery.h"
#include "multihop-callbacks.h"
#include "config.h"

/*---------------------------------------------------------------------------*/
PROCESS(dest_process, "Destination");
PROCESS(button_stats, "Change state of the button");
AUTOSTART_PROCESSES(&dest_process, &button_stats);
/*---------------------------------------------------------------------------*/
static struct route_discovery_conn rc;
static const struct route_discovery_callbacks route_discovery_callbacks = { NULL, NULL };
int count;
static int count_tmp = 0;
/*---------------------------------------------------------------------------*/
// sensinode sensors
extern const struct sensors_sensor button_1_sensor, button_2_sensor;
int disp_value = 0;//change the display value (Temperature or Light)
int disp_switch = 0;//Switch the mode of sending data.(turn on/off send periodically)
static uint8_t dbg = 1;
/*---------------------------------------------------------------------------*/
static struct multihop_conn mc;
static const struct multihop_callbacks multihop_callbacks = { 
  multihop_received, multihop_forward 
};
/*---------------------------------------------------------------------------*/
static clock_time_t time;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(dest_process, ev, data)
{
  PROCESS_EXITHANDLER(route_discovery_close(&rc); multihop_close(&mc);)
    
  PROCESS_BEGIN();

  time = clock_time();

  route_discovery_open(&rc, time, ROUTE_DISCOVERY_CHANNEL, &route_discovery_callbacks);
  multihop_open(&mc, MULTIHOP_CHANNEL, &multihop_callbacks);
  
  while(1) {
    static struct etimer et;
    
    etimer_set(&et, DESTINATION_PERIOD);
    
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

	printf("Total received:[%u], delta[%u]\n", count, count - count_tmp);
	count_tmp = count;

  }

  PROCESS_END();
}

PROCESS_THREAD(button_stats, ev, data)
{
	struct sensors_sensor *sensor;
	PROCESS_BEGIN();

	while (1) {
		PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event);

		sensor = (struct sensors_sensor *)data;
		if (sensor == &button_1_sensor) {
			// switch on/off sending data
			disp_switch = !disp_switch;
			if (dbg) printf("Disp Button Pressed,Val:[%d]\n", disp_switch);
		}
		else if (sensor == &button_2_sensor) {
			// switch which value to be displayed
			disp_value = !disp_value;
			if (dbg) printf("Switch Value Button Pressed,Val:[%d]\n", disp_value);
		}
	}
	PROCESS_END();

}

