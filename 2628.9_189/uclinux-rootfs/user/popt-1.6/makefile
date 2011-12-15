sinclude $(ROOTDIR)/config.arch

TOOL_DIR_PREFIX=$(shell dirname $(shell which $(CC)))/..

ifneq (,$(findstring linux,$(CROSS)))
	TARGET=$(CROSS:linux-=linux)
	TARGET_ARCH ?= $(CROSS:-linux-=)
else
	ifneq (,$(findstring uclibc,$(CROSS)))
		TARGET_ARCH ?= $(CROSS:-uclibc-=)
		TARGET=$(TARGET_ARCH)-linux
	else
		#echo "FIXME: Cannot determine target architecture"
		#exit 99
	endif
endif
UCLIBC_INC = -I$(ROOTDIR)/uClibc/include
LINUX_INC=-I$(ROOTDIR)/$(LINUXDIR)/include
GCC_INC=-I$(TOOL_DIR_PREFIX)/$(TARGET)/include
# THT: Order is important, as we want to use the uClibc headers first,
# and the headers from gcc last.
# BITS_PER_LONG is defined under #ifdef __KERNEL__ in 2.6 headers, so we define it here.
MY_CFLAGS += $(GCC_INC)

all:
	# This is optional software, so it may not be cleaned even with a make distclean
	-rm -f Makefile config.cache config.h config.status
	CC=$(CC) CFLAGS="$(MY_CFLAGS)" \
		./configure \
		--target=$(TARGET)		\
		--host=$(TARGET)	\
#		--transform="s,^,mipsel-linux-,;" \
#		--cache-file=config.cache.brcmstb.$(TARGET_ARCH)
	make -f Makefile

clean:
	-make -f Makefile clean
	# The least we can do is to delete all .o files
	find . -name '*.o' -exec /bin/rm -f {} \;

distclean: clean
	make -f Makefile distclean
	-rm Makefile config.cache config.h config.status

romfs:
	make -C intl $@
	$(ROMFSINST)  .libs/libpopt.so.0.0.0 /lib/libpopt.so.0.0.0
	$(ROMFSINST) -s ./libpopt.so.0.0.0 /lib/libpopt.so
	$(ROMFSINST) -s ./libpopt.so.0.0.0 /lib/libpopt.so.0


all-recursive install-data-recursive install-exec-recursive installdirs-recursive \
install-recursive uninstall-recursive check-recursive installcheck-recursive \
info-recursive dvi-recursive \
tags-recursive tags all-am\
mostlyclean-recursive clean-recursive distclean-recursive \
maintainer-clean-recursive:
	make -f Makefile $@
