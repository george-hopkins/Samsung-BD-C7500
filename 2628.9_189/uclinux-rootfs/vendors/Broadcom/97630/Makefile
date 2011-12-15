#
# Copyright (C) 2000-2005 Broadcom Corporation
#
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
#
#	Makefile -- Build instructions for Broadcom/97630
#

include $(LINUX_CONFIG)
include $(CONFIG_CONFIG)
include $(ARCH_CONFIG)

PLATFORM := 7630
VERSION := $(shell cat $(ROOTDIR)/version)
LINUX_VERSION := $(shell cat $(ROOTDIR)/$(LINUXDIR)/include/linux/utsrelease.h  | grep UTS_RELEASE|cut -d" " -f3)
LINUXFILE = vmlinux
COMPRESSED_LINUXFILE = vmlinuz
ROMFSFILE = romfs.img
CRAMFSFILE= cramfs.img
SQUASHFSFILE=squashfs.img
JFFSFILE  = jffs.img
JFFS2FILE = jffs2-128k.img
JFFS2_16K_FILE = jffs2-16k.img
JFFS2_512K_FILE = jffs2-512k.img
YAFFSFILE = yaffs-frm-cramfs.img
LINUXIMG  = $(IMAGEDIR)/$(PLATFORM)/$(COMPRESSED_LINUXFILE)
ROMFSIMG  = $(IMAGEDIR)/$(PLATFORM)/$(ROMFSFILE)
INITRDIMG = $(INITRDDIR)/$(PLATFORM)/ramdisk.gz
JFFSIMG   = $(IMAGEDIR)/$(PLATFORM)/$(JFFSFILE)
JFFS2IMG  = $(IMAGEDIR)/$(PLATFORM)/$(JFFS2FILE)
JFFS2_16K_IMG  = $(IMAGEDIR)/$(PLATFORM)/$(JFFS2_16K_FILE)
JFFS2_512K_IMG = $(IMAGEDIR)/$(PLATFORM)/$(JFFS2_512K_FILE)
YAFFSIMG   = $(IMAGEDIR)/$(PLATFORM)/$(YAFFSFILE)
CRAMFSIMG = $(IMAGEDIR)/$(PLATFORM)/$(CRAMFSFILE)
SQUASHFSIMG = $(IMAGEDIR)/$(PLATFORM)/$(SQUASHFSFILE)
INITRAMFS_DIR = $(IMAGEDIR)/$(PLATFORM)/initramfs
UMDRV     = umdrv/bcmdriver.o
# For now until we make umdrv part of the kernel module build process
UMDRVDIR  := $(ROMFSDIR)/lib/modules/$(LINUX_VERSION)/drivers/umdrv
KINITDIR  = ../common/kinit
ROMFSDIR ?= $(ROOTDIR)/romfs

DIRS =

ROMFS_DIRS = bin dev etc etc/dhcpc lib lib/modules home mnt mnt/hd mnt/nfs proc root usr var sys

ifdef CONFIG_BLK_DEV_INITRD
ROMFS_DIRS += sbin tmp var/run var/log var/tmp /var/lock dev/pts
#FSIMG = $(INITRDIMG)
#INITRDDIR = $(ROOTDIR)/$(LINUXDIR)/arch/mips/ramdisk
endif
ifdef CONFIG_ROMFS_FS
ROMFS_DIRS += sbin
FSIMG = $(ROMFSIMG)
endif
#ifdef CONFIG_JFFS_FS
#ROMFS_DIRS += sbin tmp var/run var/log var/tmp /var/lock usr/sbin
#FSIMG = $(JFFSIMG)
#endif
ifdef CONFIG_JFFS2_FS
ROMFS_DIRS += sbin tmp var/run var/log var/tmp /var/lock usr/sbin
FSIMG += $(JFFS2IMG) $(JFFS2_16K_IMG) $(JFFS2_512K_IMG)
endif
ifdef CONFIG_YAFFS_FS
ROMFS_DIRS += sbin tmp var/run var/log var/tmp /var/lock usr/sbin
FSIMG += $(YAFFSIMG)
endif
ifdef CONFIG_YAFFS_YAFFS2
ROMFS_DIRS += sbin tmp var/run var/log var/tmp /var/lock usr/sbin
FSIMG += $(YAFFSIMG)
endif
ifdef CONFIG_CRAMFS
ROMFS_DIRS += sbin tmp var/run var/log var/tmp /var/lock usr/sbin
FSIMG += $(CRAMFSIMG)
endif
ifdef CONFIG_SQUASHFS
FSIMG += $(SQUASHFSIMG)
endif
ifdef CONFIG_OPROFILE
BMIPS_DIR=mips/24K
TARGET_PREFIX=/usr/local/share/oprofile/
BUILD_PREFIX=$(ROOTDIR)/romfs/$(TARGET_PREFIX)
OPROFILE_DIR=$(ROOTDIR)/user/oprofile
endif

