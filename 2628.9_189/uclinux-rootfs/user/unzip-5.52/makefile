include $(ROOTDIR)/config.arch

TOOL_DIR_PREFIX=$(shell dirname $(shell which $(CC)))/..

ifneq (,$(findstring linux,$(CROSS)))
	TARGET=$(CROSS:linux-=linux)
	TARGET_ARCH ?= $(CROSS:-linux-=)
else
	ifneq (,$(findstring uclibc,$(CROSS)))
		TARGET_ARCH ?= $(CROSS:-uclibc-=)
		TARGET=$(TARGET_ARCH)-linux
	else
		echo "FIXME: Cannot determine target architecture"
		exit 99
	endif
endif
UCLIBC_INC = -I$(ROOTDIR)/uClibc/include
LINUX_INC=-I$(ROOTDIR)/$(LINUXDIR)/include
GCC_INC=-I$(TOOL_DIR_PREFIX)/$(TARGET)/include
# THT: Order is important, as we want to use the uClibc headers first,
# and the headers from gcc last.
# BITS_PER_LONG is defined under #ifdef __KERNEL__ in 2.6 headers, so we define it here.
MY_CFLAGS		= -DDONE=1 -DBITS_PER_LONG=32 -D_LARGEFILE64_SOURCE -D_GNU_SOURCE $(CFLAGS) -I. -I.. -Wall

all:
	make -f Makefile LOCAL_UNZIP="$(MY_CFLAGS)" CC=mipsel-linux-gcc linux_noasm

clean:
	make -f Makefile clean

distclean: clean
	make -f Makefile clean

romfs:
	$(ROMFSINST) -e CONFIG_USER_UNZIP /bin/unzip
