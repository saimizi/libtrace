
ifndef CROSS_CC
CC=$(CROSS_COMPILE)gcc
else
CC=$(CROSS_CC)
endif

MAJOR=0
MINOR=0.1
LINKER_NAME = libtrace.so
SONAME = $(LINKER_NAME).$(MAJOR)
target= $(SONAME).$(MINOR)

CFLAGS= -g -Wall -fPIC

LDFLAGS= -shared -ldl -rdynamic -Wl,-soname,$(SONAME)
objs= libtrace.o



all: $(target)
	make -C test

$(target): $(objs)
	$(CC) $^  -o $@ $(LDFLAGS)
	unlink $(SONAME) 2>/dev/null || true
	ln -s $(target) $(SONAME)
	unlink $(LINKER_NAME) 2>/dev/null || true
	ln -s $(SONAME) $(LINKER_NAME)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

install: $(target)
	install -d $(ROOTFS)/usr/local/lib
	install -m 0755 $(target) $(ROOTFS)/usr/local/lib
	cp -d $(SONAME) $(LINKER_NAME) $(ROOTFS)/usr/local/lib/
	install -d $(ROOTFS)/usr/local/include
	install -m 0644 libtrace.h $(ROOTFS)/usr/local/include
	make -C test install

clean:
	make -C test clean
	@rm *.o 2>/dev/null || true
	@rm $(SONAME) $(LINKER_NAME) $(target) 2>/dev/null || true
