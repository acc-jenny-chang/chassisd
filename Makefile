CFLAGS := -g -Os
prefix := /usr
etcprefix :=
LDFLAGS       := -static -L`pwd`/lib
# Define appropiately for your distribution
# DOCDIR := /usr/share/doc/packages/chassis

# Note when changing prefix: some of the non-critical files like
# the manpage or the init script have hardcoded prefixes

# Warning flags added implicitely to CFLAGS in the default rule
# this is done so that even when CFLAGS are overriden we still get
# the additional warnings
# Some warnings require the global optimizer and are only output with 
# -O2/-Os, so that should be tested occasionally
WARNINGS := -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter \
	    -Wstrict-prototypes -Wformat-security -Wmissing-declarations \
	    -Wdeclaration-after-statement

# The on disk database has still many problems (partly in this code and partly
# due to missing support from BIOS), so it's disabled by default. You can 
# enable it here by uncommenting the following line
# CONFIG_DISKDB = 1

all: chassis subdirs

.PHONY: install uninstall clean

OBJ := chassis.o msg.o memutil.o eventloop.o udp_client.o udp_server.o tcp_server.o crc_util.o bpdu.o \
                   packet.o netif_utils.o bpdu.o epoll_loop.o chassis_bpdu.o shutdown_mgr.o
#CLEAN := chassis dmi tsc dbquery .depend .depend.X dbquery.o ${DISKDB_OBJ}
CLEAN := chassis
DOC := chassis.pdf
SUBDIRS       := tcp_client udp_client spi_user 



ADD_DEFINES :=

ifdef CONFIG_DISKDB
ADD_DEFINES := -DCONFIG_DISKDB=1


endif

SRC := $(OBJ:.o=.c)

chassis: ${OBJ}

# dbquery intentionally not installed by default
install: subdirs-install
	install -m 755 -p chassis $(DESTDIR)${prefix}/local/accton/bin/chassis
	install -m 755 -p chassisd $(DESTDIR)${prefix}/local/accton/bin/chassisd
	install -m 644 -p -b chassis.conf $(DESTDIR)${prefix}/local/accton/bin/chassis.conf
	install -m 644 -p -b us_card_number $(DESTDIR)${prefix}/local/accton/parameter/us_card_number
	install -m 644 -p -b chassis_accton.conf $(DESTDIR)${prefix}/local/accton/bin/chassis_accton.conf
	install -m 644 -p -b chassis_facebook.conf $(DESTDIR)${prefix}/local/accton/bin/chassis_facebook.conf
	chassisd restart

uninstall: subdirs-uninstall
	chassisd stop
	rm -r -f $(DESTDIR)${prefix}/local/accton/bin/chassis
	rm -r -f $(DESTDIR)${prefix}/local/accton/bin/chassisd
	rm -r -f $(DESTDIR)${prefix}/local/accton/bin/chassis.conf
	rm -r -f $(DESTDIR)${prefix}/local/accton/parameter/us_card_number
	rm -r -f $(DESTDIR)${prefix}/local/accton/bin/chassis_accton.conf
	rm -r -f $(DESTDIR)${prefix}/local/accton/bin/chassis_facebook.conf

#clean: test-clean
clean: subdirs-clean
	rm -f ${CLEAN} ${OBJ} 

tsc:    tsc.c
	gcc -o tsc ${CFLAGS} -DSTANDALONE tsc.c ${LDFLAGS} $(LDFLAGS)


dbquery: db.o dbquery.o memutil.o

#depend: .depend

%.o: %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(WARNINGS) $(ADD_DEFINES) -o $@ $< $(LDFLAGS)


#.depend: ${SRC}
#	${CC} -MM -I. ${SRC} > .depend.X && mv .depend.X .depend

#include .depend

#Makefile: .depend

.PHONY: iccverify src test

# run the icc static verifier over sources. you need the intel compiler installed for this
DISABLED_DIAGS := -diag-disable 188,271,869,2259,981,12072,181,12331,1572

iccverify:
	icc -Wall -diag-enable sv3 $(DISABLED_DIAGS) $(ADD_DEFINES) $(SRC)	

clangverify:
	clang --analyze $(ADD_DEFINES) $(SRC)

src:
	echo $(SRC)

config-test: config.c
	gcc -DTEST=1 config.c -o config-test

test:
	$(MAKE) -C tests test DEBUG=""

VALGRIND=valgrind --leak-check=full

valgrind-test:
	$(MAKE) -C tests test DEBUG="${VALGRIND}"

test-clean:
	$(MAKE) -C tests clean

subdirs:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir; \
	done


subdirs-install:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir install; \
	done


subdirs-uninstall:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir uninstall; \
	done


subdirs-clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done