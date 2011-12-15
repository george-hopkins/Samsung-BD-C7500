## Makefile.am for DirectFB/interfaces/IDirectFBVideoProvider

idirectfbvideoproviderdir = $(MODULEDIR)/interfaces/IDirectFBVideoProvider


if V4L_PROVIDER
V4L_PROVIDER_LTLIB = libidirectfbvideoprovider_v4l.la
else
V4L_PROVIDER_LTLIB =
endif

if VIDEOBCM_PROVIDER
VIDEOBCM_PROVIDER_LTLIB = libidirectfbvideoprovider_bcm.la
else
VIDEOBCM_PROVIDER_LTLIB =
endif

INCLUDES = \
	-I$(top_srcdir)/include	\
	-I$(top_builddir)/lib	\
	-I$(top_srcdir)/lib	\
	-I$(top_srcdir)/src 	\
	-I$(top_srcdir)/systems \
	$(BCMSETTOPAPI_INTERNAL_CFLAGS) \
	-DDATADIR=\"@DATADIR@\"


idirectfbvideoprovider_LTLIBRARIES =	\
	$(V4L_PROVIDER_LTLIB)               \
	$(VIDEOBCM_PROVIDER_LTLIB)

if BUILD_STATIC
idirectfbvideoprovider_DATA = $(idirectfbvideoprovider_LTLIBRARIES:.la=.o)
endif

libidirectfbvideoprovider_v4l_la_SOURCES = idirectfbvideoprovider_v4l.c

libidirectfbvideoprovider_v4l_la_LDFLAGS = -avoid-version -module

libidirectfbvideoprovider_bcm_la_SOURCES = idirectfbvideoprovider_bcm.c

libidirectfbvideoprovider_bcm_la_LIBADD = ../IDirectFBImageProvider/mpeg2/libmpeg2decode.la

libidirectfbvideoprovider_bcm_la_LDFLAGS = -avoid-version -module

include $(top_srcdir)/rules/libobject.make
