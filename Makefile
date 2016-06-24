
ifndef CROSS_CC
CC=$(CROSS_COMPILE)gcc
else
CC=$(CROSS_CC)
endif

CFLAGS= -g -Wall -fPIC

LDFLAGS= -shared -ldl -rdynamic
objs= libtrace.o

#CFLAGS += -DDEBUG

target= libtrace.so

all: $(target)
	make -C test

$(target): $(objs)
	$(CC) $^  -o $@ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $^ -o $@

install: $(target)
	install -m 0755 $(target) $(CROSS_C_ROOT_PATH)/usr/lib
	install -m 0755 libtrace.h $(CROSS_C_ROOT_PATH)/usr/include
	make -C test install

clean:
	make -C test clean
	-rm $(target) 2>/dev/null || true
	-rm *.o 2>/dev/null || true
