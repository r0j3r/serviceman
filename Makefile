CFLAGS=-O -g -Wall -std=gnu99 -pedantic
LDFLAGS=
serv: serv.o protocol.o
protocol.o: protocol.h protocol.c
serv.o: serv.c protocol.h error.h
servctl: servctl.o protocol.o
servctl.o: servctl.c protocol.h error.h 
process-manager: process-manager.o
volume-manager: volume-manager.o notification.o
volume-manager.o: notification.h
notification.o: notification.h
early-boot-arbitrator: early-boot-arbitrator.o notification.o
pid1: pid1.o  
pid1.o: pid1.c 
