include $(ROOTDIR)/config.arch
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
ifeq ($(strip $(TARGET_ARCH)),mips)
	ENDIAN_DEFS = -D__MIPSEB__
else
	ifeq ($(strip $(TARGET_ARCH)),mipsel)
		ENDIAN_DEFS = -D__MIPSEL__
	else
		echo "FIXME: Cannot determine target endian"
		exit 99
	endif
endif
UCLIBC_INC = -I$(ROOTDIR)/uClibc/include
LINUX_INC=-I$(ROOTDIR)/$(LINUXDIR)/include
# THT: Order is important, as we want to use the uClibc headers first.
MY_CFLAGS += -I./linux-include $(UCLIBC_INC) $(LINUX_INC) -I$(ROOTDIR)/lib/libpcap $(ENDIAN_DEFS) -D__FAVOR_BSD
# Tell it where libpcap is
MY_LDFLAGS+= -L$(ROOTDIR)/lib/libpcap -lpcap

all:
	CC=$(CC) CFLAGS="$(MY_CFLAGS)" LDFLAGS="$(MY_LDFLAGS)" \
		./configure \
		--target=$(TARGET)		\
		--host=i386-pc-linux-gnu	\
		--build=i386-pc-linux-gnu	\
		--libdir=$(ROOTDIR)/lib/libpcap \
		--prefix=$(ROOTDIR)/romfs \
		--cache-file=config.cache.brcmstb.$(TARGET_ARCH)
	make -f Makefile

clean:
	make -f Makefile clean

distclean: clean
	make -f Makefile distclean

romfs:
	# PR11752 - Changed to allow make romfs to success multiple times
	$(ROMFSINST) -e CONFIG_USER_TCPDUMP_TCPDUMP /usr/bin/tcpdump
