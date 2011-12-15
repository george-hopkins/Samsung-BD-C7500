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

OBJS = delay.o error.o gettimeofday.o inet_ntop.o inet_pton.o signal.o snprintf.o string.o Thread.o 

LIBRARY = libcompat.a
CFLAGS += -I../ -I../include -DHAVE_CONFIG_H
CXXFLAGS += -I../ -I../include -DHAVE_CONFIG_H

all: $(LIBRARY)

$(LIBRARY): $(OBJS)
	$(AR) rcv $@ $(OBJS)
	-$(RANLIB) $@

clean::
	$(RM) $(OBJS) $(LIBRARY)
