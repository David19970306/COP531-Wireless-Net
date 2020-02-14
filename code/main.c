/*
 * author: Andrea Milanta
 */

/**************************************************************/
/*-------------------------INCLUDES---------------------------*/
// useful libraries
#include "random.h"

// contiki
#include "contiki.h"
#include "net/rime.h"
#include "dev/leds.h"
#include "dev/button-sensor.h"

// standard lib
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// AODV
//#include "struct2packet.h"

/******************************************************************/
 /*-------------------VALUES-------------*/
#define INF 50                 // Infinite

/*-------------------FIXED SIZES--------*/
#define DATA_PAYLOAD_LEN 11     // length of payload in data packages


/******************************************************************/
/*-------------------------DATA STRUCTURES------------------------*/

/*--------------------PACKETS-----------------*/
// data packet
struct DATA_PACKET {
    uint8_t group_hdr;
	rimeaddr_t dest;
	char payload[DATA_PAYLOAD_LEN];
};

// route request packet
struct RREQ_PACKET {
    uint8_t group_hdr;
	uint8_t req_id;
	rimeaddr_t dest;
	rimeaddr_t src;
};

// route reply packet
struct RREP_PACKET {
    uint8_t group_hdr;
	uint8_t req_id;
	rimeaddr_t dest;
	rimeaddr_t src;
	int hops;
};

/*--------------------TABLES-----------------*/
// routing table entry
struct ROUTING_TABLE_ENTRY {
    uint8_t group_hdr;
	rimeaddr_t dest;
	rimeaddr_t next;
	int hops;       // number of hops to destination
	int age;        // age of current entry
	int valid;      // bool: is the current entry valid?
};

// waiting table entry (waiting for route reply)
struct DISCOVERY_TABLE_ENTRY {
	uint8_t req_id;
	rimeaddr_t src;
	rimeaddr_t dest;
	int snd;
	int valid;
	int age;
};

// queue entry data packages to be sent
struct QUEUE_ENTRY {
	struct DATA_PACKET data_pkg;
	int age;
	int valid;
};

/*------------------------------DEFINE----------------------------*/

/*-------------------SEPARATORS------------------*/
#define ITEM_SEP ";"
#define VALUES_SEP ":"

/*-------------------PACKETS HEADER---------------*/
#define DATA_HEADER "DATA"
#define RREQ_HEADER "ROUTE_REQUEST"
#define RREP_HEADER "ROUTE_REPLY"

/*-------------------VALUE REPRESENTATION---------*/    // DO NOT MODIFY!!
#define NODE_REP "%2d"      // DO NOT MODIFY!!
#define HOPS_REP "%2d"      // DO NOT MODIFY!!
#define ID_REP "%2d"        // DO NOT MODIFY!!
#define PAYLOAD_REP "%s"    // DO NOT MODIFY!!

/*-------------------ITEM REPRESENTATION----------*/    // DO NOT MODIFY!!
#define DEST    "DEST"    VALUES_SEP NODE_REP
#define SRC     "SRC"     VALUES_SEP NODE_REP
#define HOPS    "HOPS"    VALUES_SEP HOPS_REP
#define REQ_ID  "REQ_ID"  VALUES_SEP ID_REP
#define REP_ID  "REQ_ID"  VALUES_SEP ID_REP
#define PAYLOAD "PAYLOAD" VALUES_SEP PAYLOAD_REP

/*-------------------PACKAGES REPRESENTATION------*/    // DO NOT MODIFY!!
#define DATA_REP DATA_HEADER ITEM_SEP DEST   ITEM_SEP PAYLOAD
#define RREQ_REP RREQ_HEADER ITEM_SEP REQ_ID ITEM_SEP DEST ITEM_SEP SRC ITEM_SEP
#define RREP_REP RREP_HEADER ITEM_SEP REP_ID ITEM_SEP DEST ITEM_SEP SRC ITEM_SEP HOPS ITEM_SEP

/*-------------------PACKAGES LENGTH--------------*/    // DO NOT MODIFY!!
#define DATA_PACKET_LEN sizeof(DATA_REP)-1 - 3 + DATA_PAYLOAD_LEN
#define RREQ_PACKET_LEN sizeof(RREQ_REP)-1 - 3
#define RREP_PACKET_LEN sizeof(RREP_REP)-1 - 4



