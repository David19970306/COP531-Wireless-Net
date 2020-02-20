#include <stdio.h>
#include "net/rime/rimeaddr.h"
#include "net/packetbuf.h"

#include "multihop-callbacks.h"
#include "packet.h"
#include "config.h"
#include "route.h"
#include "util.h"

extern uint8_t verbose;

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

  route_entry = route_lookup_nexthop(originator, prevhop);
  if (hops!=0 && route_entry)
  {
    route_entry->time = 0;
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
  if (prevhop)
  {
	  if (verbose) printf("MULTIHOP_FORWARD: orig %d.%d dest %d.%d last %d.%d hops %d\n",
		originator->u8[0], originator->u8[1],
		dest->u8[0], dest->u8[1],  
		prevhop->u8[0], prevhop->u8[1],
		hops
	  );
  }

  return nexthop;
}