DEVICES = \
	tty,c,5,0		console,c,5,1		ptmx,c,5,2 \
	ram0,b,1,0		ram1,b,1,1		mem,c,1,1 \
	kmem,c,1,2		null,c,1,3		zero,c,1,5 \
	random,c,1,8		urandom,c,1,9 \
	\
	tty0,c,4,0		tty1,c,4,1		tty2,c,4,2 \
	tty3,c,4,3		tty4,c,4,4		tty5,c,4,5 \
	ttyS0,c,4,64		ttyS1,c,4,65 		ttyS2,c,4,66\
	\
	hda,b,3,0		hda1,b,3,1		hda2,b,3,2 \
	hda3,b,3,3		hda4,b,3,4		hda5,b,3,5 \
	hda6,b,3,6 \
	\
	hdc,b,22,0		hdc1,b,22,1		hdc2,b,22,2 \
	hdc3,b,22,3		hdc4,b,22,4		hdc5,b,22,5 \
	hdc6,b,22,6 \
	\
	hde,b,33,0		hde1,b,33,1		hde2,b,33,2 \
	hde3,b,33,3		hde4,b,33,4		hde5,b,33,5 \
	hde6,b,33,6 \
	\
	hdf,b,33,64		hdf1,b,33,65		hdf2,b,33,66 \
	hdf3,b,33,67		hdf4,b,33,68		hdf5,b,33,69 \
	hdf6,b,33,70 \
	\
	sda,b,8,0		sda1,b,8,1		sda2,b,8,2 \
	sda3,b,8,3		sda4,b,8,4		sda5,b,8,5 \
	sda6,b,8,6 \
	\
	sdb,b,8,16		sdb1,b,8,17		sdb2,b,8,18 \
	sdb3,b,8,19		sdb4,b,8,20		sdb5,b,8,21 \
	sdb6,b,8,22 \
	\
	sdc,b,8,32		sdc1,b,8,33		sdc2,b,8,34 \
	sdc3,b,8,35		sdc4,b,8,36		sdc5,b,8,37 \
	sdc6,b,8,38 \
	\
	sdd,b,8,48		sdd1,b,8,49		sdd2,b,8,50 \
	sdd3,b,8,51		sdd4,b,8,52		sdd5,b,8,53 \
	sdd6,b,8,54 \
	\
	sr0,b,11,0		sr1,b,11,1		\
	scd0,b,11,0		scd1,b,11,1		\
	\
	mtd0,c,90,0		mtdr0,c,90,1		mtdblock0,b,31,0 \
	mtd1,c,90,2		mtdr1,c,90,3		mtdblock1,b,31,1 \
	mtd2,c,90,4		mtdr2,c,90,5		mtdblock2,b,31,2 \
	mtd3,c,90,6		mtdr3,c,90,7		mtdblock3,b,31,3 \
	mtd4,c,90,8		mtdr4,c,90,9		mtdblock4,b,31,4 \
	mtd5,c,90,10		mtdr5,c,90,11		mtdblock5,b,31,5 \
	mtd6,c,90,12		mtdr6,c,90,13		mtdblock6,b,31,6 \
	mtd7,c,90,14		mtdr7,c,90,15		mtdblock7,b,31,7 \
	mtd8,c,90,16		mtdr8,c,90,17		mtdblock8,b,31,8 \
	mtd9,c,90,18		mtdr9,c,90,19		mtdblock9,b,31,9 \
	mtd10,c,90,20		mtdr10,c,90,21		mtdblock10,b,31,10 \
	mtd11,c,90,22		mtdr11,c,90,23		mtdblock11,b,31,11 \
	mtd12,c,90,24		mtdr12,c,90,25		mtdblock12,b,31,12 \
	mtd13,c,90,26		mtdr13,c,90,27		mtdblock13,b,31,13 \
	mtd14,c,90,28		mtdr14,c,90,29		mtdblock14,b,31,14 \
	mtd15,c,90,30		mtdr15,c,90,31		mtdblock15,b,31,15 \
	mtd16,c,90,32		mtdr16,c,90,33		mtdblock16,b,31,16 \
	mtd17,c,90,34		mtdr17,c,90,35		mtdblock17,b,31,17 \
	mtd18,c,90,36		mtdr18,c,90,37		mtdblock18,b,31,18 \
	mtd19,c,90,38		mtdr19,c,90,39		mtdblock19,b,31,19 \
	mtd20,c,90,40		mtdr20,c,90,41		mtdblock20,b,31,20 \
	mtd21,c,90,42		mtdr21,c,90,43		mtdblock21,b,31,21 \
	mtd22,c,90,44		mtdr22,c,90,45		mtdblock22,b,31,22 \
	mtd23,c,90,46		mtdr23,c,90,47		mtdblock23,b,31,23 \
	mtd24,c,90,48		mtdr24,c,90,49		mtdblock24,b,31,24 \
	mtd25,c,90,50		mtdr25,c,90,51		mtdblock25,b,31,25 \
	mtd26,c,90,52		mtdr26,c,90,53		mtdblock26,b,31,26 \
	mtd27,c,90,54		mtdr27,c,90,55		mtdblock27,b,31,27 \
	mtd28,c,90,56		mtdr28,c,90,57		mtdblock28,b,31,28 \
	mtd29,c,90,58		mtdr29,c,90,59		mtdblock29,b,31,29 \
	mtd30,c,90,60		mtdr30,c,90,61		mtdblock30,b,31,30 \
	mtd31,c,90,62		mtdr31,c,90,63		mtdblock31,b,31,31 \
	\
	romblock0,b,253,0 \
	romblock1,b,253,1 \
	romblock2,b,253,2 \
	romblock3,b,253,3 \
	romblock4,b,253,4 \
	romblock5,b,253,5 \
	romblock6,b,253,6 \
	romblock7,b,253,7 \
	romblock8,b,253,8 \
	romblock9,b,253,9 \
	romblock10,b,253,10 \
	romblock11,b,253,11 \
	romblock12,b,253,12 \
	romblock13,b,253,13 \
	romblock14,b,253,14 \
	romblock15,b,253,15 \
	romblock16,b,253,16 \
	romblock17,b,253,17 \
	romblock18,b,253,18 \
	romblock19,b,253,19 \
	romblock20,b,253,20 \
	romblock21,b,253,21 \
	romblock22,b,253,22 \
	romblock23,b,253,23 \
	romblock24,b,253,24 \
	romblock25,b,253,25 \
	romblock26,b,253,26 \
	romblock27,b,253,27 \
	romblock28,b,253,28 \
	romblock29,b,253,29 \
	romblock30,b,253,30 \
	romblock31,b,253,31 \
	\
        loop0,b,7,0		loop1,b,7,1		loop2,b,7,2 \
	loop3,b,7,3		loop4,b,7,4		loop5,b,7,5 \
	loop6,b,7,6		loop7,b,7,7		loop8,b,7,8 \
	loop9,b,7,9		loop10,b,7,10		loop11,b,7,11 \
	loop12,b,7,12		loop13,b,7,13		loop14,b,7,14 \
	loop15,b,7,15 \
	mmc0,b,179,0 \
	mmc1,b,179,1 \
	power,c,16,0
