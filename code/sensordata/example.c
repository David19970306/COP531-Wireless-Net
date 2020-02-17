#include "contiki.h"
#include "net/rime.h"
#include <string.h>
#include "dev/button-sensor.h"
#include "dev/sensinode-sensors.h"
#include "dev/leds.h"

#include <stdio.h>
#define TABLELENGTH        10


#define COMMAND_ROUTREQUEST   0x20   //Command for requesting route
#define COMMAND_ROUTERESPONSE 0x21   //Command for route response
#define COMMAND_DTATTX        0x22   //Command for sending data through unicast

#define BATTERY_AVERGE_LVL    3000

typedef struct
{
	uint16_t u16Dest;                   //The destation address 
	uint16_t u16NextHop;                //The next hop which point to the destination address
	uint16_t u16Battery;
	uint16_t u16Rssi;
} tsRouteTable;


static tsRouteTable sRouteTable[TABLELENGTH];

static rimeaddr_t addr;
static uint8_t destination;
static struct unicast_conn uc;
static struct broadcast_conn bc;
static uint8_t u8DataBuffer[50];

static const struct broadcast_callbacks broadcast_callbacks = { recv_bc };
static const struct unicast_callbacks unicast_callbacks = { recv_uc };


static int rv;
static struct sensors_sensor * sensor;
static float sane = 0;
static uint16_t battery;
static uint8_t temperature1 = 0;
static uint8_t temperature2 = 0;

static uint8_t brdcstCounter = 1;
static uint8_t brdcstLimit = 4;
static uint8_t brdcstID = 1;
/*---------------------------------------------------------------------------*/
PROCESS(routenode_process, "Example unicast");
AUTOSTART_PROCESSES(&routenode_process);
/*---------------------------------------------------------------------------*/

static void
recv_uc(struct unicast_conn *c, const rimeaddr_t *from)
{

	uint8_t * data;
	uint16_t dest = 0;
	uint16_t nexthop = 0;
	uint16_t src = 0;
	uint16_t battery = 0;
	uint16_t rssi = 0;
	static int i = 0;
	static int m = 0;

	unsigned int bSuccess = 0;
	unsigned int bFound = 0;

	data = packetbuf_dataptr();

	switch (data[0])
	{
	case COMMAND_ROUTERESPONSE:
		bSuccess = 0;
		dest = data[1];
		dest = dest << 8;
		dest = dest | data[2];

		src = from->u8[1];
		src = src << 8;
		src = src | from->u8[0];

		battery = data[4];
		battery = battery << 8;
		battery = battery | data[3];
		//printf("has respo %d%d\r\n",from->u8[0],from->u8[1],battery);
		rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);

		for (i = 0; i < TABLELENGTH; i++)
		{
			if (sRouteTable[i].u16Dest == dest)
			{
				bSuccess = 1;

				if (sRouteTable[i].u16NextHop == src)
				{
					sRouteTable[i].u16Rssi = rssi;
					sRouteTable[i].u16Battery = battery;
				}
				else {
					if (battery > BATTERY_AVERGE_LVL)
					{

						//printf("Received rssi=%d, from %d\r\n",rssi,src);
						if (rssi > sRouteTable[i].u16Rssi)
						{
							//printf("stored rssi=%d, from %d\r\n",sRouteTable[i].u16Rssi,sRouteTable[i].u16NextHop);
							sRouteTable[i].u16NextHop = src;
							sRouteTable[i].u16Rssi = rssi;
							sRouteTable[i].u16Battery = battery;
						}
					}
					else {
						if (battery > sRouteTable[i].u16Battery)
						{
							sRouteTable[i].u16NextHop = src;
							sRouteTable[i].u16Rssi = rssi;
							sRouteTable[i].u16Battery = battery;
						}
					}
					break;
				}
			}
		}

		if (!bSuccess)
		{
			for (i = 0; i < TABLELENGTH; i++)
			{
				if (sRouteTable[i].u16Dest == 0x0000)
				{
					sRouteTable[i].u16Dest = dest;
					sRouteTable[i].u16NextHop = src;
					sRouteTable[i].u16Rssi = rssi;
					sRouteTable[i].u16Battery = battery;
				}
			}
		}
		break;

	default:
		break;
	}

	packetbuf_clear();
}

