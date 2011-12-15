all:
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

#distclean: clean
#	make -f Makefile distclean
#	find . -name "Makefile" |xargs rm -f


romfs:
	cp gdb/gdb gdb/gdb-strip
	mipsel-linux-strip gdb/gdb-strip
	cp gdb/gdb-strip ../../romfs/usr/bin/gdb


