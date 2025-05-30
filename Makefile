.POSIX:

.PHONY: clean uninstall install

CC = gcc
PREFIX = /usr/local

#CFLAGS =
#LDFLAGS =

# for non developers, you probably don't need to touch the below
CFLAGS += -std=c99 -Wall -c -Wno-deprecated-declarations -Iinclude -g -pthread `pkg-config --cflags pixman-1 wayland-server x11 xinerama xxf86vm babl`
LDFLAGS += -pthread `pkg-config --libs pixman-1 wayland-server x11 xinerama xxf86vm babl` -lm

EXECUTABLE = wlonx

SOURCES = wolfen-compositor.c wolfen-misc.c wolfen-pixfmt.c wolfen-shell.c wolfen-surface.c wolfen.c
OBJECTS = $(SOURCES:.c=.o)

all: $(EXECUTABLE)

# link executable
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

clean:
	rm -f $(DEPENDS) $(OBJECTS) $(EXECUTABLE)

install: all
	cp $(EXECUTABLE) $(PREFIX)/bin/$(EXECUTABLE)

uninstall:
	rm -f $(PREFIX)/bin/$(EXECUTABLE)

wolfen-compositor.o: wolfen-compositor.c \
	include/wolfen-display.h include/wolfen-pixfmt.h \
	include/wolfen-surface.h include/wolfen-misc.h include/wolfen-pixfmt.h \
	include/wolfen-compositor.h

wolfen-misc.o: wolfen-misc.c include/wolfen-misc.h

wolfen-pixfmt.o: wolfen-pixfmt.c include/wolfen-pixfmt.h include/wolfen-display.h

wolfen-surface.o: wolfen-surface.c include/wolfen-misc.h \
	include/wolfen-display.h include/wolfen-pixfmt.h \
	include/wolfen-display.h include/wolfen-surface.h include/wolfen-misc.h \
	include/wolfen-pixfmt.h
