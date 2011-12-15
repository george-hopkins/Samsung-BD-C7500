## Makefile.am for DirectFB/interfaces/IDirectFBImageProvider

SUBDIRS = mpeg2

idirectfbimageproviderdir = $(MODULEDIR)/interfaces/IDirectFBImageProvider

if BMP_PROVIDER
BMP_PROVIDER_LTLIB = libidirectfbimageprovider_bmp.la
else
BMP_PROVIDER_LTLIB = 
endif

if GIF_PROVIDER
GIF_PROVIDER_LTLIB = libidirectfbimageprovider_gif.la
else
GIF_PROVIDER_LTLIB = 
endif

if JPEG_PROVIDER
JPEG_PROVIDER_LTLIB = libidirectfbimageprovider_jpeg.la
else
JPEG_PROVIDER_LTLIB = 
endif

if PNG_PROVIDER
PNG_PROVIDER_LTLIB = libidirectfbimageprovider_png.la
else
PNG_PROVIDER_LTLIB = 
endif

#if CVF_PROVIDER
CIF_PROVIDER_LTLIB = libidirectfbimageprovider_cif.la
#else
#CVF_PROVIDER_LTLIB = 
#endif

INCLUDES = \
	-I$(top_srcdir)/include	\
	-I$(top_builddir)/lib	\
	-I$(top_srcdir)/lib	\
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/systems \
	$(BCMSETTOPAPI_INTERNAL_CFLAGS)

AM_CPPFLAGS = -DDATADIR=\"@DATADIR@\"

idirectfbimageprovider_LTLIBRARIES = \
	libidirectfbimageprovider_mpeg2.la	\
	$(BMP_PROVIDER_LTLIB)			\
	$(GIF_PROVIDER_LTLIB)			\
	$(JPEG_PROVIDER_LTLIB)			\
	$(PNG_PROVIDER_LTLIB)           \
	$(CIF_PROVIDER_LTLIB)

if BUILD_STATIC
idirectfbimageprovider_DATA = $(idirectfbimageprovider_LTLIBRARIES:.la=.o)
endif


libidirectfbimageprovider_png_la_SOURCES = idirectfbimageprovider_png.c

libidirectfbimageprovider_png_la_LIBADD = $(LIBPNG)

libidirectfbimageprovider_png_la_LDFLAGS = -avoid-version -module


libidirectfbimageprovider_mpeg2_la_SOURCES = idirectfbimageprovider_mpeg2.c

libidirectfbimageprovider_mpeg2_la_LIBADD = mpeg2/libmpeg2decode.la

libidirectfbimageprovider_mpeg2_la_LDFLAGS = -avoid-version -module


libidirectfbimageprovider_bmp_la_SOURCES = idirectfbimageprovider_bmp.c

libidirectfbimageprovider_bmp_la_LDFLAGS = -avoid-version -module


libidirectfbimageprovider_gif_la_SOURCES = idirectfbimageprovider_gif.c

libidirectfbimageprovider_gif_la_LDFLAGS = -avoid-version -module


libidirectfbimageprovider_jpeg_la_SOURCES = idirectfbimageprovider_jpeg.c

libidirectfbimageprovider_jpeg_la_LIBADD = $(LIBJPEG)

libidirectfbimageprovider_jpeg_la_LDFLAGS = \
	$(BCMSETTOPAPI_LDFLAGS) \
	-avoid-version -module

libidirectfbimageprovider_cif_la_SOURCES = idirectfbimageprovider_cif.c

libidirectfbimageprovider_cif_la_LDFLAGS = -avoid-version -module

include $(top_srcdir)/rules/libobject.make
