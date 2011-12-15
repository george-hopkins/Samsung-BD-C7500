
ifdef HOST_BUILD
CC = gcc
AR = ar
CFLAGS = -O2
else
CC = mipsel-uclibc-gcc
AR = mipsel-uclibc-ar
CFLAGS = -O2 -mabi=32 -mips32 -march=mips32 -mtune=mips32
endif

DEFINES = -DENABLE_T1 \
	-DENABLE_SPD \
	-DENABLE_CFF \
	-DENABLE_PFR \
	-DENABLE_T2KS \
	-DENABLE_OTF \
	-DENABLE_BDF \
	-DCOMPRESSED_INPUT_STREAM \
	-DENABLE_NATIVE_T1_HINTS \
	-DENABLE_AUTO_GRIDDING \
	-DENABLE_SBIT \
	-DENABLE_SBITS_TRANSFORM \
	-DENABLE_STRKCONV=32

CFLAGS += -I../../core $(DEFINES)

ifeq (y,${DEBUG})
	CFLAGS += -DDEBUG=1
endif

SOURCES= ../../core/autogrid.c \
	../../core/bdf.c \
	../../core/cstream.c \
	../../core/fft1hint.c \
	../../core/fnt.c \
	../../core/ghints.c \
	../../core/glyph.c \
	../../core/hstrat97.c \
	../../core/ldecode.c \
	../../core/pfrread.c \
	../../core/sniffer.c \
	../../core/spdread.c \
	../../core/strkconv.c \
	../../core/t1.c \
	../../core/t2k.c \
	../../core/t2ksbit.c \
	../../core/t2ksc.c \
	../../core/t2kstrk1.c \
	../../core/t2kstrm.c \
	../../core/t2ktt.c \
	../../core/truetype.c \
	../../core/tsimem.c \
	../../core/util.c

HEADERS= ../../core/agridint.h \
	../../core/autogrid.h \
	../../core/auxdata.h \
	../../core/bdf.h \
	../../core/config.h \
	../../core/dtypes.h \
	../../core/fft1hint.h \
	../../core/fnt.h \
	../../core/ghints.h \
	../../core/glyph.h \
	../../core/ldecode.h \
	../../core/orion.h \
	../../core/pclread.h \
	../../core/pfrread.h \
	../../core/scodecdefs.h \
	../../core/scoder.h \
	../../core/sdecoder.h \
	../../core/shapet.h \
	../../core/spdread.h \
	../../core/strkconv.h \
	../../core/syshead.h \
	../../core/util.h \
	../../core/t1.h \
	../../core/t1order.h \
	../../core/t2k.h \
	../../core/t2kextra.h \
	../../core/t2ksbit.h \
	../../core/t2ksc.h \
	../../core/t2kstrk1.h \
	../../core/t2kstrm.h \
	../../core/t2ktt.h \
	../../core/truetype.h \
	../../core/tsimem.h \
	../../core/tt_prvt.h

LIBNAME := libt2k

$(HEADERS):

$(SOURCES): $(HEADERS)

%.o: %.c
	echo Compiling $^
	${Q_}$(CC) $(CFLAGS) -c $<

all: $(LIBNAME)

$(LIBNAME): $(subst .c,.o, $(SOURCES))
	${Q_}$(AR) cr $@.a $(notdir $^)

clean:
	rm -f $(notdir $(subst .c,.o, $(SOURCES))) $(LIBNAME).a

install:
	@if [ -z $(DESTDIR) ]; then \
		 echo "define DESTDIR"; \
	else \
		cp $(HEADERS) $(DESTDIR)/usr/local/include; \
		cp $(LIBNAME).a $(DESTDIR)/usr/local/lib; \
	fi

