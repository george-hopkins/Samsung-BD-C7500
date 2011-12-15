/*
    TraceDaemon.c : Tracing daemon for the Linux kernel.
    Copyright (C) 1999, 2000, 2001, 2002 Karim Yaghmour (karim@opersys.com).

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* Necessary headers */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <dirent.h>
#if 0 /* Check whether we still need this. K.Y. */
#include <stdint.h>
#endif

#include <errno.h>

#include <UserTrace.h>

/* Local definitions */
#include "TraceDaemon.h"

/* If we use libc5 this macro is not defined. Thus, we define it here. */
/* K.Y. This was added by A. Heppel for SYSGO. I'll leave it here, but must
confess that anyone contemplating libc5 should actually be using uClibc. */
#ifndef MAP_FAILED
#define MAP_FAILED      ((__ptr_t) -1)
#endif

 /* Global variables */
int      gPID;                       /* The daemon's PID */
int      gBusy;                      /* Are we currently writting events to the output file */
int      gTracHandle;                /* File descriptor to tracing driver */
int      gTracDataFile[MAX_CPUS];    /* The file to which tracing data is dumped */
int      gTracRelayFile[MAX_CPUS];   /* The file to which tracing data is dumped */
int      gControlFile;               /* File for non-tracefile-specific ioctls */
int      gTracDataAreaSize;          /* The size of the are used to buffer driver data */
int      gTotalDataAreaSize;         /* The size mapped  */
int      gTracNBuffers;              /* The number of buffers to divide buffer area into, default 2 if -n option not specified */
char*    gTracDataArea[MAX_CPUS];    /* The mmapped data area */
options* gOptions;                   /* The user given options */
int      gWaitingForThreads = FALSE; /* Flag indicating whether we're waiting for threads to get timeslices before we exit */
int      gNumCPUs;                   /* The number of CPUs traced by the driver */
int      gPageShift;                 /* The PAGE_SHIFT being used by the driver */
int      gPageSize;                  /* The PAGE_SIZE being used by the driver */
unsigned long gPageMask;             /* The PAGE_MASK being used by the driver */
char     gRelayfsMntPt[PATH_MAX];    /* Relayfs mountpoint path buffer */
struct pollfd pollfds[MAX_CPUS];     /* For polling the relayfs channels */

/* This inspired by rtai/shmem */
#define FIX_SIZE(x) ((((x) - 1) & gPageMask) + gPageSize)

/* Event strings the user can specify at the command line */
char *EventOT[TRACE_MAX_EVENTS + 1] =
{
"START",
"SYS_ENTRY",
"SYS_EXIT",
"TRAP_ENTRY",
"TRAP_EXIT",
"IRQ_ENTRY",
"IRQ_EXIT",
"SCHED",
"KTIMER",
"SIRQ",
"PROCESS",
"FS",
"TIMER",
"MEM",
"SOCKET",
"IPC",
"NET",
"",   /* Buffer start */
"",   /* Buffer end */
"NEWEV",   /* New event */
"CSTM", /* Custom event */
"CHMSK",
"HEARTBEAT",
#if SUPP_RTAI
"RTMNT",
"RTUMNT",
"RTGIRQE",
"RTGIRQX",
"RTOIRQE",
"RTOIRQX",
"RTTRAPE",
"RTTRAPX",
"RTSRQE",
"RTSRQX",
"RTSWTCHL",
"RTSWTCHR",
"RTSCHED",
"RTTASK",
"RTTIMER",
"RTSEM",
"RTMSG",
"RTRPC",
"RTMBX",
"RTFIFO",
"RTSHM",
"RTPOSIX",
"RTLXRT",
"RTLXRTI",
#endif /* SUPP_RTAI */
};

/******************************************************************
 * Function :
 *    CreateOptions()
 * Description :
 *    Creates and initializes an options structure.
 * Parameters :
 *    None.
 * Return values :
 *    The new options structure.
 * History :
 *    K.Y. 21/10/99 : Initial typing.
 * Note :
 ******************************************************************/
options* CreateOptions(void)
{
  options* pOptions;    /* Options to be created */

  /* Allocate space for new options */
  if((pOptions = (options*) malloc(sizeof(options))) == NULL)
    {
    printf("Unable to allocate space for options \n");
    exit(1);
    }

  /* Initialize options */
  pOptions->ConfigDefault      = TRUE;
  pOptions->SpecifyEvents      = FALSE;
  pOptions->EventMask          = 0;
  pOptions->SpecifyDetails     = FALSE;
  pOptions->DetailsMask        = 0;
  pOptions->ConfigCPUID        = FALSE;
  pOptions->ConfigPID          = FALSE;
  pOptions->PID                = 0;
  pOptions->ConfigPGRP         = FALSE;
  pOptions->PGRP               = 0;
  pOptions->ConfigGID          = FALSE;
  pOptions->GID                = 0;
  pOptions->ConfigUID          = FALSE;
  pOptions->UID                = 0;
  pOptions->ConfigSyscallDepth = TRUE;
  pOptions->SyscallDepth       = 0;
  pOptions->ConfigSyscallBound = FALSE;
  pOptions->UpperBound         = 0;
  pOptions->LowerBound         = 0;
  pOptions->ConfigTime         = FALSE;
  pOptions->Time.tv_sec        = 0;
  pOptions->Time.tv_usec       = 0;
  pOptions->TraceFileName      = NULL;
  pOptions->ProcFileName       = NULL;

  pOptions->ModifyTraceMask    = FALSE;
  pOptions->PrintTraceMask     = FALSE;

  pOptions->UseLocking         = FALSE;
  pOptions->DataAreaSize       = 0;
  pOptions->NBuffers           = 0;
  pOptions->UseTSC             = TRUE;
  pOptions->FlightRecorder     = FALSE;
  pOptions->Status             = FALSE;
  pOptions->Stop               = FALSE;
  pOptions->Snapshot           = FALSE;

  /* Give the options to the caller */
  return pOptions;
}

/*******************************************************************
 * Function :
 *    ParseOptions()
 * Description :
 *    Parses the command line arguments and sets the options
 *    structure to the right values.
 * Parameters :
 *    pmArgc, The number of command line options given to the program
 *    pmArgv, The argument vector given to the program
 *    pmOptions, The options structure to be set
 * Return values :
 *    NONE
 * History :
 *    K. Yaghmour, 21/10/99, Initial typing.
 * Note :
 *    This function will "exit()" if something is wrong with the
 *    command line options. If this is to go in graphic mode then
 *    gtk_init() will be called.
 *******************************************************************/
