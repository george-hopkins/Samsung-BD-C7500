#
#	Makefile -- Master makefile for all libraries.
#

.EXPORT_ALL_VARIABLES:

#
# Include architecture specific build rules.
#

ifndef ROOTDIR
ROOTDIR=..
endif

UCLINUX_BUILD_LIB=1
include $(LINUX_CONFIG)
include $(CONFIG_CONFIG)
include $(ARCH_CONFIG)

#
# always build these dir_
#
dir_y  = $(LIBCDIR) libnet
dir_y  += libnandutils
#dir_y  = libnet
dir_n  =
dir_   =

ifeq ($(LIBCDIR),libc)
dir_y += libm
endif

#
# and build these if someone needs them
#
dir_$(CONFIG_LIB_LIBDES)         += libdes
dir_$(CONFIG_LIB_LIBDES_FORCE)   += libdes
dir_$(CONFIG_LIB_LIBSSL)         += libssl
dir_$(CONFIG_LIB_LIBSSL_FORCE)   += libssl
dir_$(CONFIG_LIB_LIBGMP)         += libgmp
dir_$(CONFIG_LIB_LIBGMP_FORCE)   += libgmp
dir_$(CONFIG_LIB_LIBG)           += libg
dir_$(CONFIG_LIB_LIBG_FORCE)     += libg
dir_$(CONFIG_LIB_LIBPAM)         += libpam
dir_$(CONFIG_LIB_LIBPAM_FORCE)   += libpam
dir_$(CONFIG_LIB_LIBPCAP)        += libpcap
dir_$(CONFIG_LIB_LIBPCAP_FORCE)  += libpcap
dir_$(CONFIG_LIB_LIBATM)         += libatm
dir_$(CONFIG_LIB_LIBATM_FORCE)   += libatm
dir_$(CONFIG_LIB_FREETYPE)       += freetype
dir_$(CONFIG_LIB_FREETYPE_FORCE) += freetype
dir_$(CONFIG_LIB_ZLIB)           += zlib
dir_$(CONFIG_LIB_ZLIB_FORCE)     += zlib
dir_$(CONFIG_LIB_LIBUSB) 		 += libusb
dir_$(CONFIG_LIB_LIBUSB_FORCE)   += libusb

# THT libtermcap.so is delivered as part of bash
# dir_$(CONFIG_LIB_LIBTERMCAP)	 += $(ROOTDIR)/user/bash
# dir_$(CONFIG_LIB_LIBTERMCAP_FORCE)	 += $(ROOTDIR)/user/bash

# JD - independent libtermcap
dir_$(CONFIG_LIB_LIBTERMCAP)     += libtermcap
dir_$(CONFIG_LIB_GNU_LIBTERMCAP) += libtermcap

# THT Added lpopt & libbfd for oprofile.  Need to do it here, as oprofile insists on finding it
# during configuration
dir_$(CONFIG_USER_PROFILE_POFT)   += $(ROOTDIR)/user/popt
dir_$(CONFIG_LIB_LIBBFD)          += $(ROOTDIR)/lib/binutils/
dir_$(CONFIG_LIB_LIBIBERTY)       += $(ROOTDIR)/lib/binutils
dir_$(CONFIG_LIB_LIBCPLUSPLUS)	  += libc++


dir_$(CONFIG_LIB_SNAPGEAR)       += libsnapgear libconfigdd

all:
ifeq ($(LIBCDIR),glibc/build)
	[ -d glibc/build ] || mkdir glibc/build
	if [ ! -f glibc/build/config.cache ]; then \
		cd glibc/build; \
		export CFLAGS="$(VENDOR_CFLAGS) -DEMBED -O2"; \
		unset LDFLAGS; \
		../configure  --prefix= \
			$(GLIBC_TARGET_OPTS) --build=i386-redhat-linux \
			--with-headers=$(ROOTDIR)/$(LINUXDIR)/include \
			--without-cvs --without-gd \
			--disable-profile --disable-force-install \
			--enable-add-ons ; \
	fi
endif
# Force a rebuild of the Makefiles in the directfb tree.
	if [ -d directfb ]; then $(RM) directfb/Makefile directfb/configure directfb/config.cache; fi
	echo '$$$$$$$$$$$$$$$$$ Building all libraries $$$$$$$$$$$$$$$$$$$$$$$$'
	echo 'dir_y =' "$(dir_y)"
	for i in $(dir_y) ; do [ ! -d $$i ] || $(MAKEARCH) -C $$i || exit $? ; done
	# Link uuidlib
	-$(RM) -f $(ROMFSDIR)/lib/libuuid.so
	(cd $(ROMFSDIR)/lib; ln -s libuuid.so.1 libuuid.so)

romfs:
	for i in $(dir_y) ; do \
		[ ! -d $$i ] || $(MAKEARCH) -C $$i romfs || exit $$? ; \
	done
	# Handle libtermcap.so differently, since it is built as part of bash
#ifeq ($(CONFIG_LIB_LIBTERMCAP),y)
#	$(MAKEARCH) -C $(ROOTDIR)/user/bash $(ROOTDIR)/romfs/lib/libtermcap.so
#	-$(STRIP) $(ROOTDIR)/romfs/lib/libtermcap.so
#endif
	# THT: Copy libgcc_s.so to romfs dir
	-$(RM) -f $(ROMFSDIR)/lib/libgcc_s.*
	cp -af $(GCCLIB)/libgcc_s.* $(ROMFSDIR)/lib/
	-$(STRIP) $(ROMFSDIR)/lib/*

clean:
ifeq ($(LIBCDIR),glibc/build)
	rm -rf glibc/build
endif
	-for i in $(dir_y) $(dir_n) $(dir_); do \
		[ ! -d $$i ] || $(MAKEARCH) -C $$i clean ; \
	done
ifeq ($(LIBCDIR),uClibc)
	rm -f $(LIBCDIR)/Config
endif

distclean:
	-for i in $(dir_y) $(dir_n) $(dir_); do \
		[ ! -d $$i ] || $(MAKEARCH) -C $$i distclean ; \
	done
ifeq ($(LIBCDIR),uClibc)
	rm -f $(LIBCDIR)/Config
endif
