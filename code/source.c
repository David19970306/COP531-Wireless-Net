#include "contiki.h"
#include "net/rime.h"

#include "dev/button-sensor.h"

#include "dev/leds.h"

#include <stdio.h>

/*---------------------------------------------------------------------------*/
PROCESS(source_process, "Source");
AUTOSTART_PROCESSES(&source_process);
/*---------------------------------------------------------------------------*/
#define ROUTE_DISCOVERY_TIMEOUT 60 * CLOCK_SECOND
static const struct route_discovery_callbacks route_discovery_callbacks = { NULL, NULL };
static struct route_discovery_conn rc;
static clock_time_t time;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(source_process, ev, data)
{
  PROCESS_EXITHANDLER(route_discovery_close(&rc);)
    
  PROCESS_BEGIN();

  time = clock_time();

  route_discovery_open(&rc, time, 146, &route_discovery_callbacks);

  while(1) {
    static struct etimer et;
    rimeaddr_t dest;
    
    etimer_set(&et, CLOCK_SECOND);
    
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    dest.u8[0] = 0x01;
    dest.u8[1] = 0x00;
    route_discovery_discover(&rc, &dest, ROUTE_DISCOVERY_TIMEOUT);

  }

  PROCESS_END();
}


