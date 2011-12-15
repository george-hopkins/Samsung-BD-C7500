/*
 * flash_info.c -- print info about a MTD device
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mount.h>

#include <mtd/mtd-user.h>

static const char *mtd_device;
static const char *exe_name;

int main(int argc,char *argv[])
{
#if 0
	int regcount;
	int Fd;

	if (1 >= argc)
	{
		fprintf(stderr,"Usage: flash_info device\n");
		return 16;
	}
	// Open and size the device
	if ((Fd = open(argv[1],O_RDONLY)) < 0)
	{
		fprintf(stderr,"File open error\n");
		return 8;
	}

	if (ioctl(Fd,MEMGETREGIONCOUNT,&regcount) == 0)
	{
		int i;
		region_info_t reginfo;
		printf("Device %s has %d erase regions\n", argv[1], regcount);
		for (i = 0; i < regcount; i++)
		{
			reginfo.regionindex = i;
			if(ioctl(Fd, MEMGETREGIONINFO, &reginfo) == 0)
			{
				printf("Region %d is at 0x%x with size 0x%x and "
				       "has 0x%x blocks\n", i, reginfo.offset,
				       reginfo.erasesize, reginfo.numblocks);
			}
			else
			{
				printf("Strange can not read region %d from a %d region device\n",
				       i, regcount);
			}
		}
	}
#else
	int fd;
	unsigned int erase_length;
	mtd_info_t meminfo;

	exe_name = argv[0];
	if (1 >= argc)
	{
		fprintf(stderr,"Usage: %s device\n", exe_name);
		return 16;
	}
	mtd_device = argv[1];

	if ((fd = open(mtd_device, O_RDWR)) < 0) {
		fprintf(stderr, "%s: %s: %s\n", exe_name, mtd_device, strerror(errno));
		exit(1);
	}


	if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
		fprintf(stderr, "%s: %s: unable to get MTD device info\n", exe_name, mtd_device);
		exit(1);
	}

	erase_length = meminfo.erasesize;

	printf("Type: %s\nErase block size: %d kB\n", meminfo.type == MTD_NANDFLASH ? "NAND": "NOR", erase_length >> 10);

	close(fd);

#endif
	return 0;
}
