sinclude $(ROOTDIR)/config.arch

TOOL_DIR_PREFIX=$(shell dirname $(shell which $(CC)))/..

ifneq (,$(findstring linux,$(CROSS)))
	TARGET=$(CROSS:linux-=linux)
else
	ifneq (,$(findstring uclibc,$(CROSS)))
		TARGET_ARCH ?= $(CROSS:-uclibc-=)
		TARGET=$(TARGET_ARCH)-linux
	else
#		echo "FIXME: Cannot determine target architecture"
#		exit 99
	endif
endif

AR=$(CROSS)ar
AS=$(CROSS)as
LD=$(CROSS)ld
NM=$(CROSS)nm
CC=$(CROSS)gcc
GCC=$(CROSS)gcc
CXX=$(CROSS)g++
RANLIB=$(CROSS)ranlib
LDFLAGS =  -O2  -pthread  -DHAVE_CONFIG_H

EXEC = iperf
OBJS = Client.o gnu_getopt_long.o Listener.o ReportDefault.o service.o sockets.o \
	Extractor.o Launch.o Locale.o PerfSocket.o Reporter.o Settings.o stdio.o \
	gnu_getopt.o List.o main.o ReportCSV.o Server.o SocketAddr.o tcp_window_size.o

CFLAGS += -I../ -I../include -DHAVE_CONFIG_H
CXXFLAGS += -I../ -I../include -DHAVE_CONFIG_H

USELIBS = ../compat/libcompat.a

all: $(EXEC)

$(EXEC): $(OBJS) $(USELIBS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(USELIBS) $(LDLIBS)

clean:
	-rm -f $(EXEC) *.o
