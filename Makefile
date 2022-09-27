CC=cc
DESTDIR=/usr/local

CFLAGS=-Wall -O2
LFLAGS=
LFLAGS_ASKPASS=
LFLAGS_ODUS=-lcrypt
LFLAGS_GGETTY=-lcrypt

OBJS_LIBASKPASS=libaskpass.o
OBJS_UTIL=util.o
OBJS_ASKPASS=$(OBJS_LIBASKPASS) askpass.o
OBJS_ODUS=$(OBJS_LIBASKPASS) $(OBJS_UTIL) odus.o
OBJS_GGETTY=$(OBJS_LIBASKPASS) $(OBJS_UTIL) ggetty.o

TARGET_ASKPASS=askpass
TARGET_ODUS=odus
TARGET_GGETTY=ggetty

.PHONY: all install clean

all: $(TARGET_ASKPASS) $(TARGET_ODUS) $(TARGET_GGETTY)

$(TARGET_ASKPASS): $(OBJS_ASKPASS)
	$(CC) -o $@ $^ $(LFLAGS) $(LFLAGS_ASKPASS)
$(TARGET_ODUS): $(OBJS_ODUS)
	$(CC) -o $@ $^ $(LFLAGS) $(LFLAGS_ODUS)
$(TARGET_GGETTY): $(OBJS_GGETTY)
	$(CC) -o $@ $^ $(LFLAGS) $(LFLAGS_GGETTY)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

install:
	[[ -f "$(TARGET_ODUS)"    ]] && install -vDm6755 -- "$(TARGET_ODUS)" "$(DESTDIR)/bin/"
	[[ -f "$(TARGET_ASKPASS)" ]] && install -vDm0755 -- "$(TARGET_ASKPASS)" "$(DESTDIR)/bin/"
	[[ -f "$(TARGET_GGETTY)"  ]] && install -vDm0755 -- "$(TARGET_GGETTY)" "$(DESTDIR)/bin/"

uninstall:
	cd -- "$(DESTDIR)/bin/" && rm -fv -- "$(TARGET_ODUS)" "$(TARGET_ASKPASS)" "$(TARGET_GGETTY)"

clean:
	rm -fv -- $(OBJS_LIBASKPASS) $(OBJS_UTIL) $(OBJS_ODUS) $(OBJS_ASKPASS) $(OBJS_GGETTY) $(TARGET_ODUS) $(TARGET_ASKPASS) $(TARGET_GGETTY)