/******************************************************************/
/*-----------------------FUNCTION PROTOTYPES----------------------*/

/*-------------------struct to packet------*/
void data2packet(struct DATA_PACKET* data, char* packet);
void rreq2packet(struct RREQ_PACKET* rreq, char* packet);
void rrep2packet(struct RREP_PACKET* rrep, char* packet);

/*-------------------packet to struct------*/
char packet2data(char* packet, struct DATA_PACKET* data);
char packet2rreq(char* packet, struct RREQ_PACKET* rreq);
char packet2rrep(char* packet, struct RREP_PACKET* rrep);



/**************************************************************************/
/*------------------------DEFINES-----------------------------------------*/

/*-----------NODES---------------------------*/
#define MAX_NODES 3     // total number of nodes 
#define MAX_DATA_IN_QUEUE 10    // Maximum data packages waiting to be sent
#define DISCO_SIZE MAX_NODES*MAX_NODES//------------ DO NOT MODIFY!!

/*-----------CHANNELS------------------------*/
#define BROADCAST_CHANNEL 26
#define RREP_CHANNEL 22
#define DATA_CHANNEL 23
#define RREQ_CHANNEL BROADCAST_CHANNEL //------------ DO NOT MODIFY!!

/*-----------TIME CONSTRAINTS----------------*/
#define ROUTE_DISCOVERY_TIME 1   // maximum time to obtain route to a destination
#define ROUTE_EXPIRATION_TIME 90   // maximum time a route entry is considered valid
#define DATA_PACKAGE_DELTA_TIME 30
#define MAX_QUEUEING_TIME 5    // Maximum time for a data package to remain in the queue before being discarded


/**************************************************************************/
/*-------------------------FUNCTION PROTOTYPES----------------------------*/

// Callback functions
static void route_reply_callback(struct unicast_conn *, const rimeaddr_t *);
static void data_callback(struct unicast_conn *, const rimeaddr_t *);
static void route_request_callback(struct broadcast_conn *, const rimeaddr_t *);

// Communication functions
static void sendrrep(struct RREP_PACKET* rrep, int next);
static void senddata(struct DATA_PACKET* data, int next);
static void sendrreq(struct RREQ_PACKET* rreq);

// Tables support functions
static char updateTables(struct RREP_PACKET * rrep, int from);
static int getNext(int dest);
// static void addEntryToRoutingTable(int dest);
static void addEntryToDiscoveryTable(struct DISCOVERY_TABLE_ENTRY* rreq_info);
// static int getrrepSender(struct RREP_PACKET* rrep);
static void clearDiscoveryEntry(struct RREP_PACKET* rrep);
static char isDuplicateReq(struct RREQ_PACKET* rreq);
static char enque(struct DATA_PACKET* data);

// Support functions
static void getRandomPayload(char payload[DATA_PAYLOAD_LEN]);

// Visualization functions
static void printRoutingTable();
static void printDiscoveryTable();
static void printWaitingTable();


/**************************************************************************/
/*--------------------------PROCESSES DECLARATION-------------------------*/

PROCESS(initializer, "Open all connections and initializ tables");
PROCESS(rreq_handler, "Handle RREQ_PACKET messages");
PROCESS(data_handler, "Creates and sends new DATA");
PROCESS(aging, "Controls the expiration of all tables");
PROCESS(debugger_handler, "Enables/disables the debugger if button is clicked");

AUTOSTART_PROCESSES(&initializer,
                    &rreq_handler, 
                    &data_handler,
                    &aging,
                    &debugger_handler);


/**************************************************************************/
/*-------------------------GLOBAL VARIABLES-------------------------------*/

// Debugging
static char dbg = 1;

// Connections
static struct unicast_conn rrep_conn;
static struct unicast_conn data_conn;
static struct broadcast_conn rreq_conn;

// Callbacks
static const struct unicast_callbacks rrep_cbk = {route_reply_callback};
static const struct unicast_callbacks data_cbk = {data_callback};
static const struct broadcast_callbacks rreq_cbk = {route_request_callback};

