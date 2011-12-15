# Makefile for hdparm-6.9

# DESTDIR is for non root installs (eg packages, NFS) only!
# DESTDIR =

binprefix = 
manprefix = /usr
exec_prefix = $(binprefix)/
sbindir = $(exec_prefix)sbin
mandir = $(manprefix)/share/man
oldmandir = $(manprefix)/man

ifndef CC
CC = gcc
endif
CFLAGS := -Os -W -Wall -Wbad-function-cast -Wcast-align -Wpointer-arith -Wcast-qual -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -fkeep-inline-functions -Wwrite-strings -Waggregate-return -Wnested-externs -Wtrigraphs $(CFLAGS)


LDFLAGS = -s
INSTALL = install
INSTALL_DATA = $(INSTALL) -m 644
INSTALL_DIR = $(INSTALL) -m 755 -d
INSTALL_PROGRAM = $(INSTALL)

all: hdparm

# hdparm: hdparm.o identify.o hdparm.h
hdparm: $(HDPARM_DEPS)
	$(CC) $(LDFLAGS) -o hdparm hdparm.o identify.o
 
install: all
	if [ ! -z $(DESTDIR) ]; then $(INSTALL_DIR) $(DESTDIR) ; fi
	if [ ! -z $(DESTDIR)$(sbindir) ]; then $(INSTALL_DIR) $(DESTDIR)$(sbindir) ; fi
	if [ -f $(DESTDIR)$(sbindir)/hdparm ]; then rm -f $(DESTDIR)$(sbindir)/hdparm ; fi
	$(INSTALL_PROGRAM) hdparm $(DESTDIR)$(sbindir)/hdparm

clean:
	rm -f hdparm *.o core

