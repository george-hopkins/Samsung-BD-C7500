/***************************************************************************
 *     Copyright (c) 2009, Broadcom Corporation
 *     All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed with the hope that it will be useful, but without
 * any warranty; without even the implied warranty of merchantability or
 * fitness for a particular purpose.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/sem.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <linux/fs.h>

/*
Notes of clarification:
USB parts:
We currently cope with as many as 6 partitions. The usual USB part appears 
in /sys/dev/block as a first device, representing the entire card, followed 
by devices representing partition[1-6]. We mount filesystems on i.e. partition1.
MMC/SD parts:
We currently cope with 1 partition. The usual MMC/SD part appears in /sys/dev/block 
as a first device, representing the entire card, followed by another device 
representing partition1. We mount filesystems on partition1.
We have encountered cases when parts were not formatted/partitioned. Formatting 
them with i.e. Windows Multimedia Center Edition did actually create a partition0,
only.
In those cases, we mount filesystems on partition0.
*/

#define APP_NOTIFY_PATH "/usr/local/bin/app_player_notify"
#define DEVICE_PATH_SIZE 64
#define MAX_NOMEDIUM_RETRIES 10

const char pgm[] = "/sbin/hotplug";

#ifndef MNT_FORCE
#  define MNT_FORCE	0x00000001	/* Attempt to forcibily umount */
#endif

#ifndef MNT_DETACH
#  define MNT_DETACH	0x00000002	/* Just detach from the tree */
#endif

typedef enum device2mount_s {
	MNT_UNKNOWN = 0,
	MNT_USB = 1,
	MNT_MMC = 2,
} device2mount_t, * device2mount_p;

/*
** This is an example of the scsi relevant env vars.
**
** MINOR=0-15, 16-31, ...
** PHYSDEVBUS=scsi
** MAJOR=8
** ACTION=add
** SUBSYSTEM=block
** PHYSDEVDRIVER=sd

** This is an example of the mmc relevant env vars.
**
** MINOR=0,1
** PHYSDEVBUS=mmc
** MAJOR=179
** ACTION=add
** SUBSYSTEM=block,mmc,bdi
** PHYSDEVDRIVER=mmcblk
*/

/* So we can compare the output of this func using strcmp(...) */
const char *
getenv_safe(const char *var) {
    const char *p = getenv(var);
    if (!p) p = "";
    return p;
}

/* 
    Convert a minor/major pair to a BRCM device path. 
    We should not attempt to mount the entire device, but rather i.e. the first 
    partition. See Notes above.
*/
int
majmin_to_paths(int major, int minor, int maxpathchars, char *devpath, char *mountpath)
{

    const char letter = "abcdefghijklmnop"[ minor / 16 ];
    const int number  = minor % 16;

    if (0 == number) {
        snprintf(devpath, maxpathchars, "/dev/sd%c", letter);
        snprintf(mountpath, maxpathchars, "/mnt/removable/usb%c", letter);
    } else {
        snprintf(devpath, maxpathchars, "/dev/sd%c%d", letter, number);
        snprintf(mountpath, maxpathchars, "/mnt/removable/usb%c", letter);
    }
    return 0;
}

