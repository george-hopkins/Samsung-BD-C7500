#include <unistd.h>
#include <stdlib.h>

static const char *progname = "init";


/* This is the argc and argv we pass to init */
int init_argc;
char *init_argv[2];

/*
 * This program simply exec busybox's init function.  
 * We need it because execv does not handle symlink well.
 */
int main(int argc, char *argv[])
{



	init_argv[0] = "init";
	init_argv[1] = NULL;
	execv("/sbin/init", init_argv);

 done:
	exit(-1);
}