# PR12239 - Remove Broadcom specific device nodes from root filesystem 

all:
	dirs=$(DIRS) ; \
	for i in $$dirs ; do  make -C $$i || exit $? ; done

clean:
	-dirs=$(DIRS) ; \
	for i in $$dirs; do [ ! -d $$i ] || make -C $$i clean; done
	-make -C $(KINITDIR) clean
	rm -f $(ROOTDIR)/$(LINUXDIR)/usr/initramfs.dev
	make -C $(ROOTDIR)/$(LINUXDIR)/usr clean
	#make -C $(ROOTDIR)/user/kinit clean
	rm -rf $(IMAGEDIR)/$(PLATFORM)
	make -C hotplug clean

romfs:
	[ -d $(ROMFSDIR)/$$i ] || mkdir -p $(ROMFSDIR)
	for i in $(ROMFS_DIRS); do \
		[ -d $(ROMFSDIR)/$$i ] || mkdir -p $(ROMFSDIR)/$$i; \
	done
	dirs=$(DIRS) ; for i in $$dirs; do  make -C $$i romfs || exit $? ; done
ifdef CONFIG_ROMFS_FS
	$(ROMFSINST) -s /var/tmp /tmp
endif
	rm -f $(ROMFSDIR)/dev/log
	cp -af /dev/log $(ROMFSDIR)/dev
	echo "$(VERSIONSTR) -- " `date` > $(ROMFSDIR)/etc/version
	date '+%Y.%m.%d-%T' > $(ROMFSDIR)/etc/build-date
	rm -rf $(ROMFSDIR)/include
	rm -rf $(ROMFSDIR)/man
	rm -rf $(ROMFSDIR)/share/aclocal
	rm -rf $(ROMFSDIR)/lib/directfb-*/gfxdrivers
	rm -rf $(ROMFSDIR)/lib/directfb-*/interfaces/IDirectFBVideoProvider
	make -C hotplug romfs
	mkdir -p -m 0777 $(ROMFSDIR)/mnt/removable
	mkdir -p -m 0777 $(ROMFSDIR)/mnt/removable/usba
	mkdir -p -m 0777 $(ROMFSDIR)/mnt/removable/usbb
	mkdir -p -m 0777 $(ROMFSDIR)/mnt/removable/usbc
	mkdir -p -m 0777 $(ROMFSDIR)/mnt/removable/usbd
	mkdir -p -m 0777 $(ROMFSDIR)/mnt/removable/mmc