// Routing Tables
static struct ROUTING_TABLE_ENTRY routingTable[MAX_NODES];
static struct DISCOVERY_TABLE_ENTRY discoveryTable[DISCO_SIZE];
static struct QUEUE_ENTRY waitingTable[MAX_DATA_IN_QUEUE];


/**************************************************************************/
/*--------------------------PROCESSES DEFINITION--------------------------*/

//This process activates the connections (broadcast and unicast) and initializes the routing table
PROCESS_THREAD(initializer, ev, data)
{
    int i;

    // stop all communication
    PROCESS_EXITHANDLER({
        unicast_close(&rrep_conn);
        unicast_close(&data_conn);
        broadcast_close(&rreq_conn);
    }); 
        
    PROCESS_BEGIN();

    // initialize routing table
    for (i=0; i<MAX_NODES; i++){
        routingTable[i].dest = i+1;
        routingTable[i].valid = 0;
        routingTable[i].hops = INF;
    }

    // Route Reply
    unicast_open(&rrep_conn, RREP_CHANNEL, &rrep_cbk);

    if(dbg) printf("Now listening to ROUTE_REPLY messages on channel: %d \n", RREP_CHANNEL);

    // Data
    unicast_open(&data_conn, DATA_CHANNEL, &data_cbk);
    if(dbg) printf("Now listening to DATA messages on channel: %d \n", DATA_CHANNEL);

    // Route Request
    broadcast_open(&rreq_conn, RREQ_CHANNEL, &rreq_cbk); 
    if(dbg) printf("Now listening to ROUTE_REQ  messages on channel: %d \n", RREQ_CHANNEL);

    printf("Node initialized\n");

    PROCESS_END();
}

//This process helps to perform outgoing ROUTE_REQ
PROCESS_THREAD(rreq_handler, ev, data)
{
    static struct DISCOVERY_TABLE_ENTRY* rreq_info;
    static struct RREQ_PACKET rreq;

    PROCESS_BEGIN();
        
    while(1)
    {
        leds_off(LEDS_YELLOW); 
        
        //the process waits for a post request
        PROCESS_WAIT_EVENT_UNTIL(ev != sensors_event);
        
        leds_on(LEDS_YELLOW);
        
        rreq_info = (struct DISCOVERY_TABLE_ENTRY*)data;
        rreq.req_id = rreq_info->req_id;
        rimeaddr_copy(&rreq.src, &rreq_info->src);
        rreq.src = rreq_info->src;
        rreq.dest = rreq_info->dest;
                     
        addEntryToDiscoveryTable(rreq_info);    //create entry in routing discovery table
        
        sendrreq(&rreq);    //broadcasts the ROUTE_REQUEST
    }

    PROCESS_END();
}


//This process is responsible of the periodic sending of random data
PROCESS_THREAD(data_handler, ev, data)
{
    // Timers   
    static struct etimer et;
    static int initial_delay;
        
    static uint8_t req_id = 1; //starting req_id
    static int dest;
    static int next;
        
    static struct DISCOVERY_TABLE_ENTRY rreq_info;
    static struct DATA_PACKET data_pkg;
            
    PROCESS_BEGIN();
        
    // Introduces randomicity to reduce conflicts at beginning
    initial_delay = random_rand() % DATA_PACKAGE_DELTA_TIME;
    etimer_set(&et, (CLOCK_SECOND) * initial_delay );

    while(1)
    {
        leds_off(LEDS_GREEN);
                
        PROCESS_WAIT_EVENT_UNTIL(ev != sensors_event);

        leds_on(LEDS_GREEN);
                
        //case data_handler TIMER, i.e. data has to be generated
        if(ev == PROCESS_EVENT_TIMER)
        {
            if(dbg) printf("Process that sends DATA is awake!\n");

            //get a random destination node, but not this one
            dest = 1 + random_rand() % MAX_NODES;
            if(dest==rimeaddr_node_addr.u8[0])              //not same node
                dest = (dest!=MAX_NODES)? dest+1 : 1;   //last node
            
            getRandomPayload(data_pkg.payload);    
            if(dbg) printf("Ready to send DATA message to %d: {%s}\n",
                        dest, data_pkg.payload);

            // wait for 30 secs before sending a new mex
            etimer_set(&et, CLOCK_SECOND * DATA_PACKAGE_DELTA_TIME);
        } 
        // case the event is generated by the data message CALLBACK, i.e. forward of data package
        else
        {
            dest = ((struct DATA_PACKET*)data)->dest;
            strcpy(data_pkg.payload,((struct DATA_PACKET*)data)->payload);
        }
        // set 
        data_pkg.dest = dest;

        //gets the NEXT hop to destination "dest"
        next = getNext(dest);
        
        // route available, send immediately
        if(next!=0)
        {
            senddata(&data_pkg, next);
        }
        // route not avalable
        else
        {
            // enque data package
            enque(&data_pkg);

            //configuring parameter for the ROUTE_REQ...
            rimeaddr_t saved_dest;
            rimeaddr_copy(&rreq_info->src, &rimeaddr_node_addr)
            rreq_info.req_id = req_id;
            //rreq_info.src = rimeaddr_node_addr.u8[0]; // me
            rreq_info.dest = dest;
            rreq_info.snd = rimeaddr_node_addr.u8[0]; // me
                        
            //calls the rreq_handler PROCESS in order to
            process_post(&rreq_handler, PROCESS_EVENT_CONTINUE, &rreq_info);

            // imcrement req_id
            req_id = (req_id<99) ? req_id+1 : 1;
        }

    }
    PROCESS_END();
}


