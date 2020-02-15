#include <stdio.h>
#include "contiki.h"
#include "dev/button-sensor.h"
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
AUTOSTART_PROCESSES(&source_process);
/*---------------------------------------------------------------------------*/
static struct multihop_conn mc;
static struct route_discovery_conn rc;
/*---------------------------------------------------------------------------*/int
send_packet(const rimeaddr_t *dest)
{
  struct packet *packet;
  
  packetbuf_clear();
  packetbuf_set_datalen(sizeof(struct packet));
  packet = packetbuf_dataptr();
  packet->ack = 0;
  packet->battery = get_battery_voltage();
  packet->light = get_light();

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
static clock_time_t time;
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


    if (send_packet(&dest))
    {
	  printf("MULTIHOP_SEND: Sending packet toward %d.%d.\n", dest.u8[0], dest.u8[1]);
      continue;
    }
    route_discovery_discover(&rc, &dest, ROUTE_DISCOVERY_TIMEOUT);

  }

  PROCESS_END();
}


