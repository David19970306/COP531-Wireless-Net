#include <stdio.h>
#include "contiki.h"
#include "dev/button-sensor.h"
#include "dev/sensinode-sensors.h"
#include "dev/leds.h"

#include "route-discovery.h"
#include "multihop-callbacks.h"
#include "config.h"
#include "util.h"
#include "packet.h"

/*---------------------------------------------------------------------------*/
PROCESS(dest_process, "Destination");
PROCESS(button_stats, "Change state of the button");
#if PRESSURE_MODE
AUTOSTART_PROCESSES(&dest_process, &button_stats);
#else
AUTOSTART_PROCESSES(&dest_process);
#endif
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
multihop_print_route(struct node *node, const uint8_t hops, float route_index)
{
	uint8_t i = hops;
	route_index /= 100.0;
	
	printf("MULTIHOP_RECEIVED: orig ");
	while (i-- > 0) 
	{
		printf("%d.%d -> ", 
			node->addr.u8[0], node->addr.u8[1]
		);
		node += 1;
	}
	printf("%d.%d ROUTE_INDEX = -%d.%02u\n", 
		rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
		get_decimal(route_index), get_fraction(route_index)
	);
}
void
multihop_received(struct multihop_conn *ptr,
  const rimeaddr_t *sender,
  const rimeaddr_t *prevhop,
  uint8_t hops)
{
  struct packet *packet = packetbuf_dataptr();
#if PRESSURE_MODE
  extern count;
#endif
  extern disp_switch;
  float battery;
  float light;
  float temperature;
  struct node *first_node = (void *) (packet + 1);
  struct route_entry *e;
  
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

#if ACKNOWLEDGEMENT
  packet->ack = 1;
  multihop_send(&mc, sender);
#endif

  e = route_lookup_nexthop(sender, prevhop);
  if (e)
  {
    e->time = 0;
  }

  multihop_print_route(first_node, hops, packet->route_index);

  battery = packet->battery;
  battery /= 100.0;
  light = packet->light;
  light /= 100.0;
  temperature = packet->temperature;
  temperature /= 100.0;
#if PRESSURE_MODE
  if (disp_switch) {
#endif
	  if (packet->disp) {
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
#if PRESSURE_MODE
  }
#endif


}
PROCESS_THREAD(dest_process, ev, data)
{
  PROCESS_EXITHANDLER(route_discovery_close(&rc); multihop_close(&mc);)
    
  PROCESS_BEGIN();

  time = clock_time();

  route_init();
  route_set_lifetime(ROUTE_LIFETIME);

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

