/*
 *  bbtmod.c
 *
 *  Copyright (C) 2008 Broadcom
 *
 * Overview:
 *   This utility is to generate ecc error on NAND
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
#include <asm/errno.h>
#include <asm/types.h>
#include "mtd/mtd-user.h"


#define PROGRAM "geneccerror"
#define VERSION "$Revision: 1.21 $"



// Option variables

static uint32_t  distribute = 0;    //distribute errors accross data, bbi, ecc
static uint32_t  runAutomatedTest = 0;          //run automated test
static uint32_t  runSpecificLocationTest = 0;       //run specific location test
static uint32_t	 pattern  = 0x55aa55aa;
static uint32_t	 nbbits	  = 1;
static uint32_t	 offset   = 0;          // offset in the partition
static uint32_t  ecc_error = 0;         //Do not generate errors in ecc bytes
static uint8_t   bitmask = 0;            // bitmask to apply at offset
static uint8_t   oob_offset = 0;         // offset in the oob [0-15] where to apply the bitmask
static uint8_t   oob_bitmask = 0;        // bitmask to apply at oob_offset

static uint32_t  partialPageX   = 0;    // partial page start in the block 
static uint32_t  partialPageY   = 0;    // partial page end in the block 

static char		 *mtddev   = NULL;	// mtd device name

static int generate_ecc_error(void);
static void process_options (int argc, char *argv[]);

static char   str[128];

void display_help (void)
{
printf ("Usage: geneccerror [OPTIONS] MTD-device\n"
        "Generate ECC Error on single and multiple bits\n"
        "             --help                     Display this help and exit\n"
        "             --version                  Output version information and exit\n"
        "\n"
        "There are 3 ways to use this application:\n"
        "---------------------------------------------------------------------------------------\n"
        "1- Run automated test:\n"
        "    1)Save block\n"
        "    2)Write known pattern in location 0 of the block & verify\n"
        "    3)Flipbit(s) from 1 to 0 in pattern & write it w/ ecc disabled\n"
        "    4)Enable ecc, read & see if refresh or UNCORRECTABLE\n"
        "    5)Put back saved data\n"
        "\n"
        "    Available options for (1):\n"
        "        -t           --test                 Selects the Automated Test\n"
        "        -o off       --offset=off           Address within MTD-device where to write the data.\n"
        "        -b nb_bits   --bits=bits            Number of bits to put in error. Default is 1 bit\n"
        "        -c           --errecc               Generate error in ecc bytes instead of data bytes\n"
        "---------------------------------------------------------------------------------------\n"
        "2- Generate ecc errors in pseudo-random locations:\n"
        "    1) Save block\n"
        "    2) Modify block with bit(s) flipped from 1 to 0 & write it w/ ecc disabled\n"
        "    3) Re-enable ecc\n"
        "\n"
        "    Available options for (2):\n"
        "        -o off       --offset=off           Address within MTD-device where to write the data.\n"
        "        -b nb_bits   --bits=bits            Number of bits to put in error. Default is 1 bit\n"
        "        -s ppgx      --start_range=ppgx     Start of the Partial Page Range. Default is ppg0 only\n"
        "        -e ppgy      --end_range=ppgy       End of the Partial Page Range). If not set,\n"
        "                                              then ppgx is assumed.\n"
        "        -d           --distribute           Distribute the errors in data,\n"
        "                                             bad block indicator,\n"
        "                                             ecc bytes.  If not specified, then only the data\n"
        "                                             and/or the ecc (if -c is specified)\n"
        "                                             will contain the errors.\n"
        "        -c           --errecc               Generate error in ecc bytes (overriden by -d)\n"
        "---------------------------------------------------------------------------------------\n"
        "3- Generate ecc errors in known locations:\n"
        "    1) Save block\n"
        "    2) Modify block with bit(s) xored with mask(s) & write it w/ ecc disabled\n"
        "    3) Re-enable ecc\n"
        "\n" 
        "    Available options for (3):\n"
        "        -o off       --offset=off           Address within MTD-device where to write the data.\n"
        "        -m mask      --mask=mask            Bit mask to apply at offset <off>\n"
        "                                            (ex.:0x05 : bits 0 and 2 will be modified)\n"
        "        -f oob_off   --oob_offset=oob_off   oob_offset within the partial\n"
        "                                             page defined by <off>,\n"
        "                                             where to apply the ecc error.\n"
        "        -n oob_mask  --oob_mask=oob_mask    Bit mask to apply at oob offset <oob_off>\n"
        "                                            (ex.:0x05 : bits 0 and 2 will be modified)\n");

    exit(0);
}

void display_version (void)
{
    printf(PROGRAM " " VERSION "\n"
           "\n"
           "Copyright (C) 2008-2009 Broadcom Inc.\n"
           "\n"
           PROGRAM " comes with NO WARRANTY\n"
           "to the extent permitted by law.\n"
           "\n"
           "You may redistribute copies of " PROGRAM "\n"
           "under the terms of the GNU General Public Licence.\n"
           "See the file `COPYING' for more information.\n");
    exit(0);
}


static uint32_t hextoi(char *string)
{
  uint32_t number = 0;
  int index;

	// printf("The string upon entry is: %s\n", string);

  /*
   * if it does NOT look like a hex number, assume decimal and use atoi.
   */
  if (!(string[0] == '0'
	&& (string[1] = 'x'
	    || string[1] == 'X')))
    return(atoi(string));

  /*
   * otherwise, read a hex number.
   */
  for (index = 2; string[index] != '\0'; index++)
    {
      char blah = string[index];
      int digit;

      if (blah >= '0' && blah <= '9')
	digit = blah - '0';
      else if (blah >= 'A' && blah <= 'F')
	digit = blah - 'A' + 10;
      else if (blah >= 'a' && blah <= 'f')
	digit = blah - 'a' + 10;
      else
	break;

      number = number * 16 + digit;
    }

  // printf("The final decimal value is: %lu\n", number);

  return(number);
}