initrd: kinit
	umask 022
	cp -af etc/* $(ROMFSDIR)/etc
	-test -d $(ROMFSDIR)/usr/sbin || mkdir $(ROMFSDIR)/usr/sbin
	sed "s,@VERSION@,$(VERSION),g" usr/sbin/stbutil | \
		sed "s,@PLATFORM@,$(PLATFORM),g" > $(ROMFSDIR)/usr/sbin/stbutil
    #sed 's,###mount,mount,g' etc/init.d/rcS > $(ROMFSDIR)/var/tmp/rcS.hd
	chmod +x $(ROMFSDIR)/usr/sbin/stbutil
	cp -f usr/sbin/usb* $(ROMFSDIR)/usr/sbin/
	chmod +x $(ROMFSDIR)/usr/sbin/usb*
	#Enabling 2nd serial port
	chmod u+w $(ROMFSDIR)/etc/inittab
	# THT: Disable UARTB for now 
	# sed 's,#@@@@@ttyS1,ttyS1,g' etc/inittab >$(ROMFSDIR)/etc/inittab
	#[ -d $(UMDRVDIR) ] || mkdir -p $(UMDRVDIR)
	#cp -f $(UMDRV) $(UMDRVDIR)
	echo "Stripping executables"
	for fo in `find $(ROMFSDIR) -type f -perm +111`; do \
		if file $$fo | grep "ELF" | grep "executable" > /dev/null; then \
			$(STRIP) $$fo; \
		fi; \
	done
	echo "Removing old devices"
	-for i in $(DEVICES); do \
		rm -f $(ROMFSDIR)/dev/`echo $$i | sed -e 's/,/ /g'` ; \
	done
	make -C $(ROOTDIR)/$(LINUXDIR)/usr clean
	# Make the dev list to allow normal user to build the kernel
	echo "Make new initramfs dev list"
	-rm $(ROOTDIR)/$(LINUXDIR)/usr/initramfs.dev
	touch $(ROOTDIR)/$(LINUXDIR)/usr/initramfs.dev
	for i in $(DEVICES); do \
		echo "nod /dev/`echo $$i |sed -e 's/,/ 0600 0 0 /' \
			|sed -e 's/,/ /g'`" >>$(ROOTDIR)/$(LINUXDIR)/usr/initramfs.dev; \
	done
	# Insert /dev/root
	echo "nod /dev/root 0600 0 0 b 1 0" >>$(ROOTDIR)/$(LINUXDIR)/usr/initramfs.dev
	# Make the symlink required by initramfs
	cp -f $(KINITDIR)/init $(ROMFSDIR)/init
ifdef CONFIG_OPROFILE
	-mkdir -p $(BUILD_PREFIX)/$(BMIPS_DIR)
	cp -f $(OPROFILE_DIR)/events/$(BMIPS_DIR)/events \
		$(BUILD_PREFIX)/$(BMIPS_DIR)/events
	cp -f $(OPROFILE_DIR)/events/$(BMIPS_DIR)/unit_masks \
		$(BUILD_PREFIX)/$(BMIPS_DIR)/unit_masks
endif


.PHONY: kinit initramfs_dir images

kinit:
	echo "Making in kinit"
	make -C $(KINITDIR)

$(UMDRV):
	# No longer needed to be built
	# make -C ./umdrv

# Remove JFFS2 until the mkfs.jffs2 is changed to handle nand (-N switch)
#images: $(JFFS2IMG) $(JFFS2_16K_IMG) $(JFFS2_512K_IMG) $(CRAMFSIMG) $(SQUASHFSIMG) $(YAFFSIMG)
images: $(CRAMFSIMG) $(SQUASHFSIMG) $(YAFFSIMG)

#*************************************************************************
# Be careful, removing --no-absolute-filenames or mispelling it will trash
# your PC host.  Will refuse to run if INITRAMFS_DIR is NULL, as that
# would also hose the host by overwriting the host's root fs.
# We only check for NULL, "/" and "//" as much as we can go.
# A value of "///" etc... will still trash it
#**************************************************************************
initramfs_dir: $(IMAGEDIR)/$(PLATFORM)/initramfs_data.cpio.gz
	@if [ ! -z "$(INITRAMFS_DIR)" -a "$(INITRAMFS_DIR)" != "/" -a "$(INITRAMFS_DIR)" != "//" ]; then \
		zcat $< | (cd $(INITRAMFS_DIR); cpio -ivd --no-absolute-filenames); \
	else \
		echo "Refuse to run: INITRAMFS_DIR=\"$(INITRAMFS_DIR)\" would trash your host"; exit 1;\
	fi


# Added parameters for NAND.  pagesize is for FS, and NOT 512, which is the cache size.
# Now building for both 128K and 16K erase block sizes
$(JFFS2IMG) $(JFFS2_16K_IMG) $(JFFS2_512K_IMG): initramfs_dir
	$(ROOTDIR)/user/mtd-utils/build/mkfs.jffs2 -o $(JFFS2IMG) -N --squash --verbose --little-endian \
		--eraseblock=128KiB   \
		-d $(INITRAMFS_DIR)
ifdef CONFIG_JFFS2_SUMMARY
	mv ${JFFS2IMG} ${JFFS2IMG}.nosummary
	$(ROOTDIR)/user/mtd-utils/build/sumtool -l -i ${JFFS2IMG}.nosummary -o $(JFFS2IMG) -e 128KiB
endif
	$(ROOTDIR)/user/mtd-utils/build/mkfs.jffs2 -o $(JFFS2_16K_IMG) -N --squash --verbose --little-endian \
		--eraseblock=16KiB  --pagesize=512 \
		-d $(INITRAMFS_DIR)
ifdef CONFIG_JFFS2_SUMMARY
	mv ${JFFS2_16K_IMG} ${JFFS2_16K_IMG}.nosummary
	$(ROOTDIR)/user/mtd-utils/build/sumtool -l -i ${JFFS2_16K_IMG}.nosummary -o $(JFFS2_16K_IMG) -e 16KiB
endif
	$(ROOTDIR)/user/mtd-utils/build/mkfs.jffs2 -o ${JFFS2_512K_IMG} -N --squash --verbose --little-endian \
		--eraseblock=512KiB  --pagesize=512 \
		-d $(INITRAMFS_DIR)
ifdef CONFIG_JFFS2_SUMMARY
	mv ${JFFS2_512K_IMG} ${JFFS2_512K_IMG}.nosummary
	$(ROOTDIR)/user/mtd-utils/build/sumtool -l -i ${JFFS2_512K_IMG}.nosummary -o $(JFFS2_512K_IMG) -e 512KiB
endif

$(JFFSIMG):
	$(ROOTDIR)/user/mtd-utils/build/mkfs.jffs -o $(JFFSIMG) -d $(ROMFSDIR)

# sidc - PR33102 - nandwrite is unreliable to burn a yaffs2 image onto flash.
# We use a cramfs image, do a loopback mount to load yaffs/yaffs2 image
$(YAFFSIMG):
	$(ROOTDIR)/user/cramfs/build/mkcramfs -v -n "YaffsDerived" $(INITRAMFS_DIR) $(YAFFSIMG); \
	chmod a+r $(YAFFSIMG)

# jipeng - PR22427 dhcpcd failed with cramfs
# here we create var.img to contain RW /var. This will be
# mounted by rcS if it exists.
# Use default 64KB block size for SquashFS. 
$(CRAMFSIMG) $(SQUASHFSIMG): common_ro_fs
	if [ $@ == $(CRAMFSIMG) ]; then \
		$(ROOTDIR)/user/cramfs/build/mkcramfs -v -n "Cramfs" $(INITRAMFS_DIR) $(CRAMFSIMG); \
		chmod a+r $(CRAMFSIMG); \
	else \
		$(ROOTDIR)/user/squashfs-tools/build/mksquashfs $(INITRAMFS_DIR) $(SQUASHFSIMG) -b 65536 -le -all-root -noappend -info; \
		chmod a+r $(SQUASHFSIMG); \
	fi

# syslogd opens /dev/log as a socket and this needs to be writable
# so link it over to var
.PHONY: common_ro_fs
common_ro_fs:initramfs_dir
	dd if=/dev/zero of=$(INITRAMFS_DIR)/tmpfsvar.img bs=1024 count=500
	cd $(INITRAMFS_DIR)/var; tar -cvf $(INITRAMFS_DIR)/var.img . 2>&1 >/dev/null
	cd $(INITRAMFS_DIR)/etc; tar -cvf $(INITRAMFS_DIR)/etc.img . 2>&1 >/dev/null
	cd $(INITRAMFS_DIR)/dev; ln -sf ../var/run/syslogd.socket log


$(ROMFSIMG):
	genromfs -v -V "ROMdisk" -f $(ROMFSIMG) -d $(ROMFSDIR)

.PHONY: image $(FSIMG)
image:   # Removed.  Now build later with images target $(FSIMG)
	[ -d $(INITRAMFS_DIR) ] || mkdir -p $(INITRAMFS_DIR)
	gzip -fc $(ROOTDIR)/$(LINUXDIR)/$(LINUXFILE) > $(LINUXIMG)
	cp -f $(ROOTDIR)/$(LINUXDIR)/usr/initramfs_data.cpio.gz $(IMAGEDIR)/$(PLATFORM)
	cp -f $(ROOTDIR)/$(LINUXDIR)/usr/initramfs_data_nodev.cpio $(IMAGEDIR)/$(PLATFORM)
