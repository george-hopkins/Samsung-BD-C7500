#
# Makefile for the Broadcom NAND MTD
#
EXTRA_CFLAGS	+= -I arch/mips/brcmstb/common
EXTRA_CFLAGS	+= -I $(TOPDIR)/include/asm-generic

obj-$(CONFIG_MTD_BRCMNAND)				+= bcm7xxx-nand.o brcmnand_base.o brcmnand_bbt.o brcmnand_page.o edu.o tasklet.o brcmnand_isr.o