#if 0
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
#endif

void process_options (int argc, char *argv[])
{
	int error = 0;

	for (;;) {
		int option_index = 0;
        static const char short_options[] = "dcto:m:f:n:p:b:s:e:";
		static const struct option long_options[] = {
			{"help", no_argument, 0, 0},
			{"version", no_argument, 0, 0},
			{"distribute", no_argument, 0, 'd'},
			{"errecc", no_argument, 0, 'c'},
			{"test", no_argument, 0, 't'},
            {"offset", required_argument, 0, 'o'},
            {"mask", required_argument, 0, 'm'},
            {"oob_offset", required_argument, 0, 'f'},
            {"oob_mask", required_argument, 0, 'n'},
			{"pattern", required_argument, 0, 'p'},
			{"bits", required_argument, 0, 'b'},
			{"range_start", required_argument, 0, 's'},
			{"range_end", required_argument, 0,   'e'},
			{0, 0, 0, 0},
		};

		int c = getopt_long(argc, argv, short_options,
				    long_options, &option_index);
		if (c == EOF) {
			break;
		}

		switch (c) {
		case 0:
			switch (option_index) {
			case 0:
				display_help();
				break;
			case 1:
				display_version();
				break;
			}
			break;
		case 'd':
			distribute = 1;
			break;
		case 'c':
		    ecc_error = 1;
		    break;
		    
		case 't':
			runAutomatedTest = 1;
			break;
        
        case 'o':
			offset = hextoi(optarg);
			break;
        case 'm':
            bitmask = (uint8_t)hextoi(optarg);
            runSpecificLocationTest = 1;
            break;
        case 'f':
            oob_offset =  (uint8_t)hextoi(optarg);
            break;
        case 'n':
            oob_bitmask = (uint8_t)hextoi(optarg);
            runSpecificLocationTest = 1;
            break
            ;
		case 'p':
			pattern= hextoi(optarg);
			break;
		case 'b':
			nbbits = atol(optarg);
			break;
		
		case 's':
			partialPageX = hextoi(optarg);
			break;
		case 'e':
			partialPageY = hextoi(optarg);
			break;
			
		case '?':
			error = 1;
			break;
			
	    default:
            display_help();
            break;
		}
	}

	
	if ((argc - optind) != 1 || error)
		display_help ();

	mtddev = argv[optind];


//	printf("Start Address is %08x, mtddev is %s type is %s\n", offset,  mtddev, type);
}

