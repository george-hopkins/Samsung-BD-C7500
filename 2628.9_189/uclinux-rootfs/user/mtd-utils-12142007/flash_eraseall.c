/* hlash_eraseall.c -- erase the whole of a MTD device

   Copyright (C) 2000 Arcom Control System Ltd

    Renamed to flash_eraseall.c

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

   $Id: flash_eraseall.c,v 1.24 2005/11/07 11:15:10 gleixner Exp $
*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>

#include "mtd/nand_utils.h"

#define PROGRAM "flash_eraseall"
#define VERSION "$Revision: 1.24 $"

static const char *exe_name;

// Globals
static struct nand_erase_param param = {
    .mtd_device = NULL,
    .quiet = 0,
    .jffs2 = 0,
};

void display_help (void)
{
	printf(" Usage: %s [OPTION] MTD_DEVICE\n"
	       "Erases all of the specified MTD device.\n"
	       "\n"
	       "  -j, --jffs2    format the device for jffs2\n"
	       "  -q, --quiet    don't display progress messages\n"
	       "      --silent   same as --quiet\n"
	       "      --help     display this help and exit\n"
	       "      --version  output version information and exit\n",
	       exe_name);
	exit(0);
}


void display_version (void)
{
	printf(PROGRAM " " VERSION "\n"
	       "\n"
	       "Copyright (C) 2000 Arcom Control Systems Ltd\n"
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

	exe_name = argv[0];

	for (;;) {
		int option_index = 0;
		static const char *short_options = "jq";
		static const struct option long_options[] = {
			{"help", no_argument, 0, 0},
			{"version", no_argument, 0, 0},
			{"jffs2", no_argument, 0, 'j'},
			{"quiet", no_argument, 0, 'q'},
			{"silent", no_argument, 0, 'q'},

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
		case 'j':
			param.jffs2 = 1;
			break;
		case '?':
			error = 1;
			break;
		}
	}
	if (optind == argc) {
		fprintf(stderr, "%s: no MTD device specified\n", exe_name);
		error = 1;
	}
	if (error) {
		fprintf(stderr, "Try `%s --help' for more information.\n",
			exe_name);
		exit(1);
	}

	param.mtd_device = argv[optind];
}


void flash_eraseall_callback(
                void *callback_context, 
                size_t total_input_bytes, 
                bool   erasing, 
                size_t bytes_erased_so_far, 
                bool   writing, 
                size_t bytes_written_so_far,
                size_t total_device_space)
{
    			printf("\rErasing %d Kibyte @ %x -- %2llu %% complete.",
        			     total_input_bytes / 1024, bytes_erased_so_far,
        			     (unsigned long long)
        			     bytes_erased_so_far * 100 / total_device_space);   
}

int main (int argc, char *argv[])
{
    int rc;

    process_options(argc, argv);

    rc = nandutils_flash_erase(&param, NULL, flash_eraseall_callback);

    return rc;
}


