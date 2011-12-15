/**
 * @par Copyright Information:
 *      Copyright (C) 2007, Broadcom Corporation.
 *      All Rights Reserved.
 *
 * @file file_test.cpp
 * @author Jean Roberge
 *
 * @brief Speed test for NAND flash. Test with write random content and read!
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/vfs.h>
#include <string.h>
#include <sys/mount.h>

#include <sys/time.h>

char buf[16] = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff};

int main(int argc, char* argv[])
{

    int retval;
    int file = -1;
    char *filebuf = NULL;
    char *buforg, *bufread;
    int i;
    int compareFailed = 0;
    
    printf("JR+ NANDTEST RO\n");
    
    file = open("/dev/mtd1", O_RDONLY);
    
    filebuf = (char *)malloc(0xa000);
    
    retval = read(file, filebuf, 0xa000);
        
    i = 0;
    
    bufread = filebuf;
    
    buforg  = buf;
    
    while(i < 0xa000)
    {
    
        if(bufread[i] != buforg[i%16])
        {
             printf("compared failed-> %d read: %02x original: %02x\n", i, bufread[i] & 0xff , buforg[i%16] & 0xff);
             compareFailed = 1;
        } 
        i++;
    }

    printf("Buffer Read\n");     
    i = 0xa000;
    while(i--)
    {
         printf(" %02x", *filebuf & 0xff);
         if(i%16 == 0) printf("\n");
         filebuf++;
    }
 
    if(compareFailed == 1)
    {
       printf("Compare FAILED!\n"); 
    }
    else
    {
       printf("COMPARE OK!\n"); 
    }
}
