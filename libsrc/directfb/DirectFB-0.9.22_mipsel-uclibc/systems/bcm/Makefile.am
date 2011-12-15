## Makefile.am for DirectFB/systems/bcm

INCLUDES = \
	-I$(top_srcdir)/include		\
	-I$(top_builddir)/include	\
	-I$(top_builddir)/lib		\
	-I$(top_srcdir)/lib		\
	-I$(top_srcdir)/src		\
	$(BCMSETTOPAPI_INTERNAL_CFLAGS)

internalincludedir = $(INTERNALINCLUDEDIR)/bcm

internalinclude_HEADERS = \
	bcmcore.h bcmlayer.h bcmmixer.h bcmanimator.h bcmcore_fcts.h

systemsdir = $(MODULEDIR)/systems

systems_LTLIBRARIES = libdirectfb_bcm.la

libdirectfb_bcm_la_LDFLAGS = \
	$(BCMSETTOPAPI_LDFLAGS) \
	-avoid-version	\
	-module

libdirectfb_bcm_la_SOURCES = \
	bcmcore.c bcmmixer.c bcmanimator.c bcmcore_fcts.c

include $(top_srcdir)/rules/libobject.make
