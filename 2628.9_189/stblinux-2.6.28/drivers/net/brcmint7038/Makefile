#
#
# Copyright (c) 2002-2005 Broadcom Corporation 
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
#
# File: drivers/net/brcmint7038/Makefile
#
# Makefile for the BCM7038 Ethernet driver
#

obj-$(CONFIG_BCMINTEMAC_7038) += bcmemacnet.o

bcmemacnet-objs := bcmemac.o bcmmii.o boardparms.o


#-DDUMP_DATA -- dump data
#-DUSE_BH -- use bottom half
#-DUSE_PROC -- display ethernet driver internal info
# THT for debug only EXTRA_CFLAGS += -I. -DDUMP_TRACE -DDUMP_DATA
EXTRA_CFLAGS += -I. -DUSE_PROC
#EXTRA_CFLAGS += -I. -DUSE_PROC -DDUMP_TRACE -DDUMP_DATA
# THT Not Yet EXTRA_CFLAGS += -DUSE_BH

EXTRA_CFLAGS += -O1 

CFLAGS_bcmemac.o += -DCONFIG_MTD_BRCMNAND=1 

