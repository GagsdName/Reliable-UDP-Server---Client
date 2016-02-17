#include"header.h"
int main( int argc, char *argv[] )
{
	int  sd,i, j,k = 1 ,z = 1, readbytes, lower_bound = 1, upper_bound = 0, flag = 0, expected_seq_num = 1,\
			 current_seq_num = 1, out_of_order_index = 0, window_size, retval; // receiver's seq_num window [lower_bound,lower_bound+window_size]
	struct  sockaddr_in serveraddress;
	unsigned char send_buf[BUFSIZE] = {0};
	unsigned char recv_buf[BUFSIZE] = {0};
	tlv_stream send_chain, recv_chain;
	U16 temp_u16;U32 temp_u32;
	double t1,t2;
	// Number of bytes serialized
	int32_t size = 0;
	void * temp ;
	reliable_udp_packet_t  *udp_packet = NULL ; // initial-request/acks to be sent
	reliable_udp_packet_t  *recvd_udp_packet = NULL ;// segments received
	reliable_udp_packet_t  *out_of_order[50] = {0};
	reliable_udp_packet_t  recv_buffer[1000] ;

	fd_set readfds;
	struct timeval tv, start, end;

	memset(&send_chain, 0, sizeof(send_chain));
	memset(&recv_chain, 0, sizeof(recv_chain));
	memset(&recv_buffer, 0, sizeof(recv_buffer));		
	//memset(&out_of_order, 0, sizeof(recv_buffer));	

	udp_packet = (reliable_udp_packet_t *) malloc( sizeof(reliable_udp_packet_t) ); 
	recvd_udp_packet = (reliable_udp_packet_t *) malloc( sizeof(reliable_udp_packet_t) ); 

	sd = socket( PF_INET, SOCK_DGRAM, 0 ); 
	if( sd < 0 ) {
		perror( "socket" );
		exit( 1 );
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
		printf ("PL specfiy the filename \n");
		exit(0);
	}

	if (argv[4] == NULL ) {
		printf ("PL specfiy the advertised_window size  \n");
		exit(0);
	}

	window_size = atoi(argv[4]);
	upper_bound = lower_bound + window_size;

	memset(&serveraddress,0,sizeof(struct sockaddr_in));
	serveraddress.sin_family = AF_INET;
	serveraddress.sin_port = htons(atoi(argv[2]));
	serveraddress.sin_addr.s_addr = inet_addr(argv[1]);

	udp_packet->source_port = 3000;
	temp_u16 = udp_packet->source_port;
	temp = &temp_u16; 
	add_to_stream(&send_chain, 1, sizeof(U16), temp);

	udp_packet->dest_port = htons(atoi(argv[2]));
	temp_u16 = udp_packet->dest_port;
	temp = &temp_u16; 
	add_to_stream(&send_chain, 2, sizeof(U16), temp);

	udp_packet->seq_num = 0;
	temp_u32 = udp_packet->seq_num;
	temp = &temp_u32; 
	add_to_stream(&send_chain, 3, sizeof(U32), temp);


	udp_packet->length = sizeof(U16)+sizeof(U16)+sizeof(U32)+sizeof(U32)+sizeof(U16)+sizeof(U16)+strlen(argv[3]);
	temp_u16 = udp_packet->length;
	temp = &temp_u16; 
	add_to_stream(&send_chain, 5, sizeof(U16), temp);

	printf ("packet_length of frame being sent = %d\n", udp_packet->length);
	udp_packet->chksum = 0;
	temp_u16 = udp_packet->chksum;
	temp = &temp_u16; 
	add_to_stream(&send_chain, 6, sizeof(U16), temp);

	memcpy (udp_packet->data, argv[3], strlen(argv[3]));
	temp = udp_packet->data; 
	add_to_stream(&send_chain, 7, strlen(argv[3]), temp);

	/*Serialization of stream*/
	for(i = 0; i < send_chain.used; i++)
	{
		send_buf[size] = send_chain.object[i].type;
		size++;

		memcpy(&send_buf[size], &send_chain.object[i].size, 2);
		size += 2;

		memcpy(&send_buf[size], send_chain.object[i].data, send_chain.object[i].size);
		size += send_chain.object[i].size;
	}


	if(gettimeofday(&start,NULL)) {
		printf(" start time failed\n");
		//exit(1);
	}
	sendto(sd,send_buf,size,0,(struct sockaddr *)&serveraddress,sizeof(serveraddress));

	while (1)
	{
		//	FD_ZERO(&readfds);          /* initialize the fd set */
		//	FD_SET(sd, &readfds);
		//	tv.tv_sec = 50000;             /* 10 second timeout */
		//	tv.tv_usec = 0;

		//	retval = select(sd+1, &readfds, 0, 0, &tv); 	
		//	if (FD_ISSET(sd, &readfds)){ 
		readbytes = recvfrom(sd,recv_buf,2048,0,NULL,NULL); 
		if ((-1) != readbytes)
		{

			if(gettimeofday(&end,NULL)) {
				printf("end time failed\n");
			}
			t1+=start.tv_sec+(start.tv_usec/1000000.0);
			t2+=end.tv_sec+(end.tv_usec/1000000.0);
			printf("\n\nRTT = %g ms\n",(t2-t1)/100);
			printf ("bytes read = %d\n", readbytes);

			deserialize_tlv(recv_buf, &recv_chain, readbytes); //deserialize recvd packet tag-length-value triplets

			//printf ("recv_chain->used: %d\n", recv_chain.used);

			// go through each used tlv object in the chain
			for( i =0; i < recv_chain.used; i++)
			{        
				make_recvd_udp_packet(i,&recv_chain, recvd_udp_packet);// assemble a recvd packet unit from the deserialized triplets of tag-length-values. 

				if (3 == recv_chain.object[i].type)
				{

					if ((lower_bound < recvd_udp_packet->seq_num || lower_bound == recvd_udp_packet->seq_num) &&\
							(upper_bound > recvd_udp_packet->seq_num || upper_bound == recvd_udp_packet->seq_num))
					{ 
						if (expected_seq_num != recvd_udp_packet->seq_num) 
						{
							//buffer out of order packets
							out_of_order[z] = recvd_udp_packet;
							//memcpy(&out_of_order[z], recvd_udp_packet, sizeof(reliable_udp_packet_t));	
							if (recvd_udp_packet->seq_num > current_seq_num)
								current_seq_num = recvd_udp_packet->seq_num; 
							out_of_order_index ++; 
							flag = 1;

						} 	else flag = 0;


					}

					if (0 == flag){
						udp_packet->ack_num = recvd_udp_packet->seq_num ;
						if (recvd_udp_packet->seq_num > current_seq_num)
							current_seq_num = recvd_udp_packet->seq_num;    
						//else expected_seq_num = current_seq_num + 1;  

						expected_seq_num = udp_packet->ack_num + 1;
						lower_bound = lower_bound + 1; 
					}
					if (1 == flag)
					{ 
						udp_packet->ack_num = expected_seq_num - 1;

					}

					temp_u32 = udp_packet->ack_num;
					temp = &temp_u32; 
					add_to_stream(&send_chain, 4, sizeof(U32), temp);

					printf("Ack Num = %lu\n", udp_packet->ack_num);

				}

			//	if ((7 == recv_chain.object[i].type) && (0 == flag) && ('\0' == (reliable_udp_packet_t *)out_of_order[0]) )
			//	{   
					//printf("Data Recvd: %s\n", (char *)recv_chain.object[i].data); //printing in-order packets 

			//	}
				// if (NULL != (reliable_udp_packet_t *)out_of_order[0])
				//printf("\nGot it!\n");

				//if ((0 == flag) && (0 != (reliable_udp_packet_t *)out_of_order[0]))
				//{
				//	for (j = 0; j < out_of_order_index; j++)
				//	{
						//print out of order packets 	
						//if (((char*)out_of_order[j]->data != NULL) && ((char*)out_of_order[j]->data[0] != '\0'))
						//printf ("Data Recvd: %s\n", (char *)(out_of_order[j]->data) );
						//out_of_order[j] = NULL;         
				//	}                                            
				//}

			}
			memcpy(&recv_buffer[recvd_udp_packet->seq_num], recvd_udp_packet, sizeof(reliable_udp_packet_t));
			z++;

		}
		//printf("Ack Num = %u\n", udp_packet->ack_num);


		//	if ((lower_bound == upper_bound)|| (1 == flag))
		//	{

		/*Serialization of stream*/
		for(i = 0; i < send_chain.used; i++)
		{
			send_buf[size] = send_chain.object[i].type;
			size++;

			memcpy(&send_buf[size], &send_chain.object[i].size, 2);
			size += 2;

			memcpy(&send_buf[size], send_chain.object[i].data, send_chain.object[i].size);
			size += send_chain.object[i].size;
		}


		t1 = 0.0;
		t2 = 0.0;
		sendto(sd,send_buf,size,0,(struct sockaddr *)&serveraddress,sizeof(serveraddress));
		if(gettimeofday(&start,NULL)) {
			printf("time failed\n");
			//exit(1);
		}
		upper_bound = upper_bound + window_size;
		//}
		//memset(file_readbuf,0,BUFSIZE);
		memset(send_buf,0,BUFSIZE);	
		memset(recv_buf,0,BUFSIZE);
		memset(&recv_chain, 0, sizeof(recv_chain));	
		size = 0;
		flag = 0;

		//	}
		//	else{   printf ("\nsocket-timeout: No data for 10s!!!!\n");

		//retval = select(sd+1, &readfds, 0, 0, &tv);
		//		if (retval == 0)
		//		{
		memset(send_buf,0,BUFSIZE);	
		memset(recv_buf,0,BUFSIZE);
		memset(&recv_chain, 0, sizeof(recv_chain));	
		//close(sd);
		size = 0; 
		FD_ZERO(&readfds);          /* initialize the fd set */
		tv.tv_sec = 0;             /* 10 second timeout */
		tv.tv_usec = 0;
		//		exit(1);
		//	}

		//}

		printf("\nData so far -------------------------------------");
		while(('\0' != recv_buffer[k].data[0]))
		{
                        if((NULL != out_of_order[k+1])&&('\0' != out_of_order[k+1]->data[0]))
                        {printf ("\n%s\n",(char*)out_of_order[k+1]->data); k++;continue;}
			printf ("\n%s\n",(char*)recv_buffer[k].data);
			k++;
		} 
	}


	//	close(sd);
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

int32_t deserialize_tlv(unsigned char *buf, tlv_stream *send_chain, int32_t readbytes)
{
	int32_t counter = 0;

	if(send_chain == NULL || buf == NULL)
		return -1;

	// check if the chain is empty
	if(send_chain->used != 0)
		return -1;


	while(counter < readbytes)
	{
		if(send_chain->used == 50)
		{printf("50");}

		// deserialize type
		send_chain->object[send_chain->used].type = buf[counter];
		counter++;

		// deserialize size
		memcpy(&send_chain->object[send_chain->used].size, &buf[counter], 2);
		counter+=2;

		// deserialize data itself, only if data is not NULL
		if(send_chain->object[send_chain->used].size > 0)
		{
			send_chain->object[send_chain->used].data = malloc(send_chain->object[send_chain->used].size);
			memcpy(send_chain->object[send_chain->used].data, &buf[counter], send_chain->object[send_chain->used].size);
			counter += send_chain->object[send_chain->used].size;
		}else
		{
			send_chain->object[send_chain->used].data = NULL;
		}

		// increase number of tlv objects reconstructed
		send_chain->used++;
	}
	//printf ("counter = %d\n", counter);
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


