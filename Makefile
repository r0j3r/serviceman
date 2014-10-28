VPATH=..

CC=gcc
CFLAGS= -flto -MD -MP -fPIE -O2 -g -Wall -std=gnu99 -pedantic -march=native
LDFLAGS= -flto -fuse-linker-plugin -fPIE -O2 -g -Wall -std=gnu99 -pedantic -march=native

all: boot-wrapper process-manager serviceman-shutdown early-boot-arbitrator pid1 volume-manager servctld

process-manager: next_start.o child_proc.o protocol.o process-manager.o notification.o message.o control_proc.o

volume-manager: volume-manager.o notification.o message.o

early-boot-arbitrator: message.o early-boot-arbitrator.o notification.o

pid1: pid1.o  

test_notify: test_notify.o notification.o message.o

test_notifications: test_notifications.o notification.o message.o 

boot-wrapper: boot-wrapper.o notification.o message.o 

servctld: parse_def.o protocol.o servctld.o message.o notification.o

test_parse_def: test_parse_def.o parse_def.o 

test_load_daemons: test_load_daemons.o parse_def.o

serviceman-shutdown: serviceman-shutdown.o

test_get_next_timeout: CFLAGS=-fPIE -O -g -Wall -std=gnu99 -pedantic -march=native
test_get_next_timeout: test_get_next_timeout.o

-include *.d

.PHONY: all
