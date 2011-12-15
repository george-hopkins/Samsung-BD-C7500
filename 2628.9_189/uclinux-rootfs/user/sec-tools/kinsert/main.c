
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

unsigned char  *n_key, *e_key;
int      n_key_len, e_key_len;

int extract_key(int fd_pkey);
int update_target(char *target);

char *progname;

int debug = 0;

int
main(int argc, char **argv)
{
    int		fd_pkey;
    char 	*pkey, *target;

    progname = argv[0];


    if (argc < 3 || argc > 4) {
	fprintf(stderr,"usage: %s [-d]  public_key.bin vmlinux\n", progname);
	exit(1);
    }

    if (strcmp(argv[1], "-d") == 0) {
	if (argc != 4) {
	    fprintf(stderr,"usage: %s [-d]  public_key.bin vmlinux\n", progname);
	    exit(1);
	}

	debug = 1;
	pkey = argv[2];
	target = argv[3];
    } else {
	pkey = argv[1];
	target = argv[2];
    }
    

    fd_pkey = open(pkey, O_RDONLY);

    if (fd_pkey < 0) {
	fprintf(stderr, "Unable to open pkey_file %s ", pkey);
	       perror(" ");
	exit(1);
    }

    if (extract_key(fd_pkey)) {
	exit(1);
    }

    close(fd_pkey);

    update_target(target);
	

}
