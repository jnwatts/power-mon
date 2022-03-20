# CPPFLAGS = -ggdb -O0

CXXFLAGS = -std=gnu++20 -Wall -Wno-unused -Wno-narrowing -Werror -Wno-psabi

# Mosquitto
LDLIBS += $(shell pkg-config --libs libmosquitto)
CPPFLAGS += $(shell pkg-config --cflags libmosquitto)

all: power-mon mqtt-test

power-mon: eic.o report.o
mqtt-test: report.o

eic.o: eic.h eic_constants.h eic_registers.h
report.o: report.h time.h

clean:
	rm -f *.o power-mon mqtt-test
