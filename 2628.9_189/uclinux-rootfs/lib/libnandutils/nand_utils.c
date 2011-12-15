
/* nand_utils.c -- erase and write to MTD device

    Renamed to nand_utils.c

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
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mount.h>

#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <libgen.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>

#include <mtd/mtd-user.h>
#include <mtd/jffs2-user.h>

#include <asm/types.h>
#include "mtd/mtd-user.h"

#include <errno.h>
#include <mtd/nand_utils.h>

#include <stdint.h>

static const uint32_t nandutils_crc32_table[256] = {
    0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
    0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
    0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
    0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
    0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
    0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
    0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
    0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
    0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
    0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
    0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
    0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
    0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
    0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
    0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
    0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
    0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
    0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
    0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
    0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
    0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
    0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
    0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
    0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
    0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
    0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
    0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
    0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
    0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
    0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
    0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
    0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
    0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
    0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
    0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
    0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
    0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
    0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
    0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
    0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
    0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
    0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
    0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
    0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
    0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
    0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
    0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
    0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
    0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
    0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
    0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
    0x2d02ef8dL
};


// default oob layout
static struct nand_oobinfo none_oobinfo = {
    .useecc = MTD_NANDECC_OFF,
};

// jffs2 oob layout
static struct nand_oobinfo jffs2_oobinfo = {
    .useecc = MTD_NANDECC_PLACE,
    .eccbytes = 6,
    .eccpos = { 0, 1, 2, 3, 6, 7 }
};

// yaffs oob layout
static struct nand_oobinfo yaffs_oobinfo = {
    .useecc = MTD_NANDECC_PLACE,
    .eccbytes = 6,
    .eccpos = { 8, 9, 10, 13, 14, 15}
};


static struct nand_oobinfo autoplace_oobinfo = {
    .useecc = MTD_NANDECC_AUTOPLACE
};

static struct nand_write_extra_param extra_param = {
    .bErase = 0
};
/* Return a 32-bit CRC of the contents of the buffer. */

static inline uint32_t
crc32(uint32_t val, const void *ss, int len)
{


    const unsigned char *s = ss;
        while (--len >= 0)
                val = nandutils_crc32_table[(val ^ *s++) & 0xff] ^ (val >> 8);
        return val;
}

static int fix_offset(int* p_nFileOffset, int* p_nPageCnt, int* p_nLengthToWrite, 
                      int* pMtdOffset, struct mtd_info_user* pMeminfo, 
                      int len, bool bWriteOob);

int nandutils_setExtraParms(struct nand_write_extra_param* p_ExtraParam)
{
    if (p_ExtraParam->bErase)
    {
        extra_param.bErase = true;
    }
    else
    {
        extra_param.bErase = false;
    }
}

/*
 * Function   : nandutils_nandwrite
 * Parameters :
 *              IN const char*  mtd_device: the nand device to be used
 *              IN const char*  img:  the file to write
 *              IN void *callback_context: pointer to whatever is needed 
 *              IN CallbackFunc  callback: the function to call for information update
 * 
 * Returns    : -1 on failure and  0 on success
 *
 * Function writes the passed source file to a destination partition
 */

