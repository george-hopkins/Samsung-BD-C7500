/*
 *  nandinfo.c
 *
 *  Copyright (C) 2008 Broadcom
 *
 * Overview:
 *   This utility show bad block for MTD type devices using a NAND
 */

#define _GNU_SOURCE
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <getopt.h>
#include <errno.h>
#include <stdbool.h>

#include <asm/errno.h>
#include <asm/types.h>

#include "mtd/nand_utils.h"
#include "mtd/mtd-user.h"

#define PROGRAM "nandinfo"
#define VERSION "$Revision: 1.00 $"


static char* STR_MTD_WRITEABLE  = "\nDevice is writeable";
static char* STR_MTD_BIT_WRITEABLE  = "\nSingle bits can be flipped (NOR)";

// Global
char  *mtd_device;
int    advanced_info = 0;

void display_help (void)
{
    printf("Usage: nandinfo -h for help\n"
           "Usage: nandinfo -v for version info.\n"
           "Usage: nandinfo -d for detailed informations\n"
           "Usage: nandinfo:\n"
           "Gives info on MTD devices using NAND.\n" );
    exit(0);
}

void display_version (void)
{
    printf(PROGRAM " " VERSION "\n"
           "\n"
           "Copyright (C) 2008 Broadcom Inc.\n"
           "\n"
           PROGRAM " comes with NO WARRANTY\n"
           "to the extent permitted by law.\n"
           "\n"
           "You may redistribute copies of " PROGRAM "\n"
           "under the terms of the GNU General Public Licence.\n"
           "See the file `COPYING' for more information.\n");
    exit(0);
}


static char* itoa(register int i)
{
	static char a[7]; /* Max 7 ints */
	register char *b = a + sizeof(a) - 1;
	int   sign = (i < 0);

	if (sign)
		i = -i;
	*b = 0;
	do
	{
		*--b = '0' + (i % 10);
		i /= 10;
	}
	while (i);
	if (sign)
		*--b = '-';
	return b;
}

void process_options (int argc, char *argv[])
{
    for (;;) {
        int c = getopt(argc, argv, "dhv?");
        if (c == EOF) {
            break;
        }

        switch (c) {
         case '?':
            display_help();
            break;
         case 'h':
            display_help();
            break;
         case 'v':
            display_version();
            break;
         case 'd':
            advanced_info = 1;
            break;
        }
    }
}

static char *meminfo_get_type(uint8_t type)
{
    if(type == MTD_ABSENT) return("No Chip Present");
    if(type == MTD_RAM) return("RAM");
    if(type == MTD_ROM) return("ROM");
    if(type == MTD_NORFLASH) return("NOR FLASH");
    if(type == MTD_NANDFLASH) return("NAND FLASH");
    if(type == MTD_DATAFLASH) return("DATA FLASH");
    else return ("Invalid Flash type");
}

static char *meminfo_get_flags(uint32_t type)
{

    if(type & MTD_WRITEABLE)
    { 
        return STR_MTD_WRITEABLE;
    }
    if(type & MTD_BIT_WRITEABLE)
    {
        return STR_MTD_BIT_WRITEABLE;
    }

}

#if 0
static char *meminfo_get_ecctype(uint32_t type)
{
    if(type == MTD_NANDECC_OFF) return("ECC Off");
    if(type == MTD_NANDECC_PLACE) return("YAFFS2 Legacy Mode Placement");
    if(type == MTD_NANDECC_AUTOPLACE) return("Default Placement Scheme");
    if(type == MTD_NANDECC_PLACEONLY) return("Use Given Placement Scheme");
    if(type == MTD_NANDECC_AUTOPL_USR) return("Use Given Autoplacement Scheme");
	else return("\0");
}
#endif

/*
 * Main program
 */
