
CC=gcc
CFLAGS=-fPIE -O2 -g -Wall -std=gnu99 -pedantic -march=native
LDFLAGS=-fPIE -O2 -g -Wall -std=gnu99 -pedantic -march=native

all: boot-wrapper process-manager serviceman-shutdown early-boot-arbitrator pid1 volume-manager servctld

protocol.o: protocol.h protocol.c

process-manager: next_start.o child_proc.o protocol.o process-manager.o notification.o message.o control_proc.o
child_proc.o: child_proc.h
process-manager.o: child_proc.h

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

servctld: parse_def.o protocol.o servctld.o message.o notification.o

test_parse_def: test_parse_def.o parse_def.o 

test_load_daemons: test_load_daemons.o parse_def.o

serviceman-shutdown: serviceman-shutdown.o

test_get_next_timeout: CFLAGS=-fPIE -O -g -Wall -std=gnu99 -pedantic -march=native
test_get_next_timeout: test_get_next_timeout.o

.PHONY: all
