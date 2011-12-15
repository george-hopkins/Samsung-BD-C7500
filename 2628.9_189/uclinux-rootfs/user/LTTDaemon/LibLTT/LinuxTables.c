/*
   LinuxTables.c : Data tables for Linux.
   Copyright (C) 2000, 2001 Karim Yaghmour (karim@opersys.com).
   Portions contributed by T. Halloran: (C) Copyright 2002 IBM Poughkeepsie, IBM Corporation

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

   History : 
     K.Y,  19/06/2001, Moved event database into libltt.
     K.Y., 31/05/2000, Initial typing.
*/

#include <LinuxEvents.h>
#include <LinuxTables.h>
#include <EventDB.h>

/* Default size of event structures */
int LinuxEventStructSizeDefaults[TRACE_MAX + 1] =
{
  sizeof(trace_start)             /* TRACE_START */,
  sizeof(trace_syscall_entry)     /* TRACE_SYSCALL_ENTRY */,
  0                               /* TRACE_SYSCALL_EXIT */,
  sizeof(trace_trap_entry)        /* TRACE_TRAP_ENTRY */,
  0                               /* TRACE_TRAP_EXIT */,
  sizeof(trace_irq_entry)         /* TRACE_IRQ_ENTRY */,
  0                               /* TRACE_IRQ_EXIT */,
  sizeof(trace_schedchange)       /* TRACE_SCHEDCHANGE */,
  0                               /* TRACE_KERNEL_TIMER */,
  sizeof(trace_soft_irq)          /* TRACE_SOFT_IRQ */,
  sizeof(trace_process)           /* TRACE_PROCESS */,
  sizeof(trace_file_system)       /* TRACE_FILE_SYSTEM */,
  sizeof(trace_timer)             /* TRACE_TIMER */,
  sizeof(trace_memory)            /* TRACE_MEMORY */,
  sizeof(trace_socket)            /* TRACE_SOCKET */,
  sizeof(trace_ipc)               /* TRACE_IPC */,
  sizeof(trace_network)           /* TRACE_NETWORK */,
  sizeof(trace_buffer_start)      /* TRACE_BUFFER_START */,
  0                               /* TRACE_BUFFER_END */,
  sizeof(trace_new_event)         /* TRACE_NEW_EVENT */,
  sizeof(trace_custom)            /* TRACE_CUSTOM */,
  sizeof(trace_change_mask)       /* TRACE_CHANGE_MASK */,
  0	  			  /* TRACE_HEARTBEAT */
};

/* The default table could have been modified to take care of the S/390's particularities
but this would have been a short-hand solution since the table would have only been
useable for S/390 traces only (i.e. LTT wouldn't be able to display both S/390 traces and
traces from other archs ...). Hence, the need for this function. */
int LinuxEventStructSize(db* pmDB, int pmEventID)
{
  /* Is this something else than the S/390 */
  if(pmDB->ArchType == TRACE_ARCH_TYPE_S390)
    {
    /* Is this a trap entry. This if/else may further be a switch() if other
     changes are required. */
    if(pmEventID == TRACE_TRAP_ENTRY)
      return sizeof(trace_trap_entry_s390);
    else
      return LinuxEventStructSizeDefaults[pmEventID];
    }
  else
    return LinuxEventStructSizeDefaults[pmEventID];
}

/* Event description strings */
char *LinuxEventID[TRACE_MAX + 1] = 
{
"Trace start",
"Syscall entry",
"Syscall exit",
"Trap entry",
"Trap exit",
"IRQ entry",
"IRQ exit",
"Sched change",
"Kernel timer",
"Soft IRQ",
"Process",
"File system",
"Timer",
"Memory",
"Socket",
"IPC",
"Network",
"",
"",
"Event creation",
"Custom event",
"Change trace mask",
"Heartbeat"
};

char* LinuxEventString(db* pmDB, int pmEventID, event* pmEvent)
{
  trace_custom*       pCustom;      /* Pointer to custom event */
  customEventDesc*    pCustomType;  /* Pointer to custom event type */

  /* Is this a custom event */
  if(pmEventID == TRACE_CUSTOM)
    {
    /* Get the custom event's structure */
    pCustom = CUSTOM_EVENT(DBEventStruct(pmDB, pmEvent));

    /* Loop around list of custom event description */
    for(pCustomType = pmDB->CustomEvents.Next;
	pCustomType != &(pmDB->CustomEvents);
	pCustomType = pCustomType->Next)
      if(pCustomType->Event.id == RFT32(pmDB, pCustom->id))
	return (pCustomType->Event.type);
    }

  /* Return the default string */
  return LinuxEventID[pmEventID];
}

/* Event strings the user can specify at the command line to omit or trace */
char *LinuxEventOT[TRACE_MAX + 1] =
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
"", /* Buffer start */
"", /* Buffer end */
"", /* New event */
"", /* Custom event */
"", /* Change mask */
"HEARTBEAT"
};

/* "Maximum" number of system calls on a linux box */
#define MAX_LIN_SYSCALLS   256

/* Number of system calls for each architecture. N.B. The number listed here is
actually the ID of the highest syscall for the given arch. The exact number
of system calls is this number +1. I left it this way because it is less
confusing when wanting to replace the entire table with a new one from the
kernel sources. K.Y. 11/01/2002 */
#define NB_SYSCALLS_I386   226
#define NB_SYSCALLS_PPC    208
#define NB_SYSCALLS_SH     221
#define NB_SYSCALLS_390    225
#define NB_SYSCALLS_MIPS   220
#define NB_SYSCALLS_ARM    226

