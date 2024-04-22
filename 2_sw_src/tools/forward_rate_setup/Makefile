APP = snic_feedback
CC = gcc
PKGCONF ?= pkg-config
CFLAGS += -O3
LDFLAGS = -lm

all: $(APP)

$(APP): main.o
	$(CC) $(CFLAGS) main.o -o $(APP) $(LDFLAGS)

main.o: main.c
	$(CC) -c $(CFLAGS) main.c -o main.o

clean:
	rm -f *.o $(APP)
