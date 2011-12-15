/*
 *  nandwrite.c
 *
 *  Copyright (C) 2000 Steven J. Hill (sjhill@realitydiluted.com)
 *             2003 Thomas Gleixner (tglx@linutronix.de)
 *
 * $Id: nandwrite.c,v 1.32 2005/11/07 11:15:13 gleixner Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Overview:
 *   This utility writes a binary image directly to a NAND flash
 *   chip or NAND chips contained in DoC devices. This is the
 *   "inverse operation" of nanddump.
 *
 * tglx: Major rewrite to handle bad blocks, write data with or without ECC
 *     write oob data only on request
 *
 * Bug/ToDo:
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <stdbool.h>

#include <mtd/nand_utils.h>

#define PROGRAM "nandwrite"
#define VERSION "$Revision: 1.33mfi $"

// Globals
static struct nand_write_param param = {
    .mtd_device = NULL,
    .img = NULL,
    .mtdoffset = 0,
    .quiet = 0,
    .writeoob = 0,
    .autoplace = 0,
    .forcejffs2 = 0,
    .forceyaffs = 0,
    .forcelegacy = 0,
    .noecc = 0,
    .pad = 0,
    .blockalign = 1 /*default to using 16K block size */
};

static struct nand_write_extra_param extra_param = {
    .bErase = 0
};


void display_help (void)
{
    printf(" Usage: nandwrite [OPTION] MTD_DEVICE INPUTFILE\n"
           "Writes to the specified MTD device.\n"
           "\n"
           "  -a, --autoplace         Use auto oob layout\n"
           "  -j, --jffs2             Force jffs2 oob layout (legacy support)\n"
           "  -y, --yaffs             Force yaffs oob layout (legacy support)\n"
           "  -f, --forcelegacy       Force legacy support on autoplacement enabled mtd device\n"
           "  -n, --noecc             Write without ecc\n"
           "  -o, --oob               Image contains oob data\n"
           "  -s addr, --start=addr   Set start address (default is 0)\n"
           "  -p, --pad               Pad to page size\n"
           "  -b, --blockalign=1|2|4  Set multiple of eraseblocks to align to\n"
           "  -q, --quiet             Don't display progress messages\n"
           "  -e, --erase             Erase before writing\n"    
           "      --help              Display this help and exit\n"
           "      --version           Output version information and exit\n");
    exit(0);
}

void display_version (void)
{
    printf(PROGRAM " " VERSION "\n"
           "\n"
           "Copyright (C) 2003 Thomas Gleixner \n"
           "\n"
           PROGRAM " comes with NO WARRANTY\n"
           "to the extent permitted by law.\n"
           "\n"
           "You may redistribute copies of " PROGRAM "\n"
           "under the terms of the GNU General Public Licence.\n"
           "See the file `COPYING' for more information.\n");
    exit(0);
}

void process_options (int argc, char *argv[])
{
    int error = 0;

    for (;;) {
        int option_index = 0;
        static const char *short_options = "ab:fjnopeqs:y";
        static const struct option long_options[] = {
            {"help", no_argument, 0, 0},
            {"version", no_argument, 0, 0},
            {"autoplace", no_argument, 0, 'a'},
            {"erase", no_argument, 0, 'e'},
            {"blockalign", required_argument, 0, 'b'},
            {"forcelegacy", no_argument, 0, 'f'},
            {"jffs2", no_argument, 0, 'j'},
            {"noecc", no_argument, 0, 'n'},
            {"oob", no_argument, 0, 'o'},
            {"pad", no_argument, 0, 'p'},
            {"quiet", no_argument, 0, 'q'},
            {"start", required_argument, 0, 's'},
            {"yaffs", no_argument, 0, 'y'},
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
        case 'q':
            param.quiet = 1;
            break;
        case 'a':
            param.autoplace = 1;
            break;
        case 'j':
            param.forcejffs2 = 1;
            break;
        case 'y':
            param.forceyaffs = 1;
            break;
        case 'f':
            param.forcelegacy = 1;
            break;
        case 'n':
            param.noecc = 1;
            break;
        case 'o':
            param.writeoob = 1;
            break;
        case 'p':
            param.pad = 1;
            break;
        case 's':
            param.mtdoffset = atoi (optarg);
            break;
        case 'b':
            param.blockalign = atoi (optarg);
            break;
        case 'e':
            extra_param.bErase = 1;
            break;
        case '?':
            error = 1;
            break;
        }
    }

    if ((argc - optind) != 2 || error)
        display_help ();

    param.mtd_device = argv[optind++];
    param.img = argv[optind];
}

void nandwrite_callback(
                void *callback_context, 
                size_t total_input_bytes, 
                bool   erasing, 
                size_t bytes_erased_so_far, 
                bool   writing, 
                size_t bytes_written_so_far,
                size_t total_device_space)
{
                printf ("nandwrite: Writing data to block %x\n", bytes_written_so_far);
}

/*
 * Main program
 */


int main(int argc, char **argv)
{

    process_options(argc, argv);
    
    if(extra_param.bErase)
    {
        nandutils_setExtraParms(&extra_param);
    }

    nandutils_nandwrite(&param, NULL,nandwrite_callback);
    
    return 0;

}


