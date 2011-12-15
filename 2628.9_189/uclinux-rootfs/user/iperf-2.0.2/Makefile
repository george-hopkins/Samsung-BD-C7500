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

#CXXFLAGS += $(INCSG)
#CFLAGS += $(INCSG)

DIRS = compat src
#DIRS = src

all:
	for i in $(DIRS) ; do make -C $$i $@ || exit $?; done

clean:
	for i in $(DIRS) ; do make -C $$i clean ; done

romfs:
	$(ROMFSINST) -e CONFIG_USER_IPERF src/iperf /usr/bin/iperf
	$(ROMFSINST) -e CONFIG_USER_IPERF lib/iperf /lib/bin/iperf
ifneq (,$(findstring mipsel,$(CROSS)))
	$(ROMFSINST) -e CONFIG_USER_IPERF lib/le/libgcc_s.so.1 /lib/libgcc_s.so.1
	$(ROMFSINST) -e CONFIG_USER_IPERF lib/le/libstdc++.so.6 /lib/libstdc++.so.6
else
	$(ROMFSINST) -e CONFIG_USER_IPERF lib/be/libgcc_s.so.1 /lib/libgcc_s.so.1
	$(ROMFSINST) -e CONFIG_USER_IPERF lib/be/libstdc++.so.6 /lib/libstdc++.so.6
endif
