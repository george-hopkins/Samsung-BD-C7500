/* brcmnand_util.c -- Utility needed for BrcmNAND driver

   Copyright (C) 2007 Broadcom Corp.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA

*/
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <sys/ioctl.h>

#include <mtd/mtd-user.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

int brcmnand_get_oobavail(struct nand_oobinfo* oobinfo)
{
	int ebhlen = 0;
	int i;
	
	for (i = 0; oobinfo->oobfree[i][1] && i < 8; i++) {
		int from = oobinfo->oobfree[i][0];
		int num = oobinfo->oobfree[i][1];

		ebhlen += num;
	}
	return ebhlen;
}


/**
 * brcmnand_prepare_oobbuf - [GENERIC] Prepare the out of band buffer
 * @fsbuf:	buffer given by fs driver (input oob_buf)
 * @fslen:	size of buffer
 * @oobsel:	out of band selection structure
 * @oob_buf:	Output OOB buffer
 * @outp_ooblen:	Length of real OOB buffer.
 *
 * Return:
 *  returned len, i.e. number of bytes used up from fsbuf.
 *
 * Note: The internal buffer is filled with 0xff. This must
 * be done only once, when no autoplacement happens
 * Autoplacement sets the buffer dirty flag, which
 * forces the 0xff fill before using the buffer again.
 *
*/
int
brcmnand_prepare_oobbuf (
		const u_char *fsbuf, int fslen, u_char* oob_buf, struct nand_oobinfo *oobsel, 
		int oobsize, int oobavail, int* outp_ooblen)
{

	int i, len, ofs;
	int retlen = 0;

//printf("oobsize=%d, oobavail=%d\n", oobsize, oobavail);

	*outp_ooblen = 0;
	memset (oob_buf, 0xff, oobsize );
	
	ofs = 0;
	
	for (i = 0, len = 0; len < oobavail && len < fslen && i < ARRAY_SIZE(oobsel->oobfree); i++) {
		int to = ofs + oobsel->oobfree[i][0];
		int num = oobsel->oobfree[i][1];

		if (num == 0) break; // End marker reached

//printf("i=%d, to=%d, num=%d\n", i, to, num);

		if ((len + num) > fslen) {
			memcpy(&oob_buf[to], &fsbuf[retlen], fslen-len);
			retlen += fslen-len;
			*outp_ooblen = to + fslen - len;
			break;
		}

		memcpy (&oob_buf[to], &fsbuf[retlen], num);
		len += num;
		retlen += num;
		*outp_ooblen = to + num;
	}

//printf("<-- brcmnand_prepare_oobbuf\n");
//print_oobbuf(oob_buf, oobsize);
	return retlen;
}



