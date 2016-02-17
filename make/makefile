BIN = ../bin/
OBJ = ../obj/
SRC = ../src/
INCLUDE = -I../include
CC = gcc
CFLAGS = -c -Wall -g

all:$(BIN)udp_server $(BIN)udp_client


$(BIN)udp_server:$(OBJ)udp_server.o
	$(CC) -o udp_server $(OBJ)udp_server.o
	mv ./udp_server $(BIN)

$(OBJ)udp_server.o:$(SRC)udp_server.c ../include/header.h
	$(CC) $(CFLAGS) $(SRC)udp_server.c  $(INCLUDE)
	mv ./udp_server.o $(OBJ)

$(BIN)udp_client:$(OBJ)udp_client.o
	$(CC) -o udp_client $(OBJ)udp_client.o
	mv ./udp_client $(BIN)

$(OBJ)udp_client.o:$(SRC)udp_client.c ../include/header.h
	$(CC) $(CFLAGS) $(SRC)udp_client.c  $(INCLUDE)
	mv ./udp_client.o $(OBJ)

clean :
	rm $(OBJ)*.o
	rm $(BIN)*
