## Process this file with automake to generate Makefile.in

srcdir = @srcdir@
VPATH = @srcdir@

DISTCLEANFILES = $(srcdir)/*~ $(srcdir)/\#*\#

MAINTAINERCLEANFILES=

SUBDIRS = Scripts

SUPP_RTAI = @SUPP_RTAI@

tracedaemon_SOURCES = \
	TraceDaemon.c \
	TraceDaemon.h

tracedaemon_LDADD = ../LibUserTrace/libusertrace.la

bin_PROGRAMS = tracedaemon

AM_CPPFLAGS = \
	-I$(top_srcdir)/Include \
	-DSUPP_RTAI=$(SUPP_RTAI)
