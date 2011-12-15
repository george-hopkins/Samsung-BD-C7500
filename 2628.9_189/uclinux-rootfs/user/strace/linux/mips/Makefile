# Generated automatically from Makefile.in by configure.
#
# $Id: Makefile.in,v 1.1 1999/11/01 00:46:49 wichert Exp $
#

srcdir = .

CC = mips-linux-gcc
CPP = mips-linux-gcc -E
SHELL = /bin/sh

DEFS = -DHAVE_CONFIG_H
LDLIBS = 

CFLAGS = -g
LDFLAGS = -g
WARNFLAGS = -Wall

CPPFLAGS =
INCLUDES = -I. -I.. -I$(srcdir)/.. -I../.. -I$(srcdir)

includedir = /opt/toolchains-2.6.12_3.4.6/crosstools_linux-2.6.12.0_gcc-3.4.6-19_uclibc-0.9.28-20050817-20061010/bin/../lib/gcc/mips-linux-uclibc/3.4.6/../../../../mips-linux-uclibc/sys-include

all: ioctlent.h errnoent.h signalent.h syscallent.h

ioctlent.raw: ioctlent.sh
	$(SHELL) $(srcdir)/ioctlent.sh $(includedir) >$@

ioctlent.h: ioctlent.raw ioctlsort
	./ioctlsort >$@

ioctlsort: ioctlsort.o
	$(CC) $(LDFLAGS) ioctlsort.o -o ioctlsort

ioctlsort.o: ../../ioctlsort.c
	$(CC) $(WARNFLAGS) $(DEFS) $(CPPFLAGS) $(INCLUDES) $(CFLAGS) -c $(srcdir)/../../ioctlsort.c

ioctlsort.o: ioctlent.raw

errnoent.h: ../../errnoent.sh $(includedir)/asm/errno.h
	$(SHELL) $(srcdir)/../../errnoent.sh $(includedir)/asm/errno.h >$@

signalent.h: ../../signalent.sh $(includedir)/asm/signal.h
	$(SHELL) $(srcdir)/../../signalent.sh $(includedir)/asm/signal.h >$@

#syscallent.h: ../../syscallent.sh $(includedir)/sys/syscall.h
syscallent.h:
	$(SHELL) $(srcdir)/../../syscallent.sh $(includedir)/sys/syscall.h >$@

clean:
	rm -f ioctlent.c *.raw *.tmp *.o ioctlsort

distclean: clean
	rm -f Makefile

maintainer-clean: distclean
	rm -f ioctlent.h errnoent.h signalent.h

brcmioctl:	brcm_ioctlent.sh 
	./brcm_ioctlent.sh $(LINUX_INC)
	cc -g -o ioctlsort ioctlsort.c -I$(LINUX_INC) -I$(SETTOP_INC)
	./ioctlsort > ioctlent.h
