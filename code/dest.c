#include <stdio.h>
#include "contiki.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"

#include "route-discovery.h"
#include "multihop-callbacks.h"
#include "config.h"

/*---------------------------------------------------------------------------*/
PROCESS(dest_process, "Destination");
AUTOSTART_PROCESSES(&dest_process);
/*---------------------------------------------------------------------------*/
static struct route_discovery_conn rc;
static const struct route_discovery_callbacks route_discovery_callbacks = { NULL, NULL };
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


  }

  PROCESS_END();
}