void ParseOptions(int pmArgc, char** pmArgv, options* pmOptions)
{
  int     i, j;        /* Generic indexes */
  int     lOptLen = 0; /* Length of option string */
  char*   pEnd;        /* End pointer for strtol */

  /* Read the command line arguments */
  for(i = 1; i < pmArgc; i++)
    {
    /* Get the option's length */
    lOptLen = strlen(pmArgv[i]);

#if 0
    /* DEBUG */
    printf("Analyzing option : %s \n", pmArgv[i]);
#endif

    /* Is this an option or a file name */
    if(pmArgv[i][0] == '-')
      {
      /* Is this long enough to really be an option */
      if(lOptLen < 2)
	{
        /* Hey Larry, Please give us something meaningfull! */
	printf("Incomplete option : %d \n", i);
	exit(1);
	}

      /* Depending on the option given */
      switch(pmArgv[i][1])
	{
	/* Default */
	case 'd':
	  pmOptions->ConfigDefault = TRUE;
	  break;

	/* Use lock-free buffering scheme */
	case 'f':
	  pmOptions->UseLocking = FALSE;
	  break;

	/* Use locking buffering scheme */
	case 'l':
	  pmOptions->UseLocking = TRUE;
	  break;

	/* Daemon buffer size */
	case 'b':
	case 'B':
	  /* Is the option given long enough */
	  if(lOptLen < 3)
	    {
	    printf("Daemon buffer size option without buffer size : %d \n", i);
	    exit(1);
	    }

	  /* Read the daemon's buffer size */
	  pmOptions->DataAreaSize = strtol(&(pmArgv[i][2]), &pEnd, 10);
	  if(*pEnd != '\0')
	    {
	    printf("Invalid buffer size `%s' \n", &pmArgv[i][2]);
	    exit(1);
	    }
	  break;

	/* Daemon number of buffers */
	case 'n':
	case 'N':
	  /* Is the option given long enough */
	  if(lOptLen < 3)
	    {
	    printf("Daemon number of buffers option without number of buffers : %d \n", i);
	    exit(1);
	    }

	  /* Read the daemon's number of buffers */
	  pmOptions->NBuffers = strtol(&(pmArgv[i][2]), &pEnd, 10);
	  if(*pEnd != '\0')
	    {
	    printf("Invalid number of buffers `%s' \n", &pmArgv[i][2]);
	    exit(1);
	    }
	  break;

	/* Specify the events who's details are to be logged */
	case 'D' :
	  /* Is the option given long enough */
	  if(lOptLen < 3)
	    {
	    printf("Specifying an event's details to monitor but no event given : %d \n", i);
	    exit(1);
	    }

	  /* We are specifying specific event */
	  pmOptions->SpecifyDetails = TRUE;

	  /* Using CORE we can include all core events at once into trace */
	  if (!strcmp("CORE", &(pmArgv[i][2])))
	    {
	    ltt_set_bit(TRACE_SYSCALL_ENTRY, &(pmOptions->DetailsMask));	
	    ltt_set_bit(TRACE_SYSCALL_EXIT, &(pmOptions->DetailsMask));	
	    ltt_set_bit(TRACE_TRAP_ENTRY, &(pmOptions->DetailsMask));	
	    ltt_set_bit(TRACE_TRAP_EXIT, &(pmOptions->DetailsMask));	
	    ltt_set_bit(TRACE_IRQ_ENTRY, &(pmOptions->DetailsMask));	
	    ltt_set_bit(TRACE_IRQ_EXIT, &(pmOptions->DetailsMask));	
	    ltt_set_bit(TRACE_SCHEDCHANGE, &(pmOptions->DetailsMask));	
	    }
	  else
	    {
	    /* Set event to be traced */
	    for(j = 0; j <= TRACE_MAX_EVENTS; j++)
	      {
	      /* Is this the option he gave us */
	      if(!strcmp(EventOT[j], &(pmArgv[i][2])))
		{
		ltt_set_bit(j, &(pmOptions->DetailsMask));

		/* Tracing custom events requires tracing of their declaration */
	        if (j == TRACE_CUSTOM)
	          ltt_set_bit(TRACE_NEW_EVENT, &(pmOptions->DetailsMask));

		break;
		}
	      }
	    }

	  /* Did he specify an unknown option */
	  if(j > TRACE_MAX_EVENTS)
	    {
	    printf("Option -D accompanied by unknown event %s \n", &(pmArgv[i][2]));
	    exit(1);
	    }
	/* Fall through to the next option -e/-E => the details option makes sense
	   only if the event is traced */
#if 0
	  break;
#endif

	/* Specify events to be monitored */
	case 'e':
	case 'E':
	  /* Is the option given long enough */
	  if(lOptLen < 3)
	    {
	    printf("Specifying an event to monitor but no event given : %d \n", i);
	    exit(1);
	    }

	  /* We are specifying specific event */
	  pmOptions->SpecifyEvents = TRUE;

	  /* Using CORE we can include all core events at once into trace */
	  if (!strcmp("CORE", &(pmArgv[i][2])))
	    {
	    ltt_set_bit(TRACE_SYSCALL_ENTRY, &(pmOptions->EventMask));	
	    ltt_set_bit(TRACE_SYSCALL_EXIT, &(pmOptions->EventMask));	
	    ltt_set_bit(TRACE_TRAP_ENTRY, &(pmOptions->EventMask));	
	    ltt_set_bit(TRACE_TRAP_EXIT, &(pmOptions->EventMask));	
	    ltt_set_bit(TRACE_IRQ_ENTRY, &(pmOptions->EventMask));	
	    ltt_set_bit(TRACE_IRQ_EXIT, &(pmOptions->EventMask));	
	    ltt_set_bit(TRACE_SCHEDCHANGE, &(pmOptions->EventMask));	
	    }
	  else
	    {
	    /* Set event to be traced */
	    for(j = 0; j <= TRACE_MAX_EVENTS; j++)
	      {
	      /* Is this the option he gave us */
	      if(!strcmp(EventOT[j], &(pmArgv[i][2])))
		{
		ltt_set_bit(j, &(pmOptions->EventMask));

		/* Tracing custom events requires tracing of their declaration */
	        if (j == TRACE_CUSTOM)
	          ltt_set_bit(TRACE_NEW_EVENT, &(pmOptions->EventMask));

		break;
		}
	      }
	    }

	  /* Did he specify an unknown option */
	  if(j > TRACE_MAX_EVENTS)
	    {
	    printf("Option -e or -E accompanied by unknown event %s \n", &(pmArgv[i][2]));
	    exit(1);
	    }
	  break;

	/* Trace the CPUID */
	case 'c':
	case 'C':
	  /* Set the required information */
	  pmOptions->ConfigCPUID = TRUE;
	  break;

	/* Trace only one PID */
	case 'P':
	  /* Is there enough to specify a PID */
	  if(lOptLen < 3)
	    {
	    printf("Option -P needs a PID \n");
	    exit(1);
	    }

	  /* Set the required information */
	  pmOptions->ConfigPID = TRUE;
	  pmOptions->PID = strtol(&(pmArgv[i][2]), &pEnd, 10);
	  if(*pEnd != '\0')
	    {
	    printf("Invalid pid `%s' \n", &pmArgv[i][2]);
	    exit(1);
	    }
	  break;

	/* Trace only one process group */
	case 'G':
	  /* Is there enough to specify a process group */
	  if(lOptLen < 3)
	    {
	    printf("Option -G needs a process group \n");
	    exit(1);
	    }

	  /* Set the required information */
	  pmOptions->ConfigPGRP = TRUE;
	  pmOptions->PGRP = strtol(&(pmArgv[i][2]), &pEnd, 10);
	  if(*pEnd != '\0')
	    {
	    printf("Invalid process group `%s' \n", &pmArgv[i][2]);
	    exit(1);
	    }
	  break;

	/* Trace only one GID */
	case 'g':
	  /* Is there enough to specify a GID */
	  if(lOptLen < 3)
	    {
	    printf("Option -g needs a GID \n");
	    exit(1);
	    }

	  /* Set the required information */
	  pmOptions->ConfigGID = TRUE;
	  pmOptions->GID = strtol(&(pmArgv[i][2]), &pEnd, 10);
	  if(*pEnd != '\0')
	    {
	    printf("Invalid GID `%s' \n", &pmArgv[i][2]);
	    exit(1);
	    }
	  break;

	/* Modify current trace mask */
	case 'm':
	case 'M':
	  pmOptions->ModifyTraceMask = TRUE;
	  break;

	/* Use old timestamps (gettimeofday deltas)*/
	case 'o':
	  pmOptions->UseTSC = FALSE;
	  break;

	/* Print current trace mask */
	case 'p':
	  pmOptions->PrintTraceMask = TRUE;
	  break;

	/* Trace only one UID */
	case 'u':
	case 'U':
	  /* Is there enough to specify a UID */
	  if(lOptLen < 3)
	    {
	    printf("Option -u or -U needs a UID \n");
	    exit(1);
	    }

	  /* Set the required information */
	  pmOptions->ConfigUID = TRUE;
	  pmOptions->UID = strtol(&(pmArgv[i][2]), &pEnd, 10);
	  if(*pEnd != '\0')
	    {
	    printf("Invalid UID `%s' \n", &pmArgv[i][2]);
	    exit(1);
	    }
	  break;

	/* Syscall properties */
	case 's':
	case 'S':
	  /* Is the option long enough to tell us more about the syscall setting */
	  if(lOptLen < 3)
	    {
	    printf("Option %d doesn't specify what syscall property to set \n", i);
	    exit(1);
	    }

	  /* Depending on the syscall properties */
	  switch(pmArgv[i][2])
	    {
	    /* Use depth */
	    case 'd':
	    case 'D':
	      /* Is the option long enough to tell us more about the syscall setting */
	      if(lOptLen < 4)
		{
		printf("Option %d doesn't specify the depth on which to fetch the EIP for a syscall \n", i);
		exit(1);
		}

	      /* We are fetching the eip by depth */
	      pmOptions->ConfigSyscallDepth = TRUE;
	      pmOptions->ConfigSyscallBound = FALSE;

	      /* Read the depth */
	      pmOptions->SyscallDepth = strtol(&(pmArgv[i][3]), &pEnd, 10);
	      if(*pEnd != '\0')
		{
		printf("Invalid depth `%s' \n", &pmArgv[i][3]);
		exit(1);
		}
	      break;

	    /* Specify lower limit */
	    case 'l':
	    case 'L':
	      /* Is the option long enough to tell us more about the syscall setting */
	      if(lOptLen < 4)
		{
		printf("Option %d doesn't specify the lower bound for fetch of the EIP for a syscall \n", i);
		exit(1);
		}

	      /* Was the upper bound set */
	      if(pmOptions->UpperBound == 0)
		{
		printf("Must specify upper bound before lower bound or upper bound is 0 \n");
		exit(1);
		}

	      /* We are using syscall bounds */
	      pmOptions->ConfigSyscallBound = TRUE;
	      pmOptions->ConfigSyscallDepth = FALSE;

	      /* Read the upper bound (read as 0xffffffff) */
	      pmOptions->LowerBound = strtoul(&(pmArgv[i][3]), (char**) NULL, 16);
	      break;

	    /* Specify upper limit */
	    case 'u':
	    case 'U':
	      /* Is the option long enough to tell us more about the syscall setting */
	      if(lOptLen < 4)
		{
		printf("Option %d doesn't specify the upper bound for fetch of the EIP for a syscall \n", i);
		exit(1);
		}

	      /* Read the upper bound (read as 0xffffffff) */
	      pmOptions->UpperBound = strtoul(&(pmArgv[i][3]), (char**) NULL, 16);
	      break;
	    
	    default:
	      /* I would really like to do this for you, but I wasn't taught how to! */
	      printf("Unknown option : -%c%c \n", pmArgv[i][1], pmArgv[i][2]);
	      exit(1);
	    }
	  break;

	/* Specify the time for which the daemon should run */
	case 't':
	case 'T':
	  /* Is the option long enough to tell us more about the time setting */
	  if(lOptLen < 3)
	    {
	    printf("Option %d doesn't specify what time property to set \n", i);
	    exit(1);
	    }

	  /* Depending on the time properties */
	  switch(pmArgv[i][2])
	    {
	    /* Set seconds */
	    case 's':
	    case 'S':
	      /* Is the option long enough to tell us more about the time setting */
	      if(lOptLen < 4)
		{
		printf("Option %d doesn't specify the number of seconds to run \n", i);
		exit(1);
		}

	      /* We are runing for a limited time only (like all good rebates ....) */
	      pmOptions->ConfigTime = TRUE;

	      /* Read the number of seconds */
	      pmOptions->Time.tv_sec = strtol(&(pmArgv[i][3]), &pEnd, 10);
	      if(*pEnd != '\0')
		{
		printf("Invalid number of seconds `%s' \n", &pmArgv[i][3]);
		exit(1);
		}
	      break;

	    /* Set microseconds */
	    case 'u':
	    case 'U':
	      /* Is the option long enough to tell us more about the time setting */
	      if(lOptLen < 4)
		{
		printf("Option %d doesn't specify the number of microseconds to run \n", i);
		exit(1);
		}

	      /* We are runing for a limited time only (like all good rebates ....) */
	      pmOptions->ConfigTime = TRUE;

	      /* Read the number of microseconds */
	      pmOptions->Time.tv_usec = strtol(&(pmArgv[i][3]), &pEnd, 10);
	      if(*pEnd != '\0')
		{
		printf("Invalid number of microseconds `%s' \n", &pmArgv[i][3]);
		exit(1);
		}
	      break;
	    
	    default:
	      /* I would really like to do this for you, but I wasn't taught how to! */
	      printf("Unknown option : -%c%c \n", pmArgv[i][1], pmArgv[i][2]);
	      exit(1);
	    }
	  break;

	/* 'Flight recorder' mode, implies handle 1 for now, need to be able
	    to return handle for general case */
	case 'k':
	case 'K':
	  pmOptions->FlightRecorder = TRUE;
	  break;

	case 'v':
	case 'V':
	  pmOptions->Status = TRUE;
	  break;

	/* stop flight recorder */
	case 'q':
	case 'Q':
	  pmOptions->Stop = TRUE;
	  break;

	/* write snapshot to disk */
	case 'J':
	case 'j':
	  pmOptions->Snapshot = TRUE;
	  break;

	default :
	  /* I would really like to do this for you, but I wasn't taught how to! */
	  printf("Unknown option : -%c \n", pmArgv[i][1]);
	  exit(1);
	}
      }
    else /* Definitely a file name */
      {
	/* Do we already have trace file name */
	if(pmOptions->TraceFileName == NULL)
	  {
	  /* Copy the file name, leaving space (3 chars) for cpu id */
	  pmOptions->TraceFileName = (char*) malloc(lOptLen + 3 + 1);
	  strncpy(pmOptions->TraceFileName, pmArgv[i], lOptLen + 1);
	  }
	else
	  {
	  /* Do we already have the /proc file name */
	  if(pmOptions->ProcFileName == NULL)
	    {
	    /* Copy the file name */
	    pmOptions->ProcFileName = (char*) malloc(lOptLen + 1);
	    strncpy(pmOptions->ProcFileName, pmArgv[i], lOptLen + 1);
	    }
	  else
	    {
	    /* This guy doesn't know what he's doing! Tell him about it and get outa here ... */
	    printf("Wrong number of command line arguments \n");
	    printf("Unknown : %s \n", pmArgv[i]);
	    exit(1);
	    }
	  }
      }
    }

  /* Do we have the correct arguments */
  if((((pmOptions->TraceFileName == NULL)
    ||(pmOptions->ProcFileName  == NULL))
   &&(pmOptions->ModifyTraceMask == FALSE)
   &&(pmOptions->FlightRecorder == FALSE)
   &&(pmOptions->Status == FALSE)
   &&(pmOptions->Stop == FALSE)
   &&(pmOptions->Snapshot == FALSE)
   &&(pmOptions->PrintTraceMask == FALSE))
    ||((pmOptions->TraceFileName == NULL)
     &&(pmOptions->Snapshot == TRUE)))
    {
    /* Insufficient number of arguments */
    printf("Insufficient number of command line arguments \n");
    printf("Usage: TraceDaemon [[Options]] [TraceDumpFile] [ProcInfoFile] \n");
    exit(1);
    }
}

