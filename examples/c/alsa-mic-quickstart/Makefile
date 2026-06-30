# Build the ALSA mic quickstart against an installed voxrt-wake-word.
#
# Assumes the tarball was extracted into /usr/local (or any pkg-config
# search path). If you extracted somewhere else, point pkg-config at
# it before running make:
#
#   PKG_CONFIG_PATH=~/voxrt-wake-word/lib/pkgconfig make
#
# Run:
#   ./alsa-mic-quickstart voxrt_wake_word.vxrt

CC      ?= gcc
CFLAGS  ?= -O2 -Wall -Wextra
LDFLAGS ?=

PKGCONFIG_CFLAGS := $(shell pkg-config --cflags voxrt-wake-word)
PKGCONFIG_LIBS   := $(shell pkg-config --libs voxrt-wake-word)
ALSA_LIBS        := $(shell pkg-config --libs alsa 2>/dev/null || echo -lasound)

TARGET := alsa-mic-quickstart

.PHONY: all clean

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) $(PKGCONFIG_CFLAGS) main.c \
	    $(PKGCONFIG_LIBS) $(ALSA_LIBS) \
	    $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET)
