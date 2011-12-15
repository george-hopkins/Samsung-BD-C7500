/*
 *  nandstat.c
 *
 *  Copyright (C) 2008 Broadcom
 *
 * Overview:
 *   This utility show the NAND Statistics
 */

//#define _GNU_SOURCE


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
#include <asm/errno.h>
#include <asm/types.h>
#include "mtd/mtd-user.h"

#define PROGRAM "nandstat"
#define VERSION "$Revision: 1.00 $"

// Global
char  *mtd_device;
char  *filename = NULL;	// dump file name


void display_help (void)
{
    printf("Usage: nandstst -h for help\n"
           "Usage: nandstst -v for version info.\n"
		   "Usage: nandstst -f <filename> to save statistics in a file.\n"
           "Usage: nandstst:\n"
           "Gives NAND Statistics.\n" );
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
		int option_index = 0;
		static const char *short_options = "dhvf:";
		static const struct option long_options[] = {
			{"help", no_argument, 0, 'h'},
			{"version", no_argument, 0, 'v'},
			{"display", no_argument, 0, 'd'},
			{"file", required_argument, 0, 'f'},
			{0, 0, 0, 0},
		};

		int c = getopt_long(argc, argv, short_options,
				    long_options, &option_index);
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
 		case 'f':
			if (!(filename = strdup(optarg))) {
				perror("stddup");
				exit(1);
			}
			break;
		}
	}

}


/*
 * Main program
 */
int main(int argc, char **argv)
{

    FILE  *fdmtd = NULL;

    int    fd = -1;
    int    mtdNumberOfOccurence = 0;
	char   tmpstr[1024];
	char   buffer[2048];

    struct nand_statistics nandstats;

    char   str[128];    

	FILE  *fdump = NULL;

    process_options(argc, argv);

	if(filename != NULL) 
	{
		printf("filename is %s\n", filename);
		if((fdump = fopen(filename, "a")) == NULL)
		{
			perror("nandstat: Unable to open file");
			exit(1);
		}

	}


    // See if we have NAND hardware!
    // First find out if a NAND chip is used by opening /proc/hwinfo
    if ((fdmtd = fopen("/proc/hwinfo", "r")) == NULL) 
    {
        perror("nandstat: Unable to open /proc/hwinfo");
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
                    printf("nandstat: Flash Type is not NAND!");
                }
            }
        }
                        
    }

	fclose(fdmtd);

	fdmtd = NULL;

    // Second, find out how many partitions we have 
    // Try to open /proc/mtd
    if ((fdmtd = fopen("/proc/mtd", "r")) == NULL) 
    {
        perror("nandstat: Unable to open /proc/mtd");
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

              /* Open the MTD0 device. We open the first available device by
			default. It is not really important */

            str[0] = 0;

            strcat(str, "/dev/mtd0");

            if ((fd = open(str, O_RDONLY)) == -1) 
            {
                printf("trying to open %s\n", str);
                perror("nandstat: open flash");
                exit(1);
            }

            /* Fill in MTD device capability structure */
            if (ioctl(fd, MTD_BRCMNAND_NANDSTAT, &nandstats) != 0) 
            {
                perror("nandstat: MTD_BRCMNAND_NANDSTAT");
                close(fd);
                exit(1);
            }


			// Let's see the stats!
			strcpy(buffer, "NandStat results:\n");

			sprintf(tmpstr, "%s\n", nandstats.nandDescription); 
			strcat(buffer, tmpstr);

			sprintf(tmpstr, "Bad Block due to wear:   %6d Bad Block due to wear upon reset:   %6d\n", nandstats.badBlocksDueToWearNow, nandstats.badBlocksDueToWearBefore);
			strcat(buffer, tmpstr);

			sprintf(tmpstr, "Factory Marked Bad Block:%6d Factory Marked Bad Block upon reset:%6d\n", nandstats.factoryMarkedBadBlocksNow, nandstats.factoryMarkedBadBlocksBefore);
			strcat(buffer, tmpstr);

			sprintf(tmpstr, "Reserved Block:          %6d Reserved Block upon reset:          %6d\n", nandstats.reservedBlocksNow, nandstats.reservedBlocksBefore);
			strcat(buffer, tmpstr);

			sprintf(tmpstr, "Good Block:              %6d Good Block upon reset:              %6d\n", nandstats.goodBlocksNow, nandstats.goodBlocksBefore);
			strcat(buffer, tmpstr);

			sprintf(tmpstr, "Total Number of blocks:  %6d\n", nandstats.goodBlocksNow + nandstats.badBlocksDueToWearNow + nandstats.factoryMarkedBadBlocksNow + nandstats.reservedBlocksNow);
			strcat(buffer, tmpstr);

			sprintf(tmpstr, "Ecc Correctable Error since reset:   %6d\n", nandstats.eccCorrectableErrorsNow);
			strcat(buffer, tmpstr);

			sprintf(tmpstr, "Ecc UNCorrectable Error since reset: %6d\n", nandstats.eccUncorrectableErrorsNow);
			strcat(buffer, tmpstr);

			sprintf(tmpstr, "Ecc Bytes in Error since reset:      %6d\n", nandstats.eccBytesErrors);
			strcat(buffer, tmpstr);

			sprintf(tmpstr, "Block Refreshed since reset:         %6d\n", nandstats.blockRefreshed);
			strcat(buffer, tmpstr);


			if(filename == NULL)
			{
				printf("%s", buffer);

			}
			else
			{
				fprintf(fdump,"%s", buffer);
			}




    }
    else
    {

			printf("No MTD Partitions found!!!\n");
    }
	
	if(fd != -1)
		close(fd);

	if(fdmtd != NULL)
		fclose(fdmtd);

    if(fdump != NULL)
		fclose(fdump);

    /* Return happy */
    return 0;
}