int nandutils_nandwrite(NAND_WRITE_PARAM *param, void *callback_context, callbackFunc callback)
{


    size_t  size = 0;
    size_t  read_total = 0;
    int     mtdoffset = 0;



    int cnt, fd, ifd, pagelen, baderaseblock;
    struct mtd_info_user meminfo;
    struct mtd_oob_buf oob;
    loff_t offs;
    int ret, readlen;
    struct nand_oobinfo old_oobinfo;
    int err;
    
    int i, start, len, ii;
    loff_t mtdoffs;
    
    int nLenToWrite = 0, blockstart = -1, oobinfochanged = 0;
    int nFileOffset = 0, nPadLen = 0, nPageCnt = 0;

    unsigned char* writebuf;
    unsigned char* blockbuf;
    unsigned char* readbuf;
    
    unsigned char* oobbuf;
    unsigned char* oobreadbuf;
    
    mtdoffset = param->mtdoffset;
    
    int myi;
    
    if (param->pad && param->writeoob) 
    {
        fprintf(stderr, "nand_utils: Can't pad when oob data is present.\n");
        return(1);
    }

    /* Open the device */
    if ((fd = open(param->mtd_device, O_RDWR)) == -1) 
    {
        perror("nand_utils: open flash");
        return(1);
    }

    /* Fill in MTD device capability structure */
    if (ioctl(fd, MEMGETINFO, &meminfo) != 0) 
    {
        perror("nand_utils: MEMGETINFO");
        close(fd);
        return(1);
    }

    /* Set erasesize to specified number of blocks - to match jffs2 (virtual) block size */
    meminfo.erasesize *= param->blockalign;

    size += meminfo.erasesize - 1;
    size -= size % meminfo.erasesize;

    /* Make sure device page sizes are valid */
    if (INVALID_PAGE_SIZE(meminfo.oobsize, meminfo.writesize))
    {
        fprintf(stderr, "nand_utils: Unknown flash (not normal NAND)\n");
        close(fd);
        return(1);
    }

    /* Read the current oob info */
    if (ioctl (fd, MEMGETOOBSEL, &old_oobinfo) != 0) 
    {
        perror ("nand_utils: MEMGETOOBSEL");
        close (fd);
        return (1);
    }

    // write without ecc ?
    if (param->noecc) 
    {
        if (ioctl (fd, MEMSETOOBSEL, &none_oobinfo) != 0) 
        {
            perror ("nand_utils: MEMSETOOBSEL");
            close (fd);
            return (1);
        }
        oobinfochanged = 1;
    }

    // autoplace ECC ?
    if (param->autoplace && (old_oobinfo.useecc != MTD_NANDECC_AUTOPLACE)) 
    {

        if (ioctl (fd, MEMSETOOBSEL, &autoplace_oobinfo) != 0) 
        {
            perror ("nand_utils: MEMSETOOBSEL");
            close (fd);
            return (1);
        }
        oobinfochanged = 1;
    }

    /*
     * force oob layout for jffs2 or yaffs ?
     * Legacy support
     */
    if (param->forcejffs2 || param->forceyaffs) 
    {
        struct nand_oobinfo *oobsel = param->forcejffs2 ? &jffs2_oobinfo : &yaffs_oobinfo;

        if (param->autoplace) 
        {
            fprintf(stderr, "nand_utils: Autoplacement is not possible for legacy -j/-y options\n");
            goto restoreoob;
        }
        if ((old_oobinfo.useecc == MTD_NANDECC_AUTOPLACE) && !param->forcelegacy) 
        {
            fprintf(stderr, "nand_utils: Use -f option to enforce legacy placement on autoplacement enabled mtd device\n");
            goto restoreoob;
        }
        if (meminfo.oobsize == 8) 
        {
            if (param->forceyaffs) 
            {
                fprintf (stderr, "nand_utils: YAFSS cannot operate on 256 Byte page size");
                goto restoreoob;
            }
            /* Adjust number of ecc bytes */
            jffs2_oobinfo.eccbytes = 3;
        }

        if (ioctl (fd, MEMSETOOBSEL, oobsel) != 0) 
        {
            perror ("nand_utils: MEMSETOOBSEL");
            goto restoreoob;
        }
    }
    writebuf = malloc(meminfo.writesize);
    if (writebuf == NULL)
    {
       fprintf(stderr, "Cannot allocate writebuf\n");
       goto restoreoob;
    }

    blockbuf = malloc(meminfo.erasesize);
    if (blockbuf == NULL)
    {
       fprintf(stderr, "Cannot allocate blockbuf\n");
       goto restoreoob;
    }

    readbuf = malloc(meminfo.erasesize);
    if (readbuf == NULL)
    {
       fprintf(stderr, "Cannot allocate readbuf\n");
       goto restoreoob;
    }
                
    oobreadbuf = malloc(meminfo.oobsize);
    if (oobreadbuf == NULL)
    {
       fprintf(stderr, "Cannot allocate oobreadbuf\n");
       goto restoreoob;
    }
    
    oobbuf = malloc(meminfo.oobsize);
    if (oobbuf == NULL)
    {
       fprintf(stderr, "Cannot allocate oobbuf\n");
       goto restoreoob;
    }
    memset(oobbuf, 0xff, meminfo.oobsize);
    
    oob.start = 0;
    oob.length = meminfo.oobsize;
    oob.ptr = param->noecc ? oobreadbuf : oobbuf;

    /* Open the input file */
    if ((ifd = open(param->img, O_RDONLY)) == -1) 
    {
        perror("nand_utils: open input file");
        goto restoreoob;
    }

    // get image length
    nLenToWrite = lseek(ifd, 0, SEEK_END);

    printf("nand_utils: Image Size=%d\n", nLenToWrite);

    lseek (ifd, 0, SEEK_SET);

    pagelen = meminfo.writesize + ((param->writeoob == 1) ? meminfo.oobsize : 0);

    // Check, if file is pagealigned
    if ((!param->pad) && ((nLenToWrite % pagelen) != 0)) 
    {
        fprintf(stderr, "nand_utils : Input file is not page aligned\n");
        goto closeall;
    }

    // Check, if length fits into device
    if ( ((nLenToWrite / pagelen) * meminfo.writesize) > (meminfo.size - mtdoffset)) 
    {
        fprintf(stderr, "nand_utils: Image %d bytes, NAND page %d bytes, OOB area %u bytes, device size %u bytes\n",
                nLenToWrite, pagelen, meminfo.writesize, meminfo.size);
        fprintf(stderr, "nand_utils: Input file does not fit into device\n");
        goto closeall;
    }


    nPageCnt = 0;
    /* Get data from input and write to the device, per page */
   
    while (nLenToWrite && (mtdoffset < meminfo.size)) 
    {
        // new eraseblock , check for bad block(s)
        // Stay in the loop to be sure if the mtdoffset changes because
        // of a bad block, that the next block that will be written to
        // is also checked. Thus avoiding errors if the block(s) after the
        // skipped block(s) is also bad (number of blocks depending on
        // the blockalign
//        printf("nLenToWrite: %d, mtdoffset: %08X\n",nLenToWrite,mtdoffset);
        
        while (blockstart != (mtdoffset & ~(meminfo.erasesize - 1))) 
        {
            blockstart = mtdoffset & ~(meminfo.erasesize - 1);
            offs = blockstart;
            baderaseblock = 0;
            read_total += meminfo.erasesize;
            
            if (callback) 
            {
               callback(callback_context, meminfo.erasesize, false, 0, true, read_total, 0);
            }
            
            
            usleep(3000); // this line must be kept. Otherwise the UBD does not work properly!

            /* Check all the blocks in an erase block for bad blocks */
            do 
            {
                if ((ret = ioctl(fd, MEMGETBADBLOCK, &offs)) < 0) 
                {
                    perror("nand_utils: ioctl(MEMGETBADBLOCK)");
                    goto closeall;
                }
                if (ret == 1) 
                {
                    baderaseblock = 1;
                    if (!param->quiet)
                        fprintf(stderr, "nand_utils: Bad block at %x, %u block(s) from %x will be skipped\n", (int) offs, param->blockalign, blockstart);
                }

                if (baderaseblock) 
                {
                    mtdoffset = blockstart + meminfo.erasesize;
                }
                offs +=  meminfo.erasesize / param->blockalign ;
            } while ( offs < blockstart + meminfo.erasesize );

        }

        //This pads AT THE END of the last page:
        readlen = meminfo.writesize;
        if (param->pad && (nLenToWrite < readlen))
        {
            readlen = nLenToWrite;
            nPadLen = meminfo.writesize - readlen;
            memset(writebuf + readlen, 0xff, nPadLen);
        }

        //seek position in the file:  
        ret = lseek(ifd, nFileOffset, SEEK_SET);
        if (ret < 0)
        {
            perror ("nand_utils: Seek error on input file");
            goto closeall;
        }
        /* Read Page Data from input file */
        if ((cnt = read(ifd, writebuf, readlen)) != readlen) {

            if (cnt == 0) 
            {
                // EOF
                fprintf(stderr, "nand_utils: EOF reached ENDING copy!\n");
                fprintf(stderr, "nand_utils: Return code of lseek() is %d:\n", ret);
                fprintf(stderr, "nand_utils: We were trying to SEEK FILE POSITION nFileOffset = %d\n", nFileOffset); 
                break;
            }

            perror ("nand_utils: File I/O error on input file");
            goto closeall;
        }
        /*Copy data into block buffer:*/
//        fprintf(stderr, "Copy %d data into blockbuf (%p) (file offset %d)\n",cnt, blockbuf+nFileOffset, nFileOffset);
        memcpy((void*)(blockbuf+(nPageCnt*meminfo.writesize)), writebuf, cnt);
        
        //Increment nFileOffset for the next round (data):        
        nFileOffset += cnt;
       
        //Erase block only if we want to write the 1st page:
        if((extra_param.bErase)&&(nPageCnt == 0))
        {
            int val;
            int tmp_errno;
            erase_info_t erase;

            erase.start = (int)mtdoffset;
            erase.length = meminfo.erasesize;
            
            fflush(stdout);            
            
            errno = 0;
            
            val = ioctl(fd, MEMERASE, &erase);
            tmp_errno = errno;
            if (val != 0)
            {
                printf("nand_utils: Bad Block found while erasing."
                       "Skipping, val= %d, errno=%d ",val, tmp_errno);
                /*
                    If the ioctl for MEMERASE returns an error, it also marks  
                    the block as bad,so we don't need to mark it again here:
                    Just fix the offset and skip to the next block.
                */
                    
                //Fix the offsets:
                fix_offset(&nFileOffset, &nPageCnt, &nLenToWrite, &mtdoffset, &meminfo, readlen, param->writeoob);
               
                continue; //continue...in the while() loop
              
            }
            else
            {
                printf("MEMERASE success for block @ offset 0x%08X\n",(int)mtdoffset);
            }
        }
       
        if (param->writeoob) 
        {
            /* Read OOB data from input file, return on failure */
            if ((cnt = read(ifd, oobreadbuf, meminfo.oobsize)) != meminfo.oobsize) 
            {
                perror ("nand_utils: File I/O error on input file");
                goto closeall;
            }
            
            if (!param->noecc) 
            {
                 /*
                 *  We use autoplacement and have the oobinfo with the autoplacement
                 * information from the kernel available
                 *
                 * Modified to support out of order oobfree segments,
                 * such as the layout used by diskonchip.c
                 */
                if (!oobinfochanged && (old_oobinfo.useecc == MTD_NANDECC_AUTOPLACE)) 
                {
                    for (i = 0;old_oobinfo.oobfree[i][1]; i++) 
                    {
                        
                        /* Set the reserved bytes to 0xff */
                        start = old_oobinfo.oobfree[i][0];
                        len = old_oobinfo.oobfree[i][1];
                        
                        memcpy(oobbuf + start,
                            oobreadbuf + start,
                            len);
                        
                    }
                } 
                else 
                {
                    /* Set at least the ecc byte positions to 0xff */
                    start = old_oobinfo.eccbytes;
                    len = meminfo.oobsize - start;
                    memcpy(oobbuf + start,
                        oobreadbuf + start,
                        len);
                }
            }
            /* Write OOB data first, as ecc will be placed in there*/
            oob.start = mtdoffset;

            int err;
            err = ioctl(fd, MEMWRITEOOB, &oob);
            
            //Increment file offset for next round (oob):
            nFileOffset += cnt;
            nLenToWrite -= meminfo.oobsize;
            
            if(err != 0)
            {
                if (errno == EBADMSG)
                {   //ECC uncorrectable: Need to mark the block as bad!!!
                    fprintf(stderr, "nand_utils: ECC uncorrectable error detected into NAND flash.\n"
                                    "Marking the block as bad and skipping...\n ");                
            
                    mtdoffs = (loff_t)(mtdoffset & ~(meminfo.erasesize - 1));
        
                    if (ioctl(fd, MEMSETBADBLOCK, &mtdoffs) != 0) 
                    {
                        perror("nand_utils: MEMSETBADBLOCK");
                        goto closeall;
                    }
                }
                else if (errno == EBADBLK)
                {
                    fprintf(stderr, "nand_utils: Bad Block detected into NAND flash. Skipping.\n ");
                }
                else
                {
                    fprintf(stderr, "nand_utils: Invalid errno (%d) ", errno);
                    goto closeall;
                }

                //Fix the offsets:
                fix_offset(&nFileOffset, &nPageCnt, &nLenToWrite, &mtdoffset, &meminfo, readlen, param->writeoob);
                
                continue; //continue...in the while() loop
            }
        }

        /* Write out the Page data */
        errno = 0;
        
        err = pwrite(fd, writebuf, meminfo.writesize, mtdoffset);


        if (err != meminfo.writesize) 
        {
            if (errno == EBADMSG)
            {   //ECC uncorrectable: Need to mark the block as bad!!!
                fprintf(stderr, "nand_utils: ECC uncorrectable error detected into NAND flash.\n"
                                "Marking the block as bad and skipping...\n ");                
        
                mtdoffs = (loff_t)(mtdoffset & ~(meminfo.erasesize - 1));
                
                if (ioctl(fd, MEMSETBADBLOCK, &mtdoffs) != 0) 
                {
                    perror("nand_utils: MEMSETBADBLOCK");
                    goto closeall;
                }
            }
            else if (errno == EBADBLK)
            {
                fprintf(stderr, "nand_utils: Bad Block detected into NAND flash. Skipping.\n ");
            }
            else
            {
                fprintf(stderr, "nand_utils: Invalid errno (%d) ", errno);
                goto closeall;
            }

            //Fix the offsets:
            fix_offset(&nFileOffset, &nPageCnt, &nLenToWrite, &mtdoffset, &meminfo, readlen, param->writeoob);
                
            continue; //continue...in the while() loop
        }
        else
        {
            nPageCnt++;
            
            if (nPageCnt == (meminfo.erasesize / meminfo.writesize))
            {
                // Do write disturb verification only when writing with ecc:
                if (!param->noecc) 
                {
//                    printf("Performing write disturb verification\n");
                    //Reset errno:
                    errno = 0;
 
                    //Read the block back (for catching write disturb error)
                    err = pread(fd, readbuf, meminfo.erasesize, blockstart);
                    
                    if (memcmp(blockbuf, readbuf, meminfo.erasesize) != 0)
                    {   //ECC uncorrectable: Need to mark the block as bad!!!
                        if (errno == EBADMSG)
                        {
                            fprintf(stderr, "nand_utils: ECC uncorrectable "
                                            "error found while doing block "
                                            "write disturb verification, " 
                                            "block offset 0x%08X\n"
                                            "Marking the block as bad and skipping...\n",
                                            (int)blockstart);
                        }
                        else 
                        {
                            fprintf(stderr, "nand_utils: Data does not "
                                            "compare while doing block "
                                            "write disturb verification, "
                                            "block offset 0x%08X\n"
                                            "Marking the block as bad and skipping...\n",
                                            (int)blockstart);
                        }

                        mtdoffs = (loff_t)(mtdoffset & ~(meminfo.erasesize - 1));
                    
                        if (ioctl(fd, MEMSETBADBLOCK, &mtdoffs) != 0) 
                        {
                            perror("nand_utils: MEMSETBADBLOCK");
                            goto closeall;
                        }

                        //Fix the offsets:
                        fix_offset(&nFileOffset, &nPageCnt, &nLenToWrite, &mtdoffset, &meminfo, readlen, param->writeoob);
                    }
                    else
                    {
                        nPageCnt = 0;   //Marks the beginning of a new block!
                        //decrement nLenToWrite:
                        nLenToWrite -= readlen;
                        //Increment mtdoffset one page:
                        mtdoffset += meminfo.writesize;
                    }
                }
                else
                {
                    nPageCnt = 0;   //Marks the beginning of a new block!
                    //decrement nLenToWrite:
                    nLenToWrite -= readlen;
                    //Increment mtdoffset one page:
                    mtdoffset += meminfo.writesize;
                }            
            }
            else
            {   
                //decrement nLenToWrite:
                nLenToWrite -= readlen;
                //Increment mtdoffset one page:
                mtdoffset += meminfo.writesize;
            }
        }
       
    }

 closeall:
    close(ifd);

 restoreoob:
    if (blockbuf){
        free (blockbuf);
        writebuf = NULL;
    }
    
    if (readbuf){
        free (readbuf);
        writebuf = NULL;
    }
    
    if (writebuf){
        free (writebuf);
        writebuf = NULL;
    }
    if(oobbuf){
        free (oobbuf);
        oobbuf = NULL;
    }
    if(oobreadbuf){
        free (oobreadbuf);
        oobreadbuf = NULL;
    }
     
    if (oobinfochanged) 
    {
        if (ioctl (fd, MEMSETOOBSEL, &old_oobinfo) != 0) 
        {
            perror ("nand_utils: MEMSETOOBSEL");
            close (fd);
            return (1);
        }
    }

    close(fd);

    if (nLenToWrite > 0) 
    {
        fprintf(stderr, "nand_utils: Data did not fit into device\n");

        fprintf(stderr, "IMGLEN=%d (0x%08X) nFileOffset=%08X mtdoffset=%08x "
                        "meminfo.size=%08x\n", 
                        nLenToWrite, nLenToWrite, nFileOffset, mtdoffset, meminfo.size);
        return (1);
    }

    /* Return happy */
    return 0;

}

