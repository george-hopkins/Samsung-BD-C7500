## Makefile.am for DirectFB/interfaces/IDirectFBFont

idirectfbfontdir = $(MODULEDIR)/interfaces/IDirectFBFont

if FREETYPE_PROVIDER
FREETYPE_PROVIDER_LTLIB = libidirectfbfont_ft2.la
else
FREETYPE_PROVIDER_LTLIB =
endif

INCLUDES = \
	-I$(top_srcdir)/include	\
	-I$(top_builddir)/lib	\
	-I$(top_srcdir)/lib	\
	-I$(top_srcdir)/src 	\
	-DDATADIR=\"@DATADIR@\"

AM_CFLAGS = $(FREETYPE_CFLAGS)

## gbg 1/30/2007 - Remove default font because the dynamic linker sometimes finds this font before another correct font.
##                 In order to use this default font, DirectGetInterface() must be modified to search other provders before
##                 the default provider.
##	libidirectfbfont_default.la	\
idirectfbfont_LTLIBRARIES = \
	$(FREETYPE_PROVIDER_LTLIB)

if BUILD_STATIC
idirectfbfont_DATA = $(idirectfbfont_LTLIBRARIES:.la=.o)
endif


##libidirectfbfont_default_la_SOURCES = idirectfbfont_default.c default_font.h
##libidirectfbfont_default_la_LDFLAGS = -avoid-version -module


libidirectfbfont_ft2_la_SOURCES = idirectfbfont_ft2.c

libidirectfbfont_ft2_la_LIBADD = $(FREETYPE_LIBS)

libidirectfbfont_ft2_la_LDFLAGS = -avoid-version -module


include $(top_srcdir)/rules/libobject.make
