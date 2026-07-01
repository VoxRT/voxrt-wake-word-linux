CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra

PKGCONFIG_CFLAGS := $(shell pkg-config --cflags voxrt-wake-word)
PKGCONFIG_LIBS   := $(shell pkg-config --libs voxrt-wake-word)
ALSA_LIBS        := $(shell pkg-config --libs alsa 2>/dev/null || echo -lasound)

TARGET := alsa-mic-quickstart-cpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) $(PKGCONFIG_CFLAGS) main.cpp \
	    $(PKGCONFIG_LIBS) $(ALSA_LIBS) \
	    -o $(TARGET)

clean:
	rm -f $(TARGET)
