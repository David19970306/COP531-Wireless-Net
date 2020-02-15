#include "contiki.h"
#include <stdio.h> 
#include "contiki.h"
#include "contiki-conf.h"
#include "dev/sensinode-sensors.h"
#include "net/rime.h"
#include "dev/watchdog.h"
#include "lib/random.h"

#include "sensor_value.h"//reference the header file from ownself

PROCESS(hello_world_process, "Hello world process");


AUTOSTART_PROCESSES(&hello_world_process);

PROCESS_THREAD(hello_world_process, ev, data)
{
    static struct etimer timer;
	uint16_t res;
    PROCESS_BEGIN();
    
    etimer_set(&timer, CLOCK_CONF_SECOND * 2);

    while (1)
    {
       
         PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);

       
            printf("Hello World!\r\n");
			res = getLight();
			printf("light:%d \r\n",res);

			res = getTemp();
			printf("temp:%d \r\n", res);

			res = getBattVol();
			printf("batt:%d \r\n", res);

			res = getSupplyVol();
			printf("supply:%d \r\n", res);
            etimer_reset(&timer);
       
    }

    PROCESS_END();
}