/* System call name according to it's ID for i386 */
char *LinuxI386SyscallID[NB_SYSCALLS_I386 + 1] =
{
"ni_syscall 0",		/* 0  -  old "setup" system call */
"exit",
"fork",
"read",
"write",
"open",			/* 5 */
"close",
"waitpid",
"creat",
"link",
"unlink",		/* 10 */
"execve",
"chdir",
"time",
"mknod",
"chmod",		/* 15 */
"lchown",
"ni_syscall 17",	/* old break syscall holder */
"stat",
"lseek",
"getpid",		/* 20 */
"mount",
"oldumount",
"setuid",
"getuid",
"stime",		/* 25 */
"ptrace",
"alarm",
"fstat",
"pause",
"utime",		/* 30 */
"ni_syscall 31",	/* old stty syscall holder */
"ni_syscall 32",	/* old gtty syscall holder */
"access",
"nice",
"ni_syscall 35",	/* 35 *//* old ftime syscall holder */
"sync",
"kill",
"rename",
"mkdir",
"rmdir",		/* 40 */
"dup",
"pipe",
"times",
"ni_syscall 44",	/* old prof syscall holder */
"brk",			/* 45 */
"setgid",
"getgid",
"signal",
"geteuid",
"getegid",		/* 50 */
"acct",
"umount",		/* recycled never used phys() */
"ni_syscall 53",	/* old lock syscall holder */
"ioctl",
"fcntl",		/* 55 */
"ni_syscall 56",	/* old mpx syscall holder */
"setpgid",
"ni_syscall 58",	/* old ulimit syscall holder */
"olduname",
"umask",		/* 60 */
"chroot",
"ustat",
"dup2",
"getppid",
"getpgrp",		/* 65 */
"setsid",
"sigaction",
"sgetmask",
"ssetmask",
"setreuid",		/* 70 */
"setregid",
"sigsuspend",
"sigpending",
"sethostname",
"setrlimit",		/* 75 */
"old_getrlimit",
"getrusage",
"gettimeofday",
"settimeofday",
"getgroups",		/* 80 */
"setgroups",
"old_select",
"symlink",
"lstat",
"readlink",		/* 85 */
"uselib",
"swapon",
"reboot",
"old_readdir",
"old_mmap",		/* 90 */
"munmap",
"truncate",
"ftruncate",
"fchmod",
"fchown",		/* 95 */
"getpriority",
"setpriority",
"ni_syscall 98",	/* old profil syscall holder */
"statfs",
"fstatfs",		/* 100 */
"ioperm",
"socketcall",
"syslog",
"setitimer",
"getitimer",		/* 105 */
"newstat",
"newlstat",
"newfstat",
"uname",
"iopl",			/* 110 */
"vhangup",
"ni_syscall 112",
"vm86old",
"wait4",
"swapoff",		/* 115 */
"sysinfo",
"ipc",
"fsync",
"sigreturn",
"clone",		/* 120 */
"setdomainname",
"newuname",
"modify_ldt",
"adjtimex",
"mprotect",		/* 125 */
"sigprocmask",
"create_module",
"init_module",
"delete_module",
"get_kernel_syms",	/* 130 */
"quotactl",
"getpgid",
"fchdir",
"bdflush",
"sysfs",		/* 135 */
"personality",
"ni_syscall 137",	/* for afs_syscall */
"setfsuid",
"setfsgid",
"llseek",		/* 140 */
"getdents",
"select",
"flock",
"msync",
"readv",		/* 145 */
"writev",
"getsid",
"fdatasync",
"sysctl",
"mlock",		/* 150 */
"munlock",
"mlockall",
"munlockall",
"sched_setparam",
"sched_getparam",	 /* 155 */
"sched_setscheduler",
"sched_getscheduler",
"sched_yield",
"sched_get_priority_max",
"sched_get_priority_min",  /* 160 */
"sched_rr_get_interval",
"nanosleep",
"mremap",
"setresuid",
"getresuid",		/* 165 */
"vm86",
"query_module",
"poll",
"nfsservctl",
"setresgid",		/* 170 */
"getresgid",
"prctl",
"rt_sigreturn",
"rt_sigaction",
"rt_sigprocmask",	/* 175 */
"rt_sigpending",
"rt_sigtimedwait",
"rt_sigqueueinfo",
"rt_sigsuspend",
"pread",		/* 180 */
"pwrite",
"chown",
"getcwd",
"capget",
"capset",		/* 185 */
"sigaltstack",
"sendfile",
"ni_syscall 188",	/* streams1 */
"ni_syscall 189",	/* streams2 */
"vfork",		/* 190 */
"getrlimit",
"mmap2",
"truncate64",
"ftruncate64",
"stat64",		/* 195 */
"lstat64",
"fstat64",
"lchown",
"getuid",
"getgid",		/* 200 */
"geteuid",
"getegid",
"setreuid",
"setregid",
"getgroups",            /* 205 */
"setgroups",
"fchown",
"setresuid",
"getresuid",
"setresgid",	        /* 210 */
"getresgid",
"chown",
"setuid",
"setgid",
"setfsuid",		/* 215 */
"setfsgid",
"pivot_root",
"mincore",
"madvise",
"getdents64",	        /* 220 */
"fcntl64",
"ni_syscall 222",	/* reserved for TUX */
"ni_syscall 223",	/* Reserved for Security */
"gettid",
"readahead"
""
};

/* System call name according to it's ID for PPC */
char *LinuxPPCSyscallID[NB_SYSCALLS_PPC + 1] =
{
"ni_syscall 0",	       /* 0  -  old "setup()" system call */
"exit",
"fork",
"read",
"write",
"open",	               /* 5 */
"close",
"waitpid",
"creat",
"link",
"unlink",             /* 10 */
"execve",
"chdir",
"time",
"mknod",
"chmod",	      /* 15 */
"lchown",
"ni_syscall 17",      /* old break syscall holder */
"stat",
"lseek",
"getpid",	      /* 20 */
"mount",
"oldumount",
"setuid",
"getuid",
"stime",	      /* 25 */
"ptrace",
"alarm",
"fstat",
"pause",
"utime",	      /* 30 */
"ni_syscall 31",      /* old stty syscall holder */
"ni_syscall 32",      /* old gtty syscall holder */
"access",
"nice",
"ni_syscall 35",      /* 35 */	/* old ftime syscall holder */
"sync",
"kill",
"rename",
"mkdir",
"rmdir",	      /* 40 */
"dup",
"pipe",
"times",
"ni_syscall 44",      /* old prof syscall holder */
"brk",		      /* 45 */
"setgid",
"getgid",
"signal",
"geteuid",
"getegid",	      /* 50 */
"acct",
"umount",	      /* recycled never used phys() */
"ni_syscall 53",      /* old lock syscall holder */
"ioctl",
"fcntl",	      /* 55 */
"ni_syscall 56",      /* old mpx syscall holder */
"setpgid",
"ni_syscall 58",      /* old ulimit syscall holder */
"olduname",
"umask",	      /* 60 */
"chroot",
"ustat",
"dup2",
"getppid",
"getpgrp",	      /* 65 */
"setsid",
"sigaction",
"sgetmask",
"ssetmask",
"setreuid",	      /* 70 */
"setregid",
"sigsuspend",
"sigpending",
"sethostname",
"setrlimit",	      /* 75 */
"old_getrlimit",
"getrusage",
"gettimeofday",
"settimeofday",
"getgroups",	      /* 80 */
"setgroups",
"select",
"symlink",
"lstat",
"readlink",	      /* 85 */
"uselib",
"swapon",
"reboot",
"old_readdir",
"mmap",		      /* 90 */
"munmap",
"truncate",
"ftruncate",
"fchmod",
"fchown",	      /* 95 */
"getpriority",
"setpriority",
"ni_syscall 98",      /* old profil syscall holder */
"statfs",
"fstatfs",	      /* 100 */
"ioperm",
"socketcall",
"syslog",
"setitimer",
"getitimer",	      /* 105 */
"newstat",
"newlstat",
"newfstat",
"uname",
"iopl",		      /* 110 */
"vhangup",
"ni_syscall 112",
"vm86",
"wait4",
"swapoff",	      /* 115 */
"sysinfo",
"ipc",
"fsync",
"sigreturn",
"clone",	      /* 120 */
"setdomainname",
"newuname",
"modify_ldt",
"adjtimex",
"mprotect",	      /* 125 */
"sigprocmask",
"create_module",
"init_module",
"delete_module",
"get_kernel_syms",    /* 130 */
"quotactl",
"getpgid",
"fchdir",
"bdflush",
"sysfs",	      /* 135 */
"personality",
"ni_syscall 137",     /* for afs_syscall */
"setfsuid",
"setfsgid",
"llseek",	      /* 140 */
"getdents",
"select",
"flock",
"msync",
"readv",	      /* 145 */
"writev",
"getsid",
"fdatasync",
"sysctl",
"mlock",	      /* 150 */
"munlock",
"mlockall",
"munlockall",
"sched_setparam",
"sched_getparam",     /* 155 */
"sched_setscheduler",
"sched_getscheduler",
"sched_yield",
"sched_get_priority_max",
"sched_get_priority_min", /* 160 */
"sched_rr_get_interval",
"nanosleep",
"mremap",
"setresuid",
"getresuid",	      /* 165 */
"query_module",
"poll",
"nfsservctl",
"setresgid",
"getresgid",	      /* 170 */
"prctl",
"rt_sigreturn",
"rt_sigaction",
"rt_sigprocmask",	
"rt_sigpending",      /* 175 */
"rt_sigtimedwait",
"rt_sigqueueinfo",
"rt_sigsuspend",
"pread",
"pwrite",	      /* 180 */
"chown",
"getcwd",
"capget",
"capset",
"sigaltstack",	      /* 185 */
"sendfile",
"ni_syscall 187",     /* streams1 */
"ni_syscall 188",     /* streams2 */
"vfork",
"getrlimit",          /* 190 */
"readahead",
"mmap2",
"truncate64",
"ftruncate64",
"stat64",		/* 195 */
"lstat64",
"fstat64",
"pciconfig_read",
"pciconfig_write",
"pciconfig_iobase",     /* 200 */
"ni_syscall 201",	/* 201 - reserved - MacOnLinux - new */
"getdents64",	        /* 202 */
"pivot_root",
"fcntl64",
"madvise",		/* 205 */
"mincore",
"gettid",
"",
};