//This process periodically checks the aging table to delete expired entries
PROCESS_THREAD(aging, ev, data)
{
    static struct etimer et;
    static int i, flag;
    static int dest, next;
    
    PROCESS_BEGIN();
    
    //leds_off(LEDS_RED);
    
    while(1)
    {
        etimer_set(&et, 10 * CLOCK_SECOND);
        
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
        //PROCESS_WAIT_EVENT_UNTIL(ev != sensors_event);
        
        //leds_off(LEDS_RED);
        
        // Clean routingTable
        flag = 0;
        for(i=0; i<MAX_NODES; i++)
        {
            if(routingTable[i].age > 0 && routingTable[i].valid ==1)
            {
                routingTable[i].age --;
                // if age has run out (route too old)
                if(routingTable[i].age == 0)
                {
                    routingTable[i].valid = 0;
                    routingTable[i].next= 0;
                    routingTable[i].hops = INF;
                    printf("route to %d has expired!\n",i+1);
                    leds_on(LEDS_RED);
                    flag++;
                }
            }
        }
        if (flag != 0)
            printRoutingTable();

        // Clean discovery table
        flag = 0;
        for(i=0; i<DISCO_SIZE; i++)
        {
            if(discoveryTable[i].age > 0 && discoveryTable[i].valid ==1)
            {
                // if age has run out (route request too old)
                if(discoveryTable[i].age == 0)
                {
                    discoveryTable[i].valid = 0;
                    printf("ROUTE_REQUEST from %d to %d (ID:%d) has expired!\n",
                            discoveryTable[i].src->u8[0], discoveryTable[i].dest->u8[0], discoveryTable[i].req_id);
                    flag++;
                }
                discoveryTable[i].age --;
            }
        }
        if (flag != 0)
            printDiscoveryTable();

        // Refresh waiting table
        flag = 0;
        for(i=0; i<MAX_DATA_IN_QUEUE; i++)
        {
            if(waitingTable[i].valid != 0)
            {
                dest = waitingTable[i].data_pkg.dest;
                next = getNext(dest);
                if (next != 0)
                {
                    senddata(&waitingTable[i].data_pkg, next);
                    if (dbg) printf("DATA sent towards %d via %d\n", dest, next);
                    flag++;
                }
                else
                {
                    waitingTable[i].age--;
                    if (waitingTable[i].age < 0)
                    {
                        waitingTable[i].valid = 0;
                        if (dbg) printf("DATA to %d was discarded (no route found)\n", dest);
                        flag++;
                    }
                }
            }
        }
        if (flag != 0)
            printWaitingTable();
    }
    PROCESS_END();
}

//This process enables/disables the debugger, catching the event generated
//by the clicking of the BUTTON
PROCESS_THREAD(debugger_handler, ev, data)
{
    PROCESS_BEGIN();

    //SENSORS_ACTIVATE(button_sensor);

    //while(1) 
    //{
    //    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
    //    dbg = dbg==0 ? 1 : 0;
    //}
    PROCESS_END();
}


