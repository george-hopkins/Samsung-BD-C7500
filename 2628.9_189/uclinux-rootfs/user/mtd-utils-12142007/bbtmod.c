/*
 *  bbtmod.c
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
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <stdbool.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <asm/types.h>
#include <mtd/mtd-user.h>


#include <mtd/nand_utils.h>


#define PROGRAM "bbtmod"
#define VERSION "$Revision: 1.10 $"


// Option variables

static unsigned long    offset          = 0;       // offset in the partition
static unsigned long    absolute_offset = 0;       // absolute offset from beginning of nand
static char             *mtddev         = NULL;    // mtd device name
static char             *filename       = NULL;    // dump file name
static char             *type           = NULL;    // type of block wanted g,w,r or f
static unsigned int     display         = 0;
static unsigned int     erase           = 0;
static unsigned int     randomtest      = 0;
static unsigned int     randomtest_erasedblocksonly      = 0;
static unsigned int     scan            = 0;

//value calculated
static unsigned long long random_blockstart = 0;

static int bbt_delete(void);
static int bbt_scan(void);
static int bbt_show(int mtdNumberOfOccurence);
static int show_bbt_for_a_partition(void);
static int write_bbt_and_block(void);
static void display_help (void);
static int bbt_randomblock_test(void);
static int GetRandomValue(int randfd, unsigned char* dst, int len);

static unsigned long hextoi(char *string)
{
  unsigned long number = 0;
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


static void display_help (void)
{
    printf("Usage: bbtmod [OPTIONS] MTD-device\n"
           "Modify the Bad Block Table and the block related to it\n"
           "\n"
           "           --help            display this help and exit\n"
           "           --version         output version information and exit\n"
           "-d         --display       display Bad Block Table\n"
           "-f file    --file=file     dump to file\n"
           "-t         --type          G for good block\n"
           "                           W due to wear\n"
           "                           R for reserved block\n"
           "                           F for factory marked block\n"
           "-o off     --offset=off    offset in the partition were to modify block\n"
           "-a off     --absoffset=off absolute offset from the beginning of nand\n"
           "-s         --scan          Scans the chip & copies Bad Block Table in the bbt partition\n"
           "-e         --erase         erase Bad Block Table! YOU MUST RUN 'bbtmod -s' NOW!\n"
           "-r         --random        Runs a random bad block mark into MTD-device\n"
           "-m         --random_erased Runs a random bad block mark for erased blocks only into MTD-device\n");
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


unsigned int getBBTCommandType(char *type)
{
    if(        type[0] == 'G' || type[0] == 'g') return 0;  //good
    else if(type[0] == 'W' || type[0] == 'w') return 1;     //wear
    else if(type[0] == 'R' || type[0] == 'r') return 2;     //reserved
    else if(type[0] == 'F' || type[0] == 'f') return 3;     //Factory
    else return 0;
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
    int error = 0;

    for (;;) {
        int option_index = 0;
        static const char *short_options = "dermso:a:f:t:";
        static const struct option long_options[] = {
            {"help",        no_argument,        0, 0},
            {"version",     no_argument,        0, 0},
            {"display",     no_argument,        0, 'd'},
            {"erase",       no_argument,        0, 'e'},
            {"random",      no_argument,        0, 'r'},
            {"random_erased", no_argument,      0, 'm'},
            {"scan",        no_argument,        0, 's'},
            {"offset",      required_argument,  0, 'o'},
            {"absoffset",   required_argument,  0, 'a'},
            {"file",        required_argument,  0, 'f'},
            {"type",        required_argument,  0, 't'},
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
            display = 1;
            break;
        case 'e':
            erase = 1;
            break;
        case 's':
            scan = 1;
            break;
        case 'r':
            randomtest = 1;
            break;
        case 'm':
            randomtest = 1;
            randomtest_erasedblocksonly = 1;
            break;
        case 'o':
            offset= hextoi(optarg);
            break;
        case 'a':
            absolute_offset = hextoi(optarg);
            break;
        case 'f':
            if (!(filename = strdup(optarg))) {
                perror("stddup");
                exit(1);
            }
            break;
        case 't':
            if (!(type = strdup(optarg))) {
                perror("stddup");
                exit(1);
            }
            break;
        case '?':
            error = 1;
            break;
        }
    }

    
    if (((argc - optind) != 1 || error) && display != 1 && erase != 1 && randomtest != 1 && scan != 1)
        display_help ();

    mtddev = argv[optind];


    // printf("Start Address is %08X, file is %s mtddev is %s type is %s\n", offset, filename, mtddev, type);
}

/*
 * Main program
 */