/* System call name according to it's ID for SuperH */
char *LinuxSHSyscallID[NB_SYSCALLS_SH + 1] =
{
"ni_syscall",	    	/* 0  -  old "setup()" system call */
"exit",
"fork",
"read",
"write",
"open",	            	/* 5 */
"close",
"waitpid",
"creat",
"link",
"unlink",	    	/* 10 */
"execve",
"chdir",
"time",
"mknod",
"chmod",		/* 15 */
"lchown16",
"ni_syscall",	    	/* old break syscall holder */
"stat",
"lseek",
"getpid",	    	/* 20 */
"mount",
"oldumount",
"setuid16",
"getuid16",
"stime",		/* 25 */
"ptrace",
"alarm",
"fstat",
"pause",
"utime",    	    	/* 30 */
"ni_syscall",	    	/* old stty syscall holder */
"ni_syscall",	    	/* old gtty syscall holder */
"access",
"nice",
"ni_syscall",	    	/* 35 */ /* old ftime syscall holder */
"sync",
"kill",
"rename",
"mkdir",
"rmdir",		/* 40 */
"dup",
"pipe",
"times",
"ni_syscall",	    	/* old prof syscall holder */
"brk",		    	/* 45 */
"setgid16",
"getgid16",
"signal",
"geteuid16",
"getegid16",	    	/* 50 */
"acct",
"umount",   	    	/* recycled never used phys() */
"ni_syscall",	    	/* old lock syscall holder */
"ioctl",
"fcntl",		/* 55 */
"ni_syscall",	    	/* old mpx syscall holder */
"setpgid",
"ni_syscall",	    	/* old ulimit syscall holder */
"ni_syscall",	    	/* olduname holder */
"umask",		/* 60 */
"chroot",
"ustat",
"dup2",
"getppid",
"getpgrp",	    	/* 65 */
"setsid",
"sigaction",
"sgetmask",
"ssetmask",
"setreuid16",	    	/* 70 */
"setregid16",
"sigsuspend",
"sigpending",
"sethostname",
"setrlimit",	    	/* 75 */
"old_getrlimit",
"getrusage",
"gettimeofday",
"settimeofday",
"getgroups16",	    	/* 80 */
"setgroups16",
"ni_syscall",	    	/* old select */
"symlink",
"lstat",
"readlink",	    	/* 85 */
"uselib",
"swapon",
"reboot",
"old_readdir",
"mmap",		    	/* 90 */
"munmap",
"truncate",
"ftruncate",
"fchmod",
"fchown16",	    	/* 95 */
"getpriority",
"setpriority",
"ni_syscall",	    	/* old profil syscall holder */
"statfs",
"fstatfs",	    	/* 100 */
"ni_syscall",	    	/* ioperm */
"socketcall",
"syslog",
"setitimer",
"getitimer",	    	/* 105 */
"newstat",
"newlstat",
"newfstat",
"uname",
"ni_syscall",		/* 110 */   /* iopl */
"vhangup",
"ni_syscall",	    	/* idle */
"ni_syscall",	    	/* vm86old */
"wait4",
"swapoff",	    	/* 115 */
"sysinfo",
"ipc",
"fsync",
"sigreturn",
"clone",		/* 120 */
"setdomainname",
"newuname",
"ni_syscall",	    	/* modify_ldt */
"adjtimex",
"mprotect",	    	/* 125 */
"sigprocmask",
"create_module",
"init_module",
"delete_module",
"get_kernel_syms",	/* 130 */
"quotactl",
"getpgid",
"fchdir",
"bdflush",
"sysfs",		/* 135 */
"personality",
"ni_syscall",	    	/* for afs_syscall */
"setfsuid16",
"setfsgid16",
"llseek",	    	/* 140 */
"getdents",
"select",
"flock",
"msync",
"readv",		/* 145 */
"writev",
"getsid",
"fdatasync",
"sysctl",
"mlock",		/* 150 */
"munlock",
"mlockall",
"munlockall",
"sched_setparam",
"sched_getparam",	/* 155 */
"sched_setscheduler",
"sched_getscheduler",
"sched_yield",
"sched_get_priority_max",
"sched_get_priority_min", /* 160 */
"sched_rr_get_interval",
"nanosleep",
"mremap",
"setresuid16",
"getresuid16",	    	/* 165 */
"ni_syscall",	    	/* vm86 */
"query_module",
"poll",
"nfsservctl",
"setresgid16",	    	/* 170 */
"getresgid16",
"prctl",
"rt_sigreturn",
"rt_sigaction",
"rt_sigprocmask",	/* 175 */	
"rt_sigpending",
"rt_sigtimedwait",
"rt_sigqueueinfo",
"rt_sigsuspend",
"pread",    		/* 180 */
"pwrite",
"chown16",
"getcwd",
"capget",
"capset",   	    	/* 185 */
"sigaltstack",
"sendfile",
"ni_syscall",		/* streams1 */
"ni_syscall",    	/* streams2 */
"vfork",    	    	/* 190 */
"getrlimit",
"mmap2",
"truncate64",
"ftruncate64",
"stat64",		/* 195 */
"lstat64",
"fstat64",
"lchown",   
"getuid",   
"getgid", 	    	/* 200 */
"geteuid",
"getegid",
"setreuid",
"setregid",
"getgroups",		/* 205 */
"setgroups",
"fchown",
"setresuid",
"getresuid",
"setresgid",		/* 210 */
"getresgid",
"chown",
"setuid",
"setgid",
"setfsuid",		/* 215 */
"setfsgid",
"pivot_root",
"mincore",
"madvise",
"getdents64",		/* 220 */
"fcntl64"
};

