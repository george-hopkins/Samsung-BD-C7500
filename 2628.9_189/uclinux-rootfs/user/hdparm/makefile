#
# Makefile for hdparm
#
# 09/05/03 Ton Truong <ttruong@broadcom.com>
#

# __USE_GNU needed for O_DIRECT
CFLAGS += -I$(ROOTDIR)/$(LINUXDIR)/include -D_GNU_SOURCE

.EXPORT_ALL_VARIABLES:

DESTDIR=$(ROOTDIR)/romfs

HDPARM_DEPS = hdparm.o identify.o $(ROOTDIR)/$(LINUXDIR)/include/linux/hdreg.h

all:
	make -f Makefile hdparm

.PHONEY: romfs

romfs: all
ifeq ($(CONFIG_USER_HDPARM_HDPARM),y)
	make -f Makefile  install
endif

clean distclean:
	rm -f hdparm *.o *core*