/*************************************************************************************/
/*-----------------------CALLBACKS FUNCTIONS-----------------------------------------*/

// called upon receiving a packet on RREP_CHANNEL
static void route_reply_callback(struct unicast_conn *c, const rimeaddr_t *from)
{
    char packet[RREP_PACKET_LEN];
    struct RREP_PACKET rrep;
    int i;
    
    strncpy(packet, (char *)packetbuf_dataptr(), RREP_PACKET_LEN);
    
    // case ROUTE_REPLY package received 
    if(packet2rrep(packet, &rrep)!=0)
    {
        printf("ROUTE_REPLY received from %d [ID:%d, Dest:%d, Src:%d, Hops:%d]\n",
                        from->u8[0], rrep.req_id, rrep.dest, rrep.src, rrep.hops);
    
        // Check if reply updates table
        if(updateTables(&rrep, from->u8[0]))
        {
            // if the source is not me forward reply to all nodes waiting
            if(rrep.src != rimeaddr_node_addr.u8[0])
            {
                rrep.hops = rrep.hops + 1;
                for(i=0; i<DISCO_SIZE; i++){
                    if(discoveryTable[i].valid != 0
                        && discoveryTable[i].req_id == rrep.req_id
                        && discoveryTable[i].dest == rrep.dest){
                            sendrrep(&rrep, discoveryTable[i].snd); 
                    }
                }
            }
            clearDiscoveryEntry(&rrep);
        }
        // otherwise 
        else
        {
            if(dbg) printf("Better route to %d already present\n", rrep.dest);
        }
    }
    // case unexpected package received
    else
    {
        if(dbg) printf("ERROR in ROUTE_REPLY CALLBACK: unexpected package received!\n\t content:{%s}\n",packet);
    }
}



// called upon receiving a packet on DATA_CHANNEL
static void data_callback(struct unicast_conn *c, const rimeaddr_t *from)
{
    static struct DATA_PACKET data;
    static char packet[DATA_PACKET_LEN];
    
    strncpy(packet, (char *)packetbuf_dataptr(), DATA_PACKET_LEN);
    
    // case DATA packet receive
    if(packet2data(packet, &data) != 0)
    {
        // if the destination of the message is this node
        if(data.dest == rimeaddr_node_addr.u8[0])
        {
            printf("DATA RECEIVED: {%s}\n", data.payload);
        }   
        // otherwise
        else
        {
            if(dbg) printf("Received DATA for %d\n", data.dest);
            //wakes up the event that handles the sending of DATA
            process_post(&data_handler, PROCESS_EVENT_CONTINUE, &data);
        }
    }
    // case unexpected package received
    else
    {
        if(dbg) printf("ERROR in DATA CALLBACK: unexpected package received!\n\tcontent:{%s}\n",packet);
    }
}

// called upon receiving a packet on RREQ_CHANNEL
static void route_request_callback(struct broadcast_conn *c, const rimeaddr_t *from)
{
    static struct DISCOVERY_TABLE_ENTRY rreq_info;
    static struct RREQ_PACKET rreq;
    static struct RREP_PACKET rrep;
    static char packet[RREQ_PACKET_LEN];
    
    strncpy(packet, (char *)packetbuf_dataptr(), RREQ_PACKET_LEN);

    // case ROUTE_REQUEST packge received
    if(packet2rreq(packet, &rreq) != 0)
    {
        printf("ROUTE_REQUEST received from %d [ID:%d, Dest:%d, Src:%d]\n",
                        from->u8[0], rreq.req_id, rreq.dest->u8[0], rreq.src->u8[0]);

        // case destination is me
        if(rreq.dest == rimeaddr_node_addr.u8[0])
        {                               
            rrep.req_id = rreq.req_id;
            rrep.src = rreq.src;
            rrep.dest = rreq.dest;
            rrep.hops = 0;

            //sends a new ROUTE_REPLY to the ROUTE_REQ sender
            sendrrep(&rrep, from->u8[0]);
                        
        }
        // case I am NOT the destination AND the ROUTE_REQ is new
        else if(isDuplicateReq(&rreq)==0)
        {
            rreq_info.req_id = rreq.req_id;
            rreq_info.src = rreq.src;
            rreq_info.dest = rreq.dest;
            rreq_info.snd = from->u8[0];
                    
            //wakes up the process to perform a ROUTE_REQ
            process_post(&rreq_handler, PROCESS_EVENT_CONTINUE, &rreq_info);
        }
        // case duplicated route request
        else
        {
			if (dbg) printf("Duplicated ROUTE_REQUEST: Discarded!\n");
        }
    }
    // case unexpected package received
    else
    {
        if(dbg) printf("ERROR in ROUTE_REQUEST CALLBACK: unexpected package received!\n\tcontent: {%s}\n", packet);
    }
}