/* System call name according to it's ID for S390 */       
char *Linux390SyscallID[NB_SYSCALLS_390 + 1] =             
{                                                          
"ni_syscall 0",         /* 0  -  old "setup" system call */
"exit",                                                    
"fork_glue",                                               
"read",                                                    
"write",                                                   
"open",          /* 5 */                                   
"close",                                                   
"ni_syscall  7",         /* old waitpid syscall holder */  
"creat",                                                   
"link",                                                    
"unlink",          /* 10 */                                
"execve_glue",                                             
"chdir",                                                   
"time",                                                    
"mknod",                                                   
"chmod",         /* 15 */                                  
"lchown16",                                                
"ni_syscall  17",         /* old break syscall holder */   
"ni_syscall  18",         /* old stat syscall holder */    
"lseek",                                                          
"getpid",         /* 20 */                                        
"mount",                                                          
"oldumount",                                                      
"setuid16",                                                       
"getuid16",                                                       
"stime",         /* 25 */                                         
"ptrace",                                                         
"alarm",                                                          
"ni_syscall  28",         /* old fstat syscall holder */          
"pause",                                                          
"utime",         /* 30 */                                         
"ni_syscall  31",         /* old stty syscall holder */           
"ni_syscall  32",         /* old gtty syscall holder */           
"access",                                                         
"nice",                                                           
"ni_syscall  35",         /* 35 */  /* old ftime syscall holder */
"sync",                                                           
"kill",                                                           
"rename",                                                         
"mkdir",                                                          
"rmdir",         /* 40 */                                         
"dup",                                                            
"pipe",                                                  
"times",                                                 
"ni_syscall  44",         /* old prof syscall holder */  
"brk",         /* 45 */                                  
"setgid16",                                              
"getgid16",                                              
"signal",                                                
"geteuid16",                                             
"getegid16",         /* 50 */                            
"acct",                                                  
"umount",                                                
"ni_syscall  53",         /* old lock syscall holder */  
"ioctl",                                                 
"fcntl",         /* 55 */                                
"ni_syscall  56",         /* old mpx syscall holder */   
"setpgid",                                               
"ni_syscall  58",         /* old ulimit syscall holder */
"ni_syscall  59",         /* old uname syscall holder */ 
"umask",         /* 60 */                                
"chroot",                                                
"ustat",                                                 
"dup2",                                                  
"getppid",                                               
"getpgrp",         /* 65 */                                
"setsid",                                                  
"sigaction",                                               
"ni_syscall  68",         /* old sgetmask syscall holder */
"ni_syscall  69",         /* old ssetmask syscall holder */
"setreuid16",         /* 70 */                             
"setregid16",                                              
"sigsuspend_glue",                                         
"sigpending",                                              
"sethostname",                                             
"setrlimit",            /* 75 */                           
"old_getrlimit",                                           
"getrusage",                                               
"gettimeofday",                                            
"settimeofday",                                            
"getgroups16",          /* 80 */                           
"setgroups16",                                             
"ni_syscall 82",           /* old select syscall holder */ 
"symlink ",                                                
"ni_syscall 84",           /* old lstat syscall holder */  
"readlink",             /* 85 */                           
"uselib",                                                  
"swapon",                                                  
"reboot",                                                  
"ni_syscall 89",           /* old readdir syscall holder */
"mmap",                 /* 90 */                           
"munmap",                                                  
"truncate",                                                
"ftruncate",                                               
"fchmod",                                                  
"fchown16",              /* 95 */                          
"getpriority",                                             
"setpriority",                                             
"ni_syscall 98",            /* old profil syscall holder */
"statfs",                                                  
"fstatfs",               /* 100 */                         
"ioperm",                                                  
"socketcall",                                              
"syslog",                                                  
"setitimer",                                               
"getitimer",             /* 105 */                         
"newstat",                                                 
"newlstat",                                                
"newfstat",                                                
"ni_syscall 109",            /* old uname syscall holder */
"ni_syscall 110",           /* 110 */ /* iopl for i386 */  
"vhangup",                                                 
"ni_syscall 112",            /* old "idle" system call */  
"ni_syscall 113",           /* vm86old for i386 */         
"wait4",                                                   
"swapoff",               /* 115 */                         
"sysinfo",                                                 
"ipc",                                                     
"fsync",                                                   
"sigreturn_glue",                                          
"clone_glue",            /* 120 */                         
"setdomainname",                                           
"newuname",                                                
"ni_syscall 123",            /* modify_ldt for i386 */     
"adjtimex",                                                
"mprotect",             /* 125 */                          
"sigprocmask",                                             
"create_module",                                           
"init_module",                                             
"delete_module",                                           
"get_kernel_syms",       /* 130 */                
"quotactl",                                       
"getpgid",                                        
"fchdir",                                         
"bdflush",                                        
"sysfs",                 /* 135 */                
"personality",                                    
"ni_syscall 137",            /* for afs_syscall */
"setfsuid16",                                     
"setfsgid16",                                     
"llseek",                /* 140 */                
"getdents",                                       
"select",                                         
"flock",                                          
"msync",                                          
"readv",                 /* 145 */                
"writev",                                         
"getsid",                                         
"fdatasync",                                      
"sysctl",                                         
"mlock",                 /* 150 */                
"munlock",                                        
"mlockall",                                       
"munlockall",                              
"sched_setparam",                          
"sched_getparam",        /* 155 */         
"sched_setscheduler",                      
"sched_getscheduler",                      
"sched_yield",                             
"sched_get_priority_max",                  
"sched_get_priority_min",  /* 160 */       
"sched_rr_get_interval",                   
"nanosleep",                               
"mremap",                                  
"setresuid16",                             
"getresuid16",           /* 165 */         
"ni_syscall 166",            /* for vm86 */
"query_module",                            
"poll",                                    
"nfsservctl",                              
"setresgid16",           /* 170 */         
"getresgid16",                             
"prctl",                                   
"rt_sigreturn_glue",                       
"rt_sigaction",                            
"rt_sigprocmask",        /* 175 */         
"rt_sigpending",                           
"rt_sigtimedwait",                         
"rt_sigqueueinfo",                         
"rt_sigsuspend_glue",                      
"pread",                 /* 180 */         
"pwrite",                                  
"chown16",                                 
"getcwd",                                  
"capget",                                  
"capset",                /* 185 */         
"sigaltstack_glue",                        
"sendfile",                                
"ni_syscall 188",            /* streams1 */
"ni_syscall 189",            /* streams2 */
"vfork_glue",            /* 190 */         
"getrlimit",                               
"mmap2",                                   
"truncate64",                              
"ftruncate64",                             
"stat64",                /* 195 */
"lstat64",                        
"fstat64",                        
"lchown",                         
"getuid",                         
"getgid",                /* 200 */
"geteuid",                        
"getegid",                        
"setreuid",                       
"setregid",                       
"getgroups",             /* 205 */
"setgroups",                      
"fchown",                         
"setresuid",                      
"getresuid",                      
"setresgid",             /* 210 */
"getresgid",                      
"chown",                          
"setuid",                         
"setgid",                         
"setfsuid",              /* 215 */
"setfsgid",                       
"pivot_root",                     
"mincore",                        
"madvise",                        
"getdents64",            /* 220 */
"fcntl64",                        
"",                               
"",                               
"",                               
"",                               
 };                                

