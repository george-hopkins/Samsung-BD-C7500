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
GXX_INC=-I$(TOOL_DIR_PREFIX)/$(TARGET)/include/c++/4.2.0 -I$(TOOL_DIR_PREFIX)/$(TARGET)/include
# THT: For libiberty.h
BFD_INCLUDE=-I$(ROOTDIR)/lib/binutils/bfd -I$(ROOTDIR)/lib/binutils/include
# THT: Order is important, as we want to use the uClibc headers first,
# and the headers from gcc last.
MY_CFLAGS += $(BFD_INCLUDE) -I$(ROOTDIR)/user/popt $(GCC_INC)
MY_CPPFLAGS += $(MY_CFLAGS)
MY_CXXFLAGS = $(GXX_INC) $(MY_CFLAGS)
MY_LDFLAGS += \
	-Bstatic -L$(ROOTDIR)/lib/binutils/intl \
	-L$(ROOTDIR)/user/popt/.libs \
	-L$(ROOTDIR)/lib/binutils/bfd \
	-L$(ROOTDIR)/lib/binutils/libiberty \
	-L$(ROOTDIR)/uClibc/lib

TARGET_PREFIX=/usr/local/share/oprofile/
BUILD_PREFIX=$(ROOTDIR)/romfs/$(TARGET_PREFIX)

RECURSIVE_TARGETS = all-recursive check-recursive dvi-recursive \
	html-recursive info-recursive install-data-recursive \
	install-exec-recursive install-info-recursive \
	install-recursive installcheck-recursive installdirs-recursive \
	pdf-recursive ps-recursive uninstall-info-recursive \
	uninstall-recursive

all:
	# This is optional software, so it may not be cleaned even with a make distclean
	# and will faile to build if we switch from LE to BE build and vice versa.
	-test -f Makefile && make -f Makefile distclean
	-rm -f Makefile config.cache config.h config.status
	CC=$(CC) LD=$(LD) CFLAGS="$(MY_CFLAGS)" LDFLAGS="$(MY_LDFLAGS)" CXXFLAGS="$(MY_CXXFLAGS)" CPPFLAGS="$(MY_CPPFLAGS)"\
	VERSION=\"0.9.1\" PACKAGE=\"oprofile\" \
		./configure \
		--target=$(TARGET)		\
		--host=$(TARGET)	\
		--build=i386-linux \
		--prefix=/usr/local \
		--with-kernel-support \
		--with-linux=$(ROOTDIR)/$(LINUX_DIR) \
		--with-module-dir=$(ROOTDIR)/romfs/lib/modules \
		--with-package=yes \
		--with-binutils=$(ROOTDIR)/lib/binutils \
#		--cache-file=config.cache.brcmstb.$(TARGET_ARCH)
	make -f Makefile

#libiberty.a: libiberty-$(TARGET_ARCH).a
#	-rm $@
#	ln -s $< $@

clean:
	make -f Makefile clean

distclean: clean
	make -f Makefile distclean
	-rm -f Makefile config.cache config.h config.status

$(RECURSIVE_TARGETS) all-am am--refresh:
	make -f Makefile $@

romfs:
	$(ROMFSINST) -e CONFIG_USER_PROFILE_OPROFILE_FULL ./pp/opannotate /usr/bin/opannotate
	$(ROMFSINST) -e CONFIG_USER_PROFILE_OPROFILE_FULL ./pp/oparchive /usr/bin//oparchive
	$(ROMFSINST) -e CONFIG_USER_PROFILE_OPROFILE -p 555 ./utils/opcontrol /usr/bin/opcontrol
	$(ROMFSINST) -e CONFIG_USER_PROFILE_OPROFILE_FULL ./pp/opgprof /usr/bin/opgprof
	$(ROMFSINST) -e CONFIG_USER_PROFILE_OPROFILE ./utils/ophelp /usr/bin/ophelp
	$(ROMFSINST) -e CONFIG_USER_PROFILE_OPROFILE ./pp/opreport /usr/bin/opreport
	$(ROMFSINST) -e CONFIG_USER_PROFILE_OPROFILE ./daemon/oprofiled /usr/bin/oprofiled
	# Installed in lib
	#$(ROMFSINST) -e CONFIG_USER_PROFILE_OPROFILE \
	#	$(TOOL_DIR_PREFIX)/$(TARGET)-uclibc/lib/libstdc++.so.6 \
	#	/lib/libstdc++.so.6
	#$(ROMFSINST) -s ./libstdc++.so.6 /lib/libstdc++.so
	#$(ROMFSINST) -e CONFIG_USER_PROFILE_OPROFILE \
	#	$(TOOL_DIR_PREFIX)/$(TARGET)-uclibc/lib/libgcc_s.so.1 \
	#	/lib/libgcc_s.so.1
	#$(ROMFSINST) -s ./libgcc_s.so.1 /lib/libgcc_s.so
	# Provide objdump since we already built it
	$(ROMFSINST) -e CONFIG_USER_PROFILE_OPROFILE $(ROOTDIR)/lib/binutils/binutils/objdump /usr/bin/objdump

