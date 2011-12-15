
# -*- sh -*-

# BRCM start
include $(ROOTDIR)/$(LINUXDIR)/.config
ifeq ($(CONFIG_CPU_LITTLE_ENDIAN),y)
	TARGET_ENDIAN = __MIPSEL__
	CROSS=mipsel-linux-
else
	TARGET_ENDIAN = __MIPSEB__
	CROSS=mips-linux-
endif
MY_CFLAGS := -I$(ROOTDIR)/user/lzo/include -O2 -Wall -DWITHOUT_XATTR

BUILD_CFLAGS := $(MY_CFLAGS)
BUILD_CFLAGS += -I$(ROOTDIR)/linux-2.6.x/include
BUILD_CC = gcc

WITHOUT_XATTR=1

ifneq ($(CONFIG_MTD_BRCMNAND),)
TARGETS +=  nandwrite nanddump nandinfo nandstat geneccerror bbtmod ft_mips
endif
ifneq ($(CONFIG_JFFS2_FS),)
TARGETS += jffs2dump mkfs.jffs2
endif


LZO_LIB_PATH := -L$(ROOTDIR)/user/lzo/src/.libs 
ZLIB_LIB_PATH := -L$(ROOTDIR)/lib/zlib/ 
NAND_UTILS_LIB_PATH := -L$(ROOTDIR)/lib/libnandutils


OPTFLAGS := -I$(ROOTDIR)/user/lzo/include -O2 -Wall
OPTFLAGS += -I$(ROOTDIR)/linux-2.6.x/include
# BRCM end

#OPTFLAGS := -O2 -Wall
SBINDIR=/usr/sbin
MANDIR=/usr/man
INCLUDEDIR=/usr/include
#CROSS=arm-linux-
CC := $(CROSS)gcc
CFLAGS :=  -I./include $(OPTFLAGS)

ifeq ($(origin CROSS),undefined)
  BUILDDIR := .
else
# Remove the trailing slash to make the directory name
  BUILDDIR := $(CROSS:-=)
endif


ifeq ($(WITHOUT_XATTR), 1)
  CFLAGS += -DWITHOUT_XATTR
endif

RAWTARGETS = ftl_format flash_erase flash_eraseall nanddump doc_loadbios \
	ftl_check mkfs.jffs2 flash_lock flash_unlock flash_info \
	flash_otp_info flash_otp_dump mtd_debug flashcp nandwrite nandinfo nandstat \
	geneccerror bbtmod ft_mips jffs2dump \
	nftldump nftl_format docfdisk \
	rfddump rfdformat \
	serve_image recv_image \
	sumtool #jffs2reader

TARGETS = $(foreach target,$(RAWTARGETS),$(BUILDDIR)/$(target))

SYMLINKS =

%: %.o
	$(CC) $(CFLAGS) $(LDFLAGS) -g -o $@ $^

$(BUILDDIR)/%.o: %.c
	mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -g -c -o $@ $< -g -Wp,-MD,$(BUILDDIR)/.$(<F).dep

.SUFFIXES:

all: build $(TARGETS)

IGNORE=${wildcard $(BUILDDIR)/.*.c.dep}
-include ${IGNORE}

