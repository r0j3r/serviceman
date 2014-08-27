CFLAGS=-g -Wall -std=gnu99 -pedantic
serv: serv.o protocol.o
protocol.o: protocol.h protocol.c
serv.o: serv.c protocol.h error.h
servctl: servctl.o protocol.o
servctl.o: servctl.c protocol.h error.h  
pid1: pid1.o  
pid1.o: pid1.c 