/* System call name according to it's ID for MIPS */
char *LinuxMIPSSyscallID[NB_SYSCALLS_MIPS + 1] =             
{
"syscall",			/* 0 */
"exit",
"fork",
"read",
"write",
"open",				/* 5 */
"close",
"waitpid",
"creat",
"link",
"unlink",			/* 10 */
"execve",
"chdir",
"time",
"mknod",
"chmod",			/* 15 */
"lchown",
"ni_syscall 17",
"stat",
"lseek",
"getpid",			/* 20 */
"mount",
"oldumount",
"setuid",
"getuid",
"stime",			/* 25 */
"ptrace",
"alarm",
"fstat",
"pause",
"utime",			/* 30 */
"ni_syscall 31",
"ni_syscall 32",
"access",
"nice",
"ni_syscall 35",		/* 35 */
"sync",
"kill",
"rename",
"mkdir",
"rmdir",			/* 40 */
"dup",
"pipe",
"times",
"ni_syscall 44",
"brk",				/* 45 */
"setgid",
"getgid",
"ni_syscall 48",
"geteuid",
"getegid",			/* 50 */
"acct",
"umount",
"ni_syscall 53",
"ioctl",
"fcntl",			/* 55 */
"ni_syscall 56",
"setpgid",
"ni_syscall 58",
"olduname",
"umask",			/* 60 */
"chroot",
"ustat",
"dup2",
"getppid",
"getpgrp",			/* 65 */
"setsid",
"sigaction",
"sgetmask",
"ssetmask",
"setreuid",			/* 70 */
"setregid",
"sigsuspend",
"sigpending",
"sethostname",
"setrlimit",			/* 75 */
"getrlimit",
"getrusage",
"gettimeofday",
"settimeofday",
"getgroups",			/* 80 */
"setgroups",
"ni_syscall 82",
"symlink",
"lstat",
"readlink",			/* 85 */
"uselib",
"swapon",
"reboot",
"old_readdir",
"old_mmap",			/* 90 */
"munmap",
"truncate",
"ftruncate",
"fchmod",
"fchown",			/* 95 */
"getpriority",
"setpriority",
"ni_syscall 98",
"statfs",
"fstatfs",			/* 100 */
"ioperm",
"socketcall",
"syslog",
"setitimer",
"getitimer",			/* 105 */
"newstat",
"newlstat",
"newfstat",
"uname",
"iopl",				/* 110 */
"vhangup",
"ni_syscall 112",
"vm86",
"wait4",
"swapoff",			/* 115 */
"sysinfo",
"ipc",
"fsync",
"sigreturn",
"clone",			/* 120 */
"setdomainname",
"newuname",
"sys_ni_syscall 123",
"adjtimex",
"mprotect",			/* 125 */
"sigprocmask",
"create_module",
"init_module",
"delete_module",
"get_kernel_syms",		/* 130 */
"quotactl",
"getpgid",
"fchdir",
"bdflush",
"sysfs",			/* 135 */
"personality",
"ni_syscall 137",		/* for afs_syscall */
"setfsuid",
"setfsgid",
"llseek",			/* 140 */
"getdents",
"select",
"flock",
"msync",
"readv",			/* 145 */
"writev",
"cacheflush",
"cachectl",
"sysmips",
"ni_syscall 150",		/* 150 */
"getsid",
"fdatasync",
"sysctl",
"mlock",
"munlock",			/* 155 */
"mlockall",
"munlockall",
"sched_setparam",
"sched_getparam",
"sched_setscheduler",		/* 160 */
"sched_getscheduler",
"sched_yield",
"sched_get_priority_max",
"sched_get_priority_min",
"sched_rr_get_interval",	/* 165 */
"nanosleep",
"mremap",
"accept",
"bind",
"connect",			/* 170 */
"getpeername",
"getsockname",
"getsockopt",
"listen",
"recv",				/* 175 */
"recvfrom",
"recvmsg",
"send",
"sendmsg",
"sendto",			/* 180 */
"setsockopt",
"shutdown",
"socket",
"socketpair",
"setresuid",			/* 185 */
"getresuid",
"query_module",
"poll",
"nfsservctl",
"setresgid",			/* 190 */
"getresgid",
"prctl",
"rt_sigreturn",
"rt_sigaction",
"rt_sigprocmask",		/* 195 */
"rt_sigpending",
"rt_sigtimedwait",
"rt_sigqueueinfo",
"rt_sigsuspend",
"pread",			/* 200 */
"pwrite",
"chown",
"getcwd",
"capget",
"capset",			/* 205 */
"sigaltstack",
"sendfile",
"ni_syscall 208",
"ni_syscall 209",
"mmap2",			/* 210 */
"truncate64",
"ftruncate64",
"stat64",
"lstat64",
"fstat64",			/* 215 */
"pivot_root",
"mincore",
"madvise",
"getdents64",
"fcntl64",			/* 220 */
};