/*************************************************************************************/
/*-----------------------COMMUNICATION FUNCTIOS--------------------------------------*/

//Actually sends the ROUTE_REPLY message
static void sendrrep(struct RREP_PACKET* rrep, int next)
{        
    static char packet[RREP_PACKET_LEN];
    
    static rimeaddr_t to_rimeaddr;
    to_rimeaddr.u8[0]=next;
    to_rimeaddr.u8[1]=0;
    
    rrep2packet(rrep, packet);
    packetbuf_clear();
    packetbuf_copyfrom(packet, RREP_PACKET_LEN); 
    unicast_send(&rrep_conn, &to_rimeaddr);
    
    printf("Sending ROUTE_REPLY toward %d via %d [ID:%d, Dest:%d, Src:%d, Hops:%d]\n", 
            rrep->src, next, rrep->req_id, rrep->dest, rrep->src, rrep->hops);
}

//Actually sends the DATA message
static void senddata(struct DATA_PACKET* data, int next)
{        
    static char packet[DATA_PACKET_LEN];
    
    static rimeaddr_t to_rimeaddr;
    to_rimeaddr.u8[0]=next;
    to_rimeaddr.u8[1]=0;
    
    data2packet(data, packet);
    packetbuf_clear();
    packetbuf_copyfrom(packet, DATA_PACKET_LEN); 
    unicast_send(&data_conn, &to_rimeaddr);
    
    printf("Sending DATA {%s} to %d via %d \n", 
            data->payload, data->dest, next);
}

//Actually sends the ROUTE_REQUEST message (broadcast)
static void sendrreq(struct RREQ_PACKET* rreq)
{        

    static char packet[RREQ_PACKET_LEN];
    
    rreq2packet(rreq, packet);
    packetbuf_clear();
    packetbuf_copyfrom(packet, RREQ_PACKET_LEN); 
    broadcast_send(&rreq_conn);
    printf("R_REQ [ID:%d, Dest:%d, Src:%d]\n",
            rreq->req_id, rreq->dest, rreq->src);
}


/*************************************************************************************/
/*-----------------------TABLES SUPPORT FUNCTIOS-------------------------------------*/

static char updateTables(struct RREP_PACKET * rrep, int from)
{
    int d = rrep->dest - 1;

    //if the ROUTE_REPLY received shows a better path 
    if(rrep->hops < routingTable[d].hops)
    {
        //UPDATES the routing discovery table!
        routingTable[d].dest = rrep->dest;
        routingTable[d].hops = rrep->hops;             
        routingTable[d].next = from;
        routingTable[d].age = ROUTE_EXPIRATION_TIME;
        routingTable[d].valid = 1;
        if(dbg) printf("Improved ROUTE to %d: %d HOPS!\n",
                rrep->dest, rrep->hops);
        printRoutingTable();
        return 1;
    }
    return 0;
}

// Gets the next node for the given destination
static int getNext(int dest)
{
    if (routingTable[dest-1].valid != 0)
        return routingTable[dest-1].next;    
    return 0;
}

// adds if(dbg) ic entry to discovery table.
static void addEntryToDiscoveryTable(struct DISCOVERY_TABLE_ENTRY* rreq_info)
{
    int i;
    for(i=0; i<DISCO_SIZE; i++){
        if(discoveryTable[i].valid == 0){
            discoveryTable[i].req_id = rreq_info->req_id;
            discoveryTable[i].src = rreq_info->src;
            discoveryTable[i].dest = rreq_info->dest;
            discoveryTable[i].snd = rreq_info->snd;
            discoveryTable[i].valid = 1;
            discoveryTable[i].age = ROUTE_DISCOVERY_TIME;
            break;
        }
    }
	if(dbg) printf("Adding entry to discovery table\n");
    //printDiscoveryTable();
}

