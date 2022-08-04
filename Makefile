CC=cc
CFLAGS=-Wall -O2
LFLAGS=-lm

OBJS=main.o

TARGET=odus

.PHONY: all clean

all: $(TARGET)

setuid:
	chown 0:0 -- "$(TARGET)"
	chmod ug+s -- "$(TARGET)"

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LFLAGS)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -fv -- $(OBJS) $(TARGET)