clean: host_build_clean
	rm -f $(BUILDDIR)/*.o $(TARGETS) $(BUILDDIR)/.*.c.dep $(SYMLINKS)
	if [ "$(BUILDDIR)x" != ".x" ]; then rm -rf $(BUILDDIR); fi
	make -C $(BUILDDIR)/ubi-utils clean

$(SYMLINKS):
	ln -sf ../fs/jffs2/$@ $@

$(BUILDDIR)/mkfs.jffs2:	$(BUILDDIR)/crc32.o \
			$(BUILDDIR)/compr_rtime.o \
			$(BUILDDIR)/mkfs.jffs2.o \
			$(BUILDDIR)/compr_zlib.o \
			$(BUILDDIR)/compr_lzo.o \
			$(BUILDDIR)/compr.o \
			$(BUILDDIR)/rbtree.o
	$(CC) $(LDFLAGS) -o $@ $^ $(ZLIB_LIB_PATH) -lz $(LZO_LIB_PATH) -llzo2

# jipeng - since only liblzo2.a exists, need "-static -lliblzo2" as above
#	$(CC) $(LDFLAGS) -o $@ $^ -lz -llzo2


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
	$(CC) $(LDFLAGS) -o $@ $^ -lz

$(BUILDDIR)/jffs2dump: $(BUILDDIR)/jffs2dump.o $(BUILDDIR)/crc32.o
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILDDIR)/sumtool: $(BUILDDIR)/sumtool.o $(BUILDDIR)/crc32.o
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILDDIR)/serve_image: $(BUILDDIR)/serve_image.o $(BUILDDIR)/crc32.o $(BUILDDIR)/fec.o
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILDDIR)/recv_image: $(BUILDDIR)/recv_image.o $(BUILDDIR)/crc32.o $(BUILDDIR)/fec.o
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILDDIR)/fectest: $(BUILDDIR)/fectest.o $(BUILDDIR)/crc32.o $(BUILDDIR)/fec.o
	$(CC) $(LDFLAGS) -o $@ $^



install: ${TARGETS}
	mkdir -p ${DESTDIR}/${SBINDIR}
	install -m0755 ${TARGETS} ${DESTDIR}/${SBINDIR}/
	mkdir -p ${DESTDIR}/${MANDIR}/man1
	gzip -c mkfs.jffs2.1 > ${DESTDIR}/${MANDIR}/man1/mkfs.jffs2.1.gz
	make -C $(BUILDDIR)/ubi-utils install

# BRCM start
ZINCS = zlib.h zconf.h 

.PHONY:
$(ZINCS):
	ln -sf $(ROOTDIR)/lib/zlib/$@ include/$@

build: $(ZINCS)
	[ -d build ] || mkdir build

host_build_clean:
	rm -rf build

build/mkfs.jffs2:	build/crc32.o \
			build/compr_rtime.o \
			build/mkfs.jffs2.o \
			build/compr_zlib.o \
			build/compr_lzo.o \
			build/compr.o \
			build/rbtree.o
	$(BUILD_CC) $(BUILD_LDFLAGS) -o $@ $^ -lz -llzo2

build/jffs2dump: build/jffs2dump.o build/crc32.o
	$(BUILD_CC) $(BUILD_CFLAGS) -o $@ $^

build/sumtool: build/sumtool.o build/crc32.o
	$(BUILD_CC) $(BUILD_CFLAGS) -o $@ $^

build/%.o: %.c
	$(BUILD_CC) $(BUILD_CFLAGS) -c -o $@ $<

build/%: %.c
	$(BUILD_CC) $(BUILD_CFLAGS) -o $@ $<

romfs:
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_ERASE	$(BUILDDIR)/flash_info		/bin/flash_info
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_ERASE	$(BUILDDIR)/flash_erase		/bin/flash_erase
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_ERASE -s /bin/flash_erase /bin/erase
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_ERASEALL	$(BUILDDIR)/flash_eraseall	/bin/flash_eraseall
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_ERASEALL -s /bin/flash_eraseall /bin/eraseall
ifneq ($(CONFIG_FTL),)
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_FTL_CHECK    /bin/ftl_check
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_FTL_FORMAT   /bin/ftl_format
endif
ifneq ($(CONFIG_NFTL),)
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_NFTLDUMP     /bin/nftldump
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_NFTL_FORMAT  /bin/nftl_format
endif
ifneq ($(CONFIG_MTD_BRCMNAND),)
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_NANDDUMP     $(BUILDDIR)/nanddump /bin/nanddump
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_NANDWRITE    $(BUILDDIR)/nandwrite /bin/nandwrite
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_NANDINFO     $(BUILDDIR)/nandinfo /bin/nandinfo
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_NANDSTAT     $(BUILDDIR)/nandstat /bin/nandstat
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_GENECCERROR  $(BUILDDIR)/geneccerror /bin/geneccerror
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_BBTMOD       $(BUILDDIR)/bbtmod /bin/bbtmod
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_FTMIPS       $(BUILDDIR)/ft_mips /bin/ft_mips
endif
ifneq ($(CONFIG_JFFS_FS),)
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_MKFSJFFS     $(BUILDDIR)/mkfs.jffs /bin/mkfs.jffs
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_MKFSJFFS     $(BUILDDIR)/mtd_debug /bin/mtd_debug
endif
ifneq ($(CONFIG_JFFS2_FS),)
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_MKFSJFFS2    $(BUILDDIR)/mkfs.jffs2 /bin/mkfs.jffs2
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_MKFSJFFS2    $(BUILDDIR)/mtd_debug /bin/mtd_debug
endif
ifneq ($(CONFIG_JFFS2_SUMMARY),)
	$(ROMFSINST) -e CONFIG_USER_MTDUTIL_MKFSJFFS2     $(BUILDDIR)/sumtool /bin/sumtool
endif
ifneq ($(CONFIG_MTD_DOCPROBE),)
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_DOC_LOADBIOS /bin/doc_loadbios
#	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_DOC_LOADIPL  /bin/doc_loadipl
endif
ifneq ($(CONFIG_MTD_DOC1000),)
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_DOC_LOADBIOS /bin/doc_loadbios
#	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_DOC_LOADIPL  /bin/doc_loadipl
endif
# BRCM end


