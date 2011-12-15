ifndef BDVD_NONFREE_TOOLS_DIR
  BDVD_NONFREE_TOOLS_DIR = /projects/stbdev/bdvd/sync/nonfree_tools
endif

CROSSTOOLS_DIR = $(strip $(subst /,_,\
  $(patsubst %/bin/mipsel-linux-gcc,%,$(shell which mipsel-linux-gcc))))
NONFREE_FULL_PATH = $(BDVD_NONFREE_TOOLS_DIR)/$(CROSSTOOLS_DIR)
ROOTFS_BUILT_INDICATOR = ../../lib/libtermcap/libtermcap.a


all: $(ROOTFS_BUILT_INDICATOR)
	make clean
	make gdb-cross
	rm -f gdb/mipsel-linux-gdb
	mv gdb/gdb gdb/mipsel-linux-gdb
	make clean
	make gdb-nat
	rm -f gdb/gdb-nat
	cp gdb/gdb gdb/gdb-nat

gdb-nat:
	set AR=mipsel-linux-ar
	set AS=mipsel-linux-as
	set LD=mipsel-linux-ld
	set NM=mipsel-linux-nm
	set CC=mipsel-linux-gcc
	set GCC=mipsel-linux-gcc
	set CXX=mipsel-linux-g++
	set RANLIB=mipsel-linux-ranlib
	cp gdb/configure-nat gdb/configure
	./configure --build=i686-pc-linux-gnu --target=mipsel-linux --host=mipsel-linux --disable-nls --without-included-gettext --without-uiout --disable-tui --disable-gdbtk --without-x 
	make -f Makefile all-gdb

gdb-cross:
	set AR=mipsel-linux-ar
	set AS=mipsel-linux-as
	set LD=mipsel-linux-ld
	set NM=mipsel-linux-nm
	set CC=mipsel-linux-gcc
	set GCC=mipsel-linux-gcc
	set CXX=mipsel-linux-g++
	set RANLIB=mipsel-linux-ranlib
	cp gdb/configure-cross gdb/configure
	./configure --build=i686-pc-linux-gnu --target=mipsel-linux --host=i686-pc-linux-gnu --disable-nls --without-included-gettext --without-uiout --disable-tui --disable-gdbtk --without-x 
	make -f Makefile all-gdb

clean:
	cd bfd ; make distclean ; cd ..
	cd config ; make distclean ; cd ..
	cd gdb/gdbserver ; make distclean ; cd ../
	make distclean ; cd ..
	cd intl ; make distclean ; cd ..
	cd libdecnumber/ ; make distclean ; cd ..
	cd libiberty/ ; make distclean ; cd ..
	cd opcodes/ ; make distclean ; cd ..
	cd readline/ ; make distclean ; cd ..
	cd sim ; make distclean ; cd ..
	cd texinfo/ ; make distclean ; cd ..
	find . -name "*.o" |xargs rm -f
	find . -name "*.lo" |xargs rm -f
	find . -name "*.a" |xargs rm -f
	find . -name "*.la" |xargs rm -f
	find . -name "config\.cache" |xargs rm -f
	find . -name "config\.status" |xargs rm -f
	find . -name "config\.log" |xargs rm -f
	find . -name "Makefile" |xargs rm -f

sweep:
	make clean
	rm -f gdb/gdb-nat
	rm -f gdb/mipsel-linux-gdb

# This will install mipsel-uclibc-gdb (compiled for the host server) and
# gdbserver (compiled for the target) into a directory that is accessable
# from app_player. app_player's "make gdbprep" will get the files from
# this shared directory. This install needs to be done once per toolchain.
# The binaries will be stored in a subdirectory that will be based on the
# toolchain path. The root of the target directory can be overridden by
# specifying the environment variable "BDVD_NONFREE_TOOLS_DIR".
install: all $(NONFREE_FULL_PATH) \
	$(NONFREE_FULL_PATH)/mipsel-uclibc-gdb $(NONFREE_FULL_PATH)/gdbserver

$(ROOTFS_BUILT_INDICATOR) :
	$(error "Error - Please build the rootfs before building GDB")

$(NONFREE_FULL_PATH):
	mkdir $@

$(NONFREE_FULL_PATH)/mipsel-uclibc-gdb : gdb/mipsel-linux-gdb
	cp  $< $@
	strip $@

$(NONFREE_FULL_PATH)/gdbserver : gdb/gdbserver/gdbserver
	cp  $< $@
	mipsel-linux-strip $@

