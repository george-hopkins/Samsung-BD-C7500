/**
 * @par Copyright Information:
 *      Copyright (C) 2008, Broadcom Corporation.
 *      All Rights Reserved.
 *
 * @file    throughput.cpp
 * @author  Martin Fillion
 *
 * @brief   Flash (internal/external) throughput calculation
 *
 */
/*=========================== "system" includes ==============================*/
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mount.h>
#include <linux/types.h>

#define GULP (64*1024)        //64KBytes

#define FILESIZE (5120 *1024) //5MBytes

#define DIO 0   //No direct IO for yaffs

#define FT_WRITE
#define FT_READ

typedef enum eReturnVal{
    TH_OK           = 0,

    TH_ALLOC_ERR    = 1,
    TH_UMOUNT_ERR   = 2,
    TH_MOUNT_ERR    = 3,
    
    TH_OPEN_TST_ERR = 4,
    TH_WR_TST_ERR   = 5,
    TH_RD_TST_ERR   = 6,
    
    TH_OPEN_END_ERR = 7,
    TH_WR_END_ERR   = 8,
    TH_RD_END_ERR   = 9
    
}eReturnVal_t;

    typedef unsigned int ULONG;
    typedef void* PVOID;
    
    typedef enum 
    {
        ERROR_CRITICAL,
        ERROR,
        WARNING,
        INFO,
        DEBUG1,
        DEBUG2,
        DEBUG3,
    }logLevel;
    

    #define VFS_LOG( level, message, args... ) { \
        if (level <= INFO) printf( message"\n", ## args);}

//Local variables:
static char* m_szMountDev = NULL;
static char* m_szTstFile = NULL;
static int m_nFileSize = 0;

static char* m_szMountPoint = NULL;
static char* m_szMountFs = NULL;


static int m_bMount = 0;

static int version = 8;

int bHdTst = 0;

/*Function prototyptes:*/

static int Run (int bMount);
static eReturnVal_t doit_write(const char *fileIn, int loops);
static eReturnVal_t doit_read(const char *fileIn, int loops);
void free_local(void);

eReturnVal_t doit_write(const char *fileIn, int loops)
{
    int fd;
    int i,j;
    struct timeval tv0, tv1, tv2;

    double wrThroughput = 0.0;
    const int N=(m_nFileSize)/(GULP);
    int nCnt = 0;
    
    unsigned char *buf = (unsigned char*) malloc(loops * GULP);    
    
    if (buf == NULL)
    {
        VFS_LOG(ERROR, "Could not allocate buffer for reading");
        return TH_ALLOC_ERR;
    }

    for (i=0; i<loops * GULP; i++)
    {
        buf[i] = i;
    }

    for (i=0; i<loops; i++) 
    {
        gettimeofday(&tv0, 0);
        
        fd = open(fileIn, O_CREAT | O_WRONLY | DIO);
        if (0>fd) 
        {
            VFS_LOG(ERROR, "Could not open file %s", fileIn);
            free(buf);

            return TH_OPEN_TST_ERR;
        }
        for (j=0; j<N; j++) 
        {

            VFS_LOG(DEBUG2, "Writing %d for j=%d, N=%d into %s", GULP, j, N, fileIn);
            int rval = (int)write(fd, buf, GULP);

            if (rval != GULP) 
            {
                VFS_LOG(ERROR, "Tried to write %d but read %d.", GULP, rval);
                free(buf);
                close(fd);
                sync();
                return TH_WR_TST_ERR;
            }
        }
        close(fd);
        sync();

        gettimeofday(&tv1, 0);

        tv1.tv_sec -= tv0.tv_sec;
        tv0.tv_sec  = 0;

        VFS_LOG(DEBUG1, "t0:   %5d.%06d", (int)tv0.tv_sec, (int)tv0.tv_usec);
        VFS_LOG(DEBUG1,   "t1:   %5d.%06d", (int)tv1.tv_sec, (int)tv1.tv_usec);

        if (tv1.tv_usec > tv0.tv_usec) 
        {

            tv2.tv_sec  = tv1.tv_sec  - tv0.tv_sec;
            tv2.tv_usec = tv1.tv_usec - tv0.tv_usec;
        } 
        else 
        {

            tv2.tv_sec  = tv1.tv_sec  - (tv0.tv_sec+1);
            tv2.tv_usec = 1000000 + tv1.tv_usec - tv0.tv_usec;
        }
    
        VFS_LOG(DEBUG1,   "--:   ------------");
        VFS_LOG(DEBUG1,   "t2:   %5d.%06d", (int)tv2.tv_sec, (int)tv2.tv_usec);
    

        double t = (double)tv2.tv_sec + (double)tv2.tv_usec/1000000.0;
        double b = (double)m_nFileSize * (double)loops;

        double th = b/t;

        wrThroughput += th;
        nCnt++;
    }
    
    wrThroughput /= ((double) nCnt * 1024.0 * 1024.0);
    VFS_LOG(INFO, "Write bandwidth is %8.3f  MBytes/sec", wrThroughput);
    
    free(buf);

    
    return TH_OK;
}

eReturnVal_t doit_read(const char *fileIn, int loops)
{
    const int N=m_nFileSize/GULP;
    int fd;
    int i,j;
    struct timeval tv0, tv1, tv2;

    double rdThroughput = 0.0;
    int nCnt = 0;

        
    unsigned char *buf = (unsigned char*)malloc(loops*GULP);
    if (buf == NULL)
    {
        VFS_LOG(ERROR, "Could not allocate buffer for reading");
        return TH_ALLOC_ERR;
    }
    
    for (i=0; i<loops; i++) 
    {
        /******Mount/unmount******/
        if (m_bMount)
        {
            gettimeofday(&tv0, 0);
            if (umount(m_szMountPoint) == -1)
            {
                {
                    VFS_LOG(ERROR, "Cannot unmount %s, errno %d (%s)!!!!", 
                                        m_szMountPoint, errno, strerror(errno));
                
                    rdThroughput = 0.0; 
                    return TH_UMOUNT_ERR;
                }        // No such file or directory will not prevent us from creating the link 
            }

            //Remount /mnt/lstor:
            VFS_LOG(DEBUG1,
                        "Re-mounting %s on %s",
                            m_szMountPoint, 
                                m_szMountDev);
    
            int nRet = mount(m_szMountDev, m_szMountPoint, m_szMountFs, 0, NULL);
    
            if (nRet == -1)
            {
                VFS_LOG(ERROR, "Cannot re-mount %s, errno %d (%s)!!!!", 
                                m_szMountPoint, errno, strerror(errno));
                    
                rdThroughput = 0.0; 
                return (TH_MOUNT_ERR);
            }
            gettimeofday(&tv1, 0);        

            tv1.tv_sec -= tv0.tv_sec;
            tv0.tv_sec  = 0;
        
            VFS_LOG(DEBUG1, "Time to umount/mount:")
            VFS_LOG(DEBUG1, "t0:   %5d.%06d", (int)tv0.tv_sec, (int)tv0.tv_usec);
            VFS_LOG(DEBUG1, "t1:   %5d.%06d", (int)tv1.tv_sec, (int)tv1.tv_usec);
        
            if (tv1.tv_usec > tv0.tv_usec) 
            {
                tv2.tv_sec  = tv1.tv_sec  - tv0.tv_sec;
                tv2.tv_usec = tv1.tv_usec - tv0.tv_usec;
            } 
            else 
            {
                tv2.tv_sec  = tv1.tv_sec  - (tv0.tv_sec+1);
                tv2.tv_usec = 1000000 + tv1.tv_usec - tv0.tv_usec;
            }
            VFS_LOG(DEBUG1,   "--:   ------------");
            VFS_LOG(DEBUG1,   "t2:   %5d.%06d\n", (int)tv2.tv_sec, (int)tv2.tv_usec);
        }            

        /******Open/read******/        
        gettimeofday(&tv0, 0);
        
        fd = open(fileIn, O_RDONLY | DIO);
        if (0>fd) 
        {
            VFS_LOG(ERROR, "Could not open %s for reading.", fileIn);
            free(buf);
            return TH_OPEN_TST_ERR;
        }
        for (j=0; j<N; j++) 
        {
            int rval = read(fd, buf, GULP);
            if (rval != GULP) 
            {
                VFS_LOG(ERROR, "Tried to read %d but read %d.", GULP, rval);
                
                close(fd);
                
                free(buf);
                
                return TH_RD_TST_ERR;
            }
        }
        close(fd);

        gettimeofday(&tv1, 0);        

        tv1.tv_sec -= tv0.tv_sec;
        tv0.tv_sec  = 0;

        VFS_LOG(DEBUG1, "t0:   %5d.%06d", (int)tv0.tv_sec, (int)tv0.tv_usec);
        VFS_LOG(DEBUG1,   "t1:   %5d.%06d", (int)tv1.tv_sec, (int)tv1.tv_usec);

        if (tv1.tv_usec > tv0.tv_usec) 
        {
            tv2.tv_sec  = tv1.tv_sec  - tv0.tv_sec;
            tv2.tv_usec = tv1.tv_usec - tv0.tv_usec;
        } 
        else 
        {
            tv2.tv_sec  = tv1.tv_sec  - (tv0.tv_sec+1);
            tv2.tv_usec = 1000000 + tv1.tv_usec - tv0.tv_usec;
        }
        VFS_LOG(DEBUG1,   "--:   ------------");
        VFS_LOG(DEBUG1,   "t2:   %5d.%06d", (int)tv2.tv_sec, (int)tv2.tv_usec);

        double t = (double)tv2.tv_sec + (double)tv2.tv_usec/1000000.0;
        double b = (double)m_nFileSize;

        double th = b/t;
        
        rdThroughput += th;
        nCnt++;
    }        
    
    rdThroughput = (rdThroughput / ((double)nCnt * 1024.0 * 1024.0));
    
    VFS_LOG(INFO, "Read bandwidth is %8.3f  MBytes/sec", rdThroughput);
    
    free(buf);    
   
    return TH_OK;
}



int Run (int bMount)
{
    eReturnVal_t ret = TH_OK; 

    if (!bMount)
    {
        printf("Testing without filesystem\n");        
        m_bMount = 0;
    }
    else
    {
        printf("Testing with filesystem\n");        
        m_bMount = 1;
    }

    if (m_szMountDev)
    {
        m_nFileSize = FILESIZE;

        printf("m_nFileSize: %d\n", m_nFileSize);
        ret = TH_OK;
        
#ifdef FT_WRITE
        ret = doit_write(m_szTstFile, 1);
#endif        

#ifdef FT_READ
        if (ret == TH_OK)
        {
            ret = doit_read(m_szTstFile, 1);
        }
#endif        
    }
    else
    {
        printf("m_szMountDev is NULL\n");
        return -2;
    }

    return 0;
}
void free_local(void)
{
            if (m_szMountPoint)
            {
                free(m_szMountPoint);
                m_szMountPoint = NULL;
            }

            if (m_szMountFs)
            {
                free(m_szMountFs);
                m_szMountFs = NULL;
            }

            if (m_szTstFile)
            {
                free(m_szTstFile);
                m_szTstFile = NULL;
            }

            free(m_szMountDev);
}

int main(int argc, const char *argv[])
{
    int bMount = 1;

    if (argc <= 1)
    {
        printf("usage: 'ft_mips <--version> /dev/<device>  <-raw>', where <device> can be sd<xy> or mtdblock<y>.\n" \
               "<x> is the device entry (a,b,c,d,e,f...) and <y> is the partition number (1,2,3...)\n");
        return -1;
    }

    m_szMountDev = strdup(argv[1]);     

    if (strcmp(argv[1], "--version") == 0)
    {
        printf("Version: %d\n", version);
        return 0;
    }
    if (argc == 3)
    {
        if (strcmp(argv[2], "-raw") == 0)
        {
            bMount = 0;
        }
        
        if (strcmp(argv[2], "-hd") == 0)
        {
            bHdTst = 1;
        }
    }

    if (bMount)
    {
        if (strncmp(m_szMountDev, "/dev/mtd", 8) == 0)
        {
            m_szMountPoint =  (bHdTst)? strdup("/mnt/hd") : strdup("/mnt/pstor");
            m_szTstFile = (bHdTst)? strdup ("/mnt/hd/tst") : strdup("/mnt/pstor/tst");
            m_szMountFs = strdup("yaffs2");
        }
        else
        {
            m_szMountPoint =  strdup("/mnt/removable/usba");
            m_szTstFile = strdup ("/mnt/removable/usba/tst");
            m_szMountFs = strdup("vfat");
        }
        printf("WARNING: Be sure your device is mounted as %s\n", m_szMountPoint);
    }
    else
    {
        printf("WARNING: Be sure your device has been unmounted before running"
               " this test!!!!\n\n");

        VFS_LOG(WARNING, "RUNNING THIS APPLICATION WILL DESTROY\n"
                         "THE FILESYSTEM IN THE FLASH.\n"
                         "PLEASE ERASE/RE-FORMAT IT AFTER THE TEST IS DONE." );

        m_szTstFile = strdup (m_szMountDev);
    }
    
    int ret = Run(bMount);

    free_local();

    return ret;
}