/******************************************************************
 * Function :
 *    ApplyConfig()
 * Description :
 *    Applies the configuration to the trace device and the daemon.
 * Parameters :
 *    pmFileID, the file ID of the device to operate on.
 *    pmOptions, the given options.
 * Return values :
 *    None.
 * History :
 *    K.Y. 21/10/99 : Initial typing.
 * Note :
 *    This function will exit if something goes wrong during the
 *    configuration.
 ******************************************************************/
void ApplyConfig(int pmHandle, options* pmOptions)
{
  struct itimerval      lTimer;     /* Timer used for daemon's execution */
  int                   lBitPos;    /* For buffer size calculations */
  int                   lHighestSet = 0; /* For buffer size calculations */
  int                   lMask;      /* For buffer size calculations */
  int                   lRC;        /* ioctl return code */
  FILE*                 pProcFile;  /* Proc file pointer */
  char                  lProcPath[PATH_MAX]; /* Procfile path buffer */
  char                  lRelayDirPath[PATH_MAX]; /* Relayfs dir path buffer */
  struct control_data   lControlData;

  /* Open the relayfs_path proc file for writing */
  sprintf(lProcPath, "%s/%d/relayfs_path", TRACE_PROC_ROOT, pmHandle);
  if((pProcFile = fopen(lProcPath, "w")) == NULL)
    {
    printf("open %s failed", lProcPath);
    exit(1);
    }
  
  /* Write trace/handle to relayfs_path proc file */
  sprintf(lRelayDirPath, "%s/%d", TRACE_RELAYFS_ROOT, pmHandle);
  if(fputs(lRelayDirPath, pProcFile) <= 0)
    { /* Error or EOF */
    printf("write %s failed", lProcPath);
    fclose(pProcFile);
    exit(1);
    }

  /* Need to flush, otherwise, there's no guarantee the path will be set 
     for when it's needed in the following steps. */
  if(fflush(pProcFile) != 0)
    { /* Error or EOF */
    printf("flush of %s failed", lProcPath);
    fclose(pProcFile);
    exit(1);
    }
  
  fclose(pProcFile);

  /* Do we set the tracing module to it's default configuration */
  if(pmOptions->ConfigDefault == TRUE)
    {
    /* Set the tracing module to it's default configuration */
    lControlData.tracer_handle = pmHandle;
    if(ioctl(gControlFile, TRACER_CONFIG_DEFAULT, &lControlData))
      {
      /* Don't go any further */
      printf("TraceDaemon: Unable to set tracer its default setting \n");
      exit(1);
      }
    printf("TraceDaemon: Tracer set to default config \n");
    }

  /* Set which locking scheme to use (must be set before n buffers) */
  lControlData.tracer_handle = pmHandle;
  lControlData.command_arg = (unsigned long)pmOptions->UseLocking;
  if(ioctl(gControlFile, TRACER_CONFIG_USE_LOCKING, &lControlData))
    {
    /* Don't go any further */
    printf("TraceDaemon: Lock-free tracing not supported on this platform (cmpxchg not available) - use the locking scheme (i.e. don't use the -f option) \n");
    exit(1);
    }
  printf("TraceDaemon: Using the %s tracing scheme \n", pmOptions->UseLocking ? "locking" : "lock-free");

  /* Set the timestamping method */
  lControlData.tracer_handle = pmHandle;
  lControlData.command_arg = (unsigned long)pmOptions->UseTSC;
  if(ioctl(gControlFile, TRACER_CONFIG_TIMESTAMP, &lControlData))
    {
    if(pmOptions->UseTSC == 1) 
      {
      printf("TraceDaemon: Platform doesn't support TSC timestamping. \n");
      pmOptions->UseTSC = 0;
      }
    } 

  /* Tell the user the timestamping method */
  printf("TraceDaemon: Using %s for timestamping \n", 
	 pmOptions->UseTSC ? "TSC" : "gettimeofday()");

  /* Set the number of buffers (must be set before size of buffers) */
  lControlData.tracer_handle = pmHandle;
  lControlData.command_arg = (unsigned long)gTracNBuffers;
  if(ioctl(gControlFile, TRACER_CONFIG_N_MEMORY_BUFFERS, &lControlData))
    {
    /* Don't go any further */
    printf("TraceDaemon: Unable to set number of data regions used for tracing.  Must be a power of 2 (default is 2).\n");
    exit(1);
    }
  printf("TraceDaemon: Configuring %d trace buffers \n", gTracNBuffers);

  /* Fix size of data buffers */
  if(pmOptions->UseLocking == FALSE)
    {
    /* Convert the given value to a power of two.  First, find the highest
       order bit set in the buffer size word. */
    for(lBitPos = 13; lBitPos < sizeof(gTracDataAreaSize)*8; lBitPos++)
      {
      lMask = 1L << lBitPos;
      if(gTracDataAreaSize & lMask) 
	lHighestSet = lBitPos;
      }

    /* Since gTracDataAreaSize is an int, need to exclude high bit */
    if(lHighestSet < (sizeof(gTracDataAreaSize)*8 - 1))
      gTracDataAreaSize = 1L << lHighestSet;
    else
      {
      printf("TraceDaemon: Unable to set data regions used for tracing.  The lockless scheme was being used, and the buffer size, converted to the next lowest power of 2, was too large\n");
      exit(1);
      }

    /* If new size is too small, return */
    if (gTracDataAreaSize < TRACER_LOCKLESS_MIN_BUF_SIZE)
      {
      printf("TraceDaemon: Unable to set data regions used for tracing.  The lockless scheme was being used, and the buffer size, converted to the next lowest power of 2, was too small (%d)\n", gTracDataAreaSize);
      exit(1);
      }

    /* If new size is still too large, return */
    if (gTracDataAreaSize > TRACER_LOCKLESS_MAX_BUF_SIZE)
      {
      printf("TraceDaemon: Unable to set data regions used for tracing.  The lockless scheme was being used, and the buffer size, converted to the next lowest power of 2, was too large (%d)\n", gTracDataAreaSize);
      exit(1);
      }
    }
  
  /* Set the memory area configuration */
  lControlData.tracer_handle = pmHandle;
  lControlData.command_arg = (unsigned long)gTracDataAreaSize;
  if((lRC = ioctl(gControlFile, TRACER_CONFIG_MEMORY_BUFFERS, &lControlData)))
    {
    /* Don't go any further */
    if(lRC == -EBUSY)
      printf("TraceDaemon: Unable to set data regions used for tracing - the tracer still has event writes pending from a previous trace \n");
    else if(pmOptions->UseLocking)
      printf("TraceDaemon: Unable to set data regions used for tracing \n");
    else
      printf("TraceDaemon: Unable to set data regions used for tracing.  If the -f option (lock-free scheme) is used, the  total buffer size (# buffers * buffer size) must not exceed 128M \n");
    exit(1);
    }

  /* Tell the user the size of the buffers */
  printf("TraceDaemon: Trace buffers are %d bytes \n", gTracDataAreaSize);

  if (get_arch_info())
    exit(1);

  /* At this point, the trace files exist and should be used for ioctls*/
  if (open_input())
    exit(1);

  /* Do we set the event mask */
  if(pmOptions->SpecifyEvents == TRUE)
    {
    /* Set the event mask */
    lControlData.tracer_handle = pmHandle;
    lControlData.command_arg = (unsigned long)&(pmOptions->EventMask);
    if(ioctl(gControlFile, TRACER_CONFIG_EVENTS, &lControlData))
      {

      /* Don't go any further */
      printf("TraceDaemon: Unable to set event mask \n");
      exit(1);
      }
    printf("TraceDaemon: Set event mask 0x%016llX \n", pmOptions->EventMask);
    }

  /* Do we set the details mask */
  if(pmOptions->SpecifyDetails == TRUE)
    {
    /* Set the details mask */
    lControlData.tracer_handle = pmHandle;
    lControlData.command_arg = (unsigned long)&(pmOptions->DetailsMask);
    if(ioctl(gControlFile, TRACER_CONFIG_DETAILS, &lControlData))
      {
      /* Don't go any further */
      printf("TraceDaemon: Unable to set details mask \n");
      exit(1);
      }
    printf("TraceDaemon: Set event details mask 0x%016llX \n", pmOptions->DetailsMask);
    }

  /* Do we set to log CPUID */
  if(pmOptions->ConfigCPUID == TRUE)
    {
    /* Set to log CPUID */
    lControlData.tracer_handle = pmHandle;
    if(ioctl(gControlFile, TRACER_CONFIG_CPUID, &lControlData))
      {
      /* Don't go any further */
      printf("TraceDaemon: Unable to set the tracer to record CPUID \n");
      exit(1);
      }
    printf("TraceDaemon: Tracking CPUID \n");
    }

  /* Do we set to track a given PID */
  if(pmOptions->ConfigPID == TRUE)
    {
    /* Set to log PID */
    lControlData.tracer_handle = pmHandle;
    lControlData.command_arg = (unsigned long)pmOptions->PID;
    if(ioctl(gControlFile, TRACER_CONFIG_PID, &lControlData))
      {
      /* Don't go any further */
      printf("TraceDaemon: Unable to set the tracer to track the given PID \n");
      exit(1);
      }
    printf("TraceDaemon: Tracking PID : %d \n", pmOptions->PID);
    }

  /* Do we set to track a given process group */
  if(pmOptions->ConfigPGRP == TRUE)
    {
    /* Set to log process group */
    lControlData.tracer_handle = pmHandle;
    lControlData.command_arg = (unsigned long)pmOptions->PGRP;
    if(ioctl(gControlFile, TRACER_CONFIG_PGRP, &lControlData))
      {
      /* Don't go any further */
      printf("TraceDaemon: Unable to set the tracer to track the given process group \n");
      exit(1);
      }
    printf("TraceDaemon: Tracking process group : %d \n", pmOptions->PGRP);
    }

  /* Do we set to track a given GID */
  if(pmOptions->ConfigGID == TRUE)
    {
    /* Set to log GID */
    lControlData.tracer_handle = pmHandle;
    lControlData.command_arg = (unsigned long)pmOptions->GID;
    if(ioctl(gControlFile, TRACER_CONFIG_GID, &lControlData))
      {
      /* Don't go any further */
      printf("TraceDaemon: Unable to set the tracer to track the given GID \n");
      exit(1);
      }
    printf("TraceDaemon: Tracking GID : %d \n", pmOptions->GID);
    }

  /* Do we set to track a given UID */
  if(pmOptions->ConfigUID == TRUE)
    {
    /* Set to log UID */
    lControlData.tracer_handle = pmHandle;
    lControlData.command_arg = (unsigned long)pmOptions->UID;
    if(ioctl(gControlFile, TRACER_CONFIG_UID, &lControlData))
      {
      /* Don't go any further */
      printf("TraceDaemon: Unable to set the tracer to track the given UID \n");
      exit(1);
      }
    printf("TraceDaemon: Tracking UID : %d \n", pmOptions->UID);
    }

  /* Do we set the syscall eip depth fetch */
  if(pmOptions->ConfigSyscallDepth == TRUE)
    {
    /* Set to fetch eip depth */
    lControlData.tracer_handle = pmHandle;
    lControlData.command_arg = (unsigned long)pmOptions->SyscallDepth;
    if(ioctl(gControlFile, TRACER_CONFIG_SYSCALL_EIP_DEPTH, &lControlData))
      {
      /* Don't go any further */
      printf("TraceDaemon: Unable to set syscall fetch depth \n");
      exit(1);
      }
    printf("TraceDaemon: Fetching eip for syscall on depth : %d \n", pmOptions->SyscallDepth);
    }

  /* Do we set the syscall eip bounds */
  if(pmOptions->ConfigSyscallBound == TRUE)
    {
    /* Set to eip according to bounds */
    lControlData.tracer_handle = pmHandle;
    lControlData.command_arg = (unsigned long)pmOptions->LowerBound;
    if(ioctl(gControlFile, TRACER_CONFIG_SYSCALL_EIP_LOWER, &lControlData))
      {
      /* Don't go any further */
      printf("TraceDaemon: Unable to set syscall fetch lower bound \n");
      exit(1);
      }
    /* Set to eip according to bounds */
    lControlData.tracer_handle = pmHandle;
    lControlData.command_arg = (unsigned long)pmOptions->UpperBound;
    if(ioctl(gControlFile, TRACER_CONFIG_SYSCALL_EIP_UPPER, &lControlData))
      {
      /* Don't go any further */
      printf("TraceDaemon: Unable to set syscall fetch upper bound \n");
      exit(1);
      }
    printf("TraceDaemon: Fetching eip for syscall between 0x%X and 0x%X \n",
	   pmOptions->LowerBound,
	   pmOptions->UpperBound);
    }

  /* Do we run the daemon only for a given time */
  if(pmOptions->ConfigTime == TRUE)
    {
    /* Set timer properties */
    memset(&(lTimer.it_interval), 0, sizeof(lTimer.it_interval));
    memcpy(&(lTimer.it_value), &(pmOptions->Time), sizeof(lTimer.it_value));

    /* Set an alarm to wake us up when we're done */
    if(setitimer(ITIMER_REAL, &lTimer, NULL) < 0)
      {
      /* Don't go any further */
	printf("TraceDaemon: Unable to set timer to regulate daemon's trace time \n");
	exit(1);
      }
    printf("TraceDaemon: Daemon will run for : (%ld, %ld) \n", pmOptions->Time.tv_sec, pmOptions->Time.tv_usec);
    }
}

