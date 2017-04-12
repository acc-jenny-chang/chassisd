export CC=gcc
export CXX=g++
export LD=ld

TARGET1 := bcm5389
TARGET2 := bcm5396
OBJS    := $(TARGET1).o 
#LIBS    := -lpthread -lresolv
CFLAGS  := -O2 -s -w -Wall -Wstrict-prototypes -ARCH=x86_64 -m64 


all: clean $(TARGET1) $(TARGET2)
$(TARGET1): $(OBJS)
	$(CC) -o $@ $(OBJS) $(PCI_OBJS) $(LIBS)

$(TARGET1).o: $(TARGET1).c
	$(CC) $(CFLAGS) -c $< $(INCS)

$(TARGET2):
	cp -f $(TARGET1) bcm5396 

install:
	cp -f $(TARGET1) /sbin/
	cp -f $(TARGET1) bcm5396
	cp -f bcm5396 /sbin

uninstall:
	rm -f /sbin/$(TARGET1)
	rm -f /sbin/$(TARGET2)

clean:
	rm -f $(TARGET1) $(TARGET2) $(OBJS)
