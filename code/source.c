#include <stdio.h>
#include "contiki.h"
#include "dev/button-sensor.h"
#include "dev/sensinode-sensors.h"
#include "dev/leds.h"
#include "net/rime/multihop.h"
#include "net/packetbuf.h"

#include "route.h"
#include "route-discovery.h"
#include "multihop-callbacks.h"
#include "config.h"
#include "packet.h"
#include "sensor-value.h"

/*---------------------------------------------------------------------------*/
PROCESS(source_process, "Source");
PROCESS(button_stats, "Change state of the button");
PROCESS(net_pressure_handler, "The test of Net pressure.");
#if PRESSURE_MODE
AUTOSTART_PROCESSES(&net_pressure_handler);
#else
AUTOSTART_PROCESSES(&source_process, &button_stats);
#endif
/*---------------------------------------------------------------------------*/
static struct multihop_conn mc;
static struct route_discovery_conn rc;
/*---------------------------------------------------------------------------*/
uint8_t disp_value = 0;//change the display value (Temperature or Light)
uint8_t disp_switch = 0;//Switch the mode of sending data.(turn on/off send periodically)
static clock_time_t time;
/*---------------------------------------------------------------------------*/
// sensinode sensors
extern const struct sensors_sensor button_1_sensor, button_2_sensor;
static uint8_t dbg = 1;
uint8_t verbose = 1;

#if ACKNOWLEDGEMENT
static uint8_t acknowledged;
#endif
/*---------------------------------------------------------------------------*/
void
multihop_received(struct multihop_conn *ptr,
  const rimeaddr_t *sender,
  const rimeaddr_t *prevhop,
  uint8_t hops)
{
	struct packet *packet = packetbuf_dataptr();
	struct route_entry *e;

	if (packet->group_num != GROUP_NUMBER)
	{
		return;
	}
	e = route_lookup_nexthop(sender, prevhop);
	if (e)
	{
		e->time = 0;
	}
#if ACKNOWLEDGEMENT
	if (packet->ack)
	{
		acknowledged = 1;
		process_post(&source_process, PROCESS_EVENT_CONTINUE, NULL);
	}
#endif
}
/*---------------------------------------------------------------------------*/
int
pressure_send_packet(const rimeaddr_t *dest)
{
	struct packet *packet;

	packetbuf_clear();
	packetbuf_set_datalen(sizeof(struct packet));
	packet = packetbuf_dataptr();
	packet->group_num = GROUP_NUMBER;
	packet->ack = PRESSURE_UNIQ_NUMBER;
	packet->battery = 9999;
	packet->light = 9999;
	packet->temperature = 9999;
	packet->disp = 0;


	if (dbg) printf("Sending data content:batt[%u]light[%u]temp[%u];d[%u]\n",
		packet->battery, packet->light, packet->temperature, packet->disp);
	return multihop_send(&mc, dest);
}
/*---------------------------------------------------------------------------*/
void
route_discovery_new_route(struct route_discovery_conn *c, const rimeaddr_t *to)
{
	printf("ROUTE_DISCOVERY: New route to %d.%d is found.\n",
		to->u8[0], to->u8[1]
	);
	process_post(&source_process, PROCESS_EVENT_CONTINUE, NULL);
}
void
route_discovery_timedout(struct route_discovery_conn *c)
{
	printf("ROUTE_DISCOVERY: Cannot discover route.\n");
	process_post(&source_process, PROCESS_EVENT_CONTINUE, NULL);
}
static const struct route_discovery_callbacks route_discovery_callbacks = {
  route_discovery_new_route, route_discovery_timedout
};
/*---------------------------------------------------------------------------*/
static const struct multihop_callbacks multihop_callbacks = {
  multihop_received, multihop_forward
};
void
write_packet(struct packet *packet)
{
	packet->group_num = GROUP_NUMBER;
	packet->battery = get_battery_voltage();
	packet->light = get_light();
	packet->temperature = get_temperature();
	packet->disp = disp_value;
}
void
remove_route_to(const rimeaddr_t *dest)
{
	struct route_entry *route_entry;
	route_entry = route_lookup(dest);
	if (route_entry)
	{
		route_remove(route_entry);
	}
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(source_process, ev, data)
{
	rimeaddr_t dest;
	uint8_t no_route;
	
	dest.u8[0] = 0xdf;
	dest.u8[1] = 0xdf;
	PROCESS_EXITHANDLER(route_discovery_close(&rc); multihop_close(&mc);)

	PROCESS_BEGIN();

	time = clock_time();

	route_discovery_open(&rc, time, ROUTE_DISCOVERY_CHANNEL, &route_discovery_callbacks);
	multihop_open(&mc, MULTIHOP_CHANNEL, &multihop_callbacks);

	route_init();
	route_set_lifetime(ROUTE_LIFETIME);

	while (1) {

		static struct etimer et;
		struct packet *packet;

		etimer_set(&et, SOURCE_PERIOD);

		if (disp_switch)
		{

			while (1)
			{
#if ACKNOWLEDGEMENT
				static struct etimer et_delivery;
#endif
				static struct etimer et_pause;
				struct route_entry *e = NULL;
				
				no_route = (route_lookup(&dest) == NULL);
				while (!route_lookup(&dest))
				{
					route_discovery_discover(&rc, &dest, ROUTE_DISCOVERY_TIMEOUT);
					PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);
				}
				if (no_route)
				{
					etimer_set(&et_pause, PAUSE_TIME);
					PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et_pause));				
				}
				e = route_lookup(&dest);
				printf("MULTIHOP_SEND: Sending packet toward %d.%d via %d.%d.\n", 
					dest.u8[0], dest.u8[1],
					e->nexthop.u8[0],
					e->nexthop.u8[1]
				);
				
#if ACKNOWLEDGEMENT
				acknowledged = 0;
#endif

				packetbuf_clear();
				packetbuf_set_datalen(sizeof(struct packet));
				packet = packetbuf_dataptr();
				packet->ack = 0;
				write_packet(packet);
				
				multihop_send(&mc, &dest);

#if ACKNOWLEDGEMENT
				etimer_set(&et_delivery, SOURCE_ACK_WAIT_TIME);
				PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et_delivery) || ev == PROCESS_EVENT_CONTINUE);
				/* If the packet is anknowledged, break from the loop. */
				if (acknowledged)
				{
					printf("MULTIHOP_ACK: Packet is acknowledged.\n");
					break;
				}
				printf("MULTIHOP_ACK: Packet is NOT acknowledged. Removing invalid route.\n");
				/* 
					Otherwise, remove the route from route table and 
					continue with the loop. 
				*/
				remove_route_to(&dest);
