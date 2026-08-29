CFLAGS += -Wall
CFLAGS += -Werror
TARGETS = hdjd explore

ifdef DEBUG
CFLAGS += -g -DDEBUG
endif

all: $(TARGETS)

hdjd: LDLIBS += $(shell pkg-config --libs libusb-1.0)
hdjd: LDLIBS += -framework CoreMIDI -framework CoreFoundation
hdjd: hdjd.o usb.o coremidi.o

explore: LDLIBS += $(shell pkg-config --libs libusb-1.0)
explore: CFLAGS += $(shell pkg-config --cflags libusb-1.0)
usb.o: CFLAGS += $(shell pkg-config --cflags libusb-1.0)


clean:
	rm -f $(TARGETS) *.o