// clears discovery entry
static void clearDiscoveryEntry(struct RREP_PACKET* rrep)
{
    int i;
    for(i=0; i<DISCO_SIZE; i++){
        if(discoveryTable[i].valid != 0
            && discoveryTable[i].req_id == rrep->req_id
            // && discoveryTable[i].src == rrep->src        // enable for ditinguish on reply id
            && discoveryTable[i].dest == rrep->dest)
                discoveryTable[i].valid = 0;
                return;
    }
    return;
}

// Checks if the received ROUTE_REQ was already in the discovery Table
static char isDuplicateReq(struct RREQ_PACKET* rreq)
{        
    int i;
    for(i=0; i<DISCO_SIZE; i++){
        if(discoveryTable[i].valid != 0
            && discoveryTable[i].req_id == rreq->req_id
            && discoveryTable[i].src == rreq->src
            && discoveryTable[i].dest == rreq->dest)
                return 1;
    }
    return 0;        
}

// Adds data package to queue
static char enque(struct DATA_PACKET* data)
{
    int i;
    for(i=0; i<MAX_DATA_IN_QUEUE; i++) {
        if(waitingTable[i].valid == 0) {
            memcpy(&waitingTable[i].data_pkg, data, sizeof(data));
            waitingTable[i].age = MAX_QUEUEING_TIME;
            return 1;
        }
    }
    return 0;
}

/*************************************************************************************/
/*-----------------------SUPPORT FUNCTIOS--------------------------------------*/

//Generates a random payload
static void getRandomPayload(char payload[DATA_PAYLOAD_LEN])
{
    static int i;
    static int middle;
    static int rand;
    middle = DATA_PAYLOAD_LEN/2 - 1;
    rand = random_rand() %100;
    for(i=0;i<DATA_PAYLOAD_LEN-1;i++)
    {
        if(i==(middle-1) || i==(middle+2))
            payload[i] = ' ';
        else if(i==middle)
            payload[i] = rand/10 + '0';
        else if(i==(middle+1))
            payload[i] = rand%10 + '0';
        else
            payload[i] = '*';
    }
    payload[i] = '\0';
}


/*************************************************************************************/
/*-----------------------VISULIZATION FUNCTIOS---------------------------------------*/

// Prints the Routing Table
static void printRoutingTable()
{
    int i;
    char flag = 0;

    printf("Routing Table");
    for(i=0; i<MAX_NODES;i++)
    {
        if(routingTable[i].valid!= 0)
        {
            printf("\n {RouteT: Dest:%d; Next:%d; Hops:%d; Age:%d}",
                    routingTable[i].dest, routingTable[i].next,
                    routingTable[i].hops, routingTable[i].age);
            flag ++;
        }
    }
    if(flag==0)
        printf(" is empty\n");
    else
        printf("\n");
}

//Helps to print the Discovery Table
static void printDiscoveryTable()
{
    int i, flag = 0;
 
    printf("Discovery Table");
    for(i=0; i<DISCO_SIZE;i++)
    {
        if(discoveryTable[i].valid!= 0)
        {
            printf("{ID:%d; Src:%d; Dest:%d; Snd:%d;} \n",
                    discoveryTable[i].req_id,
                    discoveryTable[i].src,
                    discoveryTable[i].dest,
                    discoveryTable[i].snd);
            flag++;
        }
    }
    if(flag==0)
        printf("Discovery Table is empty \n");
}

// prints Waiting table
static void printWaitingTable()
{
    int i, flag = 0;
 
    printf("Data Waiting Table");
    for(i=0; i<MAX_DATA_IN_QUEUE;i++)
    {
        if(waitingTable[i].valid!= 0)
        {
            printf("\n    {Dest:%d; Age:%d;}",
                    waitingTable[i].data_pkg.dest,
                    waitingTable[i].age);
            flag++;
        }
    }
    if(flag==0)
        printf(" is empty \n");
    else
        printf("\n");
}

/*************************************************************************************/
/*-----------------------PART RREQ PACKET SUPPORTING FUNC---------------------------------------*/

