
ifndef CROSS_CC
CC=$(CROSS_COMPILE)gcc
else
CC=$(CROSS_CC)
endif

TARGET= trace_test
OBJS= main.o

CFLAGS = -g -Wall -Wno-unused-function -fPIC -finstrument-functions
LDFLAGS =  -L.. -Wl,-rpath .. -ltrace  -rdynamic -ldl

all:$(TARGET)

$(TARGET):$(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	- rm -fr $(TARGET) $(OBJS) 2>/dev/null
install:$(TARGET)
	install -m755 $(TARGET) $(ROOTFS)/usr/bin/
