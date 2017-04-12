INCLUDE =.
CFLAGS = -g
CC = cc
LDFLAGS       := -static -L`pwd`/lib
prefix := /usr

.PHONY: clean

OBJ := tcp_client.o

SRC := $(OBJ:.o=.c)

all: chassis_agent

chassis_agent: ${OBJ}
	gcc -o chassis_agent ${OBJ} $(LDFLAGS)

#%.o: %.c
#	$(CC) -c $(CFLAGS) -o $@ $<

CLEAN := chassis_agent

clean: 
	rm -f ${CLEAN} ${OBJ} 
install:
	install -m 755 -p ./chassis_agent $(DESTDIR)${prefix}/local/accton/bin/chassis_agent


uninstall:
	rm -f $(DESTDIR)${prefix}/local/accton/bin/chassis_agent	
