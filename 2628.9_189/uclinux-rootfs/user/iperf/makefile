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
UCLIBC_INC = -I$(ROOTDIR)/uClibc/include
LINUX_INC=-I$(ROOTDIR)/$(LINUXDIR)/include
# THT: Order is important, as we want to use the uClibc headers first.
MY_CFLAGS += $(UCLIBC_INC) $(LINUX_INC)

all:
	CC=$(CC) CFLAGS="$(MY_CFLAGS)" \
		./configure \
		--target=$(TARGET)		\
		--host=$(TARGET)	\
		--cache-file=config.cache.brcmstb.$(TARGET_ARCH) 
	make -f Makefile MAKE="make -f Makefile"

clean:
	make -f Makefile clean

distclean: clean
	make -f Makefile distclean

romfs:
	# PR11752 - Changed to allow make romfs to success multiple times
	$(ROMFSINST) -e CONFIG_USER_IPERF src/iperf /usr/bin/iperf
	$(ROMFSINST) -e CONFIG_USER_IPERF lib/iperf /lib/bin/iperf
ifneq (,$(findstring mipsel,$(CROSS)))
	$(ROMFSINST) -e CONFIG_USER_IPERF lib/le/libgcc_s.so.1 /lib/libgcc_s.so.1
	$(ROMFSINST) -e CONFIG_USER_IPERF lib/le/libstdc++.so.5 /lib/libstdc++.so.5
else
	$(ROMFSINST) -e CONFIG_USER_IPERF lib/be/libgcc_s.so.1 /lib/libgcc_s.so.1
	$(ROMFSINST) -e CONFIG_USER_IPERF lib/be/libstdc++.so.5 /lib/libstdc++.so.5
endif