int 
main(int argc, const char *argv[])
{
    int major, minor;
    const char *p0, *p1;
    char devpath[DEVICE_PATH_SIZE], mountpath[DEVICE_PATH_SIZE];
    struct stat st;

    char mmc_devpath[DEVICE_PATH_SIZE], mmc_mountpath[DEVICE_PATH_SIZE];
    struct stat mmc_st;
    device2mount_t device2mount = MNT_UNKNOWN;
    
    fprintf(stderr,"hotplug was invoked.\n"); 

    /* Allow error output to be seen on the console */
    fclose(stderr);
    stderr  = fopen("/dev/console", "w");

#if 0 
    fprintf(stderr,"\ngetenv_safe(\"MAJOR\") = %s.\n", getenv_safe("MAJOR"));
    fprintf(stderr,"\ngetenv_safe(\"MINOR\") = %s.\n", getenv_safe("MINOR"));
    fprintf(stderr,"\ngetenv_safe(\"PHYSDEVDRIVER\") = %s.\n", getenv_safe("PHYSDEVDRIVER"));
    fprintf(stderr,"\ngetenv_safe(\"SUBSYSTEM\") = %s.\n", getenv_safe("SUBSYSTEM"));
    fprintf(stderr,"\ngetenv_safe(\"PHYSDEVBUS\") = %s.\n", getenv_safe("PHYSDEVBUS"));
#endif

    if (strcmp("179", getenv_safe("MAJOR")) == 0 &&
        strcmp("mmcblk", getenv_safe("PHYSDEVDRIVER")) == 0 &&
        strcmp("block", getenv_safe("SUBSYSTEM")) == 0 &&
        strcmp("mmc", getenv_safe("PHYSDEVBUS")) == 0        
        ) {
        device2mount = MNT_MMC;        
    } else {
    /*  Right now, we only care about new scsi disk devices.
        mfi: 
        In 2624, "sd" doesn't come up with "block" and "scsi".
        What we're really looking for is "block" and "scsi", so 
        we have put the "sd" comparison under #if 0
    */    
        //if(strcmp("8", getenv_safe("MAJOR"))) return 0; 
#if 1   
        if (strcmp("sd", getenv_safe("PHYSDEVDRIVER"))) return 0;
#endif
        if(strcmp("block", getenv_safe("SUBSYSTEM"))) return 0; 
        if(strcmp("scsi", getenv_safe("PHYSDEVBUS"))) return 0; 
        
        device2mount = MNT_USB;        
    } 
    
    if (device2mount == MNT_UNKNOWN){
        fprintf(stderr,"Unknown device!\n");
        return 1;
    }
    
    if (device2mount == MNT_USB) {
        /* Grok major and minor numbers of device */
        if ((p0 = getenv("MINOR")) && (p1 = getenv("MAJOR"))) {
            minor = atoi(p0);
            major = atoi(p1);
        } else {
            fprintf(stderr,"Cannot get env minor or major\n");
            return 0;
        }
    
        if (8 != major) { 
            fprintf(stderr,"8 != major , major=%d\n", major);
            return 0; 
        }
    
        if (majmin_to_paths(major, minor, DEVICE_PATH_SIZE, devpath, mountpath)) {
            fprintf(stderr,"majmin_to_paths returns !=0\n", major);
            return 0;
        }
    
        /* Hopefully, mountpath should already exist.  If not, try to create it */
        if (stat(mountpath, &st)) {
            if (mkdir(mountpath, 0744)) {
                return 1;
            }
        }
    }

    if (device2mount == MNT_MMC) {
        snprintf(mmc_mountpath, DEVICE_PATH_SIZE, "/mnt/removable/mmc");
        snprintf(mmc_devpath, DEVICE_PATH_SIZE, "/dev/mmc");
        
        if (stat(mmc_mountpath, &mmc_st)) {
            if (mkdir(mmc_mountpath, 0744)) {
                return 1;
            }
        }
        if (stat("/sys/block/mmcblk0", &mmc_st) == 0) {
            if (stat("/sys/block/mmcblk0/mmcblk0p1", &mmc_st) == 0) {
                /*
                we do not mount part0 if part1 does exist. in that case, 
                par1 is used for the filesystem.
                */
                if(strcmp("0", getenv_safe("MINOR"))) return 0;
                snprintf(mmc_devpath, DEVICE_PATH_SIZE, "/dev/mmc1");
            } else {
                snprintf(mmc_devpath, DEVICE_PATH_SIZE, "/dev/mmc0");
            }
        }
    }

    if (0 == strcmp("add", getenv_safe("ACTION"))) {
        int i, mounted=0, rdonly_mounted=0;
        const char *fstype;
        const char *option;
        
        int mmc_mounted = 0, mmc_rdonly_mounted = 0;
        const char * mmc_fstype;
        const char * mmc_option;
        
        key_t semkey;
        int semid;
        struct sembuf sem[2]; /* sembuf defined in sys/sem.h */
        static const char *fstypes[] = { "vfat", "msdos", "ext3", "ext2" };
        static const char *options[] = { "shortname=winnt", 0, 0, 0 };

        /*
         * Use a semaphore to serialize this section of code because
         * USB flash devices with partitions will run this for each
         * partition simultaneously. This causes a problem when
         * both sda (the raw partition) and sda1 try to mount in
         * parallel because one can cause EBUSY errors for the other.
         */
         /* Get unique key for semaphore. */
        if ((semkey = ftok(argv[0], 'a')) == (key_t) -1) {
            fprintf(stderr,
                    "%s: ftok error - %s\n",
                    strerror(errno));
            return 1;
        }

        /* Get semaphore ID associated with this key. */
        if ((semid = semget(semkey, 0, 0)) == -1) {

            /* Semaphore does not exist - Create. */
            if ((semid = semget(semkey, 1, IPC_CREAT | 0666)) == -1) {

                /* another instance of this process created the semaphore */
                if (errno == EEXIST) {
                    if ((semid = semget(semkey, 0, 0)) == -1) {
                        fprintf(stderr,
                                "%s: Error getting semaphore - %s\n",
                                strerror(errno));
                        return 1;
                    }
                }
                else {
                    fprintf(stderr,
                            "%s: Error creating semaphore - %s\n",
                            strerror(errno));
                    return 1;
                }
            }
        }

        /* Set up two semaphore operations */
        sem[0].sem_num = 0;
        sem[1].sem_num = 0;
        sem[0].sem_flg = SEM_UNDO; /* Release semaphore on exit */
        sem[1].sem_flg = SEM_UNDO; /* Release semaphore on exit */
        sem[0].sem_op = 0; /* Wait for zero */
        sem[1].sem_op = 1; /* Add 1 to lock it*/

        /* Wait for semaphore */
        semop(semid, sem, 2);

        if (device2mount == MNT_USB) {
            /*
             * Try to mount the device.
             * Handle the odd case where a Sandisk Curzer thumb
             * drive returns ENOMEDIUM for 9 seconds after being
             * installed
             */
            int nomedium_retries = 0;
            for (i=0; i < sizeof(fstypes)/sizeof(fstypes[0]); i++) {
                fstype = fstypes[i];
                option = options[i];
                
                if (0 == mount(devpath, mountpath, fstype, (MS_NOATIME | MS_NODIRATIME | MS_DIRSYNC), (const void *)option)) {
                    mounted = 1;
                    break;
                } else if (errno==EACCES || errno==EROFS) {
                    if (0 == mount(devpath, mountpath, fstype, (MS_NOATIME | MS_NODIRATIME | MS_DIRSYNC | MS_RDONLY) , (const void *)option)) {
                        rdonly_mounted = 1;
                        break;
                    }
                } else if (errno==ENOMEDIUM) {
                    if (nomedium_retries++ < MAX_NOMEDIUM_RETRIES) {
                        i--;  /* wait 1 second and retry the same mount */
                        sleep(1);
                    }
                }
            }
        }

        if (device2mount == MNT_MMC) {
            mmc_fstype = "vfat";
            mmc_option = "shortname=winnt";
            
            if (0 == mount(mmc_devpath, mmc_mountpath, mmc_fstype, 
            (MS_NOATIME | MS_NODIRATIME | MS_DIRSYNC) , (const void *)mmc_option)) {
                    mmc_mounted = 1;
            } else if (errno==EACCES || errno==EROFS) {
                    if (0 == mount(mmc_devpath, mmc_mountpath, mmc_fstype, 
                    (MS_NOATIME | MS_NODIRATIME | MS_DIRSYNC | MS_RDONLY) , 
                    (const void *)mmc_option)) {
                            mmc_rdonly_mounted = 1;
                    }
            }   
        }	
        
        /* unlock the semaphore */
        sem[0].sem_op = -1; /* Decrement to unlock */
        semop(semid, sem, 1);

        /* delete the semaphore */
        semctl(semid, 0, IPC_RMID);

        if (device2mount == MNT_USB) {
            if (mounted || rdonly_mounted) {
                fprintf(stderr, "%s: mounted %s%s on %s as %s.\n", pgm, (rdonly_mounted?"read-only ":""), devpath, mountpath, fstype);
                if (0 == stat(APP_NOTIFY_PATH, &st)) {
                    setenv("LD_LIBRARY_PATH", "/usr/local/lib:/lib:/usr/lib", 1);
                    execl(APP_NOTIFY_PATH, APP_NOTIFY_PATH, "1", "mount", devpath, mountpath, (const char*)0);
                }
            } else {
                fprintf(stderr, "%s: failed to mount %s on %s.\n", pgm, devpath, mountpath);
            }
        }	
        
        if (device2mount == MNT_MMC) {    
            if (mmc_mounted || mmc_rdonly_mounted) {
                fprintf(stderr, "%s: mounted %s%s on %s as %s.\n", pgm, 
                (mmc_rdonly_mounted?"read-only ":""), mmc_devpath, mmc_mountpath, mmc_fstype);
                if (0 == stat(APP_NOTIFY_PATH, &mmc_st)) {
                    setenv("LD_LIBRARY_PATH", "/usr/local/lib:/lib:/usr/lib", 1);
                    execl(APP_NOTIFY_PATH, APP_NOTIFY_PATH, "1", "mount", mmc_devpath, 
                    mmc_mountpath, (const char*)0);
                }
            } else {
                fprintf(stderr, "%s: failed to mount %s on %s.\n", pgm, mmc_devpath, mmc_mountpath);
            }
        }	
        
    } else if (0 == strcmp("remove", getenv_safe("ACTION"))) {
        if (device2mount == MNT_USB) {
            if (-1 == umount2(mountpath, MNT_DETACH|MNT_FORCE)) {                
                fprintf(stderr, "%s: failed to unmount %s from %s (errno=%d).\n", pgm, devpath, mountpath, errno);
            } else {
                fprintf(stderr, "%s: unmounted %s from %s.\n", pgm, devpath, mountpath);
                if (0 == stat(APP_NOTIFY_PATH, &st)) {
                    setenv("LD_LIBRARY_PATH", "/usr/local/lib:/lib:/usr/lib", 1);
                    execl(APP_NOTIFY_PATH, APP_NOTIFY_PATH, "2", "umount", "0", mountpath, (const char*)0);
                }
            }
        }  
   
        if (device2mount == MNT_MMC) {
            if (-1 == umount2(mmc_mountpath, MNT_DETACH|MNT_FORCE)) { 
                /*
                we cannot assume we mount only partitions represented by 
                devices with MINOR != 0. See Notes at the begining of the file.
                There is no way to discriminate it at umount time. At this point,
                /sys/block/mmcblk0 is no longer valid. We will display only the 
                success case.
                */
                if (0){
                    fprintf(stderr, "%s: failed to unmount %s from %s (errno=%d).\n", 
                    pgm, mmc_devpath, mmc_mountpath, errno);
                }
            } else {
                fprintf(stderr, "%s: unmounted %s from %s.\n", pgm, mmc_devpath, mmc_mountpath);
                if (0 == stat(APP_NOTIFY_PATH, &mmc_st)) {
                    setenv("LD_LIBRARY_PATH", "/usr/local/lib:/lib:/usr/lib", 1);
                    execl(APP_NOTIFY_PATH, APP_NOTIFY_PATH, "2", "umount", "0", 
                    mmc_mountpath, (const char*)0);
                }
            }
        }
    } else {
        return 0;
    }

    fclose(stderr);
    return 0;
}