/*******************************************************************
 * Function :
 *    ModifyEventMask()
 * Description :
 *    Modify the current event mask.
 * Parameters :
 *    pmOptions, the options structure.
 * Return values :
 *    NONE
 * History :
 *    K.Y., 15/04/2002, Initial typing.
 * Note :
 *******************************************************************/
void ModifyEventMask(options* pmOptions)
{
  /* Attach to the active tracing device */
  if(trace_attach() < 0)
    {
    printf("TraceDaemon: Unable to attach to trace device \n");
    exit(1);
    }  

  /* Set the event mask */
  if(trace_set_event_mask(pmOptions->EventMask) < 0)
    {
    printf("TraceDaemon: Unable to set new event mask \n");
    exit(1);
    }

  /* Print out the new event mask */
  printf("TraceDaemon: Set event mask 0x%016llX \n", pmOptions->EventMask);

  /* Detach from active tracing session */
  if(trace_detach() < 0)
    {
    printf("TraceDaemon: Unable to detach to trace device \n");
    exit(1);
    }  
}

/*******************************************************************
 * Function :
 *    PrintEventMask()
 * Description :
 *    Print the current event mask.
 * Parameters :
 *    NONE
 * Return values :
 *    NONE
 * History :
 *    K.Y., 15/04/2002, Initial typing.
 * Note :
 *******************************************************************/
void PrintEventMask(void)
{
  trace_event_mask    lEventMask;    /* The event mask */

  /* Attach to the active tracing device */
  if(trace_attach() < 0)
    {
    printf("TraceDaemon: Unable to attach to trace device \n");
    exit(1);
    }  

  /* Set the event mask */
  if(trace_get_event_mask(&lEventMask) < 0)
    {
    printf("TraceDaemon: Unable to get current event mask \n");
    exit(1);
    }

  /* Print out the new event mask */
  printf("TraceDaemon: Current event mask 0x%016llX \n", lEventMask);

  /* Detach from active tracing session */
  if(trace_detach() < 0)
    {
    printf("TraceDaemon: Unable to detach to trace device \n");
    exit(1);
    }  
}

/* Process buffers for the lockless scheme.  Called when poll() indicates
   that another buffer is possibly ready.  The pmForceAll parameter is
   used to force all buffers to be either written or discarded (used
   when the daemon signals tracing to stop).  The driver ensures that
   we can never get lapped.  The buffers_produced member of buffer
   control may be larger in reality than the snapshot we're looking at
   indicates, but since it can never lap buffers_consumed we don't
   care - we'll just pick up the new buffer(s) next time. */
static void ProcessBuffers(struct buffer_control * pmBufferControl, int pmForceAll)
{
  uint32_t lBuffersReady, lStartBuffer, lEndBuffer;
  uint32_t lBufferIndex, i;
  uint32_t lBuffersConsumed = 0;
  char * lBufPtr;
  uint32_t lNBuffers = pmBufferControl->n_buffers;
  uint32_t lBufSize = pmBufferControl->buf_size;
  int lForceFree = FALSE; /* Flag indicating we need to free a buffer */ 
  struct buffers_committed lBuffersCommitted;
  struct control_data   lControlData;

  /* The number of buffers that have been completely reserved, not
     necessarily filled, though. */
  lBuffersReady = pmBufferControl->buffers_produced - pmBufferControl->buffers_consumed;

  /* This code implements a modification of the K42 scheme.  It doesn't
     force buffers to be written or discarded until it's absolutely 
     necessary (i.e. the buffer would be lapped soon). Thus we read as
     many buffers as we can starting with the one following the last 
     consumed.  In the normal case, as soon as we try to write a buffer
     that's not exactly filled, we break out of the buffer-writing loop
     and wait until the next buffer switch signal to pick it up.  
     If the daemon has filled up all the buffers, or if we detect while
     processing the buffers that the driver is currently writing to the
     last free buffer, we force a buffer free (and possibly lose that 
     buffer's) contents.  After the trace has been stopped and we've 
     waited 10ms/per thread for any sleeping threads to finish any 
     pending event writes they may have in progress, we force all 
     buffers to be either written or discarded.  The signal/ioctl buffer
     switch protocol ensures that we'll never actually be lapped, at the
     cost of lost events in the driver and an increased complexity in 
     the slow path of the driver's trace_reserve() code. */

  /* If the next buffer switch would cause a lap (and loss of events
     in the driver), we need to force the next unconsumed buffer free. */
  if(lBuffersReady >= lNBuffers - 1)
    lForceFree = TRUE; /* Need to force the next buffer free */

  /* We start with the buffer following the last one consumed */
  lStartBuffer = pmBufferControl->buffers_consumed % lNBuffers;

  /* We end with the last buffer possibly ready */
  lEndBuffer = lStartBuffer + lBuffersReady;
  for(i = lStartBuffer; i < lEndBuffer; i++) 
    {
    lBufferIndex = i % lNBuffers;
    /* If this buffer is complete */
    if((gOptions->UseLocking == TRUE) ||
       pmBufferControl->buffer_complete[lBufferIndex]) 
      {
      lBufPtr = gTracDataArea[pmBufferControl->cpu_id]  
	+ lBufferIndex*lBufSize;

      /* Write the filled buffer to file */
      if(write(gTracDataFile[pmBufferControl->cpu_id], lBufPtr, lBufSize) < 0)
	{
	fprintf(stdout, "TraceDaemon: Unable to read from data space \n");
	exit(1);
	}
      lBuffersConsumed++;
      }
    else /* Buffer is incomplete. */
      {
      /* We can't wait for it any longer. Discard. */
      if(lForceFree || pmForceAll)
	{
	printf("TraceDaemon: WARNING!!!! \n");
	printf("TraceDaemon: INCOMPLETE EVENT BUFFER LOST (buffer size: %d, buffer index: %d) \n",
	       lBufSize, lBufferIndex);
	lBuffersConsumed++; /* Discard. */
	}
      else
	/* We still have time to wait for this buffer (and any
	   others following it) to complete, so we'll stop here 
	   for now. */
	break;
      }
    /* Only a first buffer would need to be forced free. */
    lForceFree = FALSE;
    }

  lBuffersCommitted.cpu_id = pmBufferControl->cpu_id;
  lBuffersCommitted.buffers_consumed = lBuffersConsumed;
  
  /* Tell the driver how many buffers we comitted */
  lControlData.tracer_handle = gTracHandle;
  lControlData.command_arg = (unsigned long)&lBuffersCommitted;
  if(ioctl(gControlFile, TRACER_DATA_COMITTED, &lControlData))
    {
      if(gWaitingForThreads == FALSE) /* This is end of trace signal */ 
	{
	  fprintf(stdout, "TraceDaemon: Unable to commit data \n");
	  exit(1);
	}
    }

  /* Are there still other cpus to process? */
  if(pmBufferControl->buffer_switches_pending != 0) 
    {
      int         lRetValue;
      struct buffer_control lBufferControl;

      lBufferControl.cpu_id = -1;
      lControlData.tracer_handle = gTracHandle;
      lControlData.command_arg = (unsigned long)&lBufferControl;
      lRetValue = ioctl(gControlFile, TRACER_GET_BUFFER_CONTROL, &lControlData);
      if(lRetValue != 0) 
	{
	  fprintf(stdout, "TraceDaemon: Unable to get buffer control data \n");
	  exit(1);
	}
      else
	{
	  if(lBufferControl.buffer_control_valid == TRUE)
	    /* Force all buffers to be written or discarded */
	    ProcessBuffers(&lBufferControl, pmForceAll);
	}
    }

  return;
}

/* Set a timer to wait 10 ms for each process currently running, to allow
   any sleeping threads in the process of writing events wake up and finish.
   If we weren't able to set the timer, return 1, otherwise 0. */