/* System call name according to its ID for ARM */
char *LinuxARMSyscallID[NB_SYSCALLS_ARM + 1] =
{
"ni_syscall 0",			/* 0 */
"exit",
"fork",
"read",
"write",
"open",				/* 5 */
"close",
"ni_syscall 7",					/* was sys_waitpid */
"creat",
"link",
"unlink",			/* 10 */
"execve",
"chdir",
"time",						/* used by libc4 */
"mknod",
"chmod",			/* 15 */
"lchown16",
"ni_syscall 17",				/* was sys_break */
"ni_syscall 18",				/* was sys_stat */
"lseek",
"getpid",			/* 20 */
"mount",
"oldumount",					/* used by libc4 */
"setuid16",
"getuid16",
"stime",			/* 25 */
"ptrace",
"alarm",					/* used by libc4 */
"ni_syscall 28",				/* was sys_fstat */
"pause",
"utime",			/* 30 */	/* used by libc4 */
"ni_syscall 31",				/* was sys_stty */
"ni_syscall 32",				/* was sys_getty */
"access",
"nice",
"ni_syscall 35",		/* 35 */	/* was sys_ftime */
"sync",
"kill",
"rename",
"mkdir",
"rmdir",			/* 40 */
"dup",
"pipe",
"times",
"ni_syscall 44",				/* was sys_prof */
"brk",				/* 45 */
"setgid16",
"getgid16",
"ni_syscall 48",				/* was sys_signal */
"geteuid16",
"getegid16",			/* 50 */
"acct",
"umount",
"ni_syscall 53",				/* was sys_lock */
"ioctl",
"fcntl",			/* 55 */
"ni_syscall 56",				/* was sys_mpx */
"setpgid",
"ni_syscall 58",				/* was sys_ulimit */
"ni_syscall 59",				/* was sys_olduname */
"umask",			/* 60 */
"chroot",
"ustat",
"dup2",
"getppid",
"getpgrp",			/* 65 */
"setsid",
"sigaction",
"ni_syscall 68",				/* was sys_sgetmask */
"ni_syscall 69",				/* was sys_ssetmask */
"setreuid16",			/* 70 */
"setregid16",
"sigsuspend",
"sigpending",
"sethostname",
"setrlimit",			/* 75 */
"old_getrlimit",				/* used by libc4 */
"getrusage",
"gettimeofday",
"settimeofday",
"getgroups16",			/* 80 */
"setgroups16",
"old_select",					/* used by libc4 */
"symlink",
"ni_syscall 84",				/* was sys_lstat */
"readlink",			/* 85 */
"uselib",
"swapon",
"reboot",
"old_readdir",					/* used by libc4 */
"old_mmap",			/* 90 */	/* used by libc4 */
"munmap",
"truncate",
"ftruncate",
"fchmod",
"fchown16",			/* 95 */
"getpriority",
"setpriority",
"ni_syscall 98",				/* was sys_profil */
"statfs",
"fstatfs",			/* 100 */
"ni_syscall 101",
"socketcall",
"syslog",
"setitimer",
"getitimer",			/* 105 */
"newstat",
"newlstat",
"newfstat",
"ni_syscall 109",				/* was sys_uname */
"ni_syscall 110",		/* 110 */	/* was sys_iopl */
"vhangup",
"ni_syscall 112",
"syscall",					/* call a syscall */
"wait4",
"swapoff",			/* 115 */
"sysinfo",
"ipc",
"fsync",
"sigreturn",
"clone",			/* 120 */
"setdomainname",
"newuname",
"ni_syscall 123",
"adjtimex",
"mprotect",			/* 125 */
"sigprocmask",
"create_module",
"init_module",
"delete_module",
"get_kernel_syms",		/* 130 */
"quotactl",
"getpgid",
"fchdir",
"bdflush",
"sysfs",			/* 135 */
"personality",
"ni_syscall 137",				/* _sys_afs_syscall */
"setfsuid16",
"setfsgid16",
"llseek",			/* 140 */
"getdents",
"select",
"flock",
"msync",
"readv",			/* 145 */
"writev",
"getsid",
"fdatasync",
"sysctl",
"mlock",			/* 150 */
"munlock",
"mlockall",
"munlockall",
"sched_setparam",
"sched_getparam",		/* 155 */
"sched_setscheduler",
"sched_getscheduler",
"sched_yield",
"sched_get_priority_max",
"sched_get_priority_min",	/* 160 */
"sched_rr_get_interval",
"nanosleep",
"mremap",
"setresuid16",
"getresuid16",			/* 165 */
"ni_syscall 166",
"query_module",
"poll",
"nfsservctl",
"setresgid16",			/* 170 */
"getresgid16",
"prctl",
"rt_sigreturn",
"rt_sigaction",
"rt_sigpending",		/* 175 */
"rt_sigprocmask",
"rt_sigtimedwait",
"rt_sigqueueinfo",
"rt_sigsuspend",
"pread",			/* 180 */
"pwrite",
"chown16",
"getcwd",
"capget",
"capset",			/* 185 */
"sigaltstack",
"sendfile",
"ni_syscall 188",
"ni_syscall 189",
"vfork",			/* 190 */
"getrlimit",
"mmap2",
"truncate64",
"ftruncate64",
"stat64",			/* 195 */
"lstat64",
"fstat64",
"lchown",
"getuid",
"getgid",			/* 200 */
"geteuid",
"getegid",
"setreuid",
"setregid",
"getgroups",			/* 205 */
"setgroups",
"fchown",
"setresuid",
"getresuid",
"setresgid",			/* 210 */
"getresgid",
"chown",
"setuid",
"setgid",
"setfsuid",			/* 215 */
"setfsgid",
"getdents64",
"pivot_root",
"mincore",
"madvise",			/* 220 */
"fcntl64",
"ni_syscall 222",				/* TUX */
"ni_syscall 223",				/* Security */
"gettid",
"readahead",			/* 225 */
"",
};


/* Number of traps possible on i386 */
#define NB_TRAPS_I386   19

/* Trap according to its ID on i386 */
char *LinuxI386TrapID[NB_TRAPS_I386] =
{
"divide error",
"debug",
"nmi",
"int3",
"overflow",
"bounds",
"invalidop",
"device notavailable",
"double fault",
"copro segment overrun",
"invalid TSS",
"segment not present",
"stack segment",
"GPF", /* most people know this one by name */
"page fault",
"spurious interrupt",
"copro error",
"alignment check",
"reserved"
};

/* Number of traps possible on PPC (see arch/ppc/kernel/head.S for details) */
#define NB_TRAPS_PPC   48 /* From 0x0100 to 0x2f00 */

