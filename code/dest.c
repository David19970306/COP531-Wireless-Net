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
/*---------------------------------------------------------------------------*/
// sensinode sensors
extern const struct sensors_sensor button_1_sensor, button_2_sensor;
uint8_t disp_value = 0;//change the display value (Temperature or Light)
uint8_t disp_switch = 0;//Switch the mode of sending data.(turn on/off send periodically)

#if PRESSURE_MODE
int count;
#endif

static int count_tmp = 0;
static uint8_t dbg = 1;
/*---------------------------------------------------------------------------*/
static struct multihop_conn mc;
static const struct multihop_callbacks multihop_callbacks = { 
  multihop_received, multihop_forward 
};
/*---------------------------------------------------------------------------*/
static clock_time_t time;
/*---------------------------------------------------------------------------*/
void
multihop_received(struct multihop_conn *ptr,
  const rimeaddr_t *sender,
  const rimeaddr_t *prevhop,
  uint8_t hops)
{
  struct packet *packet = packetbuf_dataptr();
  uint8_t type;
#if PRESSURE_MODE
  extern count;
#endif
  extern disp_switch;
  float battery;
  float light;
  float temperature;
  struct node *first_node = (void *) (packet + 1);
  
  if (packet->group_num != GROUP_NUMBER)
  {
	  return;
  }



#if PRESSURE_MODE
  if (packet->ack == PRESSURE_UNIQ_NUMBER) {
	  count++;
  }
  else {
	  count = 0;
  }
#else
  if (packet->ack)
  {
  	return;
  }
#endif

  packet->ack = 1;
  multihop_send(&mc, dest);
  
  multihop_print_route(first_node, hops, packet->route_index);

  type = packet->disp;
  battery = packet->battery;
  battery /= 100.0;
  light = packet->light;
  light /= 100.0;
  temperature = packet->temperature;
  temperature /= 100.0;
  if (disp_switch) {
	  if (type) {
		  printf("DATA_PACKET: light %d.%02u battery %d.%02uV\n",
			  get_decimal(light), get_fraction(light),
			  get_decimal(battery), get_fraction(battery)
		  );
	  }
	  else {
		  printf("DATA_PACKET: temperature %d.%02uC battery %d.%02uV\n",
			  get_decimal(temperature), get_fraction(temperature),
			  get_decimal(battery), get_fraction(battery)
		  );
	  }
  }


}
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

#if PRESSURE_MODE
	printf("Total received:[%u], delta[%u]\n", count, count - count_tmp);
	count_tmp = count;
#endif

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