int main(int argc, char **argv)
{

    FILE  *fdmtd;
    char   str[128];
    int    mtdNumberOfOccurence = 0;

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
            // printf("%s", str);
            if(strstr(str,"Flash Type:") != NULL)
            {
                if(strstr(str,"NAND") == NULL)
                {
                    printf("bbtmod: Flash Type is not NAND!");
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
            // printf("%s", str);
            // Count the number of occurence of "mtd" on each line...
            if(strstr(str,"mtd") != NULL)
            {
                 mtdNumberOfOccurence++;
            }
        }
                        
    }

    printf("\nThere is %d MTD type partitions\n", mtdNumberOfOccurence);

    if(erase == 1 && mtddev != NULL)
    {
        bbt_delete();
    }
    else if (scan == 1 && mtddev != NULL)
    {
        bbt_scan();
    }
    else if(mtdNumberOfOccurence != 0)
    {
        if (randomtest == 1 && mtddev != NULL)
            bbt_randomblock_test();
     
        else if(display == 1 && mtddev == NULL) // Let's get the bad block table
            bbt_show(mtdNumberOfOccurence);

        else if(type == NULL && mtddev != NULL) // Show the bad block just for a partition
            show_bbt_for_a_partition();

        else if(type != NULL && mtddev != NULL) // Let's modify the bad block table and the block in question
            write_bbt_and_block();

        else
            display_help();
    }
    else
    {
        printf("No MTD Partitions found!!!\n");
    }
    /* Return happy */

    fclose(fdmtd);

    return 0;
}


static int bbt_delete(void)
{
    int    fd;
    int    ret;
    char   str[128];
    loff_t offs = 0;
    struct mtd_info_user meminfo;

    printf("Erasing BBT...\n");

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

#if 1
    if ((ret = ioctl(fd, MTD_BRCMNAND_DELETEBBT, &offs)) < 0) 
    {
        printf("BBT Deleted\n");
    }
 
#endif
    close(fd);

    return 0;

}

static int bbt_show(int mtdNumberOfOccurence)
{
    int    fd = -1;;
    int    badblockfound = 0;
    int    ret;
    int    i, ofd = -1;
         
    int    mtdoffset = 0;
//    int    blockalign = 1; 
    int    blockstart = -1;

    char   str[128];
    char   file_buf[60];

    struct mtd_info_user meminfo;

    loff_t offs;
    
//    char   mtdAsciiNumber[3];
    char  *mtdAsciiReturnedNumber;


        if ((filename != NULL) && ((ofd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0644)) == -1))
        {
            perror ("open outfile");
            exit(1);
        }

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

             //printf("Looking for Bad Block in %s:\n", str);

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


            // Let's print out what we know up to now
            if(filename == NULL)
                printf("Partition: %s Size: 0x%08X (%09d)\n", str, meminfo.size, meminfo.size);
            else
            {
                sprintf(file_buf,"Partition: %s Size: 0x%08X (%09d)\n", str, meminfo.size, meminfo.size);
                write(ofd, file_buf, strlen(file_buf));
            }

            // Now find out those bad blocks!
            while (mtdoffset < meminfo.size)
            {

                while (blockstart != (mtdoffset & (~meminfo.erasesize + 1))) 
                {
                    blockstart = mtdoffset & (~meminfo.erasesize + 1);
                    offs = blockstart;

                    //printf("bbtmod address: %x end while condition: %x\n",(int) offs, blockstart + meminfo.erasesize);

                    if ((ret = ioctl(fd, MTD_BRCMNAND_READBBTCODE, &offs)) < 0) 
                    {
                        printf("Return Code: %d\n", (int) ret);
                        perror("bbtmod: ioctl(MTD_BRCMNAND_READBBTCODE)");
                        goto closeall;
                    }


                    if (ret == 1) 
                    {
                        badblockfound = 1;

                        if(filename == NULL)
                            printf("Bad block DUE TO WEAR at 0x%08X\n", (int) offs);
                        else
                        {
                            sprintf(file_buf,"Bad block DUE TO WEAR at 0x%08X\n", (int) offs);
                            write(ofd, file_buf, strlen(file_buf));
                        }
                    }
                    else if (ret == 2)
                    {
                        badblockfound = 1;
                        
                        if(filename == NULL)
                            printf("Bad Block RESERVED at 0x%08X\n", (int) offs);
                        else
                        {
                            sprintf(file_buf,"Bad Block RESERVED at 0x%08X\n", (int) offs);
                            write(ofd, file_buf, strlen(file_buf));
                        }
                    }
                    else if(ret == 3)
                    {
                        badblockfound = 1;
                        
                        if(filename == NULL)
                            printf("Bad block FACTORY MARKED at 0x%08X\n", (int) offs);
                        else
                        {
                            sprintf(file_buf,"Bad block FACTORY MARKED at 0x%08X\n", (int) offs);
                            write(ofd, file_buf, strlen(file_buf));
                        }


                    }
                    else if(ret == 4)
                    {
                        badblockfound = 1;
                        
                        if(filename == NULL)
                            printf("Bad block of UNKNOWN TYPE at 0x%08X\n", (int) offs);
                        else
                        {
                            sprintf(file_buf,"Bad block of UNKNOWN TYPE at 0x%08X\n", (int) offs);
                            write(ofd, file_buf, strlen(file_buf));
                        }

                    }


                }

                //Increment mtdOffset one block:
                mtdoffset = blockstart + meminfo.erasesize;        

            }

           if(badblockfound == 0)
           {
                if(filename == NULL)
                    printf("No bad Block Found\n");
                else
                {
                    sprintf(file_buf,"No bad Block Found\n");
                    write(ofd, file_buf, strlen(file_buf));
                }
           }


        }

