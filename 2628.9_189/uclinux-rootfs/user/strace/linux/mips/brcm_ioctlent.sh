#!/bin/sh 
# ********************************************************************************
#   Broadcom Corp. Confidential
#   Copyright 2003, 2002, 2001, 2000, 1999 Broadcom Corp.  
#   All Rights Reserved.
#
#   THIS SOFTWARE MAY ONLY BE USED SUBJECT TO AN EXECUTED SOFTWARE LICENSE
#   AGREEMENT  BETWEEN THE USER AND BROADCOM.  YOU HAVE NO RIGHT TO USE OR
#   EXPLOIT THIS MATERIAL EXCEPT SUBJECT TO THE TERMS OF SUCH AN AGREEMENT.
#
#   $brcm_Workfile:$
#   $brcm_Revision:$
#   $brcm_Date:$
#
#   Module Description: Generate ioctl header file from the SetTop tree.
#   Butchered from ioctlent.sh
#   Expected to be run in this same directory, in the format of
#
#	brcm_ioctlent.sh <your_linux_inc>
#
#   to generate ioctlsort.c
#
#   Or using the Makefile
#
#	make LINUX_INC=<your_linux_inc> SETTOP_INC=<your_settop_inc> brcmioctl 
#
#   will generate an ioctlent.h based on the current source tree.
#
#   Directory hardwired:
#
#   SETTOP_ROOT=/work/build/
#   SETTOP_PATH=${SETTOP_ROOT}SetTop/linux/driver/7320/include
#
# ********************************************************************************

# Copyright (c) 1993, 1994, 1995 Rick Sladkey <jrs@world.std.com>
# All rights reserved.
#
# Copyright (c) 1995, 1996 Michael Elizabeth Chastain <mec@duracef.shout.net>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#	$Id: ioctlent.sh,v 1.1 1999/11/01 00:46:49 wichert Exp $

SETTOP_ROOT=$2/
SETTOP_PATH=${SETTOP_ROOT}/linux/driver/7320/include
TEMP_FILE=/tmp/ioctl$$
IGNORE_IOCTL=/tmp/ignoreioctl$$
TEMP_HEADER=/tmp/ioctlheader$$
IOCTLSORT=./ioctlsort.c
IOCTL_H=./ioctls.h

# Files to find.
file_find="asm/*.h linux/*.h scsi/*.h $SETTOP_PATH/*.h"

# Files to stop.
file_stop='asm/byteorder.h linux/config.h linux/pci.h linux/xd.h'

# Defs to find.
# Work on the kernel source to convert all to df_iowr.
# Don't know how to find low-numbered ioctls in linux/mc146818rtc.h.
df_name='^[	 ]*#[	 ]*define[	 ]+[A-Z_][A-Z0-9_]*[	 ]+'
df_iowr='_IO|_IOR|_IOW|_IOWR'
df_NNNN='0[Xx](03|06|22|46|4B|4C|53|54|56|89|90)[0-9A-Fa-f][0-9A-Fa-f]'
df_4359='0[Xx]4359[0-9A-Fa-f][0-9A-Fa-f]'	# linux/cyclades.h
df_470N='470[0-9]'				# linux/fs.h        (only in 1.2.13)
df_smix='MIXER_READ|MIXER_WRITE'		# linux/soundcard.h
df_12NN='12[3-4][0-9]'				# linux/umsdos_fs.h (only in 1.2.13)
df_tail='([()	 ]|$)'
def_find="$df_name($df_iowr|$df_NNNN|$df_4359|$df_470N|$df_smix|$df_12NN)$df_tail"

# Defs to stop.
ds_tail='_MAGIC|_PATCH'
ds_fdmp='FD(DEF|GET|SET)MEDIAPRM'		# linux/fd.h aliases (only in 1.2.13)
ds_mtio='MTIOC(GET|SET)CONFIG'			# linux/mtio.h needs config (only in 1.2.13)
def_stop="$ds_tail|$ds_fdmp|$ds_mtio"