/* Trap according to it's ID on PPC 4xx (see PowerPC manual for more details) */
/*
 Maximum string length is 30 -- see hardcoded value of lString[] in
 EGIDrawLinuxEvents().
 123456789012345678901234567890
 */
char *LinuxPPC4xxTrapID[NB_TRAPS_PPC] =
{
"0x0000", /* Nothing for the first one */ 
"Critical Interrupt",
"Machine check",
"Data Storage",
"Instruction Storage",
"External Interrupt",
"Alignment",
"Program Check",
"Page Fault",
"0x0900",
"0x0A00",
"0x0B00",
"System call",
"0x0D00",
"0x0E00",
"0x0F00",
"Programmable Interval Timer",
"Data TLB Miss",
"Instruction TLB Miss",
"0x1300",
"0x1400",
"0x1500",
"0x1600",
"0x1700",
"0x1800",
"0x1900",
"0x1A00",
"0x1B00",
"0x1C00",
"0x1D00",
"0x1E00",
"0x1F00",
"Debug",
"0x2100",
"0x2200",
"0x2300",
"0x2400",
"0x2500",
"0x2600",
"0x2700",
"0x2800",
"0x2900",
"0x2A00",
"0x2B00",
"0x2C00",
"0x2D00",
"0x2E00",
"0x2F00"
};

/* Trap according to its ID on PPC 6xx/7xx/74xx/8260/POWER3 (see PowerPC manual for more details) */
char *LinuxPPC6xxTrapID[NB_TRAPS_PPC] =
{
"0x0000", /* Nothing for the first one */ 
"Reset",
"Machine check",
"Data Access",
"Instruction Access",
"External Interrupt",
"Alignment",
"Program Check",
"FP Unavailable",
"Decrementer",
"0x0A00",
"0x0B00",
"System call",
"Trace",
"0x0E00",
"0x0F00",
"Instruction TLB Miss",
"Data TLB Miss on Load",
"Data TLB Miss on Store",
"Instruction Address Breakpoint",
"0x1400 (sys management)",
"0x1500",
"0x1600",
"TAU",
"0x1800",
"0x1900",
"0x1A00",
"0x1B00",
"0x1C00",
"0x1D00",
"0x1E00",
"0x1F00",
"Run mode",
"0x2100",
"0x2200",
"0x2300",
"0x2400",
"0x2500",
"0x2600",
"0x2700",
"0x2800",
"0x2900",
"0x2A00",
"0x2B00",
"0x2C00",
"0x2D00",
"0x2E00",
"0x2F00"
};

/* Trap according to its ID on PPC 8xx (see PowerPC manual for more details) */
char *LinuxPPC8xxTrapID[NB_TRAPS_PPC] =
{
"0x0000", /* Nothing for the first one */ 
"Reset",
"Machine check",
"Data Access",
"Instruction Access",
"External Interrupt",
"Alignment",
"Program Check",
"FP Unavailable",
"Decrementer",
"0x0A00",
"0x0B00",
"System call",
"Trace",
"0x0E00",
"0x0F00",
"Software Emulation",
"Instruction TLB Miss",
"Data TLB Miss on Store",
"Instruction TLB Error",
"Data TLB Error",
"0x1500",
"0x1600",
"TAU",
"0x1800",
"0x1900",
"0x1A00",
"0x1B00",
"0x1C00",
"0x1D00",
"0x1E00",
"0x1F00",
"0x2000",
"0x2100",
"0x2200",
"0x2300",
"0x2400",
"0x2500",
"0x2600",
"0x2700",
"0x2800",
"0x2900",
"0x2A00",
"0x2B00",
"0x2C00",
"0x2D00",
"0x2E00",
"0x2F00"
};

/* Trap according to its ID on PPC iSeries (see PowerPC manual for more details) */
char *LinuxPPCiSeriesTrapID[NB_TRAPS_PPC] =
{
"0x0000", /* Nothing for the first one */ 
"Reset",
"Machine check",
"Data Access",
"Instruction Access",
"External Interrupt",
"Alignment",
"Program Check",
"FP Unavailable",
"Decrementer",
"0x0A00",
"0x0B00",
"System call",
"Single Step",
"0x0E00",
"0x0F00",
"0x1000",
"0x1100",
"0x1200",
"Instruction Address Breakpoint",
"0x1400 (sys management)",
"0x1500",
"0x1600",
"TAU",
"0x1800",
"0x1900",
"0x1A00",
"0x1B00",
"0x1C00",
"0x1D00",
"0x1E00",
"0x1F00",
"Run mode",
"0x2100",
"0x2200",
"0x2300",
"0x2400",
"0x2500",
"0x2600",
"0x2700",
"0x2800",
"0x2900",
"0x2A00",
"0x2B00",
"0x2C00",
"0x2D00",
"0x2E00",
"0x2F00"
};


/* Number of traps possible on SuperH (see arch/sh/kernel/entry.S for details) */
#define NB_TRAPS_SH   48

/* Trap according to its EXPEVT/INTEVT ID>>5 (see SuperH manual for more details) */
char *LinuxSHTrapID[NB_TRAPS_SH] =
{
"Reset",    	    	    	    /* 0x000 */
"UDI Trap", 	    	    	    /* 0x020 */
"TLB Miss Load",    	    	    /* 0x040 */
"TLB Miss Store",    	    	    /* 0x060 */
"Initial Page Write",	    	    /* 0x080 */
"TLB Protection Violation Load",    /* 0x0A0 */
"TLB Protection Violation Store",   /* 0x0C0 */
"Address Error Load",	    	    /* 0x0E0 */
"Address Error Store",	    	    /* 0x100 */
"FPU Error",	    	    	    /* 0x120 SH4 only */
"TLB Multiple Hit",    	    	    /* 0x140 */
"System Call",	    	    	    /* 0x160 trapa instruction */
"Illegal Instruction", 	    	    /* 0x180 */
"Slot Illegal Instruction", 	    /* 0x1A0 */
"NMI", 	    	    	    	    /* 0x1C0 */
"User Break Point" 	    	    /* 0x1E0 */

/*
 * Codes 0x200 and up are various internal and external interrupts,
 * the exact configuration of which depends on the CPU model
 */
 
#if 0
/* These SH4 exceptions are currently not recorded in the kernel tracer */
"FPU Disable",	    	    	    /* 0x800 */
"Slot FPU Disable" 	    	    /* 0x820 */
#endif
};

#define NB_TRAPS_390   65              
                                       
