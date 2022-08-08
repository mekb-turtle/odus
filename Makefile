CC=cc
DESTDIR=/usr/local
CFLAGS=-Wall -O2
LFLAGS=-lm
LFLAGS_ODUS=-lcrypt

OBJS_LIBASKPASS=libaskpass.o
OBJS_ASKPASS=$(OBJS_LIBASKPASS) askpass.o
OBJS_ODUS=$(OBJS_LIBASKPASS) odus.o

TARGET_ASKPASS=askpass
TARGET_ODUS=odus

.PHONY: all install clean

all: $(TARGET_ODUS) $(TARGET_ASKPASS)

$(TARGET_ODUS): $(OBJS_ODUS)
	$(CC) -o $@ $^ $(LFLAGS) $(LFLAGS_ODUS)
$(TARGET_ASKPASS): $(OBJS_ASKPASS)
	$(CC) -o $@ $^ $(LFLAGS)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

install:
	install -vDm6755 -- $(TARGET_ODUS) $(DESTDIR)/bin/
	install -vDm0755 -- $(TARGET_ASKPASS) $(DESTDIR)/bin/

uninstall:
	cd -- $(DESTDIR)/bin/ && rm -fv -- $(TARGET_ODUS) $(TARGET_ASKPASS)

clean:
	rm -fv -- $(OBJS_LIBASKPASS) $(OBJS_ODUS) $(OBJS_ASKPASS) $(TARGET_ODUS) $(TARGET_ASKPASS)

