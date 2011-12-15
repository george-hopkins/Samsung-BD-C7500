
# -*- sh -*-

CPPFLAGS += -I./include $(ZLIBCPPFLAGS) $(LZOCPPFLAGS)

CPPFLAGS += -I../../linux-2.6.x/include

NAND_UTILS_LIB_PATH := -L../../lib/libnandutils

ifeq ($(WITHOUT_XATTR), 1)
  CPPFLAGS += -DWITHOUT_XATTR
endif

SUBDIRS = mkfs.ubifs ubi-utils

TARGETS = ftl_format flash_erase flash_eraseall nanddump doc_loadbios \
	ftl_check mkfs.jffs2 flash_lock flash_unlock flash_info \
	flash_otp_info flash_otp_dump mtd_debug flashcp nandwrite nandtest \
    nandinfo nandstat 	geneccerror bbtmod ft_mips \
	jffs2dump \
	nftldump nftl_format docfdisk \
	rfddump rfdformat \
	serve_image recv_image \
	sumtool #jffs2reader

SYMLINKS =

include common.mk

clean::
	-rm -f $(SYMLINKS)
ifneq ($(BUILDDIR)/.git,)
ifneq ($(BUILDDIR),.)
ifneq ($(BUILDDIR),$(PWD))
	rm -rf $(BUILDDIR)
endif
endif
endif



$(SYMLINKS):
	ln -sf ../fs/jffs2/$@ $@

$(BUILDDIR)/mkfs.jffs2: $(addprefix $(BUILDDIR)/,\
	crc32.o compr_rtime.o mkfs.jffs2.o compr_zlib.o compr_lzo.o \
	compr.o rbtree.o)
LDFLAGS_mkfs.jffs2 = $(ZLIBLDFLAGS) $(LZOLDFLAGS)
LDLIBS_mkfs.jffs2  = -lz -llzo2

$(BUILDDIR)/flash_eraseall: $(BUILDDIR)/crc32.o $(BUILDDIR)/flash_eraseall.o
	$(CC) $(LDFLAGS) -o $@ $^ $(NAND_UTILS_LIB_PATH) -lnandutils

$(BUILDDIR)/nandwrite: $(BUILDDIR)/crc32.o $(BUILDDIR)/nandwrite.o
	$(CC) $(LDFLAGS) -o $@ $^ $(NAND_UTILS_LIB_PATH) -lnandutils

$(BUILDDIR)/nanddump: $(BUILDDIR)/crc32.o $(BUILDDIR)/nanddump.o
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILDDIR)/nandinfo: $(BUILDDIR)/crc32.o $(BUILDDIR)/nandinfo.o
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILDDIR)/nandstat: $(BUILDDIR)/crc32.o $(BUILDDIR)/nandstat.o
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILDDIR)/geneccerror: $(BUILDDIR)/crc32.o $(BUILDDIR)/geneccerror.o
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILDDIR)/bbtmod: $(BUILDDIR)/crc32.o $(BUILDDIR)/bbtmod.o
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILDDIR)/ft_mips: $(BUILDDIR)/crc32.o $(BUILDDIR)/ft_mips.o
	$(CC) $(LDFLAGS) -o $@ $^


$(BUILDDIR)/jffs2reader: $(BUILDDIR)/jffs2reader.o
LDFLAGS_jffs2reader = $(ZLIBLDFLAGS) $(LZOLDFLAGS)
LDLIBS_jffs2reader  = -lz -llzo2

$(BUILDDIR)/jffs2dump: $(BUILDDIR)/jffs2dump.o $(BUILDDIR)/crc32.o

$(BUILDDIR)/sumtool: $(BUILDDIR)/sumtool.o $(BUILDDIR)/crc32.o

$(BUILDDIR)/serve_image: $(BUILDDIR)/serve_image.o $(BUILDDIR)/crc32.o $(BUILDDIR)/fec.o

$(BUILDDIR)/recv_image: $(BUILDDIR)/recv_image.o $(BUILDDIR)/crc32.o $(BUILDDIR)/fec.o

$(BUILDDIR)/fectest: $(BUILDDIR)/fectest.o $(BUILDDIR)/crc32.o $(BUILDDIR)/fec.o



install:: ${TARGETS}
	mkdir -p ${DESTDIR}/${SBINDIR}
	install -m 0755 ${TARGETS} ${DESTDIR}/${SBINDIR}/
	mkdir -p ${DESTDIR}/${MANDIR}/man1
	gzip -9c mkfs.jffs2.1 > ${DESTDIR}/${MANDIR}/man1/mkfs.jffs2.1.gz
