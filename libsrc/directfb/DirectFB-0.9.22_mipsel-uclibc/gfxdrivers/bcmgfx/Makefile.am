## Makefile.am for DirectFB/gfxdrivers/bcmgfx

INCLUDES = \
	-I$(top_srcdir)/include	\
	-I$(top_builddir)/lib	\
	-I$(top_srcdir)/lib	\
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/systems \
	$(BCMSETTOPAPI_INTERNAL_CFLAGS)

bcmgfx_LTLIBRARIES = libdirectfb_bcmgfx.la

if BUILD_STATIC
bcmgfx_DATA = $(bcmgfx_LTLIBRARIES:.la=.o)
endif

bcmgfxdir = $(MODULEDIR)/gfxdrivers

libdirectfb_bcmgfx_la_SOURCES =	\
	bcmgfx.h		\
	bcmgfx.c		\
	bcmgfx_state.h		\
	bcmgfx_state.c

libdirectfb_bcmgfx_la_LDFLAGS = \
	$(BCMSETTOPAPI_LDFLAGS) \
	-export-dynamic	\
	-avoid-version	\
	$(DFB_LDFLAGS)


include $(top_srcdir)/rules/libobject.make