# Validate arg count.
if [ $# -ne 2 ]
then
	echo "usage: $0 linux-include-directory settop-root" >&2
	exit 1
fi

#
# Grep through the files for ioctl declarations.
#
(
	# Construct list: find files minus stop files.
	cd $1 || exit
	file_list=`(ls $file_find $file_stop $file_stop 2>/dev/null) | sort | uniq -u`

	# Grep matching #define lines.
	# Transform to C structure form.
	# Filter out stop list.
	egrep "$def_find" $file_list |
		sed -n -e 's/^\(.*\):#[	 ]*define[	 ]*\([A-Z_][A-Z0-9_]*\).*$/	{ "\1",	"\2",	\2	},/p' |
		egrep -v "$def_stop"
) > $IOCTL_H

#
# Create a list of header files we want to include.
#
awk '{ print "#include \"" substr($2, 2, length($2) - 3) "\"" }' $IOCTL_H | sort -u >> $TEMP_HEADER

cat $TEMP_HEADER > $TEMP_FILE

#
# Create a list of ioctl headers that we are not interested in, or can't handle
#
echo "au1000
nile4
ac97
agpgart
apm
atm
cciss
cyclades
cycx_
i2o
i8k
intermezzo
isdn
ixjuser
joystick
linux/lp
linux/lvm
matroxfb
meye
ncp
serial167
sonypi
sonet
soundcard
telephony
udfs
toshiba
umsdos
vt
SHMIQ_ON
SHMIQ_OFF
EXT3_IOC_WAIT_FOR_READONLY
MTIOC_ZFTAPE_GETBLKSZ
linux/pci_ids
linux/in6
linux/ps2esdi
asm/cpu
linux/wireless
linux/quota
linux/zorro_ids
watchdog" > $IGNORE_IOCTL

#
# Generate the ioctls.h header file.
#
cat $IOCTL_H | grep -v -f $IGNORE_IOCTL | sed 's,^,%%,g' >> $TEMP_FILE
(
    cd $1; cpp -D__MIPSEL__ -I./ -I$SETTOP_PATH $TEMP_FILE |
    grep "^%%" | sed -e "s,$SETTOP_ROOT,,g" -e 's,^%%,,g' |
    sed -e 's,sizeof(sizeof[^)]*,((0,g' -e 's,sizeof([^)]*,(0,g'
) > $IOCTL_H

# Generate the output file.
cat > $IOCTLSORT <<EOF
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
/* This file is automatically generated by brcm_ioctlent.sh, do not edit.
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
#include <stdio.h>
#include <stdlib.h>
#include <asm/ioctl.h>
#include <linux/types.h>

struct ioctlent {
	char*	header;
	char*	name;
	unsigned long	code;
};

struct ioctlent ioctls[] = {
EOF
cat $IOCTL_H >> $IOCTLSORT
cat >> $IOCTLSORT <<EOF
};

int nioctls = sizeof(ioctls) / sizeof(ioctls[0]);


int compare(const void* a, const void* b) {
	unsigned long code1 = ((struct ioctlent *) a)->code;
	unsigned long code2 = ((struct ioctlent *) b)->code;
	return (code1 > code2) ? 1 : (code1 < code2) ? -1 : 0;
}


int main(int argc, char** argv) {
	int i;

	for (i = 0; i < nioctls; i++) { 
		/* Only use the type and nr fields of the ioctl. */
		ioctls[i].code &= 0xffff;
	}
	qsort(ioctls, nioctls, sizeof(ioctls[0]), compare);
	for (i = 0; i < nioctls; i++)
		printf("\t{\"%s\",\t\"%s\",\t%#lx},\n",
			ioctls[i].header, ioctls[i].name, ioctls[i].code);
	
	return 0;
}
EOF

# Clean up.
rm -f $TEMP_FILE $IGNORE_IOCTL $TEMP_HEADER $IOCTL_H
