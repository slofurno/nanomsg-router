cflags = -std=c11 

all: server worker

server: server.c
	gcc $(cflags) -o server server.c -lnanomsg -lmill

worker: worker.c
	gcc $(cflags) -o worker worker.c -lnanomsg -lmill
