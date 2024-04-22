APP = dpdk-tx
CC = gcc
PKGCONF ?= pkg-config
CFLAGS += -O3 $(shell $(PKGCONF) --cflags libdpdk) -DALLOW_EXPERIMENTAL_API
LDFLAGS = $(shell $(PKGCONF) --libs libdpdk) -lm

all: $(APP)

$(APP): main.o
	$(CC) $(CFLAGS) main.o -o $(APP) $(LDFLAGS)

main.o: main.c
	$(CC) -c $(CFLAGS) main.c -o main.o

clean:
	rm -f *.o $(APP)