int main(int argc, char **argv)
{

    FILE  *fdmtd;

    int    fd;
    int    badblockfound = 0;
    int    ret;
    int    i;
    int    mtdNumberOfOccurence = 0;
    int    mtdoffset = 0;
//    int    blockalign = 1; 
    int    blockstart = -1;

    struct mtd_info_user meminfo;

    loff_t offs;

    char   str[128];    
//    char   mtdAsciiNumber[3];
    char  *mtdAsciiReturnedNumber;

    process_options(argc, argv);

    // See if we have NAND hardware!
    // First find out if a NAND chip is used by opening /proc/hwinfo
    if ((fdmtd = fopen("/proc/hwinfo", "r")) == NULL) 
    {
        perror("nandinfo: Unable to open /proc/hwinfo");
        exit(1);
    }

    // Read the file /proc/hwinfo
    // Read the file and look for the "NAND" string
    while(!feof(fdmtd))
    {
        if(fgets(str,126,fdmtd))
        {
            // printf("%s", str);
            if(strstr(str,"Flash Type:") != NULL)
            {
                if(strstr(str,"NAND") == NULL)
                {
                    printf("nandinfo: Flash Type is not NAND!");
                }
            }
        }
                        
    }

    // Second, find out how many partitions we have 
    // Try to open /proc/mtd
    if ((fdmtd = fopen("/proc/mtd", "r")) == NULL) 
    {
        perror("nandinfo: Unable to open /proc/mtd");
        exit(1);
    }

    // Read the file /proc/mtd
    while(!feof(fdmtd))
    {
        if(fgets(str,126,fdmtd))
        {
            // printf("%s", str);
            // Count the number of occurence of "mtd" on each line...
            if(strstr(str,"mtd") != NULL)
            {
                 mtdNumberOfOccurence++;
            }
        }
                        
    }

    printf("\nThere is %d MTD type partitions\n", mtdNumberOfOccurence);

        
    if(mtdNumberOfOccurence != 0)
    {

        // Now pass each MTD partitions one by one!
        for (i = 0; i < mtdNumberOfOccurence; i++)
        {

            // Initialize some counters
            mtdoffset = 0;
            blockstart = -1;  
            offs = 0; 
            badblockfound = 0; 

             /* Open the MTDx device */
            str[0] = 0;

            strcat(str, "/dev/mtd");

            mtdAsciiReturnedNumber = itoa(i);

            strcat(str, mtdAsciiReturnedNumber);        

            if ((fd = open(str, O_RDONLY)) == -1) 
            {
                printf("trying to open %s\n", str);
                perror("nandinfo: open flash");
                exit(1);
            }

            /* Fill in MTD device capability structure */
            if (ioctl(fd, MEMGETINFO, &meminfo) != 0) 
            {
                perror("nandinfo: MEMGETINFO");
                close(fd);
                exit(1);
            }

            // Let's print out what we know up to now
            if(advanced_info == 1)
            {
                printf("Detailed MEMINFO Structure Values for %s: Size: 0x%08X (%09d)\n", str, meminfo.size, meminfo.size);
                printf("-------------------------------------------------------------------------------\n");
                printf("type:       0x%08X: %s\n", meminfo.type, meminfo_get_type(meminfo.type));
                printf("flags:      0x%08X: %s\n", meminfo.flags, meminfo_get_flags(meminfo.flags));
                printf("Erase size: 0x%08X (%9d)\n", meminfo.erasesize, meminfo.erasesize);
                printf("Page Size:  0x%08X (%9d)\n", meminfo.writesize, meminfo.writesize);
                printf("oobsize:    0x%08X (%9d)\n", meminfo.oobsize, meminfo.oobsize);
//   ecc type & ecc size are now obsolete - see mtdchar.c:              
//                printf("ecc type:   0x%08X: %s\n", meminfo.ecctype, meminfo_get_ecctype(meminfo.ecctype));
//                printf("ecc size:   0x%08X (%9d)\n", meminfo.eccsize, meminfo.eccsize);
            }
            else
            {
                printf("Partition: %s Size: 0x%08X (%09d)\n", str, meminfo.size, meminfo.size);
            }

            //printf("Looking for Bad Block in %s:\n", str);

            /* Make sure device page sizes are valid */
            if (INVALID_PAGE_SIZE(meminfo.oobsize, meminfo.writesize)) 
            {
                  fprintf(stderr, "nandinfo: Unknown flash (not normal NAND)\n");
                  close(fd);
                  exit(1);
            }

            // Now find out those bad blocks!
            while (mtdoffset < meminfo.size)
            {

                while (blockstart != (mtdoffset & (~meminfo.erasesize + 1))) 
                {
                    blockstart = mtdoffset & (~meminfo.erasesize + 1);
                    offs = blockstart;

                    //printf("nandinfo address: %x end while condition: %x\n",(int) offs, blockstart + meminfo.erasesize);

                    if ((ret = ioctl(fd, MEMGETBADBLOCK, &offs)) < 0) 
                    {
                        perror("nandinfo: ioctl(MEMGETBADBLOCK)");
                        goto closeall;
                    }

                    if (ret == 1) 
                    {
                        badblockfound = 1;
                        printf("Bad block offset %x\n", (int) offs);
                    }


                }

                //Increment mtdOffset one block:
                mtdoffset = blockstart + meminfo.erasesize;        

            }

closeall:
            close(fd);

             if(badblockfound == 0) printf("No bad Block Found\n");
            //printf("nandinfo END\n");
        }
    }
    else
    {
        printf("No MTD Partitions found!!!\n");
    }
    /* Return happy */
    return 0;
}

