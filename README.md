# Reliable-UDP-Server---Client
 

Reliable UDP

                                             Header Design
                                             
As specified in the problem statement(checked in master) , following are the assumptions specified for header design for the reliable udp which is again assumed to be a simplified version of the tcp header:

 1) Packet size should be less than 1500 bytes.

 2) Since, header size is fixed, there is no need for the provision of an 'options' field.

 3) Only flag to be supported is the ACK flag.

 4) Advertised Window is an entity which is shared offline provided through command prompt.

 5) No urgent data is expected so URG pointer is omitted.

 Taking the aforementioned assumptions into consideration, following is a sample reliable udp header:

 1) source port : 16 bits

 2) destination port: 16 bits

 3) sequence number : 32 bits

 4) acknowledgement number : 32 bits

 5) length: 16 bits

 6) checksum: 16 bits

 7) Ack Flag: 1 bit

 8) Spare: 15 bits

                                  Implementation of header and reliable UDP packet

 The header discussed above is implemented using a structure with name reliable_udp_packet_t in the header.h file. This structure also includes a char array called 'data' which holds the value of data of the
segment/packet data of any single segment. The same structure is used by the server file as well. To compose the packet/segment at the client end and send it over to the server, the fields of the packet are
added in a tag-length-value chain triplet format array and then serialized in a buffer. Serialization is done to support machine portability and prevent corruption of data. At the server the buffer is “deserialized” in a taglength-
value triplet and then used to process information. The same process is followed when composing a packet on the server side and sending it to the client side.

 Steps to run:

 1) Untar using tar –xPvf reliable_udp_resubmission.tar and go to the make directory.

 2) Use commands: make clean; make all

 3) Binaries will be formed in /bin directory

 4) To run on the same system , requested file is to be kept in the bin directory. 221baker.txt which I used for testing purposes is kept in the /bin and /src directories and can also be used for review
purposes.

 5) After a make command is run, the make directory will be cleaned and the data file – 221baker.txt would need to be copied from /src folder to /bin folder before execution of binaries from there.

 6) To run server on a different system, requested file should be in the same directory as the server.

 7) Server binary can be run as . /udp_server <server_ip> <server_port_number> <advertised sender window> Eg. /udp_server 10.10.10.1 4121 5 (server ip entered should be present on the eth0 or io
interface before running the binary. Also advertised window here is assumed to be equal initially for both server and client. So, receiver and sender window is equal intially)

 8) Client binary can be run as ./udp_client <server_ip> <server_port> <filename> <advertised receiver window> Eg. /udp_client 10.10.10.1 4121 221baker.txt 5

 9) If server does not send any data for 10 seconds to the currently communicating sequential client , the connection at the client's end times out.

                                                    Sliding Window Implementation

 The sliding window implementation comprises the advertised window on both server and client binary execution initially which is assumed to be the same for both server and client initially.

 Sender side implementation
 
  1) When sender has a new segment to transmit, it takes an unused sequence number from the window and transmits the packet and sets timeout for the transmission.

 2) If sender does not have a frame to transmit, it stops and the client will timeout if it does not receive any data from the sender (server) for 10 s.

 3) The sender on receiving an ACK from the client will send the frame with the next sequence number and slide the window to the right by one. (lower bound increased by one) . This is implemented in the code by
decrementing a counter corresponding to the sender window by one every time an ACK is received and when this variable reaches zero , it is then basically re-initialized to the value of advertised sender window.
Sequence number is a variable that keeps on increasing with every frame sent and so all sequence numbers in a particular window are unique and unused.

                                                      Receiver Side Implementation

 1) If the value of the advertised window is entered as 5 say, the receiver window for sequence numbers becomes [lower-bound, lower-bound + advertised_window].

 2) The receiver will receive packets only if their sequence numbers lie within the window range.

 3) If the received frame has a sequence number which is in-order with the previous one, which is detected using a flag, the data contained in the frame is printed onto the console.

 4) If the frame received is out of order is placed in an “out-of-order” buffer and is retrieved only when frames with sequence numbers less than the out of order frame's sequence numbers have arrived and their
data duly displayed.

 5) Receiver will send a Cumulative Ack -

 a) if an out-of order segment is received for the last in-order sequence number's frame acknowledging that data in packets till that sequence number has been received successfully.

 b) if all sequence numbers in the window have been received and the window slides over to the next group of sequence numbers.

                                            Jacobson – Karel's Algorithm and Congestion Control

 Jacobson-Karel's Algorithm has been implemented and the timeout value calculated as indicated in the textbook

 Congestion control comprises of the stages: Slow Start and Congestion Avoidance.

 1) Slow Start: The sending rate increases exponentially sending packets in the numbers: 1, 2, ,4, 8.. and so on. The 'cwnd' grows by one MSS (1500 bytes) for every ACK received, until it reaches ssthresh.

 2) Congestion Avoidance: When cwnd reaches ssthresh, the value of cwnd = ssthresh/2 initially. Later for every ACK received it is incremented as cwnd = cwnd+ MSS (MSS/cwnd).

High Latency Communication

Latency was simulated using sleep functions which are currently commented in code. Following is an effect of high latency in Congestion Control Algorithms:
In both the slow start and congestion avoidance stages frequent timeouts occur and the congestion window size decreases to a small value leading to Silly Window Syndrome.

Variable Packet Loss Rate

Random numbers were generated in the range of the sliding window and packets were probabilistically dropped. This also leads to smaller congestion window sizes.
