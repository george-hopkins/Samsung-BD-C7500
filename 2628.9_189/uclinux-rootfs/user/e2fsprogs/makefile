all:
	-@if [ ! -f config.cache  ]; then \
	CFLAGS='$(filter-out -fno-builtin, $(CFLAGS))' \
		CC=$(CC) ./configure \
			--target=$(MACHINE)-linux \
			--host=$(MACHINE)-linux \
			--build=`/bin/arch`-linux \
			--enable-elf-shlibs \
			--enable-dynamic-e2fsck \
			--enable-fsck \
			--with-root-prefix=$(ROMFSDIR) \
			--prefix=$(ROMFSDIR)/usr; \
	fi
	touch -m configure.in
	sleep 2
	touch -m configure
	make -f Makefile

clean:
	make -f Makefile clean
	-/bin/rm -f config.cache

distclean: clean
	make -f Makefile distclean

romfs:
	$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_DEBUGFS_DEBUGFS debugfs/debugfs /bin/debugfs
	$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_E2FSCK_E2FSCK e2fsck/e2fsck /bin/e2fsck
		$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_E2FSCK_E2FSCK -s /bin/e2fsck /bin/fsck.ext3
		$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_E2FSCK_E2FSCK -s /bin/e2fsck /bin/fsck.ext2
	$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_MISC_BADBLOCKS misc/badblocks /bin/badblocks
	$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_MISC_CHATTR misc/ /bin/chattr
	$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_MISC_DUMPE2FS misc/dumpe2fs /bin/dumpe2fs
	$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_MISC_E2LABEL misc/e2label /bin/e2label
	$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_MISC_FSCK misc/fsck /bin/fsck
	$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_MISC_LSATTR misc/lsattr /bin/lsattr
	$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_MISC_MKE2FS misc/mke2fs /bin/mke2fs
		$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_MISC_MKE2FS -s /bin/mke2fs /bin/mkfs.ext2
		$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_MISC_MKE2FS -s /bin/mke2fs /bin/mkfs.ext3
	$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_MISC_MKLOST_FOUND misc/mklost+found /bin/mklost+found
	$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_MISC_TUNE2FS misc/tune2fs /bin/tune2fs
	$(ROMFSINST) -e CONFIG_USER_E2FSPROGS_MISC_UUIDGEN misc/uuidgen /bin/uuidgen
# PR11932 - Fix bad symbolic links in root filesystem
	cd $(ROMFSDIR)/usr/lib; for i in libcom_err.so libe2p.so libext2fs.so libss.so libuuid.so; do ln -sf /lib/$$i $$i; done
# PR29678 
	cd lib/et; cat compile_et | sed -e 's/^\(\w*DIR\)\=.*/\1\=\"\/usr\/share\/et\/\"/' | sed -e 's/^AWK\=.*/AWK\=\/bin\/awk/' > $(ROMFSDIR)/usr/bin/compile_et
# PR29679
	cd lib/ss; cat mk_cmds | sed -e 's/^\(\w*DIR\)\=.*/\1\=\"\/usr\/share\/ss\/\"/' | sed -e 's/^AWK\=.*/AWK\=\/bin\/awk/' > $(ROMFSDIR)/usr/bin/mk_cmds