static int WaitForStragglers(void)
{
  struct sysinfo lSysInfo;
  struct itimerval lTimer;     /* Timer used for lockless waiting */

  /* We're now waiting for everyone to get a timeslice */
  gWaitingForThreads = TRUE;
  signal(SIGALRM, SigHandler);

  /* Set timer properties */
  memset(&(lTimer.it_interval), 0, sizeof(lTimer.it_interval));

  /* Wait at least until each thread has a chance to wake up and
     finish its event write. */
  if(sysinfo(&lSysInfo) == 0) 
    {
    /* Each thread gets 10,000 microseconds */
    long int lTotaluSecs = lSysInfo.procs * 10000;
    lTimer.it_value.tv_sec = lTotaluSecs/1000000;
    lTimer.it_value.tv_usec = lTotaluSecs - lTimer.it_value.tv_sec*1000000;

    /* Don't wait TOO long to exit, 10 secs max is plenty */ 
    if(lTimer.it_value.tv_sec > 10) 
      lTimer.it_value.tv_sec = 10;
    }
  else /* Couldn't get sysinfo for some reason, just wait a 
	  reasonable amount of time */
    {
    /* procs will be used in printf message - put something in */
    lSysInfo.procs = 0; 

    /* Wait a couple seconds if we don't have better info */
    lTimer.it_value.tv_sec = 2;
    lTimer.it_value.tv_usec = 0;
    }

  /* Set an alarm to wake us up when we're done */
  if(setitimer(ITIMER_REAL, &lTimer, NULL) < 0)
      return 1;
  else
    {
    printf("TraceDaemon: Daemon will wait for (%ld, %ld) to allow %u processes to finish writing events\n",
	   lTimer.it_value.tv_sec, lTimer.it_value.tv_usec, lSysInfo.procs);
    return 0;
    }
}

/* Set a timer to wait 10 ms for each process currently running, to allow
   any sleeping threads in the process of writing events wake up and finish.
   If we weren't able to set the timer, return 1, otherwise 0. */
#if 0 /* DEBUG - used to delay consumption and max out the driver buffers */
static int sTestingFullBuffers = 1;

/* DEBUG - simulate very busy daemon to test all driver buffers full */
void SimulateBusyDaemon(void)
{
  struct timeval lCurrentTime, lStartTime;
  int lWaitNSecs = 10;

  gettimeofday(&lStartTime, NULL);
  fprintf(stderr, "waiting %d secs to let all buffers fill\n", lWaitNSecs);

  /* This causes millions of events, but that's what we want */
  for(;;) 
    {
    gettimeofday(&lCurrentTime, NULL);
    if((lCurrentTime.tv_sec - lStartTime.tv_sec) >= lWaitNSecs) 
      break;
    }

  fprintf(stderr, "DONE waiting %d secs to let all buffers fill\n", lWaitNSecs);
}
#endif

/*******************************************************************
 * Function :
 *    SigHandler()
 * Description :
 *    The signal handling routine.
 * Parameters :
 *    pmSignalID, The ID of the signal that ocurred
 * Return values :
 *    NONE
 * History :
 *    K. Yaghmour, 05/99, Initial typing.
 * Note :
 *    This function will "exit()" if something is wrong with the
 *    command line options.
 *******************************************************************/
void SigHandler(int pmSignalID)
{
  int                   lEventsLost[MAX_CPUS];   /* Number of events lost */
  char*                 lTempDataArea;           /* Temp pointer to data area */
  int                   lRetValue;               /* Return val of buffer control ioctl */
  struct buffer_control lBufferControl;          /* Filled in by buffer control ioctl */
  struct control_data   lControlData;
  
  int         i, lLostFlag;
  
  /* Depending on the received signal */
 
  switch(pmSignalID)
    {
    default :
      fprintf(stdout, "TraceDaemon: Received unknown signal ... stopping \n");
    case SIGTERM :
    case SIGALRM :
      /* When the end of trace timer times out, we set a flag and another
	 timer in order to let all existing threads get their timeslices and
	 write any pending events.  We don't want to rely on the details of 
	 how the scheduler works, so we assume 10ms per thread will be more
	 than enough time to cover everyone.  We thus can get a SIGALRM for 
	 two reasons - end-of-trace timer times out and wait-for-threads
	 timer times out. */
      /* Get the current value of driver's buffer_control struct */
      lBufferControl.cpu_id = -1;
      lControlData.tracer_handle = gTracHandle;
      lControlData.command_arg = (unsigned long)&lBufferControl;
      lRetValue = ioctl(gControlFile, TRACER_GET_BUFFER_CONTROL, &lControlData);
      if(lRetValue != 0) 
	{
	fprintf(stdout, "TraceDaemon: Unable to get buffer control data \n");
	exit(1);
	}
      if(gWaitingForThreads == FALSE) /* This is end of trace signal */ 
	{
	/* Stop the driver - no new events can then be logged */
	lControlData.tracer_handle = gTracHandle;
	ioctl(gControlFile, TRACER_STOP, &lControlData);
	if(WaitForStragglers() != 0)
	  /* OK, then we'll take our chances and not wait.  Force all 
	     buffers to be written or discarded now. */
	  ProcessBuffers(&lBufferControl, TRUE);
	else
	  return; /* We'll be back */ 
	}
      else /* This is the end of our wait */
	/* Force all buffers to be written or discarded */
	ProcessBuffers(&lBufferControl, TRUE);
    }

  /* Read the number of events lost */
  lLostFlag = FALSE;
  for(i = 0; i < gNumCPUs; i++)
    {
    lControlData.tracer_handle = gTracHandle;
    lControlData.command_arg = (unsigned long)i;
    if((lEventsLost[i] = ioctl(gControlFile, TRACER_GET_EVENTS_LOST, &lControlData)) != 0)
      lLostFlag = TRUE;
    }

  if(lLostFlag == TRUE) 
    {
    printf("TraceDaemon: WARNING!!!! \n");
    for(i = 0; i < gNumCPUs; i++)
      /* Where there any events lost */
      if(lEventsLost[i] > 0)
	printf("TraceDaemon: %d EVENTS HAVE BEEN LOST FOR CPU %d\n", lEventsLost[i], i);
    printf("TraceDaemon: Check the size of the buffers \n");
    printf("TraceDaemon: Current buffer size %d \n", gTracDataAreaSize);
    }

end_of_trace:

  /* OK we're out of here */
  for(i = 0; i < gNumCPUs; i++) 
    close(gTracDataFile[i]);
      
  /* Tell the user that we're done */
  fprintf(stdout, "\nTraceDaemon: End of tracing \n");
  _exit(0);
}

/*******************************************************************
 * Function :
 *    MapProc()
 * Description :
 *    Function maping out /proc.
 * Parameters :
 *    pmProcFile, file where the information is to be written
 * Return values :
 *    NONE
 * History :
 *    K. Yaghmour, 05/99, Initial typing.
 *******************************************************************/
void MapProc(int pmProcFile)
{
  int             i;                               /* Generic index */
  int             lIRQ;                            /* IRQ ID */
  int             lPID;                            /* Process' PID */
  int             lPPID;                           /* Parent's PID */
  int             lNbCPU;                          /* Number of CPUs */
  int             lSizeRead;                       /* Quantity of data read from file */
  int             lStatusFile;                     /* File containd process status */
  char            lFileName[TD_FILE_NAME_STRLEN];  /* A string containing a complete file name */
  char            lDataBuf[TD_STATUS_STRLEN];      /* Status file content */
  char            lName[TD_PROCES_NAME_STRLEN];    /* Name of the process */
  char*           lString;                         /* Generic string */
  DIR*            pDir;                            /* Directory pointer */
  FILE*           pIntFile;                        /* Int pile pointer */
  struct dirent*  pDirEntry;                       /* Directory entry */

  /* Open /proc */
  if((pDir = opendir("/proc")) == NULL)
    {
    printf("\nYour system doesn't support /proc \n");
    exit(1);
    }

  /* Read the directory through */
  while((pDirEntry = readdir(pDir)) != NULL)
    {
    /* Is this a directory entry describing a PID */
    if(isdigit(pDirEntry->d_name[0]))
      {
      /* Get the PID in number form */
      lPID = atoi(pDirEntry->d_name);

      /* Open the status file */
      snprintf(lFileName, TD_FILE_NAME_STRLEN, "/proc/%d/status", lPID);
      if((lStatusFile = open(lFileName, O_RDONLY, 0)) < 0)
	continue;

      /* Read a little bit */
      if((lSizeRead = read(lStatusFile, lDataBuf, TD_STATUS_STRLEN)) == 0)
	continue;

      /* Close the file */
      close(lStatusFile);
      
      /* Find Name and Parent PID */
      lString  = strstr(lDataBuf, "Name");
      sscanf(lString, "Name: %s", lName);
      lString  = strstr(lDataBuf, "PPid");
      sscanf(lString, "PPid: %d", &lPPID);

      /* Print out the precious information */
      snprintf(lDataBuf, TD_STATUS_STRLEN, "PID: %d; PPID: %d; NAME: %s\n", lPID, lPPID, lName);
      write(pmProcFile, lDataBuf, strlen(lDataBuf));
      }
    }

  /* Close the directory */
  closedir(pDir);

  /* Open the interrupts information file */
  if((pIntFile = fopen("/proc/interrupts", "r")) == NULL)
    {
    printf("open /proc/interrupts failed");
    return;
    }
  
  /* The first line is the column header, usually just CPU0, CPU1, etc.
     Count the number of CPUs based on "CPUn" occurences.  I didn't
     see anything else in /proc that would have been any easier
     and I need to be in this pseudofile anyway... */
  if(fgets(lDataBuf, 200, pIntFile) <= 0)
    { /* Error or EOF */
    printf("read /proc/interrupts failed");
    fclose(pIntFile);
    return;
    }

  /* Initialize the number of CPUs */
  lNbCPU = 0;

  /* Print header for CPU 0 */
  snprintf(lName, TD_PROCES_NAME_STRLEN, "CPU%d", lNbCPU);

  /* Find entry for CPU 0 */
  lString = strstr(lDataBuf, lName);

  /* Go through rest of string */
  while (lString)
    {
    /* Look for next CPU info */
    lNbCPU++;
    snprintf(lName, TD_PROCES_NAME_STRLEN, "CPU%d", lNbCPU);
    lString = strstr(lString, lName);
    }

  /* How many CPUs did we find */
  if (lNbCPU <= 0 || lNbCPU > 128)
    {  
    /* Missing or too weird */
    printf("Can't determine CPU count /proc/interrupts\n");
    fclose(pIntFile);
    return;
    }

  /* At the beginning of each valid IRQ line is a digit.  Scan for 
     it, skip through the CPU hit fields, skip the IRQ supplier (or 
     whatever that field is) and grab the rest.  This assumes the 
     IRQ supplier has no white space in the middle.  Outer fscanf
     stops on EOF, "NMI", "ERR", or other non-numeric chars. */

  /* Go to the next interrupt description in the file */
  while(fscanf(pIntFile, "%d:", &lIRQ) == 1)
    {
    /* Is this a valid  */
    if(lIRQ > 256)
      {
      printf("IRQ %d is out of range in /proc/interrutps\n", lIRQ);
      fclose(pIntFile);
      return;
      }

    /* For all CPUs */
    for(i = 0; i < lNbCPU; i++)
      /* Read one int per CPU */
      if(fscanf(pIntFile, "%s", lDataBuf) != 1)
	{
	printf("Bad INT count field seen in /proc/interrutps\n");
	fclose(pIntFile);
	return;
	}
	    
    /* The trailing space is crucial in the next format because
       it gets rid of all whitespace after the supplier field.
       That sets it up for the final fgets() */
     
    /* Read a whitespace */
    if(fscanf(pIntFile, "%s ", lDataBuf) != 1)
      {
      printf("Bad INT source field seen in /proc/interrutps\n");
      fclose(pIntFile);
      return;
      }

    /* Get the rest of the line */
    fgets(lName, 100, pIntFile);

    /* Format string describing the IRQ */
    snprintf(lDataBuf, TD_STATUS_STRLEN, "IRQ: %d; NAME: %s", lIRQ, lName);

    /* Write string to proc file */
    write(pmProcFile, lDataBuf, strlen(lDataBuf));
    }

  /* Close the interrupt information file */
  fclose(pIntFile);
}