closeall:
        if(fd != -1)
           close(fd);

        if(ofd != -1)
            close(ofd);

        return 0;
}


static int write_bbt_and_block(void)
{

    int    fd = -1;
    int    ret;

    char   str[128];

    struct mtd_info_user meminfo;

  //  loff_t offs;

    struct nand_bbt_command command;
    
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

            //printf("Looking for Bad Block in %s:\n", str);

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
            command.offset = offset;
            command.code = getBBTCommandType(type);
            command.absolute_offset = absolute_offset;

            //printf("\ncommand.offset is %d command.code is %d\n", (unsigned int) command.offset, (unsigned int) command.code);

            if ((ret = ioctl(fd, MTD_BRCMNAND_WRITEBBTCODE, &command)) < 0) 
            {
                printf("Return Code: %d\n", (int) ret);
                        perror("bbtmod: ioctl(MTD_BRCMNAND_WRITEBBTCODE)");
                        goto closeall;
            }

 

closeall:
            if(fd != -1)
                close(fd);

            return 0;

}

static int show_bbt_for_a_partition(void)
{

    int    fd = -1;
    int    badblockfound = 0;
    int    ret;
    int    ofd = -1;

    int    mtdoffset = 0;
//    int    blockalign = 1; 
    int    blockstart = -1;

    char   str[128];
    char   file_buf[60];

    struct mtd_info_user meminfo;

    loff_t offs;
    

            // Initialize some counters
            mtdoffset = 0;
            blockstart = -1;  
            offs = 0; 
            badblockfound = 0; 

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

            //printf("Looking for Bad Block in %s:\n", str);

            /* Make sure device page sizes are valid */
            if (!(meminfo.oobsize == 16 && meminfo.writesize == 512) &&
                !(meminfo.oobsize == 8 && meminfo.writesize == 256) &&
                !(meminfo.oobsize == 64 && meminfo.writesize == 2048)&&
                !(meminfo.oobsize == 128 && meminfo.writesize == 4096)) 
            {
                  fprintf(stderr, "bbtmod: Unknown flash (not normal NAND)\n");
                  close(fd);
                  exit(1);
            }

            if ((filename != NULL) && ((ofd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0644)) == -1))
            {
                perror ("open outfile");
                close(fd);
                exit(1);
            }

            // Let's print out what we know up to now
            if(filename == NULL)
                printf("Partition: %s Size: 0x%08X (%09d)\n", str, meminfo.size, meminfo.size);
            else
            {
                sprintf(file_buf,"Partition: %s Size: 0x%08X (%09d)\n", str, meminfo.size, meminfo.size);
                write(ofd, file_buf, strlen(file_buf));
            }

            // Now find out those bad blocks!
            while (mtdoffset < meminfo.size)
            {

                while (blockstart != (mtdoffset & (~meminfo.erasesize + 1))) 
                {
                    blockstart = mtdoffset & (~meminfo.erasesize + 1);
                    offs = blockstart;

                    //printf("bbtmod address: %x end while condition: %x\n",(int) offs, blockstart + meminfo.erasesize);

                    if ((ret = ioctl(fd, MTD_BRCMNAND_READBBTCODE, &offs)) < 0) 
                    {
                        printf("Return Code: %d\n", (int) ret);
                        perror("bbtmod: ioctl(MTD_BRCMNAND_READBBTCODE)");
                        goto closeall;
                    }


                    if (ret == 1) 
                    {
                        badblockfound = 1;
                        if(filename == NULL)
                            printf("Bad block DUE TO WEAR at %08X\n", (int) offs);
                        else
                        {
                            sprintf(file_buf,"Bad block DUE TO WEAR at 0%08X\n", (int) offs);
                            write(ofd, file_buf, strlen(file_buf));
                        }
                    }
                    else if (ret == 2)
                    {
                        badblockfound = 1;
                        
                        if(filename == NULL)
                            printf("Bad Block RESERVED at %08X\n", (int) offs);
                        else
                        {
                            sprintf(file_buf,"Bad Block RESERVED at 0%08X\n", (int) offs);
                            write(ofd, file_buf, strlen(file_buf));
                        }
                    }
                    else if(ret == 3)
                    {
                        badblockfound = 1;
                        
                        if(filename == NULL)
                            printf("Bad block FACTORY MARKED at %08X\n", (int) offs);
                        else
                        {
                            sprintf(file_buf,"Bad block FACTORY MARKED at 0%08X\n", (int) offs);
                            write(ofd, file_buf, strlen(file_buf));
                        }


                    }
                    else if(ret == 4)
                    {
                        badblockfound = 1;
                        
                        if(filename == NULL)
                            printf("Bad block of UNKNOWN TYPE at %08X\n", (int) offs);
                        else
                        {
                            sprintf(file_buf,"Bad block of UNKNOWN TYPE at 0%08X\n", (int) offs);
                            write(ofd, file_buf, strlen(file_buf));
                        }

                    }


                }

                //Increment mtdOffset one block:
                mtdoffset = blockstart + meminfo.erasesize;        

            }

           if(badblockfound == 0)
           {
                if(filename == NULL)
                    printf("No bad Block Found\n");
                else
                {
                    sprintf(file_buf,"No bad Block Found\n");
                    write(ofd, file_buf, strlen(file_buf));
                }
           }

