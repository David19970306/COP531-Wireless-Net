#ifndef __PACKET_H__
#define __PACKET_H__

struct packet {
	uint8_t ack;
	uint16_t light;
	uint16_t battery;
	uint16_t temperature;
	uint8_t disp;
	uint16_t route_index;
};

#endif