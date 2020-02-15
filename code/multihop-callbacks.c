#include <stdio.h>

#include "multihop-callbacks.h"
#include "packet.h"
#include "config.h"
#include "route.h"
#include "util.h"

rimeaddr_t *
multihop_forward(struct multihop_conn *ptr,
        const rimeaddr_t *originator,
        const rimeaddr_t *dest,
        const rimeaddr_t *prevhop,
        uint8_t hops)
{
  rimeaddr_t *nexthop;
  struct route_entry *route_entry;

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

  printf("MULTIHOP_FORWARD: orig %d.%d dest %d.%d last %d.%d hops %d\n",
    originator->u8[0], originator->u8[1],
    dest->u8[0], dest->u8[0],  
    prevhop->u8[0], prevhop->u8[0],
    hops
  );

  return nexthop;
}

void
multihop_received(struct multihop_conn *ptr,
  const rimeaddr_t *sender,
  const rimeaddr_t *prevhop,
  uint8_t hops)
{
  struct packet *packet = packetbuf_dataptr();
  float battery;
  float light;

  printf("MULTIHOP_RECEIVED: orig %d.%d last %d.%d hops %d\n",
    sender->u8[0], sender->u8[1],
    prevhop->u8[0], prevhop->u8[0],
    hops
  );
  battery = packet->battery;
  battery /= 100.0;
  light = packet->light;
  light /= 100.0;
  printf("DATA_PACKET: light %d.%02u battery %d.%02u\n", 
    get_decimal(light), get_fraction(light),
	get_decimal(battery), get_fraction(battery)
  );
}
