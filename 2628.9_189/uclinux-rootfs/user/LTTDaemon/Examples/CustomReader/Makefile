# 
# This is the makefile for custom trace reader.
#
# Initial creation date : 07/09/2001 K.Y.
#

# Support
# If 1, will support RTAI (www.rtai.org) traces 
SUPP_RTAI = 1
# If 1, the visualizer will only support native traces
TARGET_NATIVE = 0
# If 1, visualizer will read structs as unpacked
LTT_UNPACKED_STRUCTS = 0

# Definitions
PREFIX      = /usr
EXEC_PREFIX = $(PREFIX)
CC          = gcc
DEBUG       = -g
DEFFLAGS    = -DSUPP_RTAI=$(SUPP_RTAI) -DTARGET_NATIVE=$(TARGET_NATIVE)
DEFFLAGS    += -DLTT_UNPACKED_STRUCTS=$(LTT_UNPACKED_STRUCTS)
INCLUDEDIRS = -I../../Include
CFLAGS      = -O2 -Wall $(DEFFLAGS) $(DEBUG)
LFLAGS      = -lltt
DEPFILE     = .depend

# Files (note that the INCLUDES variable isn't used anywhere, it's there for reference's sake)
INCLUDES = LTTTypes.h LinuxEvents.h Tables.h LinuxTables.h EventDBI.h EventDB.h
OBJS = TraceReader.o

ifeq ($(SUPP_RTAI),1)
INCLUDES += RTAIEvents.h RTAITables.h RTAIDB.h
endif

# Installation variables
EXEC_NAME    = tracereader

# Rules
all: TraceReader

.depend:
	@echo "Building dependencies"
	rm -rf .depend
	touch .depend
	makedepend -f .depend $(DEFFLAGS) $(INCLUDEDIRS) *.c
	@echo "Dependencies built"

depend dep: .depend

.c.o:
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<

TraceReader: ${OBJS}
	$(CC) -o $(EXEC_NAME) ${OBJS} $(LFLAGS)

clean: 
	rm -rf *.o $(EXEC_NAME) core

distclean:
	rm -rf *~
	rm -rf .depend .depend.bak
	rm -rf *.o $(EXEC_NAME) core

-include .depend