/*
 * Function   : nandutils_flash_erase
 * Parameters :
 *              IN const char*  mtd_device: the nand device to be used
 *              IN void *callback_context: pointer to whatever is needed 
 *              IN CallbackFunc  callback: the function to call for information update
 * 
 * Returns    : -1 on failure and  0 on success
 *
 * Function to erase partition
 */


int nandutils_flash_erase(NAND_ERASE_PARAM *param, void *callback_context, callbackFunc callback)
{
   
    mtd_info_t meminfo;
    int fd, clmpos = 0, clmlen = 8;
    erase_info_t erase;
    int isNAND, bbtest = 1;
    uint32_t pages_per_eraseblock, available_oob_space;

    static const char *exe_name;

    struct jffs2_unknown_node cleanmarker;
    int target_endian = __BYTE_ORDER;
    ulong stacksize = 8*1024;

    if ((fd = open(param->mtd_device, O_RDWR)) < 0) {
        fprintf(stderr, "%s: %s: %s\n", exe_name, param->mtd_device, strerror(errno));
        return(1);
    }


    if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
        fprintf(stderr, "%s: %s: unable to get MTD device info\n", exe_name, param->mtd_device);
        return(1);
    }

    erase.length = meminfo.erasesize;
    isNAND = (meminfo.type == MTD_NANDFLASH ? 1 : 0);

    printf("isNAND: %d\n", isNAND);

    if (param->jffs2) {
        cleanmarker.magic = cpu_to_je16 (JFFS2_MAGIC_BITMASK);
        cleanmarker.nodetype = cpu_to_je16 (JFFS2_NODETYPE_CLEANMARKER);
        if (!isNAND)
            cleanmarker.totlen = cpu_to_je32 (sizeof (struct jffs2_unknown_node));
        else {
            struct nand_oobinfo oobinfo;

            if (ioctl(fd, MEMGETOOBSEL, &oobinfo) != 0) {
                fprintf(stderr, "%s: %s: unable to get NAND oobinfo\n", exe_name, param->mtd_device);
                exit(1);
            }

            /* Check for autoplacement */
            if (oobinfo.useecc == MTD_NANDECC_AUTOPLACE) {
                /* Get the position of the free bytes */
                if (!oobinfo.oobfree[0][1]) {
                    fprintf (stderr, " Eeep. Autoplacement selected and no empty space in oob\n");
                    exit(1);
                }
                clmpos = oobinfo.oobfree[0][0];
                clmlen = oobinfo.oobfree[0][1];
                if (clmlen > 8)
                    clmlen = 8;
            } else {
                /* Legacy mode */
                switch (meminfo.oobsize) {
                    case 8:
                        clmpos = 6;
                        clmlen = 2;
                        break;
                    case 16:
                        clmpos = 8;
                        clmlen = 8;
                        break;
                    case 64:
                        clmpos = 16;
                        clmlen = 8;
                        break;
                }
            }
            cleanmarker.totlen = cpu_to_je32(8);
        }
        cleanmarker.hdr_crc =  cpu_to_je32 (crc32 (0, &cleanmarker,  sizeof (struct jffs2_unknown_node) - 4));
    }

