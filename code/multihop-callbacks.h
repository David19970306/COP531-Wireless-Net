#ifndef __MULTIHOP_CALLBACKS_H__
#define __MULTIHOP_CALLBACKS_H__


#include "net/rime/multihop.h"

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
        uint8_t hops);

void
multihop_received(struct multihop_conn *ptr,
  const rimeaddr_t *sender,
  const rimeaddr_t *prevhop,
  uint8_t hops);

#endif