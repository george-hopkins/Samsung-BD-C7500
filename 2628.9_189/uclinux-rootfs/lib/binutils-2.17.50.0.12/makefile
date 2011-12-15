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
MY_CFLAGS +=
SUBDIRS = libiberty bfd opcodes etc gas gprof intl ld mips

all:
	# This is optional software, so it may not be cleaned even with a make distclean
	# and will faile to build if we switch from LE to BE build and vice versa.
	-test -f Makefile && make -f Makefile distclean
	-rm -f Makefile config.cache config.h config.status
	-for lib in $(SUBDIRS); do rm -f $$lib/config.cache; done
	CC=$(CC) CFLAGS="$(MY_CFLAGS)" \
		./configure \
		--target=$(TARGET)		\
		--host=$(TARGET)	\
		--build=i386-linux \
		--enable-multilib \
#		--disable-nls \
#		--cache-file=config.cache.brcmstb.$(TARGET_ARCH)
	make -f Makefile

distclean-host:
	make -f Makefile $@

all-host:
	make -f Makefile $@

all-target:
	make -f Makefile $@

info-host:
	make -f Makefile $@

info-target:
	make -f Makefile $@

dvi-host:
	make -f Makefile $@

dvi-target:
	make -f Makefile $@

pdf-host:
	make -f Makefile $@

pdf-target:
	make -f Makefile $@

html-host:
	make -f Makefile $@

html-target:
	make -f Makefile $@

TAGS-host:
	make -f Makefile $@

TAGS-target:
	make -f Makefile $@

clean:
	make -f Makefile clean

distclean: clean
	make -f Makefile distclean
	-rm Makefile config.cache config.h config.status
	-for lib in $(SUBDIRS); do rm -f $$lib/config.cache; done

romfs:
	$(ROMFSINST) -e CONFIG_LIB_LIBIBERTY libiberty/.libs/libiberty.so /lib/libiberty.so
