LANG=C
CFLAGS = -Os -DEMBED -Dlinux -D__linux__ -D__uClinux__ -Dunix -fno-builtin -I$(ROOTDIR)/uClibc/include -I$(ROOTDIR)/$(LINUXDIR)/include -L./builtins -L./lib/glob -L./lib/malloc -L./lib/readline -L./lib/sh -L./lib/tilde -L../lib

export CFLAGS

CXXFLAGS=

FFLAGS=

export CXXFLAGS FFLAGS

INSTALL_DIR=$(ROOTDIR)/user/ncurses/instdir
export INSTALL_DIR
CC=mipsel-linux-gcc ; \
BUILD_CC=gcc ; \
export CC BUILD_CC CFLAGS INSTALL_DIR ; \

all:
	( \
	set -x ; \
	mkdir -p narrowc widec ; \
	cd narrowc ; \
	mkdir -p $(INSTALL_DIR) ; \
	[ ! -L configure ] && ln -s ../configure . ; \
	./configure --build=i686-linux-gnu --host=$(MACHINE)-linux --target=$(MACHINE)-linux --datadir=/usr/share --enable-colorfgbg --enable-hard-tabs --enable-overwrite --enable-xmc-glitch --localstatedir=/var --prefix=/usr --sysconfdir=/etc --with-build-cppflags=-D_FORTIFY_SOURCE=2 --with-chtype=long --with-install-prefix=$(INSTALL_DIR) --with-ospeed=unsigned --without-ada --with-shared --with-progs --enable-termcap --includedir=/usr/include/ncurses ; \
	[ $$? -ne 0 ] && exit -1 ; \
	make libs ; \
	make -C progs ; \
	make install.libs install.progs install.includes install.man install.data ; \
	cd ../widec ; \
	[ ! -L configure ] && ln -s ../configure . ; \
	./configure --build=i686-linux-gnu --host=$(MACHINE)-linux --target=$(MACHINE)-linux --datadir=/usr/share --enable-colorfgbg --enable-hard-tabs --enable-overwrite --enable-xmc-glitch --localstatedir=/var --prefix=/usr --sysconfdir=/etc --with-build-cppflags=-D_FORTIFY_SOURCE=2 --with-chtype=long --with-install-prefix=$(INSTALL_DIR) --with-ospeed=unsigned --without-ada --with-shared --enable-termcap --enable-widec --without-progs --includedir=/usr/include/ncursesw ; \
	make libs ; \
	make install.libs install.includes install.man ; \
	cd .. ; \
	make romfs ; \
	)

# "top" is the only app using ncurses and it's statically linking it so
# we don't need to install the library
# Curses is configured to use termcap instead of terminfo so we don't \
# need to install the terminfo database.
romfs:
#	if [ $(CONFIG_LIB_NCURSES) = y ]; then \
#		( mkdir -p $(ROMFSDIR)/usr/share; \
#		  cd $(INSTALL_DIR)/usr/share; \
#		  file_list=`find terminfo \( -name \*vt100\* -o -name \*xterm\* \)` ;\
#		  echo "file_list = $$file_list" ; \
#		  [ "x$$file_list" != "x" ] && tar -cf - $$file_list | tar -C $(ROMFSDIR)/usr/share -xvpf - ; \
#		  rm -f $(ROMFSDIR)/lib/terminfo ; \
#		  ln -sf /usr/share/terminfo $(ROMFSDIR)/lib/terminfo ; \
# 
#		  cp -f $(INSTALL_DIR)/usr/lib/libncurses.so.5.6 $(ROMFSDIR)/lib/libncurses.so.5.6 ; \
#		  rm -f $(ROMFSDIR)/lib/libncurses.so.5 ; \
#		  ln -sf libncurses.so.5.6 $(ROMFSDIR)/lib/libncurses.so.5 ; \
#		  rm -f $(ROMFSDIR)/lib/libcurses.so ; \
#		  ln -sf libncurses.so.5.6 $(ROMFSDIR)/lib/libcurses.so ; \
#		  rm -f $(ROMFSDIR)/lib/libncurses.so ; \
#		  ln -sf libncurses.so.5 $(ROMFSDIR)/lib/libncurses.so ; \
#		  chmod 755 $(ROMFSDIR)/lib/lib*.so* ; \
#		  ); \
#	fi

