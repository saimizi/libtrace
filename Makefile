
CC=$(CROSS_CC)
CFLAGS= -g -Wall -fPIC

is_root_root=$(shell ls -ld $${CROSS_C_ROOT_PATH} 2>/dev/null | awk '{print $$3}')

LDFLAGS= -shared
objs= libtrace.o

#CFLAGS += -DDEBUG

target= libtrace.so

all: $(target)

$(target): $(objs)
	$(CC) $^ $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $^ -o $@

ifeq ($(is_root_root),root)
install: $(target)
	sudo install -m 666 $(target) $(CROSS_C_ROOT_PATH)/usr/lib
	sudo install -m 666  libtrace.h $(CROSS_C_ROOT_PATH)/usr/include
else
install: $(target)
	install -m 666 $(target) $(CROSS_C_ROOT_PATH)/usr/lib
	install -m 666  libtrace.h $(CROSS_C_ROOT_PATH)/usr/include
endif

clean:
	-rm $(target) 2>/dev/null || true
	-rm *.o 2>/dev/null || true
