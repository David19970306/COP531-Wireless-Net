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
AUTOSTART_PROCESSES(&source_process, &button_stats, &net_pressure_handler);
/*---------------------------------------------------------------------------*/
static struct multihop_conn mc;
static struct route_discovery_conn rc;
/*---------------------------------------------------------------------------*/
static uint8_t disp_value = 0;//change the display value (Temperature or Light)
static uint8_t disp_switch = 0;//Switch the mode of sending data.(turn on/off send periodically)
static clock_time_t time;
/*---------------------------------------------------------------------------*/
// sensinode sensors
extern const struct sensors_sensor button_1_sensor, button_2_sensor;
static uint8_t dbg = 1;

/*---------------------------------------------------------------------------*/
int
send_packet(const rimeaddr_t *dest, uint8_t disp_value)
{
  struct packet *packet;
  
  packetbuf_clear();
  packetbuf_set_datalen(sizeof(struct packet));
  packet = packetbuf_dataptr();
  packet->ack = 0;
  packet->battery = get_battery_voltage();
  packet->light = get_light();
  packet->temp = get_temp();
  packet->disp = disp_value;

  if(dbg) printf("Sending data content:batt[%u]light[%u]temp[%u];d[%u]\n",
	  packet->battery, packet->light, packet->temp, packet->disp);
  return multihop_send(&mc, dest);
}
/*---------------------------------------------------------------------------*/
void 
route_discovery_new_route(struct route_discovery_conn *c, const rimeaddr_t *to)
{
  printf("ROUTE_DISCOVERY: New route to %d.%d is found.\n",
    to->u8[0], to->u8[1]
  );
}
void
route_discovery_timedout(struct route_discovery_conn *c)
{
  printf("ROUTE_DISCOVERY: Cannot discover route.\n");
}
static const struct route_discovery_callbacks route_discovery_callbacks = { 
  route_discovery_new_route, route_discovery_timedout 
};
/*---------------------------------------------------------------------------*/
static const struct multihop_callbacks multihop_callbacks = { 
  multihop_received, multihop_forward 
};
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(source_process, ev, data)
{
  rimeaddr_t dest;
  dest.u8[0] = 0xdf;
  dest.u8[1] = 0xdf;
  PROCESS_EXITHANDLER(route_discovery_close(&rc); multihop_close(&mc);)
    
  PROCESS_BEGIN();

  time = clock_time();

  route_discovery_open(&rc, time, ROUTE_DISCOVERY_CHANNEL, &route_discovery_callbacks);
  multihop_open(&mc, MULTIHOP_CHANNEL, &multihop_callbacks);
  
  route_init();
  route_set_lifetime(SOURCE_ROUTE_LIFETIME);
  

  while(1) {

    static struct etimer et;

    etimer_set(&et, SOURCE_PERIOD);
    
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

	if (disp_switch) {
		// send packet
		if (send_packet(&dest, disp_value))
		{
			printf("MULTIHOP_SEND: Sending packet toward %d.%d.\n", dest.u8[0], dest.u8[1]);
			continue;
		}
		route_discovery_discover(&rc, &dest, ROUTE_DISCOVERY_TIMEOUT);
	}

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

PROCESS_THREAD(net_pressure_handler, ev, data)
{
	// define variables
	int i;
	rimeaddr_t dest;
	uint16_t delay_usecond;
	delay_usecond = 100;
	struct packet *packet;
	uint32_t start_tick;
	uint32_t end_tick;

	PROCESS_EXITHANDLER(multihop_close(&mc);)
	PROCESS_BEGIN();

	int time_wait_count = 100; //count 4000 = 1s
	time = clock_time();
	multihop_open(&mc, MULTIHOP_CHANNEL, &multihop_callbacks);
	// wait 2 seconds
	etimer_set(&et, CLOCK_SECOND * 2);
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
	// get start time; also clock_time_t clock_time()
	start_tick = RTIMER_NOW();

	for (i = 0; i <= 99; i++) {
		//wait a little time to control sending speed
		delay_usecond(delay_usecond);
		packetbuf_clear();
		packetbuf_set_datalen(sizeof(struct packet));
		packet = packetbuf_dataptr();
		multihop_send(&mc, dest);
	}
	end_tick = RTIMER_NOW();
	printf("Elapse tick:[%u]", end_tick - start_tick);
	PROCESS_END();

}