# These Libraries are not needed by STB because no app needs them at this time
#		  cp -f $(INSTALL_DIR)/usr/lib/libncurses++.a $(ROMFSDIR)/lib/libncurses++.a ; \
#		  cp -f $(INSTALL_DIR)/usr/lib/libncurses_g.a $(ROMFSDIR)/lib/libncurses_g.a ; \
#		  cp -f $(INSTALL_DIR)/usr/lib/libncurses.a $(ROMFSDIR)/lib/libncurses.a ; \
#		  cp -f $(INSTALL_DIR)/usr/lib/libform.a $(ROMFSDIR)/lib/libform.a ; \
#		  cp -f $(INSTALL_DIR)/usr/lib/libform_g.a $(ROMFSDIR)/lib/libform_g.a ; \
#		  cp -f $(INSTALL_DIR)/usr/lib/libform.so.5.6 $(ROMFSDIR)/lib/libform.so.5.6 ; \
#		  cp -f $(INSTALL_DIR)/usr/lib/libmenu.a $(ROMFSDIR)/lib/libmenu.a ; \
#		  cp -f $(INSTALL_DIR)/usr/lib/libmenu_g.a $(ROMFSDIR)/lib/libmenu_g.a ; \
#		  cp -f $(INSTALL_DIR)/usr/lib/libmenu.so.5.6 $(ROMFSDIR)/lib/libmenu.so.5.6 ; \
#		  cp -f $(INSTALL_DIR)/usr/lib/libpanel.a $(ROMFSDIR)/lib/libpanel.a ; \
#		  cp -f $(INSTALL_DIR)/usr/lib/libpanel_g.a $(ROMFSDIR)/lib/libpanel_g.a ; \
#		  cp -f $(INSTALL_DIR)/usr/lib/libtic.a $(ROMFSDIR)/lib/libtic.a ; \
#		  cp -f $(INSTALL_DIR)/usr/lib/libtic_g.a $(ROMFSDIR)/lib/libtic_g.a ; \
#		  cp -f $(INSTALL_DIR)/usr/lib/libpanel.so.5.6 $(ROMFSDIR)/lib/libpanel.so.5.6 ; \
#		  cp -f $(INSTALL_DIR)/usr/lib/libtic.so.5.6 $(ROMFSDIR)/lib/libtic.so.5.6 ; \
#		  cp -f $(INSTALL_DIR)/usr/lib/libtinfo.a $(ROMFSDIR)/lib/libtinfo.a ; \
#		  cp -f $(INSTALL_DIR)/usr/lib/libtinfo_g.a $(ROMFSDIR)/lib/libtinfo_g.a ; \
#		  cp -f $(INSTALL_DIR)/usr/lib/libtinfo.so.5.6 $(ROMFSDIR)/lib/libtinfo.so.5.6 ; \
#		  rm -f $(ROMFSDIR)/lib/libform.so ; \
#		  ln -sf libform.so.5 $(ROMFSDIR)/lib/libform.so ; \
#		  rm -f $(ROMFSDIR)/lib/libform.so.5 ; \
#		  ln -sf libform.so.5.6 $(ROMFSDIR)/lib/libform.so.5 ; \
#		  rm -f $(ROMFSDIR)/lib/libmenu.so ; \
#		  ln -sf libmenu.so.5 $(ROMFSDIR)/lib/libmenu.so ; \
#		  rm -f $(ROMFSDIR)/lib/libmenu.so.5 ; \
#		  ln -sf libmenu.so.5.6 $(ROMFSDIR)/lib/libmenu.so.5 ; \
#		  rm -f $(ROMFSDIR)/lib/libpanel.so ; \
#		  ln -sf libpanel.so.5 $(ROMFSDIR)/lib/libpanel.so ; \
#		  rm -f $(ROMFSDIR)/lib/libpanel.so.5 ; \
#		  ln -sf libpanel.so.5.6 $(ROMFSDIR)/lib/libpanel.so.5 ; \
#		  rm -f $(ROMFSDIR)/lib/libtic.so ; \
#		  ln -sf libtic.so.5 $(ROMFSDIR)/lib/libtic.so ; \
#		  rm -f $(ROMFSDIR)/lib/libtic.so.5 ; \
#		  ln -sf libtic.so.5.6 $(ROMFSDIR)/lib/libtic.so.5 ; \
#		  rm -f $(ROMFSDIR)/lib/libtinfo.so ; \
#		  ln -sf libtinfo.so.5 $(ROMFSDIR)/lib/libtinfo.so ; \
#		  rm -f $(ROMFSDIR)/lib/libtinfo.so.5 ; \
#		  ln -sf libtinfo.so.5.6 $(ROMFSDIR)/lib/libtinfo.so.5 ; \
#		  rm -f $(ROMFSDIR)/lib/libcurses.a ; \
#		  ln -sf libncurses.a $(ROMFSDIR)/lib/libcurses.a ; \


clean \
distclean \
mostlyclean \
realclean:
	find . \( -name \.o -o -name \*.a -o -name \*.so\* \) | xargs rm -f
	-/bin/rm -f config.cache
	rm -f config.cache config.log config.status
	rm -rf $(INSTALL_DIR) narrowc widec 
