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


//#if PRESSURE_MODE
//PROCESS(net_pressure_handler, "The test of Net pressure.");
//AUTOSTART_PROCESSES(&net_pressure_handler);
//#else
PROCESS(source_process, "Source");
PROCESS(button_stats, "Change state of the button");
AUTOSTART_PROCESSES(&source_process, &button_stats);
//#endif
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

	//if (dbg) printf("Sending data content:batt[%u]light[%u]temp[%u];d[%u]\n",
	//	packet->battery, packet->light, packet->temperature, packet->disp);
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
mock_write_packet(struct packet *packet)
{
	packet->group_num = GROUP_NUMBER;
	packet->battery = 999;
	packet->light = 999;
	packet->temperature = 999;
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
	
#if PRESSURE_MODE
	static int i = 0;
	static int send_count = 0;
	static clock_time_t count, start_count, end_count, diff;
	rimeaddr_t dest;
	dest.u8[0] = 0xff;
	dest.u8[1] = 0xee;
#else
	rimeaddr_t dest;
	dest.u8[0] = 0xdf;
	dest.u8[1] = 0xdf;
#endif
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
#if PRESSURE_MODE
		// process wait time
		etimer_set(&et, 200);  // 4000 is 1s
		rtimer_init();
		start_count = clock_time();
#else
		etimer_set(&et, SOURCE_PERIOD);
#endif

		if (disp_switch)
		{
			printf("MULTIHOP_SEND: Sending packet toward %d.%d.\n", 
				dest.u8[0], dest.u8[1]
			);

			while (1)
			{
#if ACKNOWLEDGEMENT
				static struct etimer et_delivery;
#endif
				
				while (!route_lookup(&dest))
				{
					route_discovery_discover(&rc, &dest, ROUTE_DISCOVERY_TIMEOUT);
					PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);
				}

#if ACKNOWLEDGEMENT
				acknowledged = 0;
#endif

				packetbuf_clear();
				packetbuf_set_datalen(sizeof(struct packet));
				packet = packetbuf_dataptr();
#if PRESSURE_MODE
				packet->ack = PRESSURE_UNIQ_NUMBER;
				mock_write_packet(packet);
				send_count++;
#else
				packet->ack = 0;
				write_packet(packet);
#endif
				multihop_send(&mc, &dest);

#if ACKNOWLEDGEMENT
				etimer_set(&et_delivery, 10);
				PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et_delivery) || ev == PROCESS_EVENT_CONTINUE);
				/* If the packet is anknowledged, break from the loop. */
				if (acknowledged)
				{
					break;
				}
				/* 
					Otherwise, remove the route from route table and 
					continue with the loop. 
				*/
				remove_route_to(&dest);
#endif
			}
		}

		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
#if PRESSURE_MODE
	i++;
	if (i >= 2000) {
		// restart the counter
		static struct etimer et;
		struct packet *packet;
		end_count = clock_time();
		diff = end_count - start_count;
		printf("ticks = [~%u ms]\n", diff * 8);
		packet = packetbuf_dataptr();
		packet->ack = 0;
		mock_write_packet(packet);
		multihop_send(&mc, &dest);
		printf("DataPacketACK:[%u]", send_count);
		// wait 2 second unitl next rount
		etimer_set(&et, CLOCK_SECOND * 2);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
		rtimer_init();
		start_count = clock_time();
		i = 0;
		send_count = 0;
	}
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
			disp_switch = disp_switch ? 0:1;
			printf("Disp Button Pressed, Val:[%d]\n", disp_switch);
		}
		else if (sensor == &button_2_sensor) {
			// switch which value to be displayed
			disp_value = disp_value ? 0:1;
			printf("Switch Value Button Pressed, Val:[%d]\n", disp_value);
		}
	}
	PROCESS_END();

}

//PROCESS_THREAD(net_pressure_handler, ev, data)
//{
//	// define variables
//	int i = 0;
//	//uint16_t time_wait_count = 100;
//	struct packet *packet;
//
//	static clock_time_t count, start_count, end_count, diff;
//	static struct etimer et;
//
//	rimeaddr_t dest;
//	dest.u8[0] = 0xFF;
//	dest.u8[1] = 0xEE;
//
//	PROCESS_EXITHANDLER(route_discovery_close(&rc); multihop_close(&mc);)
//	PROCESS_BEGIN();
//	time = clock_time();
//	route_discovery_open(&rc, time, ROUTE_DISCOVERY_CHANNEL, &route_discovery_callbacks);
//	multihop_open(&mc, MULTIHOP_CHANNEL, &multihop_callbacks);
//
//	route_init();
//	route_set_lifetime(ROUTE_LIFETIME);
//	// process wait time
//	etimer_set(&et, 100);  // 4000 is 1s
//	rtimer_init();
//	start_count = clock_time();
//
//
//	while (1) {
//#if ACKNOWLEDGEMENT
//		static struct etimer et_delivery;
//#endif
//
//		while (!route_lookup(&dest))
//		{
//			route_discovery_discover(&rc, &dest, ROUTE_DISCOVERY_TIMEOUT);
//			PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);
//		}
//
//#if ACKNOWLEDGEMENT
//		acknowledged = 0;
//#endif
//
//		packetbuf_clear();
//		packetbuf_set_datalen(sizeof(struct packet));
//		packet = packetbuf_dataptr();
//		packet->ack = PRESSURE_UNIQ_NUMBER;
//		mock_write_packet(packet);
//
//		multihop_send(&mc, &dest);
//
//#if ACKNOWLEDGEMENT
//		etimer_set(&et_delivery, SOURCE_ACK_WAIT_TIME);
//		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et_delivery) || ev == PROCESS_EVENT_CONTINUE);
//		/* If the packet is anknowledged, break from the loop. */
//		if (acknowledged)
//		{
//			break;
//		}
//		/*
//			Otherwise, remove the route from route table and
//			continue with the loop.
//		*/
//		remove_route_to(&dest);
//#endif
//	}
//}
//	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
//	i++;
//	if(i == 99){
//		// restart the counter
//		packet->ack = 0;
//		multihop_send(&mc, &dest);
//		end_count = clock_time();
//		diff = end_count - start_count;
//		printf("ticks = [~%u ms]\n", diff * 8);
//		// wait 2 second unitl next rount
//		etimer_set(&et, CLOCK_SECOND * 2);
//		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
//		rtimer_init();
//		i = 0;
//	}
//
//	//for (i = 0; i <= 99; i++) {
//
//	//	//wait a little time to control sending speed
//	//	pressure_send_packet(&dest);
//	//	clock_delay(100);
//	//}
//	PROCESS_END();
//
//}

