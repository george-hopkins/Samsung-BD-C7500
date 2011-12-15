CFLAGS += -fno-builtin
export CFLAGS

all:
	( cd source ; \
	export SMB_BUILD_CC_NEGATIVE_ENUM_VALUES=yes ; \
	export libreplace_cv_READDIR_GETDIRENTRIES=no ; \
	export libreplace_cv_READDIR_GETDENTS=no ; \
	export linux_getgrouplist_ok=no ; \
	export samba_cv_REPLACE_READDIR=no ; \
	export samba_cv_HAVE_WRFILE_KEYTAB=yes ; \
	export samba_cv_HAVE_KERNEL_OPLOCKS_LINUX=yes ; \
	export samba_cv_HAVE_IFACE_IFCONF=yes ; \
	CC=$(CC) ./configure \
		--target=$(MACHINE)-linux \
		--host=$(MACHINE)-linux \
		--build=`/bin/arch`-linux \
		--disable-shared \
		--disable-cups \
		--disable-iprint \
		--disable-pie \
		--disable-fam \
		--bindir=/bin \
		--sbindir=/bin \
		--libdir=/var/smb \
		--localstatedir=/var/smb \
		--with-configdir=/var/smb \
		--with-privatedir=/var/smb \
		--with-rootsbindir=/bin \
		--with-lockdir=/var/smb/locks \
		--with-piddir=/var/smb/locks \
		--without-ldap \
		--without-cifsmount \
		--without-sendfile-support \
		--prefix=; \
	make -f Makefile ; \
	)

clean:
	( cd source; make -f Makefile clean )
	( cd source; -/bin/rm -f config.cache )

distclean: clean
	( cd source; make -f Makefile distclean )
	( find source -name config.h | xargs rm -f )
	( find source -name Makefile | xargs rm -f )

romfs:
	( cd source ; \
	mkdir -p $(ROMFSDIR)/etc/samba ;\
	$(ROMFSINST) -e CONFIG_USER_SAMBA smb/smb.conf /etc/samba/smb.conf ;\
	$(ROMFSINST) -e CONFIG_USER_SAMBA_NMBD bin/nmbd /bin/nmbd ;\
	$(ROMFSINST) -e CONFIG_USER_SAMBA_SMBD bin/smbd /bin/smbd ;\
	)
