/*
   RTAITables.c : Data tables for RTAI.
   Copyright (C) 2000, 2001 Karim Yaghmour (karim@opersys.com).
 
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

#include <linux/sys.h>  /* for NR_syscalls */

#include <LinuxEvents.h>
#include <RTAIEvents.h>
#include <RTAITables.h>

/* Size of event structures */
int RTAIEventStructSizeDefaults[TRACE_RTAI_MAX + 1] =
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
  0                                      /* TRACE_RTAI_MOUNT */,
  0                                      /* TRACE_RTAI_UMOUNT */,
  sizeof(trace_rtai_global_irq_entry)    /* TRACE_RTAI_GLOBAL_IRQ_ENTRY */,
  0                                      /* TRACE_RTAI_GLOBAL_IRQ_EXIT */,
  sizeof(trace_rtai_own_irq_entry)       /* TRACE_RTAI_OWN_IRQ_ENTRY */,
  0                                      /* TRACE_RTAI_OWN_IRQ_EXIT */,
  sizeof(trace_rtai_trap_entry)          /* TRACE_RTAI_TRAP_ENTRY */,
  0                                      /* TRACE_RTAI_TRAP_EXIT */,
  sizeof(trace_rtai_srq_entry)           /* TRACE_RTAI_SRQ_ENTRY */,
  0                                      /* TRACE_RTAI_SRQ_EXIT */,
  sizeof(trace_rtai_switchto_linux)      /* TRACE_RTAI_SWITCHTO_LINUX */,
  sizeof(trace_rtai_switchto_rt)         /* TRACE_RTAI_SWITCHTO_RT */,
  sizeof(trace_rtai_sched_change)        /* TRACE_RTAI_SCHED_CHANGE */,
  sizeof(trace_rtai_task)                /* TRACE_RTAI_TASK */,
  sizeof(trace_rtai_timer)               /* TRACE_RTAI_TIMER */,
  sizeof(trace_rtai_sem)                 /* TRACE_RTAI_SEM */,
  sizeof(trace_rtai_msg)                 /* TRACE_RTAI_MSG */,
  sizeof(trace_rtai_rpc)                 /* TRACE_RTAI_RPC */,
  sizeof(trace_rtai_mbx)                 /* TRACE_RTAI_MBX */,
  sizeof(trace_rtai_fifo)                /* TRACE_RTAI_FIFO */,
  sizeof(trace_rtai_shm)                 /* TRACE_RTAI_SHM */,
  sizeof(trace_rtai_posix)               /* TRACE_RTAI_POSIX */,
  sizeof(trace_rtai_lxrt)                /* TRACE_RTAI_LXRT */,
  sizeof(trace_rtai_lxrti)               /* TRACE_RTAI_LXRTI */
};

int RTAIEventStructSize(db* pmDB, int pmEventID)
{
  /* No need for special treatment for S/390 traces since RTAI doesn't run on
   S/390 ... yet ... */
  return RTAIEventStructSizeDefaults[pmEventID];
}

/* Event description strings */
char *RTAIEventID[TRACE_RTAI_MAX + 1] = 
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
"Change mask",
"RT-Mount RTAI",
"RT-UMount RTAI",
"RT-Global IRQ entry",
"RT-Global IRQ exit",
"RT-CPU-own IRQ entry",
"RT-CPI-own IRQ exit",
"RT-Trap entry",
"RT-Trap exit",
"RT-SRQ entry",
"RT-SRQ exit",
"RT-Switch to Linux",
"RT-Switch to RT",
"RT-Sched change",
"RT-Task",
"RT-Timer",
"RT-Semaphore",
"RT-Message",
"RT-RPC",
"RT-Mail box",
"RT-FIFO",
"RT-Shared mem",
"RT-Posix",
"RT-LXRT",
"RT-LXRT-informed",
};

char* RTAIEventString(db* pmDB, int pmEventID, event* pmEvent)
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
      if(pCustomType->Event.id == pCustom->id)
	return (pCustomType->Event.type);
    }

  /* Return the default string */
  return RTAIEventID[pmEventID];
}

/* Event strings the user can specify at the command line to omit or trace */
char *RTAIEventOT[TRACE_RTAI_MAX + 1] =
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
"",   /* New event */
"CSTM", /* Custom event */
"CHMSK",
"HEARTBEAT",
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
};