/**
 *	read_all_ready - read and save all ready sub-buffers
 */
static void read_all_ready(void)
{
  int         i, lRetValue;
  struct buffer_control lBufferControl; /* Filled in by buffer control ioctl */
  struct control_data   lControlData;

#if 0 /* DEBUG - simulate very busy daemon to test all driver buffers full */
  if(sTestingFullBuffers) 
    {
    SimulateBusyDaemon();
    sTestingFullBuffers = 0;
    }
#endif

  /* Get the current value of driver's buffer_control struct */
  lBufferControl.cpu_id = -1;
  lControlData.tracer_handle = gTracHandle;
  lControlData.command_arg = (unsigned long)&lBufferControl;
  lRetValue = ioctl(gControlFile, TRACER_GET_BUFFER_CONTROL, &lControlData);
  if(lRetValue != 0) 
    {
    fprintf(stdout, "TraceDaemon: Unable to get buffer control data \n");
    exit(1);
    }

  /* There weren't any buffers available after all */
  if(lBufferControl.buffer_control_valid == FALSE)
    return;

  /* Are we currently busy */
  if(gBusy == TRUE)
    {
    if(gOptions->UseLocking == FALSE)
      return; /* No big deal, we can afford to miss a signal */
    else
      {
      fprintf(stdout, "TraceDaemon: Received poll while busy ... \n");
      goto end_of_trace;
      }
    }
    
  /* We are busy */
  gBusy = TRUE;

  ProcessBuffers(&lBufferControl, FALSE);

  /* We arent busy anymore */
  gBusy = FALSE;

  /* We're done */
  return;
      
end_of_trace:

  /* OK we're out of here */
  for(i = 0; i < gNumCPUs; i++) 
    close(gTracDataFile[i]);
      
  /* Tell the user that we're done */
  fprintf(stdout, "\nTraceDaemon: End of tracing \n");
  _exit(0);
}

/**
 *	get_arch_info - common routine for getting arch info
 */
int get_arch_info(void)
{
  struct ltt_arch_info lArchInfo; /* arch info from driver */
  struct control_data   lControlData;

  lControlData.tracer_handle = gTracHandle;
  lControlData.command_arg = (unsigned long)&lArchInfo;

  if(ioctl(gControlFile, TRACER_GET_ARCH_INFO, &lControlData)) 
    {
    printf("TraceDaemon: Unable to get arch info from tracer \n");
    return -1;
    }

  gNumCPUs = lArchInfo.n_cpus;
  gPageShift = lArchInfo.page_shift;
  gPageSize = 1UL << gPageShift;
  gPageMask = ~(gPageSize - 1);

  printf("TraceDaemon: Tracer is configured for %d CPUS \n", gNumCPUs);

  return 0;
}

/**
 *	open_input - common routine for opening and mapping trace files
 */
int open_input(void)
{
  int              i;  /* Scratch vars */
  char             lRelayDirPath[PATH_MAX]; /* Relayfs dir path buffer */
  int              lTotalDataAreaSize; /* Total buffer memory in driver */

  /* We found relayfs mount point previously */
  printf("TraceDaemon: relayfs mount point: %s\n", gRelayfsMntPt);

  for(i = 0; i < gNumCPUs; i++) 
    {
    /* trace files are: relayfs mnt pt/trace/handle/cpu0-cpuX */
    sprintf(lRelayDirPath, "%s/%s/%u/cpu%d", gRelayfsMntPt, TRACE_RELAYFS_ROOT, gTracHandle, i);
    if((gTracRelayFile[i] = open(lRelayDirPath, O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP)) < 0)
      {
      /* File ain't good */
      printf("TraceDaemon: Unable to open relay file %s (errno = %d) \n", lRelayDirPath, errno);
      return -1;
      }
    }

  /* The output file is ready */
  printf("TraceDaemon: Relay file(s) ready \n");

  /* Need to match driver sizes i.e. include page size calculation */
  lTotalDataAreaSize = FIX_SIZE(gTracDataAreaSize * gTracNBuffers);

  gTotalDataAreaSize = lTotalDataAreaSize;

  /* Map each of the per-CPU buffers into our address space*/
  for(i = 0; i < gNumCPUs; i++)
    {
    gTracDataArea[i] = mmap(NULL, (size_t)lTotalDataAreaSize, PROT_READ | PROT_WRITE, MAP_PRIVATE, gTracRelayFile[i], 0);

    if(gTracDataArea[i] == MAP_FAILED)
      {
      printf("TraceDaemon: Unable to map tracer's data area into daemon's address space, error = %s \n", strerror(errno));
      for(i = 0; i < gNumCPUs; i++)
	close(gTracRelayFile[i]);
      return -1;
      }
    }

  return 0;
}

/**
 *	open_input - common routine for opening output files
 */
