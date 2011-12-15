all:
	CC=$(CC) RANLIB=$(RANLIB) AR=$(AR) ./configure --shared --prefix=$(ROMFSDIR)
	make CC="$(CC) $(CFLAGS)" LD='$(LD) $(LDFLAGS)' -f Makefile
	make -f Makefile install

clean:
	make -f Makefile clean

distclean: clean
	make -f Makefile distclean
	rm -f Makefile

romfs:
	make -f Makefile install
