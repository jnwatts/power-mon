CURRENT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

CPPFLAGS = -ggdb -O0

CXXFLAGS = -std=gnu++20 -Wall -Wno-unused -Wno-narrowing -Werror -Wno-psabi

# Mosquitto
LDLIBS += $(shell pkg-config --libs libmosquitto)
CPPFLAGS += $(shell pkg-config --cflags libmosquitto)

# Mysql/Mariadb
LDLIBS += $(shell mysql_config --libs)
CPPFLAGS += $(shell mysql_config --cflags)

# Mysql++
MYSQLPP_DIR=$(CURRENT_DIR)/mysql++-3.3.0
MYSQLPP_INSTALL=$(CURRENT_DIR)/mysql++
LDLIBS += -L$(MYSQLPP_INSTALL)/lib -lmysqlpp
CPPFLAGS += -I$(MYSQLPP_INSTALL)/include/mysql++

# cJSON
LDLIBS += $(shell pkg-config --libs libcjson)
CPPFLAGS += $(shell pkg-config --cflags libcjson)

all: power-mon-aggregator

deps: mysql++

collect.o: collect.h
database.o: database.h
power-mon-aggregator: database.o collect.o

.PHONY: mysql++
mysql++:
	if [ ! -f $(MYSQLPP_DIR)/Makefile ]; then \
		(cd $(MYSQLPP_DIR); ./configure --prefix=/) \
	fi
	+$(MAKE) -C $(MYSQLPP_DIR) install DESTDIR=$(MYSQLPP_INSTALL)

clean:
	rm -f *.o power-mon-aggregator
