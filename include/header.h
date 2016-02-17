#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/types.h>
#include<errno.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<stdlib.h>
#include<string.h>
#include<arpa/inet.h>
#include<time.h>
#include <sys/time.h>
#define MYPORT 4121
#define SERVER_IP "10.10.10.1"
#define BUFSIZE 1464 //i.e approx 1 MSS (i.e < 1 MSS + header size ~ 1 MSS)

typedef	unsigned long	U32;
typedef	unsigned short	U16;
typedef	unsigned char	U8;



typedef struct 
{
    int8_t type;    // type
    uint8_t * data; // pointer to data
    int16_t size;   // size of data
}tlv;

// TLV chain data structure. Contains array of (50) tlv
// objects. 
typedef struct
{
    tlv object[7];
    uint8_t used; // keep track of tlv elements used
}tlv_stream;

typedef struct {

#define UDP_HEADER_ACK_PRESENT         0x08

	U16 source_port;
	U16 dest_port;
	U32 seq_num;
	U32 ack_num;
	U16 length;
	U16 chksum;
        char data[BUFSIZE];
        U8 ack_flag;//ack flag
} reliable_udp_packet_t; //udp_packet composition

/* function declarations*/
int32_t add_to_stream(tlv_stream *a, unsigned char type, int16_t size, unsigned char *bytes);
int32_t deserialize_tlv(unsigned char *readbuf, tlv_stream *chain2, int32_t readbytes);
void make_recvd_udp_packet(int i, tlv_stream * recv_chain, reliable_udp_packet_t * recvd_udp_packet);