#endif
			}
		}

		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

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
			printf("Button 1 is pressed, sensors reporting has %s.\n", disp_switch?"started":"stopped");
		}
		else if (sensor == &button_2_sensor) {
			// switch which value to be displayed
			disp_value = !disp_value;
			printf("Button 2 is pressed, switching to %s reporting.\n", disp_value?"light":"temperature");
		}
	}
	PROCESS_END();

}

PROCESS_THREAD(net_pressure_handler, ev, data)
{
	// define variables
	int i;
	//uint16_t time_wait_count = 100;
	struct packet *packet;

	static clock_time_t count, start_count, end_count, diff;
	static struct etimer et;

	rimeaddr_t dest;
	dest.u8[0] = 0xFF;
	dest.u8[1] = 0xEE;
	PROCESS_EXITHANDLER(multihop_close(&mc);)
	PROCESS_BEGIN();

	//int time_wait_count = 100; //count 4000 = 1s
	multihop_open(&mc, MULTIHOP_CHANNEL, &multihop_callbacks);
	// wait 2 seconds
	//etimer_set(&et, CLOCK_SECOND * 2);
	//PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
	// get start time; also clock_time_t clock_time()

	while (1) {
		route_discovery_discover(&rc, &dest, ROUTE_DISCOVERY_TIMEOUT);
		etimer_set(&et, CLOCK_SECOND * 1);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

		if (route_lookup(&dest))
		{
			printf("The path is found.\n");
			break;
		}
	}


	rtimer_init();
	start_count = clock_time();

	for (i = 0; i <= 99; i++) {

		//wait a little time to control sending speed
		//delay_usecond(100);
		pressure_send_packet(&dest);
		clock_delay(100);
		//printf("%d\n",i+1);

	}
	end_count = clock_time();
	diff = end_count - start_count;
	printf("ticks = [~%u ms]\n", diff * 8);
	PROCESS_END();

}

