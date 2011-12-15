## Makefile.am for directfb_bcm_samplecode/src

fontsdatadir = @DATADIR@/fonts

AM_CFLAGS = @DIRECTFB_CFLAGS@ -D_GNU_SOURCE $(FREETYPE_CFLAGS) $(CAIRO_CFLAGS)

AM_CXXFLAGS = @DIRECTFB_CFLAGS@ -D_GNU_SOURCE

LIBADDS = \
	@DIRECTFB_LIBS@

AM_CPPFLAGS = \
	-DDATADIR=\"@DATADIR@\"

bin_PROGRAMS = 						\
	dfbbcm_gfxblendingtest 			\
	dfbbcm_multilayerblendingtest 	\
	dfbbcm_multilayerhdmvtest 		\
	dfbbcm_imagetest 		\
	dfbbcm_myfx 					\
	dfbbcm_fliptest 				\
	dfbbcm_hddvddecodeconverttest 	\
	dfbbcm_videographicsstress 		\
	dfbbcm_fontbench				\
	dfbbcm_windowtest				\
	dfbbcm_cairo                    \
	dfbbcm_sync_faa                      \
	dfbbcm_img_faa                      \
	dfbbcm_bogus                    \
	dfbbcm_setvideomode             \
	dfbbcm_sparks                   \
	dfbbcm_skunkworks               \
	dfbbcm_gfxbench                 \
	dfbbcm_cursortest               \
	dfbbcm_backgroundlayer          \
	dfbbcm_ihdmenu                  \
	dfbbcm_glyphtest                \
	dfbbcm_videowindowclippos       \
	dfbbcm_videopump                \
	dfbbcm_blittest                 \
	dfbbcm_halffont                 \
	dfbbcm_rotate

SOURCEADDS = \
	dfbbcm_utils.c dfbbcm_utils.h dfbbcm_playbackrinbufthread.c dfbbcm_playbackrinbufthread.h dfbbcm_playbackscagatthread.c dfbbcm_playbackscagatthread.h dfbbcm_gfxperfthread.c dfbbcm_gfxperfthread.h

dfbbcm_gfxblendingtest_SOURCES = dfbbcm_gfxblendingtest.c $(SOURCEADDS)
dfbbcm_gfxblendingtest_LDADD = $(LIBADDS)

dfbbcm_multilayerblendingtest_SOURCES = dfbbcm_multilayerblendingtest.c $(SOURCEADDS)
dfbbcm_multilayerblendingtest_LDADD = $(LIBADDS)

dfbbcm_multilayerhdmvtest_SOURCES = dfbbcm_multilayerhdmvtest.c $(SOURCEADDS)
dfbbcm_multilayerhdmvtest_LDADD = $(LIBADDS)

dfbbcm_myfx_SOURCES= dfbbcm_myfx.c $(SOURCEADDS)
dfbbcm_myfx_LDADD = $(LIBADDS)

dfbbcm_hddvddecodeconverttest_SOURCES = dfbbcm_hddvddecodeconverttest.c $(SOURCEADDS)
dfbbcm_hddvddecodeconverttest_LDADD = $(LIBADDS)

dfbbcm_fliptest_SOURCES = dfbbcm_fliptest.c $(SOURCEADDS)
dfbbcm_fliptest_LDADD = $(LIBADDS)

dfbbcm_videographicsstress_SOURCES = dfbbcm_videographicsstress.c $(SOURCEADDS)
dfbbcm_videographicsstress_LDADD = $(LIBADDS)

dfbbcm_fontbench_SOURCES = dfbbcm_fontbench.c $(SOURCEADDS)
dfbbcm_fontbench_LDADD = $(LIBADDS) $(FREETYPE_LIBS)

dfbbcm_windowtest_SOURCES = dfbbcm_windowtest.c $(SOURCEADDS)
dfbbcm_windowtest_LDADD = $(LIBADDS)

dfbbcm_cairo_SOURCES = dfbbcm_cairo.c $(SOURCEADDS)
dfbbcm_cairo_LDADD = $(LIBADDS) $(CAIRO_LIBS)

dfbbcm_sync_faa_SOURCES = dfbbcm_sync_faa.c $(SOURCEADDS)
dfbbcm_sync_faa_LDADD = $(LIBADDS)

dfbbcm_img_faa_SOURCES = dfbbcm_img_faa.c $(SOURCEADDS)
dfbbcm_img_faa_LDADD = $(LIBADDS)

dfbbcm_bogus_SOURCES = dfbbcm_bogus.c $(SOURCEADDS)
dfbbcm_bogus_LDADD = $(LIBADDS)

dfbbcm_setvideomode_SOURCES = dfbbcm_setvideomode.c $(SOURCEADDS)
dfbbcm_setvideomode_LDADD = $(LIBADDS)

dfbbcm_sparks_SOURCES = dfbbcm_sparks.cpp
dfbbcm_sparks_LDADD = $(LIBADDS)

dfbbcm_skunkworks_SOURCES = dfbbcm_skunkworks.c $(SOURCEADDS)
dfbbcm_skunkworks_LDADD = $(LIBADDS)

dfbbcm_gfxbench_SOURCES = dfbbcm_gfxbench.c
dfbbcm_gfxbench_LDADD = $(LIBADDS)

dfbbcm_imagetest_SOURCES = dfbbcm_imagetest.c $(SOURCEADDS)
dfbbcm_imagetest_LDADD = $(LIBADDS) -lmng

dfbbcm_backgroundlayer_SOURCES = dfbbcm_backgroundlayer.c $(SOURCEADDS)
dfbbcm_backgroundlayer_LDADD = $(LIBADDS)

dfbbcm_ihdmenu_SOURCES = dfbbcm_ihdmenu.c $(SOURCEADDS)
dfbbcm_ihdmenu_LDADD = $(LIBADDS)
dfbbcm_glyphtest_SOURCES = dfbbcm_glyphtest.c $(SOURCEADDS)
dfbbcm_glyphtest_LDADD = $(LIBADDS)

dfbbcm_videowindowclippos_SOURCES = dfbbcm_videowindowclippos.c $(SOURCEADDS)
dfbbcm_videowindowclippos_LDADD = $(LIBADDS)

dfbbcm_videopump_SOURCES = dfbbcm_videopump.c $(SOURCEADDS)
dfbbcm_videopump_LDADD = $(LIBADDS)

dfbbcm_blittest_SOURCES = dfbbcm_blittest.c $(SOURCEADDS)
dfbbcm_blittest_LDADD = $(LIBADDS)

dfbbcm_halffont_SOURCES = dfbbcm_halffont.c
dfbbcm_halffont_LDADD = $(LIBADDS)

dfbbcm_rotate_SOURCES = dfbbcm_rotate.c $(SOURCEADDS)
dfbbcm_rotate_LDADD = $(LIBADDS)
