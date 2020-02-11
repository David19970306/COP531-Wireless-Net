#include "contiki.h"
#include "net/rime.h"

#include "dev/button-sensor.h"

#include "dev/leds.h"

#include <stdio.h>

/*---------------------------------------------------------------------------*/
PROCESS(dest_process, "Destination");
AUTOSTART_PROCESSES(&dest_process);
/*---------------------------------------------------------------------------*/
#define ROUTE_DISCOVERY_TIMEOUT 60 * CLOCK_SECOND
static const struct route_discovery_callbacks route_discovery_callbacks = { NULL, NULL };
static struct route_discovery_conn rc;
static clock_time_t time;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(dest_process, ev, data)
{
  PROCESS_EXITHANDLER(route_discovery_close(&rc);)
    
  PROCESS_BEGIN();

  time = clock_time();

  route_discovery_open(&rc, time, 146, &route_discovery_callbacks);

  while(1) {
    static struct etimer et;
    
    etimer_set(&et, CLOCK_SECOND);
    
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

  }

  PROCESS_END();
}


