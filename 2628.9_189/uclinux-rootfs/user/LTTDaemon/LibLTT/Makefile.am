## Process this file with automake to generate Makefile.in

srcdir = @srcdir@
VPATH = @srcdir@

DISTCLEANFILES = $(srcdir)/*~ $(srcdir)/\#*\#

MAINTAINERCLEANFILES =

SUPP_RTAI		= @SUPP_RTAI@
TARGET_NATIVE		= @TARGET_NATIVE@
LTT_UNPACKED_STRUCTS	= @LTT_UNPACKED_STRUCTS@

lib_LTLIBRARIES = libltt.la

if WITH_SUPP_RTAI
libltt_la_LIBADD = RTAITables.lo RTAIDB.lo
endif

libltt_la_SOURCES = \
	Tables.c \
	LinuxTables.c \
	EventDB.c \
	EventOptions.c \
	EventDBI.h

libltt_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE)

AM_CPPFLAGS = \
	-I$(top_srcdir)/Include \
	-DSUPP_RTAI=$(SUPP_RTAI) \
	-DTARGET_NATIVE=$(TARGET_NATIVE) \
	-DLTT_UNPACKED_STRUCTS=$(LTT_UNPACKED_STRUCTS)

EXTRA_DIST = RTAITables.c RTAIDB.c