int open_output(void)
{
  int              i, len, ready;  /* Scratch vars for filename formatting */
  char             *extpos, *dirpos; /* Scratch vars for filename formatting */
  char             lCPUIDStr[3]; /* Scratch area for cpuid formatting */

  /* Open the file(s) to which the tracing data should be written */
  len = strlen(gOptions->TraceFileName);
  if(gNumCPUs > 1) 
    {
    extpos = strrchr(gOptions->TraceFileName, '.');
    dirpos = strrchr(gOptions->TraceFileName, '/');
    if(extpos && (dirpos < extpos)) 
      {
      for(i = len - (extpos-gOptions->TraceFileName); i >= 0; i--)
	*(extpos + i + sizeof(lCPUIDStr)) = *(extpos + i);
      len = extpos - gOptions->TraceFileName;
      }
    }
  
  for(i = 0; i < gNumCPUs; i++) 
    {
    /* Only if there is more than 1 cpu do we add the cpu id */
    if(gNumCPUs > 1)
      {
      sprintf(lCPUIDStr, "%03d", i);
      strncpy(gOptions->TraceFileName + len, lCPUIDStr, sizeof(lCPUIDStr));
      }
    if((gTracDataFile[i] = open(gOptions->TraceFileName, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0)
      {
      /* File ain't good */
      printf("\nUnable to open output file %s (errno = %d) \n", gOptions->TraceFileName, errno);
      return -1;
      }
    }

  /* The output file is ready */
  printf("TraceDaemon: Output file(s) ready \n");

  return 0;
}

/**
 *	close_output - common routine for closing output files
 */
void close_output(void)
{
  int i;

  for(i = 0; i < gNumCPUs; i++) 
    close(gTracDataFile[i]);
}

/**
 *	close_input - common routine for closing input files
 */
void close_input(void)
{
  int i;

  for(i = 0; i < gNumCPUs; i++)
    close(gTracRelayFile[i]);
}


/**
 *	write_data - write data directly into destination buffer
 */
#define write_direct(DEST, SRC, SIZE) \
do\
{\
   memcpy(DEST, SRC, SIZE);\
   DEST += SIZE;\
} while (0);

#define TRACER_LAST_EVENT_SIZE   (sizeof(uint8_t) \
				  + sizeof(uint8_t) \
				  + sizeof(uint32_t) \
				  + sizeof(trace_buffer_end) \
				  + sizeof(uint16_t) \
				  + sizeof(uint32_t))

/**
 *	get_first_buf_start - get first sub-buffer's start buffer event data
 */
static void get_first_buf_start(int pmCPUID, struct buffer_control *pmBufferControl, uint32_t *pmTimeDelta, trace_buffer_start *pmStartBufferEvent)
{
  uint32_t lStartBuffer, lEndBuffer;
  char * lBufPtr;
  uint32_t lNBuffers = pmBufferControl->n_buffers;
  uint32_t lBufSize = pmBufferControl->buf_size;
  uint32_t lCurBuf;
  
  lCurBuf = pmBufferControl->cur_idx / lBufSize;

  if(pmBufferControl->buffers_produced >= lNBuffers)
    {
    /* We start with the buffer following the current one */
      lStartBuffer = lCurBuf + 1 >= lNBuffers ? 0 : lCurBuf + 1;
      lEndBuffer = lStartBuffer + lNBuffers - 1;
    }
  else
    {
    lStartBuffer = 0;
    lEndBuffer = lCurBuf;
    }

  lBufPtr = gTracDataArea[pmCPUID] + lStartBuffer*lBufSize;
  lBufPtr += sizeof(uint8_t);
  *pmTimeDelta = *((uint32_t *)lBufPtr);
  lBufPtr += sizeof(uint32_t);
  memcpy(pmStartBufferEvent, lBufPtr, sizeof(trace_buffer_start));
}

/**
 *	dump_start_sub_buffer - create and dump a phony first sub-buffer,
 *	since a flight recorder trace doesn't have a real one.
 */
static void dump_start_sub_buffer(int pmCPUID, struct buffer_control *pmBufferControl, trace_start *pmTraceStart)
{
  char * lBufPtr;
  uint32_t lNBuffers;
  uint32_t lBufSize;
  uint32_t lCurBuf;
  int lRetValue = 0;
  uint8_t lCPUID;
  uint8_t lEventID;
  uint32_t lTimeDelta;
  uint16_t lDataSize;
  uint32_t lSizeLost;
  char *lTempSubBuffer, *lCurWritePos;
  trace_buffer_start lStartBufferEvent;
  trace_buffer_end lEndBufferEvent;
  struct control_data   lControlData;

  pmBufferControl->cpu_id = pmCPUID;
  lControlData.tracer_handle = gTracHandle;
  lControlData.command_arg = (unsigned long)pmBufferControl;
  lRetValue = ioctl(gControlFile, TRACER_GET_BUFFER_CONTROL, &lControlData);

  if(lRetValue != 0 || pmBufferControl->buffer_control_valid == FALSE) 
    {
    fprintf(stdout, "TraceDaemon: Unable to get buffer control data \n");
    exit(1);
    }

  lNBuffers = pmBufferControl->n_buffers;
  lBufSize = pmBufferControl->buf_size;

  lTempSubBuffer = (char *)malloc(lBufSize);
  if(!lTempSubBuffer)
    {
    fprintf(stdout, "TraceDaemon: Unable to allocate temporary sub-buffer \n");
    exit(1);
    }

  memset(lTempSubBuffer, 0, lBufSize);

  lCurWritePos = lTempSubBuffer;

  get_first_buf_start(pmCPUID, pmBufferControl, &lTimeDelta, &lStartBufferEvent);

  lStartBufferEvent.Time.tv_sec -= 1;
  lStartBufferEvent.TSC -= 1000;

  lEventID = TRACE_BUFFER_START;
  write_direct(lCurWritePos, &lEventID, sizeof(lEventID));
  
  lTimeDelta -= 1000;

  write_direct(lCurWritePos, &lTimeDelta, sizeof(lTimeDelta));

  write_direct(lCurWritePos, &lStartBufferEvent, sizeof(trace_buffer_start));

  lDataSize = sizeof(lEventID)
	  + sizeof(lTimeDelta)
	  + sizeof(lStartBufferEvent)
	  + sizeof(lDataSize);
  write_direct(lCurWritePos, &lDataSize, sizeof(lDataSize));
  
  lEventID = TRACE_START;
  write_direct(lCurWritePos, &lEventID, sizeof(lEventID));
  
  lTimeDelta = 0;
  write_direct(lCurWritePos, &lTimeDelta, sizeof(lTimeDelta));

  write_direct(lCurWritePos, pmTraceStart, sizeof(trace_start));

  lDataSize = sizeof(lEventID)
	  + sizeof(lTimeDelta)
	  + sizeof(trace_start)
	  + sizeof(lDataSize);
  write_direct(lCurWritePos, &lDataSize, sizeof(lDataSize));

  lSizeLost = lTempSubBuffer + lBufSize - lCurWritePos;

  lEndBufferEvent.Time.tv_sec = 0;
  lEndBufferEvent.Time.tv_usec = 0;
  lEndBufferEvent.TSC = 0;

  if(pmTraceStart->LogCPUID)
    {
    lCPUID = (uint8_t)pmCPUID;
    write_direct(lCurWritePos, &lCPUID, sizeof(lCPUID));
    }

  lEventID = TRACE_BUFFER_END;
  write_direct(lCurWritePos, &lEventID, sizeof(lEventID));
  
  lTimeDelta = 0;
  write_direct(lCurWritePos, &lTimeDelta, sizeof(lTimeDelta));

  write_direct(lCurWritePos, &lEndBufferEvent, sizeof(trace_buffer_end));

  lDataSize = sizeof(lEventID)
	  + sizeof(lTimeDelta)
	  + sizeof(lEndBufferEvent)
	  + sizeof(lDataSize);
  write_direct(lCurWritePos, &lDataSize, sizeof(lDataSize));

  *((uint32_t *) (lTempSubBuffer + lBufSize - sizeof(lSizeLost))) = lSizeLost;

  /* Write the filled buffer to file */
  if(write(gTracDataFile[pmCPUID], lTempSubBuffer, lBufSize) < 0)
    {
    fprintf(stdout, "TraceDaemon: Unable to write first sub-buffer \n");
    exit(1);
    }
}

/**
 *	dump_sub_buffers - dump all complete flight recorder sub-buffers
 */
static void dump_sub_buffers(int pmCPUID, struct buffer_control *pmBufferControl, int use_locking)
{
  uint32_t lStartBuffer, lEndBuffer;
  uint32_t lBufferIndex, i;
  char * lBufPtr;
  char * lBufIDPtr;
  uint32_t lNBuffers = pmBufferControl->n_buffers;
  uint32_t lBufSize = pmBufferControl->buf_size;
  uint32_t lCurBuf;

  lCurBuf = pmBufferControl->cur_idx / lBufSize;
  
  if(pmBufferControl->buffers_produced >= lNBuffers)
    {
    /* We start with the buffer following the current one */
    lStartBuffer = lCurBuf + 1 >= lNBuffers ? 0 : lCurBuf + 1;
    lEndBuffer = lStartBuffer + lNBuffers - 1;
    }
  else
    {
    lStartBuffer = 0;
    lEndBuffer = lCurBuf;
    }
  
  for(i = lStartBuffer; i < lEndBuffer; i++) 
    {
    lBufferIndex = i % lNBuffers;
	    
    /* If this buffer is complete */
    if(use_locking == TRUE ||
       pmBufferControl->buffer_complete[lBufferIndex]) 
      {
      lBufPtr = gTracDataArea[pmCPUID] + lBufferIndex*lBufSize;
      lBufIDPtr = lBufPtr;
      lBufIDPtr += sizeof(uint8_t);
      lBufIDPtr += sizeof(uint32_t);
      lBufIDPtr += sizeof(struct timeval);
      lBufIDPtr += sizeof(uint32_t);
      *lBufIDPtr += 1;

      /* Write the filled buffer to file */
      if(write(gTracDataFile[pmCPUID], lBufPtr, lBufSize) < 0)
	{
	fprintf(stdout, "TraceDaemon: Unable to read from data space \n");
	exit(1);
	}
      }
    else /* Buffer is incomplete. */
      {
      printf("TraceDaemon: WARNING!!!! \n");
      printf("TraceDaemon: INCOMPLETE EVENT BUFFER LOST (buffer size: %d, buffer index: %d) \n", lBufSize, lBufferIndex);
      }
    }
}

/**
 *	dump_end_sub_buffer - dump the final (currently being written)
 *	sub-buffer.
 */
static void dump_end_sub_buffer(int pmCPUID, struct buffer_control *pmBufferControl, int pmLogCPUID)
{
  char * lBufPtr;
  char * lBufIDPtr;
  uint32_t lNBuffers = pmBufferControl->n_buffers;
  uint32_t lBufSize = pmBufferControl->buf_size;
  uint32_t lCurIdx = pmBufferControl->cur_idx;
  char *lTempSubBuffer, *lCurWritePos;
  trace_buffer_end lEndBufferEvent;
  uint32_t lCurBuf;
  uint8_t lCPUID;
  uint8_t lEventID;
  uint32_t lTimeDelta;
  uint16_t lDataSize;
  uint32_t lSizeLost;

  lTempSubBuffer = (char *)malloc(lBufSize);
  if(!lTempSubBuffer) 
    {
    fprintf(stdout, "TraceDaemon: Unable to allocate temporary sub-buffer \n");
    exit(1);
    }
  memset(lTempSubBuffer, 0, lBufSize);

  lCurBuf = lCurIdx / lBufSize;

  lBufPtr = gTracDataArea[pmCPUID] + lCurBuf*lBufSize;

  lBufIDPtr = lBufPtr;
  lBufIDPtr += sizeof(uint8_t);
  lBufIDPtr += sizeof(uint32_t);
  lBufIDPtr += sizeof(struct timeval);
  lBufIDPtr += sizeof(uint32_t);
  *lBufIDPtr += 1;

  memcpy(lTempSubBuffer, lBufPtr, lCurIdx % lBufSize);

  lCurWritePos = lTempSubBuffer + lCurIdx % lBufSize;

  lSizeLost = lTempSubBuffer + lBufSize - lCurWritePos;

  lEndBufferEvent.Time.tv_sec = 0;
  lEndBufferEvent.Time.tv_usec = 0;
  lEndBufferEvent.TSC = 0;

  if(pmLogCPUID)
    {
    lCPUID = (uint8_t)pmCPUID;
    write_direct(lCurWritePos, &lCPUID, sizeof(lCPUID));
    }
  
  lEventID = TRACE_BUFFER_END;
  write_direct(lCurWritePos, &lEventID, sizeof(lEventID));
  
  lTimeDelta = 0;
  write_direct(lCurWritePos, &lTimeDelta, sizeof(lTimeDelta));

  write_direct(lCurWritePos, &lEndBufferEvent, sizeof(trace_buffer_end));

  lDataSize = sizeof(lEventID)
	  + sizeof(lTimeDelta)
	  + sizeof(lEndBufferEvent)
	  + sizeof(lDataSize);
  write_direct(lCurWritePos, &lDataSize, sizeof(lDataSize));

  *((uint32_t *) (lTempSubBuffer + lBufSize - sizeof(lSizeLost))) = lSizeLost;

  /* Write the filled buffer to file */
  if(write(gTracDataFile[pmCPUID], lTempSubBuffer, lBufSize) < 0)
    {
    fprintf(stdout, "TraceDaemon: Unable to write last sub-buffer \n");
    exit(1);
    }
}

/**
 *	trace_pause - pause the flight recorder
 */
static int trace_pause(void)
{
  int lRetValue = 0;
  struct control_data   lControlData;

  /* Stop tracing while we take the snapshot */
  lControlData.tracer_handle = gTracHandle;
  if(lRetValue = ioctl(gControlFile, TRACER_PAUSE, &lControlData))
    {
    /* Don't go any further */
    printf("TraceDaemon: Unable to pause flight recorder\n");
    }

  return lRetValue;
}

/**
 *	trace_unpause - unpause the flight recorder
 */
static int trace_unpause(void)
{
  int lRetValue = 0;
  struct control_data   lControlData;

  /* Stop tracing while we take the snapshot */
  lControlData.tracer_handle = gTracHandle;
  if(lRetValue = ioctl(gControlFile, TRACER_UNPAUSE, &lControlData))
    {
    /* Don't go any further */
    printf("TraceDaemon: Unable to unpause flight recorder\n");
    }

  return lRetValue;
}

/**
 *	get_status_data - get the status of all trace channels
 */
int get_status_data(struct tracer_status *pmStatus)
{
  int                 lTracHandle;
  struct control_data lControlData;
	
  /* Open the tracer */
  lControlData.tracer_handle = 2;
  if((lTracHandle = ioctl(gControlFile, TRACER_ALLOC_HANDLE, &lControlData)) < 0)
    return -1;

  lControlData.tracer_handle = lTracHandle;
  lControlData.command_arg = (unsigned long)pmStatus;
  if(ioctl(gControlFile, TRACER_GET_STATUS, &lControlData) != 0)
    return -1;

  return 0;
}

/**
 *	snapshot - dump the contents of the flight recorder channels
 */
int snapshot(int pmHandle)
{
  int i, lRetValue;
  trace_start lTraceStart;
  struct buffer_control lBufferControl;
  struct control_data   lControlData;
  struct tracer_status lStatus;
  struct trace_info *info;

  if(get_status_data(&lStatus) != 0) 
    {
    fprintf(stdout, "TraceDaemon: Unable to get tracer status\n");
    exit(1);
    }

  info = &lStatus.traces[FLIGHT_HANDLE];
  if(!info->active)
    {
    fprintf(stdout, "TraceDaemon: Flight recorder not active\n");
    exit(1);
    }
  gTracNBuffers = info->n_buffers;
  gTracDataAreaSize = info->buf_size;

  if (get_arch_info())
    exit(1);

  if (open_input())
    exit(1);

  if (trace_pause())
    exit(1);
	
  lControlData.tracer_handle = pmHandle;
  lControlData.command_arg = (unsigned long)&lTraceStart;
  lRetValue = ioctl(gControlFile, TRACER_GET_START_INFO, &lControlData);

  if(lRetValue != 0) 
    {
    fprintf(stdout, "TraceDaemon: Unable to get trace start data \n");
    exit(1);
    }
	
  if (open_output())
    {
    close_input();
    exit(1);
    }
	
  for(i = 0; i < gNumCPUs; i++)
    {
    dump_start_sub_buffer(i, &lBufferControl, &lTraceStart);
    dump_sub_buffers(i, &lBufferControl, info->use_locking);
    dump_end_sub_buffer(i, &lBufferControl, lTraceStart.LogCPUID);
    }
	
  if (trace_unpause())
    {
    close_input();
    close_output();
    exit(1);
    }
}

/**
 *	get_status - get and print the status of all trace channels
 */
int get_status(void)
{
  int i, j, lRetValue = 0;
  struct tracer_status lStatus;
  struct trace_info *info;
  int lTracHandle;
  struct control_data   lControlData;

  if(get_status_data(&lStatus) != 0) 
    {
    fprintf(stdout, "TraceDaemon: Unable to get tracer status \n");
    return -1;
    }

  fprintf(stdout, "Trace status (%d CPUs):\n", lStatus.num_cpus);
  for (i = 0; i < MAX_NR_TRACES; i++)
    {
    info = &lStatus.traces[i];
    if(!info->active)
      {
      fprintf(stdout, "    Trace %d: not active\n", i);
      continue;
      }
    else
      fprintf(stdout, "    Trace %d: active\n", i);

    fprintf(stdout, "        Trace handle: %d\n", info->trace_handle);
    fprintf(stdout, "        Mode: %s\n", info->flight_recorder ? "Flight Recorder Mode" : "Trace Mode");
    fprintf(stdout, "        Timestamping mode: %s\n", info->using_tsc ? "TSC" : "do_gettimeofday");
    fprintf(stdout, "        Buffering scheme: %s\n", info->use_locking ? "Locking" : "Lockless");
    fprintf(stdout, "        Buffer size: %u (%u sub-buffers x %u bytes)\n", info->n_buffers * info->buf_size, info->n_buffers, info->buf_size);
    fprintf(stdout, "        Traced event mask: %x\n", info->traced_events);
    fprintf(stdout, "        Event detail mask: %x\n", info->log_event_details_mask);
    fprintf(stdout, "        Sub-buffers produced (per CPU):\n");
    for (j = 0; j < lStatus.num_cpus; j++)
      fprintf(stdout, "            CPU %u: %u\n", j, info->buffers_produced[j]);
    }

  return 0;
}

/**
 *	init_ltt - we need to make sure there's a control file.
 *	           We can't get it done in module_init due to init order.
 *	           This goes away when we're a module.
 */
int init_ltt(void)
{
  char                  lProcPath[PATH_MAX]; /* Procfile path buffer */
  FILE*                 pProcFile;  /* Proc file pointer */

  /* Open the relayfs_path proc file for writing */
  sprintf(lProcPath, "%s/init", TRACE_PROC_ROOT);
  if((pProcFile = fopen(lProcPath, "w")) == NULL)
    {
    printf("open %s failed", lProcPath);
    return -1;
    }
  
  /* Write anything to init proc file */
  if(fputs("1", pProcFile) <= 0)
    { /* Error or EOF */
    printf("write %s failed", lProcPath);
    fclose(pProcFile);
    exit(1);
    }

  fclose(pProcFile);

  return 0;
}

/*******************************************************************
 * Function :
 *    main()
 * Description :
 *    The daemon entry point.
 * Parameters :
 *    pmArgc, the number of command line arguments.
 *    pmArgv, the argument vector.
 * Return values :
 *    NONE (even if declared as int).
 * History :
 *    K. Yaghmour, 05/99, Initial typing.
 *******************************************************************/
int main(int pmArgc, char** pmArgv)
{
  int                  lPID;                    /* The pid of the daemon */
  int                  lProcFile;               /* File to which current process information is dumped */
  struct sigaction     lSigAct;                 /* Structure passed on to sigaction() */
  int                  i;                       /* Scratch vars for filename formatting */
  int                  ready;                   /* Scratch vars for filename formatting */
  struct cpu_mmap_data lMmapData;               /* mmap info for driver */
  int                  lMountRC;
  struct control_data  lControlData;
  char                 lRelayDirPath[PATH_MAX]; /* Relayfs dir path buffer */

  /* Check for relayfs before doing anything else */
  lMountRC = get_relayfs_mntpt(gRelayfsMntPt);
  if(lMountRC) 
    {
    /* relayfs ain't mounted */
    printf("\nTraceDaemon: relayfs not mounted. exiting.\n");
    exit(1);
    }

  if(init_ltt())
    {
    printf("\nTraceDaemon: couldn't init control file. exiting.\n");
    exit(1);
    }

  /* trace files are: relayfs mnt pt/trace/handle/cpu0-cpuX */
  sprintf(lRelayDirPath, "%s/%s/%s", gRelayfsMntPt, TRACE_RELAYFS_ROOT, TRACE_CONTROL_FILE);
  if((gControlFile = open(lRelayDirPath, O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP)) < 0)
    {
    /* File ain't good */
    printf("TraceDaemon: Unable to open control file %s (errno = %d) \n", lRelayDirPath, errno);
    exit(1);
    }

  /* Initialize the busy flag */
  gBusy = FALSE;

  /* Allocate space for the options */
  gOptions = CreateOptions();

  /* Parse the given options */
  ParseOptions(pmArgc, pmArgv, gOptions);

  /* Initialize buffer size and count to defaults or specified values */
  if(gOptions->UseLocking == TRUE)
    {
    if(gOptions->DataAreaSize == 0) 
      gTracDataAreaSize = TRACER_LOCKING_DEFAULT_BUF_SIZE;
    else
      gTracDataAreaSize = gOptions->DataAreaSize;

    if(gOptions->NBuffers == 0)
      gTracNBuffers = TRACER_LOCKING_DEFAULT_N_BUFFERS;
    else
      gTracNBuffers = gOptions->NBuffers;
    }
  else /* Using lock-free */
    {
    if(gOptions->DataAreaSize == 0) 
      gTracDataAreaSize = TRACER_LOCKLESS_DEFAULT_BUF_SIZE;
    else
      gTracDataAreaSize = gOptions->DataAreaSize;

    if(gOptions->NBuffers == 0) 
      gTracNBuffers = TRACER_LOCKLESS_DEFAULT_N_BUFFERS;
    else
      gTracNBuffers = gOptions->NBuffers;
    }

  if(gOptions->Status == TRUE) {
	  get_status();
	  exit(0);
  }

  if(gOptions->Snapshot == TRUE) {
	  gTracHandle = FLIGHT_HANDLE;
	  snapshot(gTracHandle);
	  exit(0);
  }

  if(gOptions->Stop == TRUE) {
	  gTracHandle = FLIGHT_HANDLE;
	  /* At this point, the trace files exist and should be used for ioctls*/
	  lControlData.tracer_handle = gTracHandle;
	  if (ioctl(gControlFile, TRACER_STOP, &lControlData) == 0)
		  printf("TraceDaemon: Flight recorder stopped\n");
	  else
		  printf("TraceDaemon: Flight recorder not running\n");

	  close_input();
	  
	  exit(0);
  }

  /* Figure out whether we want trace or flight recorder */
  if(gOptions->FlightRecorder)
  gTracHandle = 1; /* flight recorder */
  else
	  gTracHandle = 0; /* trace */

  /* Do we only need to change trace mask? */
  if(gOptions->ModifyTraceMask == TRUE)
    {
    /* Only change the event mask */
    ModifyEventMask(gOptions);

    /* Leave immediately */
    _exit(0);
    }

  /* Do we only need to print the current trace mask? */
  if(gOptions->PrintTraceMask == TRUE)
    {
    /* Only change the event mask */
    PrintEventMask();

    /* Leave immediately */
    _exit(0);
    }

  /* Become daemon */
  if((lPID = fork()) < 0)
    {
    /* Print an error message */
    printf("\nUnable to become daemon \n");

    /* Exit */
    exit(1);
    }
  else if(lPID)
    /* The parent dies */
    exit(0);

  /* Become session leader */
  gPID = setsid();

  /* Initialize structure passed to sigaction() */
  lSigAct.sa_handler = SigHandler;          /* Signal hanlder */
  sigfillset(&(lSigAct.sa_mask));           /* Signal mask */
  lSigAct.sa_flags = 0;                     /* Action flags (No re-register or unblock) */

  /* Set up the signal handler */
  if(sigaction(SIGIO, &lSigAct, NULL)) exit(1);
  if(sigaction(SIGTERM, &lSigAct, NULL)) exit(1);
  if(sigaction(SIGALRM, &lSigAct, NULL)) exit(1);

  /* Open the tracer */
  lControlData.tracer_handle = gTracHandle;
  if((gTracHandle = ioctl(gControlFile, TRACER_ALLOC_HANDLE, &lControlData)) < 0)
    {
    /* Device ain't good */
    printf("\nTraceDaemon: Unable to get tracer handle, errcode = %d\n", gTracHandle);
    exit(1);
    }

  /* We opened the tracer */
  if(gTracHandle == 0)
    /* We opened the driver */
    printf("TraceDaemon: Tracer open \n");
  else
    /* We opened the driver */
    printf("TraceDaemon: Flight recorder open \n");

  /* Apply the configuration, need to do this before we get cpu info */
  ApplyConfig(gTracHandle, gOptions);

  if(gTracHandle == 1) { /* for now */ 
    /* Fire up the tracing */
    lControlData.tracer_handle = gTracHandle;
    if(ioctl(gControlFile, TRACER_START, &lControlData) < 0)
      {
      /* Don't go any further */
      printf("TraceDaemon: Unable to start flight recorder \n");
      exit(1);
      }
    else
      {
      /* Don't go any further */
      printf("TraceDaemon: Flight recorder started\n");
      exit(0);
      }
  }

  open_output();

  /* Fire up the tracing */
  lControlData.tracer_handle = gTracHandle;
  if(ioctl(gControlFile, TRACER_START, &lControlData) < 0)
    {
    /* Don't go any further */
    printf("TraceDaemon: Unable to start tracing \n");
    exit(1);
    }

  /* Map out /proc into user given file */
  if((lProcFile = creat(gOptions->ProcFileName, S_IRUSR| S_IWUSR | S_IRGRP | S_IROTH)) < 0)
    {
    /* File ain't good */
    printf("\nUnable to open output file to dump current process information \n");
    exit(1);
    }

  /* Map out /proc */
  MapProc(lProcFile);
  
  /* Tell the user that mapping /proc is done */
  printf("TraceDaemon: Done mapping /proc \n");
  
  /* Close the output file */
  close(lProcFile);

  /* Sleep forever */
  while (1)
    {
    if (gWaitingForThreads)
      pause();
    
    for (i = 0; i < gNumCPUs; i++)
      {
      pollfds[i].fd = gTracRelayFile[i];
      pollfds[i].events = POLLIN;
      }
    
    ready = poll(pollfds, gNumCPUs, -1);
    if (ready < 0)
      continue;

    for (i = 0; i < gNumCPUs; i++)
      {
      if (pollfds[i].revents & POLLIN)
	{
	read_all_ready();
	break;
	}
      }
    }

  /* SHOULD NEVER GET HERE! */
  exit(1);
}

