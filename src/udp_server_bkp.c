#include"header.h"
int main(int argc, char *argv[])
{

	int listensocket, readbytes, binderror, i, j = 0, retval,file_fd, seq_num = 0,\
						       cwnd, ssthresh, continue_count = 1, sender_window = 0,\
						       congestion_avoidance = 0, temp_int;
	long file_len;
	struct  sockaddr_in serveraddress,cliaddr;
	tlv_stream tlv_recv_chain, tlv_send_chain;
	tlv_stream *recv_chain = malloc(sizeof(tlv_stream));
	tlv_stream *send_chain = malloc(sizeof(tlv_stream));
	socklen_t len;
	unsigned char readbuf[BUFSIZE] = {0};
	unsigned char sendbuf[BUFSIZE]= {0};
	char file_readbuf[BUFSIZE];
	ssize_t read_chars;
	int32_t size = 0; 
	void * temp ;
	double t1,t2, alpha = 0.125, beta = 0.25 ; //Initializations for Jacobsen-Karel's Algo
	struct timeval start, end, sample_rtt, estimated_rtt,dev_rtt, timeout;
	fd_set readfds;
	reliable_udp_packet_t  *udp_packet = NULL ; //segment to be sent
	reliable_udp_packet_t  *recvd_udp_packet = NULL ; //initial-request/ack received
	U16 temp_u16;U32 temp_u32;

	cwnd = 1500; //i.e 1 MSS
	ssthresh = 64000; //i.e.64 kb
	timeout.tv_sec = 1;             /* 1 second default timeout for acks */
	timeout.tv_usec = 0;
	memset(&tlv_recv_chain, 0, sizeof(tlv_recv_chain));	
	memset(&tlv_send_chain, 0, sizeof(tlv_send_chain));	
	recv_chain = &tlv_recv_chain;
	send_chain = &tlv_send_chain;
	udp_packet = (reliable_udp_packet_t *) malloc( sizeof(reliable_udp_packet_t) ); 
	recvd_udp_packet = (reliable_udp_packet_t *) malloc( sizeof(reliable_udp_packet_t) ); 
	listensocket = socket(AF_INET, SOCK_DGRAM, 0 );

	if (listensocket < 0 )
	{
		perror("socket" );
		exit(1);
	}

	if (argv[1] == NULL ) {
		printf ("PL specfiy the IP address of the server. \n");
		exit(0);
	}
	if (argv[2] == NULL ) {
		printf ("PL specfiy the port number for the server. \n");
		exit(0);
	}
	if (argv[3] == NULL ) {
		printf ("PL specfiy the advertised-window size for the server \n");
		exit(0);
	}

	sender_window = atoi(argv[3]);
	temp_int = sender_window; 
	serveraddress.sin_family = AF_INET;
	serveraddress.sin_port = htons(atoi(argv[2]));/*PORT NO*/
	serveraddress.sin_addr.s_addr = inet_addr(argv[1]);/*ADDRESS*/
	binderror=bind(listensocket,(struct sockaddr*)&serveraddress,sizeof(serveraddress));
	if (-1 == binderror)
	{
		perror("BIND !!!!!!!");
		exit(1);
	}
	listen(listensocket,5);


	for (;;)
	{
		printf("Server: I am waiting \n");
		len=sizeof(cliaddr);
		//Client Request
		readbytes = recvfrom(listensocket,readbuf,1000,0,(struct sockaddr *)&cliaddr,&len);

		printf ("\n\nbytes read = %d", readbytes);
		deserialize_tlv(readbuf, recv_chain, readbytes);
		//printf ("recv_chain->used: %d\n", recv_chain->used);

		// go through each used tlv object in the chain
		for( i =0; i < recv_chain->used; i++)
		{

			printf("type:%d\n",recv_chain->object[i].type);
			if (7 == recv_chain->object[i].type)
			{   printf("Filename: %s\n", (char *)recv_chain->object[i].data);
				memset(file_readbuf,0,BUFSIZE);
				memcpy(file_readbuf, (char *)recv_chain->object[i].data, recv_chain->object[i].size);
			}
		}

		if(( file_fd = open(&file_readbuf[0],O_RDONLY)) == -1) {  /* open the file for reading */
			printf ("Failed to open file: File not found\n");
		}

		file_len = (long)lseek(file_fd, (off_t)0, SEEK_END); /* lseek to the file end to find the length */
		(void)lseek(file_fd, (off_t)0, SEEK_SET); /* lseek back to the file start ready for reading */ 
                
                if (0.0 == file_len) printf ("No data in file!");
		udp_packet->source_port = 4121;
		temp_u16 = udp_packet->source_port;
		temp = &temp_u16; 
		add_to_stream(send_chain, 1, sizeof(U16), temp);

		udp_packet->dest_port = 3000;
		temp_u16 = udp_packet->dest_port;
		temp = &temp_u16; 
		add_to_stream(send_chain, 2, sizeof(U16), temp);

		udp_packet->ack_num = 0 ;
		temp_u32 = udp_packet->ack_num;
		temp = &temp_u32; 
		add_to_stream(send_chain, 4, sizeof(U32), temp);

		udp_packet->chksum = 0;
		temp_u16 = udp_packet->chksum;
		temp = &temp_u16; 
		add_to_stream(send_chain, 6, sizeof(U16), temp);

		/* send file in < 1 MSS blocks (i.e < 1 MSS + header size ~ 1 MSS) - last block may be smaller */
		while (	(read_chars = read(file_fd, file_readbuf, BUFSIZE)) > 0 ) 
		{

			udp_packet->length = sizeof(16)+sizeof(16)+sizeof(32)+sizeof(16)+sizeof(32)+read_chars ;
			temp_u16 = udp_packet->length;
			temp = &temp_u16; 
			add_to_stream(send_chain, 5, sizeof(U16), temp);

			seq_num = seq_num + 1;
			udp_packet->seq_num = seq_num; 
			temp_u32 = udp_packet->seq_num;
			temp = &temp_u32; 
			add_to_stream(send_chain, 3, sizeof(U32), temp);
			printf ("Seq_NUM= %lu\n", udp_packet->seq_num);

			printf("%s", file_readbuf);
			temp = file_readbuf;
			add_to_stream(send_chain, 7, read_chars, temp);

			/*Serialization of stream*/
			for(i = 0; i < send_chain->used; i++)
			{
				sendbuf[size] = send_chain->object[i].type;
				size++;

				memcpy(&sendbuf[size], &send_chain->object[i].size, 2);
				size += 2;

				memcpy(&sendbuf[size], send_chain->object[i].data, send_chain->object[i].size);
				size += send_chain->object[i].size;
			}

			if(gettimeofday(&start,NULL)) {
				printf("time failed\n");
				//exit(1);
			}

			sendto(listensocket,sendbuf,size,0,(struct sockaddr *)&cliaddr,sizeof(cliaddr)) ;
			sender_window-- ;
			memset(file_readbuf,0,BUFSIZE);
			memset(sendbuf,0,BUFSIZE);	
			memset(readbuf,0,BUFSIZE);
			memset(&tlv_send_chain, 0, sizeof(tlv_send_chain));	
			send_chain = &tlv_send_chain;	

			if ((j < continue_count ) && (1 != continue_count) && (0 != sender_window))
			{j++;  
				//continue;
			}


			if (!sender_window)
			{       
				sender_window = temp_int; 
				FD_ZERO(&readfds);          /* initialize the fd set */
				FD_SET(listensocket, &readfds);

				t1 = timeout.tv_sec/100;
				printf ("timeout = %g ms", t1);
				t1 = 0.0; 
				retval = select(listensocket+1, &readfds, 0, 0, &timeout); 
                                if (retval == 0)
			        {
                                   printf ("Timeout!!");
                                 }
	
				if (FD_ISSET(listensocket, &readfds)){ 

					//Ack For Segments sent
					readbytes = recvfrom(listensocket,readbuf,1000,0,(struct sockaddr *)&cliaddr,&len);
				}	
				if(gettimeofday(&end,NULL)) {
					printf("end time failed\n");
				}
				t1+=start.tv_sec+(start.tv_usec/1000000.0);
				t2+=end.tv_sec+(end.tv_usec/1000000.0);
				sample_rtt.tv_sec = (t2-t1);

				estimated_rtt.tv_sec = (1 - alpha)*estimated_rtt.tv_sec + alpha*sample_rtt.tv_sec;
				dev_rtt.tv_sec = (1 - beta)*dev_rtt.tv_sec + beta*(sample_rtt.tv_sec - estimated_rtt.tv_sec);
				timeout.tv_sec = estimated_rtt.tv_sec + 4*dev_rtt.tv_sec;

				deserialize_tlv(readbuf, recv_chain, readbytes);
				// go through each used tlv object in the chain
				for( i =0; i < recv_chain->used; i++)
				{        
					make_recvd_udp_packet(i, recv_chain, recvd_udp_packet); 
					if (3 == recv_chain->object[i].type)
					{
						udp_packet->ack_num = (*recv_chain->object[i].data) + 1;
						temp_u32 = udp_packet->ack_num;
						temp = &temp_u32; 
						add_to_stream(send_chain, 4, sizeof(U32), temp);
						//printf("Ack Num = %lu\n", udp_packet->ack_num);
						if (0 == congestion_avoidance)
                                                {
                                                        printf ("\nIn Slow Start\n"); 
                                                        cwnd = cwnd + 1500;
							continue_count = 2 * continue_count;
                                                }
						else cwnd = cwnd + 1500*(1500/cwnd); 

						if (ssthresh == cwnd)
						{
							cwnd = cwnd / 2;
							continue_count = 1;
							congestion_avoidance = 1;
                                                        printf ("\nStart of Congestion Avoidance Phase\n");
						}

					}


					/*if (4 ==  recv_chain->object[i].type)
					  { 
					  if (((*recv_chain->object[i].data)-1) == seq_num)
					  {
					  cwnd = cwnd + 1500;     
					  continue_count = continue_count + 1;

					  } 
					  }*/


				}
			} 
			t1 = 0.0;
			t2 = 0.0;
			size = 0;
		}

		close (file_fd);
		//	memset(file_readbuf,0,BUFSIZE);
		memset(sendbuf,0,BUFSIZE);	
		memset(readbuf,0,BUFSIZE);
		memset(&tlv_send_chain, 0, sizeof(tlv_send_chain));	
		send_chain = &tlv_send_chain;		
		continue_count= 1;
		j = 0;
	}
	sleep(1);	/* allow socket to drain before signalling the socket is closed */
	close(listensocket);
	return 0;
}


