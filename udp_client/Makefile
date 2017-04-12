INCLUDE =.
CFLAGS = -g
CC = gcc
LDFLAGS       := -static -L`pwd`/lib
prefix := /usr

.PHONY: clean

OBJ := udp_client.o

SRC := $(OBJ:.o=.c)

all: chassis_client

chassis_client: ${OBJ}
	gcc -o chassis_client ${OBJ} $(LDFLAGS)

#%.o: %.c
#	$(CC) -c $(CFLAGS) -o $@ $<

CLEAN := chassis_client

clean: 
	rm -f ${CLEAN} ${OBJ} 
	
install:
	install -m 755 -p ./chassis_client $(DESTDIR)${prefix}/local/accton/bin/chassis_client


uninstall:
	rm -f $(DESTDIR)${prefix}/local/accton/bin/chassis_client	