//printf("*****  THT: About to erase, meminfo.erasesize=%d\n", meminfo.erasesize);

    for (erase.start = 0; erase.start < meminfo.size; erase.start += meminfo.erasesize) {
        if (bbtest) {
            loff_t offset = erase.start;
            int ret = ioctl(fd, MEMGETBADBLOCK, &offset);
//printf("***** THT: ioctl(fd, MEMGETBADBLOCK, offset=%08x) returns %d, sizeof(offset)=%d\n", erase.start, ret, sizeof(offset));
            if (ret > 0) {
                if (!param->quiet)
                    printf ("\nSkipping bad block at 0x%08x\n", erase.start);
                continue;
            } else if (ret < 0) {
                if (errno == EOPNOTSUPP) {
                    bbtest = 0;
                    if (isNAND) {
                        fprintf(stderr, "%s: %s: Bad block check not available\n", exe_name, param->mtd_device);
                        return(1);
                    }
                } else {
                    fprintf(stderr, "\n%s: %s: MTD get bad block failed: %s\n", exe_name, param->mtd_device, strerror(errno));
                    return(1);
                }
            }
        }

        if (callback) 
        {
            callback(callback_context, meminfo.erasesize, true, erase.start, false, 0, meminfo.size);
        }

        if (ioctl(fd, MEMERASE, &erase) != 0) {
            fprintf(stderr, "\n%s: %s: MTD Erase failure: %s\n", exe_name, param->mtd_device, strerror(errno));
            continue;
        }

        /* format for JFFS2 ? */
        if (!param->jffs2)
            continue;

        /* write cleanmarker */
                /* write cleanmarker */
        if (isNAND) {
            struct mtd_oob_buf oob;
            oob.ptr = (unsigned char *) &cleanmarker;
            oob.start = erase.start + clmpos;
            oob.length = clmlen;
            if (ioctl (fd, MEMWRITEOOB, &oob) != 0) {
                fprintf(stderr, "\n%s: %s: MTD writeoob failure: %s\n", exe_name, param->mtd_device, strerror(errno));
                continue;
            }
        } else {
            if (lseek (fd, erase.start, SEEK_SET) < 0) {
                fprintf(stderr, "\n%s: %s: MTD lseek failure: %s\n", exe_name, param->mtd_device, strerror(errno));
                continue;
            }
            if (write (fd , &cleanmarker, sizeof (cleanmarker)) != sizeof (cleanmarker)) {
                fprintf(stderr, "\n%s: %s: MTD write failure: %s\n", exe_name, param->mtd_device, strerror(errno));
                continue;
            }
        }
        if (!param->quiet)
            printf (" Cleanmarker written at %x.", erase.start);
    }
    if (!param->quiet)
        printf("\n");

    return 0;

}
/*
    Synopsis:
        fix_offset(int* p_nFileOffset, int* p_nPageCnt, int* p_nLengthToWrite, 
                      int* pMtdOffset, struct mtd_info_user* pMeminfo, 
                      int len, bool bWriteOob)
    Parameters:
        p_nFileOffset       : In/Out,   file offset to recalc
        p_nPageCnt          : Out,      page cnt to reset
        p_nLengthToWrite    : In/Out,   length to write to recalc
        pMtdOffset,         : In/Out,   mtd offset to recalc
        pMeminfo,           : In        mtd_info
        len,                : In        length read from input file
        bWriteOob           : In        Are we writing the oob as well as the data?

   Return:
        Success (0)  
*/
static int fix_offset(int* p_nFileOffset, int* p_nPageCnt, int* p_nLengthToWrite, 
                      int* pMtdOffset, struct mtd_info_user* pMeminfo, 
                      int len, bool bWriteOob)
{
    //Calculate the start of the current block:
    int blockStartPosition = *pMtdOffset & (~pMeminfo->erasesize + 1);

    loff_t mtdoffs = (loff_t)blockStartPosition;

    /*
        Fix for PR8370 - When finding new bad blocks/ECC errors, nand_utils
        now skips a block (131072 bytes) instead of a page (2048 bytes) 
    */
            
    fprintf(stderr, "Current File Offset=%08X\n", *p_nFileOffset);

    /* In the file, we have to back from the number of bytes that 
            were written in the NAND for the actual block!!! */

    //Decrement nFileOffset in order to get back to the 'actual' position (not the next):
    if (bWriteOob) 
    {
        *p_nFileOffset -= len; 
        *p_nFileOffset -= pMeminfo->oobsize;
    }
    else
    {
        *p_nFileOffset -= len;
    }

    //Decrement nFileOffset with the number of bytes that were written in the NAND:
    *p_nFileOffset -= (*pMtdOffset - blockStartPosition);

    if (bWriteOob) 
    {//Remove the number of oob areas corresponding the the current location:
        *p_nFileOffset -= (((*pMtdOffset - blockStartPosition)/pMeminfo->writesize) * pMeminfo->oobsize);
    }

//fprintf(stderr, "%s: Old *p_nLengthToWrite: %08X, ",__FUNCTION__, *p_nLengthToWrite);
    //Length to write has been decremented from the number of blocks written, so re-increment it by that same number:
    *p_nLengthToWrite += (*pMtdOffset - blockStartPosition);
//fprintf(stderr, "New *p_nLengthToWrite: %08X, ", *p_nLengthToWrite);
    
    fprintf(stderr, "Recalculated File Offset=%08X\n", *p_nFileOffset);

    //Make *pMtdOffset block-aligned:
    *pMtdOffset &= (~pMeminfo->erasesize + 1);

    //Move *pMtdOffset to the next block:
    *pMtdOffset += pMeminfo->erasesize;
    
    //Reset page count:
    *p_nPageCnt = 0;

    /*End of Fix for PR8370*/
    return 0;
 }