int32_t add_to_stream(tlv_stream *a, unsigned char type, int16_t size, unsigned char *bytes)
{
	if(a == NULL || bytes == NULL)
		return -1;

	// all elements used in chain?
	if(a->used == 50)
		return -1;

	int index = a->used;
	a->object[index].type = type;
	a->object[index].size = size;
	a->object[index].data = malloc(size);
	memcpy(a->object[index].data, bytes, size);

	// increase number of tlv objects used in this chain
	a->used++;

	// success
	return 0;
}

int32_t deserialize_tlv(unsigned char *readbuf, tlv_stream *recv_chain, int32_t readbytes)
{
	int32_t counter = 0;

	if(recv_chain == NULL || readbuf == NULL)
		return -1;

	// check if the chain is empty
	if(recv_chain->used != 0)
		return -1;


	while(counter < readbytes)
	{
		if(recv_chain->used == 50)
		{printf("50");}

		// deserialize type
		recv_chain->object[recv_chain->used].type = readbuf[counter];
		counter++;

		// deserialize size
		memcpy(&recv_chain->object[recv_chain->used].size, &readbuf[counter], 2);
		counter+=2;

		// deserialize data itself, only if data is not NULL
		if(recv_chain->object[recv_chain->used].size > 0)
		{
			recv_chain->object[recv_chain->used].data = malloc(recv_chain->object[recv_chain->used].size);
			memcpy(recv_chain->object[recv_chain->used].data, &readbuf[counter], recv_chain->object[recv_chain->used].size);
			counter += recv_chain->object[recv_chain->used].size;
		}else
		{
			recv_chain->object[recv_chain->used].data = NULL;
		}

		// increase number of tlv objects reconstructed
		recv_chain->used++;
	}
	printf ("counter = %d\n", counter);
	// success
	return 0;

}
void make_recvd_udp_packet(int i, tlv_stream * recv_chain, reliable_udp_packet_t * recvd_udp_packet)
{

	switch(recv_chain->object[i].type){
		case 1 : recvd_udp_packet->source_port  = *(U16*)recv_chain->object[i].data; break;
		case 2 : recvd_udp_packet->dest_port  = *(U16*)recv_chain->object[i].data; break;
		case 3 : recvd_udp_packet->seq_num  = *(U32*)recv_chain->object[i].data;break;
		case 4 : recvd_udp_packet->ack_num = *(U32*)recv_chain->object[i].data;break;
		case 5 : recvd_udp_packet->length  = *(U16*)recv_chain->object[i].data;break;
		case 6 : recvd_udp_packet->chksum  = *(U16*)recv_chain->object[i].data;break;
		case 7 : memcpy (recvd_udp_packet->data, recv_chain->object[i].data, recv_chain->object[i].size); break;

		default: return;
	}
	return;
}