/*---------------------struct to packet-----------------------*/

void rreq2packet(struct RREQ_PACKET* rreq, char* packet) {
	sprintf(packet, RREQ_REP, rreq->req_id, rreq->dest, rreq->src);
}

void rrep2packet(struct RREP_PACKET* rrep, char* packet) {
	sprintf(packet, RREP_REP, rrep->req_id, rrep->dest, rrep->src, rrep->hops);
}

void data2packet(struct DATA_PACKET* data, char* packet) {
	sprintf(packet, DATA_REP, data->dest, data->payload);
}


/*---------------------packet to struct------------------------*/

// read route request package
char packet2rreq(char* packet, struct RREQ_PACKET* rreq)
{
	static char reqId[2];
	static char src[2];
	static char dest[2];
	if (strncmp(packet, RREQ_HEADER, sizeof(RREQ_HEADER) - 1) == 0)
	{
		// id
		int idx = sizeof(RREQ_HEADER) - 1 + sizeof(ITEM_SEP) - 1 + sizeof(REQ_ID) - 1 - (sizeof(ID_REP) - 1);
		reqId[0] = packet[idx];
		reqId[1] = packet[idx + 1];
		rreq->req_id = atoi(reqId);
		// dest
		idx += 2 + sizeof(ITEM_SEP) - 1 + sizeof(DEST) - 1 - (sizeof(NODE_REP) - 1);
		dest[0] = packet[idx];
		dest[1] = packet[idx + 1];
		rreq->dest = atoi(dest);
		// source
		idx += 2 + sizeof(ITEM_SEP) - 1 + sizeof(SRC) - 1 - (sizeof(NODE_REP) - 1);
		src[0] = packet[idx];
		src[1] = packet[idx + 1];
		rreq->src = atoi(src);

		return 1;
	}
	return 0;
}

// read route reply packet
char packet2rrep(char* packet, struct RREP_PACKET* rrep)
{
	static char repId[2];
	static char src[2];
	static char dest[2];
	static char hops[2];
	if (strncmp(packet, RREP_HEADER, sizeof(RREP_HEADER) - 1) == 0)
	{
		// id
		int idx = sizeof(RREP_HEADER) - 1 + sizeof(ITEM_SEP) - 1 + sizeof(REP_ID) - 1 - (sizeof(ID_REP) - 1);
		repId[0] = packet[idx];
		repId[1] = packet[idx + 1];
		rrep->req_id = atoi(repId);
		// dest
		idx += 2 + sizeof(ITEM_SEP) - 1 + sizeof(DEST) - 1 - (sizeof(NODE_REP) - 1);
		dest[0] = packet[idx];
		dest[1] = packet[idx + 1];
		rrep->dest = atoi(dest);
		// source
		idx += 2 + sizeof(ITEM_SEP) - 1 + sizeof(SRC) - 1 - (sizeof(NODE_REP) - 1);
		src[0] = packet[idx];
		src[1] = packet[idx + 1];
		rrep->src = atoi(src);
		// hops
		idx += 2 + sizeof(ITEM_SEP) - 1 + sizeof(HOPS) - 1 - (sizeof(HOPS_REP) - 1);
		hops[0] = packet[idx];
		hops[1] = packet[idx + 1];
		rrep->hops = atoi(hops);

		return 1;
	}
	return 0;
}

// read data packet
char packet2data(char* packet, struct DATA_PACKET* data)
{
	static char dest[2];
	if (strncmp(packet, DATA_HEADER, sizeof(DATA_HEADER) - 1) == 0)
	{
		// dest
		int idx = sizeof(DATA_HEADER) - 1 + sizeof(ITEM_SEP) - 1 + sizeof(DEST) - 1 - (sizeof(NODE_REP) - 1);
		dest[0] = packet[idx];
		dest[1] = packet[idx + 1];
		data->dest = atoi(dest);

		// payload
		idx = idx + 2 + sizeof(ITEM_SEP) - 1 + sizeof(PAYLOAD) - 1 - (sizeof(PAYLOAD_REP) - 1);
		strncpy(data->payload, packet + idx, DATA_PAYLOAD_LEN);

		return 1;
	}
	return 0;
}
