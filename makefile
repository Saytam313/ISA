all: server client

server: isaserver.c 
	gcc isaserver.c -o isaserver
client: isaclient.c
	gcc isaclient.c -o isaclient