/* Trap according to its ID on 390 */ 
char *Linux390TrapID[NB_TRAPS_390] =   
{                                      
"0x00000000", /* Nothing for the first one */ 
"Operation",                           
"Privileged oper",                     
"Execute",                             
"Protection",                          
"Addressing",                          
"Specification",                       
"Data",                                
"Fixed-pt overflow",                   
"Fixed-pt divide",                     
"Decimal overflow",                    
"Decimal divide",                      
"HFP exp. overflow",                   
"HFP exp. underflow",                  
"HFP significance",                    
"HFP divide",                          
"Segment translation",                 
"Page translation",   
"Translation spec",   
"Special operation",  
"reserved",           
"Operand",            
"Trace table",        
"reserved",           
"reserved",           
"reserved",           
"reserved",           
"reserved",           
"Space-switch event", 
"Square root",        
"reserved",           
"PC-transl spec",     
"AFX translation",    
"ASX translation",    
"ASN-transl spec",    
"LX translation",     
"EX translation",     
"Primary authority",  
"Secondary auth",     
"reserved",           
"reserved",           
"ALET specification", 
"ALEN translation",   
"ALE sequence",       
"ASTE validity",      
"ASTE sequence",      
"Extended authority", 
"reserved",           
"Stack full",         
"Stack empty",        
"Stack specification",
"Stack type",         
"Stack operation",    
"reserved",           
"reserved",           
"reserved",           
"ASCE type",          
"Region first trans", 
"Region second trans",
"Region third trans",
"reserved",          
"reserved",          
"reserved",          
"reserved",          
"Monitor event"    
}; 

#define NB_TRAPS_MIPS   32
                                       
/* Trap according to its ID on MIPS */ 
/*
 Maximum string length is 30 -- see hardcoded value of lString[] in
 EGIDrawLinuxEvents().
 123456789012345678901234567890
 */
char *LinuxMIPSTrapID[NB_TRAPS_MIPS] =
{
"External Interrupt",
"TLB Modification",
"TLB Load",
"TLB Store",
"Address Error on Load",
"Address Error on Store",
"Bus Error on Instruction fetch",
"Bus Error on Data Load",
"Syscall",
"Breakpoint",
"Reserved Instruction",
"Coprocessor Unusable",
"Arithmetic Overflow",
"Trap",
"Virtual Coherency Error Icache",	/* future: MC */
"Floating Point",			/* future: NDC */
"Coprocessor 2",
"reserved 17",
"reserved 18",
"reserved 19",
"reserved 20",
"reserved 21",
"reserved 22",
"Watch",
"Machine Check",
"reserved 25",
"reserved 26",
"reserved 27",
"reserved 28",
"reserved 29",
"reserved 30",
"Virtual Coherency Error Dcache"
};

/* Number of traps possible on ARM */
#define NB_TRAPS_ARM   19

/* Trap according to its ID on ARM */
char *LinuxARMTrapID[NB_TRAPS_ARM] =
{
"reserved 0",
"debug",
"reserved 2",
"reserved 3",
"reserved 4",
"reserved 5",
"undefined instruction",	/* invalidop */
"reserved 7",
"reserved 8",
"reserved 9",
"reserved 10",
"segment not present",
"reserved 12",
"reserved 13",
"page fault",
"reserved 15",
"reserved 16",
"reserved 17",
"data abort"			/* machine check */
};

static char* NullString = "";

/* For S/390 traps only */
static char LocalTrapString[17] = "ttttttttttttttttt";
static char* NumString = (char *)LocalTrapString;

/* System call name according to it's ID */
char* LinuxSyscallString(db*  pmDB, int pmID)
{
  switch(pmDB->ArchType)
    {
    case TRACE_ARCH_TYPE_I386:
      if(pmID <= NB_SYSCALLS_I386)
	return LinuxI386SyscallID[pmID];
      else
	return NullString;
      break;

    case TRACE_ARCH_TYPE_PPC:
      if(pmID <= NB_SYSCALLS_PPC)
	return LinuxPPCSyscallID[pmID];
      else
	return NullString;
      break;

    case TRACE_ARCH_TYPE_SH:
      if(pmID <= NB_SYSCALLS_SH)
	return LinuxSHSyscallID[pmID];
      else
	return NullString;
      break;

    case TRACE_ARCH_TYPE_S390:
      if(pmID <= NB_SYSCALLS_390)
	return Linux390SyscallID[pmID];
      else
	return NullString;
      break;

    case TRACE_ARCH_TYPE_MIPS:
      if(pmID <= NB_SYSCALLS_MIPS)
	return LinuxMIPSSyscallID[pmID];
      else
	return NullString;

    case TRACE_ARCH_TYPE_ARM:
      if(pmID <= NB_SYSCALLS_ARM)
	return LinuxARMSyscallID[pmID];
      else
	return NullString;
      break;
    };

  /* Just in case */
  return NullString;
}

/* Trap according to it's ID */
char *LinuxTrapString(db*  pmDB, uint64_t pmID)
{
  int    lID = (int) pmID;  /* The signed integer version of the 64-bit value */

  switch(pmDB->ArchType)
    {
    case TRACE_ARCH_TYPE_I386:
      if(lID < NB_TRAPS_I386)
	return LinuxI386TrapID[lID];
      else
	return NullString;
      break;

    case TRACE_ARCH_TYPE_PPC:
      /* Shift the ID */
      lID = lID >> 8;

      /* Is this ID reasonable? */
      if(lID < NB_TRAPS_PPC)
	/* Depending on the variant */
	switch(pmDB->ArchVariant)
	  {
	  case TRACE_ARCH_VARIANT_PPC_4xx :
	    return LinuxPPC4xxTrapID[lID];
	  case TRACE_ARCH_VARIANT_PPC_6xx :
	    return LinuxPPC6xxTrapID[lID];
	  case TRACE_ARCH_VARIANT_PPC_8xx :
	    return LinuxPPC8xxTrapID[lID];
	  case TRACE_ARCH_VARIANT_PPC_ISERIES :
	    return LinuxPPCiSeriesTrapID[lID];
	  }
      else
	return NullString;
      break;

    case TRACE_ARCH_TYPE_SH:
      if(lID < NB_TRAPS_SH)
	return LinuxSHTrapID[lID];
      else
	return NullString;
      break;

    case TRACE_ARCH_TYPE_S390:
      if(lID < NB_TRAPS_390)
	return Linux390TrapID[lID];
      else
        {
	/* if not in the range, */
	/* format numeric into character form */
	/* since we have a 64 bit number, we have to format it as a long long %016llx -mg */
        sprintf(NumString,"0x%016llx", pmID);
        return NumString; 
        }
      break;

    case TRACE_ARCH_TYPE_MIPS:
      if(lID < NB_TRAPS_MIPS)
	return LinuxMIPSTrapID[lID];
      else
	return NullString;

    case TRACE_ARCH_TYPE_ARM:
      if(lID < NB_TRAPS_ARM)
	return LinuxARMTrapID[lID];
      else
	return NullString;
      break;
    };

  /* Just in case */
  return NullString;
}
 
