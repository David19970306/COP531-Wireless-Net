#ifndef __MULTIHOP_CALLBACKS_H__
#define __MULTIHOP_CALLBACKS_H__


#include "net/rime/multihop.h"


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