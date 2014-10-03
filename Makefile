CC=gcc
CFLAGS=-fPIE -flto -O -g -Wall -std=gnu99 -pedantic -march=native
LDFLAGS=-fPIE -flto -fuse-linker-plugin -O -g -Wall -std=gnu99 -pedantic -march=native
serv: serv.o protocol.o
protocol.o: protocol.h protocol.c
serv.o: serv.c protocol.h error.h
servctl: servctl.o protocol.o
servctl.o: servctl.c protocol.h error.h 
process-manager: process-manager.o
volume-manager: volume-manager.o notification.o message.o
volume-manager.o: notification.h
notification.o: notification.h
early-boot-arbitrator: message.o early-boot-arbitrator.o notification.o
early-boot-arbitrator.o: message.h notification.h
message.o: message.h
pid1: pid1.o  
pid1.o: pid1.c 
test_notify: test_notify.o notification.o message.o
test_notifications: test_notifications.o notification.o message.o 
boot-wrapper: boot-wrapper.o notification.o message.o 