closeall:
        if(fd != -1)
           close(fd);

        if(ofd != -1)
            close(ofd);

        return 0;
}



static int bbt_randomblock_test(void)
{
    unsigned int random,rm;
    char   str[128];
    struct mtd_oob_buf oob;
    struct mtd_info_user meminfo;
    struct nand_bbt_command command;
    int i,j;
    bool bFF = true;
    bool bReady = true;
    int fd = -1; 
    int randfd = -1;
    unsigned char* u8 = (unsigned char*)&random;
    unsigned char* readbuf = NULL;
    unsigned char* oobbuf = NULL;

    int badblock = 0;
   
    printf("Entering bbt_randomblock_test\n");    
    
    /* Open the MTDx device */
    str[0] = 0;

    int    mtdoffset = 0;
    
    strcat(str, mtddev);

    printf("Open urandom device\n");       
    //Open urandom device:
    if ((randfd = open("/dev/urandom", O_RDONLY)) == -1) 
    {
		printf("Could not open /dev/urandom\n");
		goto closeall;
	}

    printf("Open %s\n", str);       
    //Find all erased block in a partition:
    if ((fd = open(str, O_RDONLY)) == -1) 
    {
        printf("trying to open %s\n", str);
        perror("bbtmod: open flash");
        goto closeall;
    }
    
    printf("MEMGETINFO on %s\n", str);           
    /* Fill in MTD device capability structure */
    if (ioctl(fd, MEMGETINFO, &meminfo) != 0) 
    {
        perror("bbtmod: MEMGETINFO");
        goto closeall;    
    }

	readbuf = malloc(meminfo.erasesize);
	if (readbuf == NULL)
	{
	   fprintf(stderr, "Cannot allocate readbuf\n");
	   goto closeall;
	}
	oobbuf = malloc(meminfo.oobsize);
	if (oobbuf == NULL)
	{
	   fprintf(stderr, "Cannot allocate oobbuf\n");
	   goto closeall;
	}
	
	/* Read the real oob length */
	oob.start = 0;
	oob.length = meminfo.oobsize;
    oob.ptr = oobbuf;
     // Let's print out what we know up to now
    printf("Partition: %s Size: 0x%08X (%09d)\n", str, meminfo.size, meminfo.size);

    for (mtdoffset = 0; (mtdoffset < meminfo.size); mtdoffset++)
    {
        //Get a random number:
        GetRandomValue(randfd, u8, 8);

        //Convert it into a block number within the partition:
        random &= (meminfo.size-1);
        printf("Random value: %08X\n", random);
        
        rm = random & (meminfo.erasesize-1);

        random_blockstart = (unsigned long long)(random-rm);
               
        printf("Is this a bad block?\n" ) ;        
        if ((badblock = ioctl(fd, MEMGETBADBLOCK, &random_blockstart)) < 0) 
        {
			perror("ioctl(MEMGETBADBLOCK)");
			goto closeall;
		}
        
        if (badblock)
        {
            printf("Block located @ %d is marked bad", (unsigned int)random_blockstart);
        }
        else if (randomtest_erasedblocksonly == 1)
        {
            printf("Reading data @ %08X\n", (int)random_blockstart ) ;
            bFF = true;
            
            for (j = 0; j < ((meminfo.erasesize/meminfo.writesize)&&(bFF == true)); j++)
            {
                /* Read data */
                int cnt = pread(fd, readbuf, meminfo.writesize, random_blockstart+(j * meminfo.writesize));
                if (cnt != meminfo.writesize) 
                {
                    printf("returned by pread: %d", cnt);
                    goto closeall;
                }

                printf("##########################Checking if block is erased##########################\n" ) ;        

                for (i = 0; i < meminfo.writesize; i++)
                {
                   if (readbuf[i] != 0xFF)
                   {
                        bFF = false;
                        printf("readbuf[0x%08X] = 0x%02X\n",i, readbuf[i]);
                        break; //out of the for loop
                   }
                }
            }
            //If all block is filled with 0xFF:
            if (bFF)
            {
                printf("random_blockstart = %08X \n", (unsigned int) random_blockstart) ;        
                printf("Block is erased\n" ) ;
                bReady = true;
                break;
            }
        }
        else
        {
            printf("random_blockstart = %08X \n", (unsigned int) random_blockstart) ;        
            bReady = true;
            break;
        }
    }
    if (bReady)
    {
        printf("Marking bad block @ %08X\n", (unsigned int)random_blockstart);
        command.offset = random_blockstart;
        command.code = getBBTCommandType("W");
        command.absolute_offset = absolute_offset;

        if ((ioctl(fd, MTD_BRCMNAND_WRITEBBTCODE, &command)) < 0) 
        {
            perror("bbtmod: ioctl(MTD_BRCMNAND_WRITEBBTCODE)");
            goto closeall;
        }
        printf("Block %08X marked bad.\n", (unsigned int)random_blockstart);
    }
    else
    {
        printf("No block can be marked bad\n");
    }
    if (randfd != -1)
        close(randfd);
    if (fd != -1)
        close (fd);
    return 0;    

closeall:
    if (randfd != -1)
        close(randfd);
    if (fd != -1)
        close (fd);
        
    return 1;
}

static int GetRandomValue(int randfd, unsigned char* dst, int len)
{
    int result;
	while (len) 
	{
		if ((result = read(randfd, dst, len)) == -1)
			return -1;
		dst += result;
		len -= result;
	}
	return result;
}


static int bbt_scan(void)
{
    int    fd;
    int    ret;
    char   str[128];
    loff_t offs = 0;
    struct mtd_info_user meminfo;

    printf("Rescanning BBT...\n");
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

#if 1
    if ((ret = ioctl(fd, MTD_BRCMNAND_SCANBBT, &offs)) < 0) 
    {
        printf("BBT scanned.\n");
    }
 
#endif
    close(fd);

    return 0;
   
}
