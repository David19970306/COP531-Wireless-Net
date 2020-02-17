#include <stdio.h>
#include "net/rime/rimeaddr.h"
#include "net/packetbuf.h"

#include "multihop-callbacks.h"
#include "packet.h"
#include "config.h"
#include "route.h"
#include "util.h"

struct node {
  rimeaddr_t addr;
  // uint16_t battery;
  // uint16_t rssi;
};

rimeaddr_t *
multihop_forward(struct multihop_conn *ptr,
        const rimeaddr_t *originator,
        const rimeaddr_t *dest,
        const rimeaddr_t *prevhop,
        uint8_t hops)
{
  rimeaddr_t *nexthop;
  struct route_entry *route_entry;
  struct packet *packet;
  struct node *current_node;

  /* Drop if number of hops is larger than MULTIHOP_MAX_HOPS. */
  if (hops > MULTIHOP_MAX_HOPS)
  {
    return NULL;
  }

  route_entry = route_lookup(dest);
  if (!route_entry)
  {
	  return NULL;
  }
  nexthop = &route_entry->nexthop;

  /* Drop if nexthop is the same as prevhop or originator to prevent loops. */
  if(rimeaddr_cmp(nexthop, prevhop) || rimeaddr_cmp(nexthop, originator)) {
    return NULL;
  }
  
  
  packetbuf_set_datalen(sizeof(struct packet) + (hops + 1) * sizeof(struct node));
  packet = packetbuf_dataptr();
  
  if (packet->group_num != GROUP_NUMBER)
  {
	  return NULL;
  }
  
  current_node = (void *) (packet + 1);
  current_node += hops;
  if (hops == 0)
  {
	  packet->route_index = route_entry->cost * 100;
  }
  // current_node->battery = get_battery_voltage();
  rimeaddr_copy(&current_node->addr, &rimeaddr_node_addr);
  // current_node->rssi = (hops==0) ? 0 : packetbuf_attr(PACKETBUF_ATTR_RSSI);

  printf("MULTIHOP_FORWARD: orig %d.%d dest %d.%d last %d.%d hops %d\n",
    originator->u8[0], originator->u8[1],
    dest->u8[0], dest->u8[0],  
    prevhop->u8[0], prevhop->u8[0],
    hops
  );

  return nexthop;
}

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
  uint8_t type;
  uint8_t ack;
  extern count;
  extern disp_switch;
  float battery;
  float light;
  float temperature;
  struct node *first_node = (void *) (packet + 1);
  
  if (packet->group_num != GROUP_NUMBER)
  {
	  return;
  }
  
  multihop_print_route(first_node, hops, packet->route_index);

  // printf("MULTIHOP_RECEIVED: orig %d.%d last %d.%d hops %d\n",
    // sender->u8[0], sender->u8[1],
    // prevhop->u8[0], prevhop->u8[0],
    // hops
  // );
  ack = packet->ack;
  if (ack == PRESSURE_UNIQ_NUMBER) {
	  count++;
  }
  else {
	  count = 0;
  }
  type = packet->disp;
  battery = packet->battery;
  battery /= 100.0;
  light = packet->light;
  light /= 100.0;
  temperature = packet->temperature;
  temperature /= 100.0;
  if (disp_switch) {
	  if (type) {
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
  }
 // printf("DATA_PACKET: light %d.%02u battery %d.%02uV Temperature %d.%02uC\n", 
 //   get_decimal(light), get_fraction(light),
	//get_decimal(battery), get_fraction(battery),
	//get_decimal(temperature), get_fraction(temperature)
 // );
}