static void
recv_bc(struct broadcast_conn *c, rimeaddr_t *from)
{


	/*from->u8[0],
	from->u8[1],
	packetbuf_datalen(),
	(char *)packetbuf_dataptr());*/

	packetbuf_clear();
}


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(routenode_process, ev, data)
{
	static struct etimer et;
	static uint8_t i = 0;
	static uint8_t m = 0;
	static int dec;
	static float frac;
	static uint16_t u16Dest = 0xffff;
	static uint8_t bFound = 0;

	PROCESS_EXITHANDLER(unicast_close(&uc);)
		PROCESS_BEGIN();

	for (i = 0; i < TABLELENGTH; i++)
	{
		sRouteTable[i].u16Dest = 0x0000;
		sRouteTable[i].u16NextHop = 0xffff;
		sRouteTable[i].u16Battery = 0;
		sRouteTable[i].u16Rssi = 0;
	}


	puts("start");
	broadcast_open(&bc, 128, &broadcast_callbacks);
	unicast_open(&uc, 129, &unicast_callbacks);


	etimer_set(&et, CLOCK_SECOND * 2);
	while (1)
	{

		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
		if (i == 0)
		{
			sensor = sensors_find(ADC_SENSOR);

			rv = sensor->value(ADC_SENSOR_TYPE_TEMP);
			if (rv != -1) {
				sane = ((rv * 0.61065 - 773) / 2.45);
				dec = sane;
				temperature1 = dec;
				frac = sane - dec;
				temperature2 = (unsigned int)(frac * 100);
				//printf("  Temp=%d.%02u C (%d)\n\r", dec, (unsigned int)(frac*100), rv);
			}


			rv = sensor->value(ADC_SENSOR_TYPE_VDD);

			if (rv != -1) {
				sane = rv * 3.75 / 2047;
				battery = sane * 1000;

				//printf("  Supply=%d\n\r", battery);
				u8DataBuffer[0] = COMMAND_ROUTREQUEST;
				u8DataBuffer[1] = u16Dest >> 8;
				u8DataBuffer[2] = u16Dest;
				u8DataBuffer[3] = brdcstCounter;
				u8DataBuffer[4] = brdcstLimit;
				u8DataBuffer[5] = brdcstID;
				brdcstID++;
				packetbuf_copyfrom(u8DataBuffer, 6);
				broadcast_send(&bc);
				//printf("brdcst");
			}
		}
		else {

			for (m = 0; m < TABLELENGTH; m++)
			{
				if (u16Dest == sRouteTable[m].u16Dest)
				{
					bFound = 1;

					break;
				}
			}

			if (bFound) {
				u8DataBuffer[0] = COMMAND_DTATTX;
				u8DataBuffer[1] = u16Dest >> 8;
				u8DataBuffer[2] = u16Dest;
				u8DataBuffer[3] = rimeaddr_node_addr.u8[0];
				u8DataBuffer[4] = rimeaddr_node_addr.u8[1];
				u8DataBuffer[5] = temperature1;
				u8DataBuffer[6] = temperature2;

				u8DataBuffer[7] = battery >> 8;
				u8DataBuffer[8] = battery;
				packetbuf_copyfrom(u8DataBuffer, 9);
				addr.u8[0] = sRouteTable[m].u16NextHop;
				addr.u8[1] = sRouteTable[m].u16NextHop >> 8;
				//printf("next %d%d",addr.u8[0],addr.u8[1]);
				unicast_send(&uc, &addr);
			}
		}

		if (i == 0)
		{
			etimer_set(&et, CLOCK_SECOND * 2);
			i = 1;
		}
		else {
			etimer_set(&et, CLOCK_SECOND * 1);
			i = 0;
		}

	}

	PROCESS_END();
}