/*
 * Main program
 */

int main(int argc, char **argv)
{

    FILE  *fdmtd = NULL;
    int    mtdNumberOfOccurence = 0;
	int returncode;

    process_options(argc, argv);


    // See if we have NAND hardware!
    // First find out if a NAND chip is used by opening /proc/hwinfo
    if ((fdmtd = fopen("/proc/hwinfo", "r")) == NULL) 
    {
        perror("bbtmod: Unable to open /proc/hwinfo");
        exit(1);
    }


    // Read the file /proc/hwinfo
    // Read the file and look for the "NAND" string
    while(!feof(fdmtd))
    {
        if(fgets(str,126,fdmtd))
        {
            if(strstr(str,"Flash Type:") != NULL)
            {
                if(strstr(str,"NAND") == NULL)
                {
                    printf("bbtmod: Flash Type is not NAND!");
                    fclose(fdmtd);
                    exit(1);
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
        perror("bbtmod: Unable to open /proc/mtd");
        exit(1);
    }

    // Read the file /proc/mtd
    while(!feof(fdmtd))
    {
        if(fgets(str,126,fdmtd))
        {
            // Count the number of occurence of "mtd" on each line...
            if(strstr(str,"mtd") != NULL)
            {
                 mtdNumberOfOccurence++;
            }
        }
                        
    }

    // printf("\nThere is %d MTD type partitions\n", mtdNumberOfOccurence);
	if(fdmtd != NULL)
			fclose(fdmtd);


    if(mtdNumberOfOccurence != 0)
    {

		returncode = generate_ecc_error();
		
		if(returncode == -2)
		{
			printf("geneccerror(): Bad parameters!\n");
		}
		else if(returncode == -1)
		{
			printf("geneccerror(): TEST FAILED!\n");
		}


    }
    else
    {
        printf("No MTD Partitions found!!!\n");
    }

    /* Return happy */
    return 0;
}



static int generate_ecc_error(void)
{

	int    fd = -1;
    int    ret;
	int	   i;

    struct mtd_info_user meminfo;
	struct nand_generate_ecc_error_command command;

	uint32_t numberOfFailedTests = 0;
    uint32_t numberOfUnknownResults = 0;
	uint32_t numberOfSuccessfulTests = 0;

    uint32_t tmp = 0;
    uint32_t maxPartialPage = 0;

    /* Open the MTDx device */
    str[0] = 0;

    strcat(str, mtddev);

    if ((fd = open(str, O_RDONLY)) == -1) 
    {
        printf("trying to open %s\n", str);
        perror("bbtmod: open flash");
        exit(1);
    }


    /* Fill in MTD device capability structure */
    if (ioctl(fd, MEMGETINFO, &meminfo) != 0) 
    {
        perror("bbtmod: MEMGETINFO");
        close(fd);
        exit(1);
    }

    // Let's print out what we know up to now
    printf("Partition: %s Size: 0x%08X (%09d)\n", str, meminfo.size, meminfo.size);
            
    usleep (500000);

    /* Make sure device page sizes are valid */
    if (!(meminfo.oobsize == 16 && meminfo.writesize == 512) &&
        !(meminfo.oobsize == 8 && meminfo.writesize == 256) &&
        !(meminfo.oobsize == 64 && meminfo.writesize == 2048) &&
        !(meminfo.oobsize == 128 && meminfo.writesize == 4096)) 
    {
        fprintf(stderr, "bbtmod: Unknown flash (not normal NAND)\n");
        close(fd);
        exit(1);
    }
    
    if (runSpecificLocationTest)
    {
    //Fill the command structure:
        //Not used parameters:
        command.runAutomatedTest = 0;
        command.pattern = 0;
        command.bits = 0;
        command.partialPageX = 0;
        command.partialPageY = 0;
        command.distribute = 0;
        command.ecc_error = 0;
        //used parameters:
        command.runSpecificLocationTest = 1;
        command.offset = offset;
        command.bitmask = bitmask;

        if (oob_offset >= 16)
        {
            printf("Warning: oob_offset >= 16.  Setting it to 15\n");
            oob_offset = 15;
        }
    	command.oob_offset = oob_offset;
        command.oob_bitmask = oob_bitmask;
    }
    else
    {
        command.runSpecificLocationTest = 0;
        command.bitmask = 0;
        
        command.oob_offset = 0;
        command.oob_bitmask = 0;
        //Make sure that the offset is block-aligned:
        if ((offset & (meminfo.erasesize - 1)) != 0)
        {
            offset = offset - (offset & (meminfo.erasesize - 1));
            printf("Adjusted offset to the base address of a block: 0x%08X\n", offset);
        }
        //Make sure that partialPageX is >= partialPageY    
        if (partialPageX > partialPageY)
        {
            printf("Inverting partialPageX & partialPageY because partialPageX is greater than partialPageY\n");
            tmp =   partialPageY;
            partialPageY = partialPageX;
            partialPageX = tmp;
        }

        //Make sure that partialPageX & partialPageY are within the block:
        printf("meminfo.eccsize: %d\n", meminfo.eccsize);
        maxPartialPage = (meminfo.erasesize / 512) - 1;

        if (partialPageX > maxPartialPage)
        {
            printf("partialPageX is too big: Setting it to %d\n", maxPartialPage);
            partialPageX = maxPartialPage;
        }                  

        if (partialPageY > maxPartialPage)
        {
            printf("partialPageY is too big: Setting it to %d\n", maxPartialPage);
            partialPageY = maxPartialPage;
        }                  

    //Fill the command structure:
        if (runAutomatedTest)
        {    
            command.runAutomatedTest = runAutomatedTest;
            command.pattern = pattern;
            command.ecc_error = ecc_error;
            command.offset = offset;
        	command.bits = nbbits;

            command.partialPageX = 0;
            command.partialPageY = 0;
            command.distribute = 0;
        }
        else
        {
            command.runAutomatedTest = 0;
            command.pattern = 0;

            command.ecc_error = ecc_error;
            command.offset = offset;
        	command.bits = nbbits;
        	
            command.partialPageX = partialPageX;
            command.partialPageY = partialPageY;
            command.distribute = distribute;
        }
    }
	if ((ret = ioctl(fd, MTD_BRCMNAND_GENECCERROR, &command)) < 0) 
    {
        printf("Return Code: %d\n", (int) ret);
                perror("geneccerror: ioctl(MTD_BRCMNAND_GENECCERROR)");
        goto closeall;
    }
    else
    {
        if (command.total_number_of_tests > 0)
        {
            for(i=0;i<command.total_number_of_tests;i++)
            {
                if(command.test_result[i] == GENECCERROR_SUCCESS)
                    numberOfSuccessfulTests++;
                else if(command.test_result[i] == GENECCERROR_FAILED)
	   		    numberOfFailedTests++;
                else if(command.test_result[i] == GENECCERROR_UNKNOWN)
                    numberOfUnknownResults++;
            }
            printf("\nTotal Number Of Tests: %d\nNumber of SUCCESSFUL Tests: %d\nNumber of FAILED Tests: %d\nNumber of UNKNOWN Results: %d\n",
                    command.total_number_of_tests, numberOfSuccessfulTests, numberOfFailedTests, numberOfUnknownResults);  
        }
    }
    
closeall:
    if(fd != -1)
        close(fd);
        
    return 0;
}



