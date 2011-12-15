/*
 * TraceDaemon.h
 *
 * Copyright (C) 1999, 2000 Karim Yaghmour.
 *
 * This is distributed under GPL.
 *
 * Header for trace toolkit.
 *
 * History : 
 *    K.Y., 01/10/1999, Initial typing.
 *
 */

#ifndef __TRACE_DAEMON_MAIN_HEADER__
#define __TRACE_DAEMON_MAIN_HEADER__

#include <LTTTypes.h>
#include <DevCommand.h>
#include <LinuxEvents.h>
#include <RTAIEvents.h>

#include <sys/time.h>
#include <sys/types.h>

/* Number of traced events */
#if SUPP_RTAI
#define TRACE_MAX_EVENTS            TRACE_RTAI_MAX
#else
#define TRACE_MAX_EVENTS            TRACE_MAX
#endif

/* String lengths */
#define TD_FILE_NAME_STRLEN       40  /* File name string length */
#define TD_STATUS_STRLEN         500  /* String length to read from process status file */
#define TD_PROCES_NAME_STRLEN    256  /* Length of process name */
#define MAX_CPUS                  32  /* Same as kernel NR_CPUS */

#define TRACE_HANDLE               0  /* Pre-defined handle */
#define FLIGHT_HANDLE              1  /* Pre-defined handle */
 
/* Options structure */
typedef struct _options
{
  int                ConfigDefault;      /* Use the tracing device's default configuration */
  int                SpecifyEvents;      /* Does the user want to specify the events to trace */
  trace_event_mask   EventMask;          /* The mask specifying the events to be traced */
  int                SpecifyDetails;     /* Does the user want to specify for which events he wants the details */
  trace_event_mask   DetailsMask;        /* The mask specifying for which events the details are logged */
  int                ConfigCPUID;        /* Should the CPUID be logged */
  int                ConfigPID;          /* Does the user want to log only the events belonging to a certain PID */
  int                PID;                /* The designated PID */
  int                ConfigPGRP;         /* Does the user want to log only the events belonging to a certain process group */
  int                PGRP;               /* The designated process group */
  int                ConfigGID;          /* Does the user want to log only the events belonging to a certain GID */
  int                GID;                /* The designated GID */
  int                ConfigUID;          /* Does the user want to log only the events belonging to a certain UID */
  int                UID;                /* The designated UID */
  int                ConfigSyscallDepth; /* Fetch a certain syscall depth */
  int                SyscallDepth;       /* The designated depth */
  int                ConfigSyscallBound; /* Fetch the syscall eip that fits in a certain address bounds */
  uint32_t           UpperBound;         /* The upper bound eip */
  uint32_t           LowerBound;         /* The lower bound eip */
  int                ConfigTime;         /* Should the deamon only run for a given period of time */
  struct timeval     Time;               /* Time for which daemon should run */
  char*              TraceFileName;      /* Name of file used to dump raw trace */
  char*              ProcFileName;       /* Name of file contaning /proc information as of daemon's startup */

  int                ModifyTraceMask;    /* If true, then only modify trace masks and do not become daemon */
  int                PrintTraceMask;     /* If true, then only print the curren trace mask and do not become daemon */
  int                UseLocking;         /* If true, then use the locking version */
  int                DataAreaSize;       /* Size of each buffer */
  int                NBuffers;           /* Number of buffers */
  int                UseTSC;             /* Use TSC rather than time deltas */
  int                FlightRecorder;     /* i.e. 'flight recorder' mode */
  int                Stop;               /* stop and release channel */
  int                Snapshot;           /* Save channel contents to file */
  int                Status;             /* Save channel contents to file */
} options;

/* Lock-free tracing definitions */

/* For the lockless scheme:

   A trace index is composed of two parts, a buffer number and a buffer 
   offset.  The actual number of buffers allocated is a run-time decision, 
   although it must be a power of two for efficient computation.  We define 
   a maximum number of bits for the buffer number, because the fill_count 
   array in buffer_control must have a fixed size.  offset_bits must be at 
   least as large as the maximum event size+start/end buffer event size+
   lost size word (since a buffer must be able to hold an event of maximum 
   size).  Making offset_bits larger reduces fragmentation.  Making it 
   smaller increases trace responsiveness. */

#define TRACE_MIN_BUFFER_SIZE CUSTOM_EVENT_MAX_SIZE+8192
#define TRACE_MAX_BUFFERS 256 /* Max # of buffers */
/* We need at least enough room for the max custom event, and we also need
   room for the start and end event.  We also need it to be a power of 2. */
#define TRACER_LOCKLESS_MIN_BUF_SIZE CUSTOM_EVENT_MAX_SIZE + 8192 /* 16K */
/* Because we use atomic_t as the type for fill_counts, which has only 24
   usable bits, we have 2**24 = 16M max for each buffer. */
#define TRACER_LOCKLESS_MAX_BUF_SIZE 0x1000000 /* 16M */
/* Since we multiply n buffers by the buffer size, this provides a sanity
   check, much less than the 256*16M=4G possible. */
#define TRACER_LOCKLESS_MAX_TOTAL_BUF_SIZE 0x8000000 /* 128M */
#define TRACER_LOCKLESS_DEFAULT_BUF_SIZE 0x80000 /* 512K */
#define TRACER_LOCKLESS_DEFAULT_N_BUFFERS 4
#define TRACER_LOCKING_MAX_TOTAL_BUF_SIZE 256000000 /* 256M */
#define TRACER_LOCKING_DEFAULT_BUF_SIZE  1000000
#define TRACER_LOCKING_DEFAULT_N_BUFFERS 2

#define TRACE_MAX_BUFFER_NUMBER(bufno_bits) (1UL << (bufno_bits))
#define TRACE_BUFFER_SIZE(offset_bits) (1UL << (offset_bits))
#define TRACE_BUFFER_OFFSET_MASK(offset_bits) (TRACE_BUFFER_SIZE(offset_bits) - 1)

#define TRACE_BUFFER_NUMBER_GET(index, offset_bits) ((index) >> (offset_bits))
#define TRACE_BUFFER_OFFSET_GET(index, mask) ((index) & (mask))
#define TRACE_BUFFER_OFFSET_CLEAR(index, mask) ((index) & ~(mask))

#define TRACE_PROC_ROOT "/proc/ltt"

/* The maximum number of CPUs the kernel might run on, must match kernel */
#define MAX_NR_CPUS 32
#define MAX_NR_TRACES 2

/* Per-trace status info */
struct trace_info
{
	int			active;
	unsigned int		trace_handle;
	int			paused;
	int			flight_recorder;
	int			use_locking;
	int			using_tsc;
	uint32_t		n_buffers;
	uint32_t		buf_size;
	trace_event_mask	traced_events;
	trace_event_mask	log_event_details_mask;
	uint32_t		buffers_produced[MAX_NR_CPUS];
};

/* Status info for all traces */
struct tracer_status
{
	int num_cpus;
	struct trace_info traces[MAX_NR_TRACES];
};

/* Data structure for sharing per-buffer information between driver and 
   daemon (via ioctl) */
struct buffer_control
{
	int16_t cpu_id;
	uint32_t buffer_switches_pending;
	uint32_t buffer_control_valid;

	uint32_t buf_size;
	uint32_t n_buffers;
	uint32_t cur_idx;
	uint32_t buffers_produced;
	uint32_t buffers_consumed;
	/* atomic_t has only 24 usable bits, limiting us to 16M buffers */
	int32_t buffer_complete[TRACE_MAX_BUFFERS];
};

/* Data structure for sharing buffer-commit information between driver and 
   daemon (via ioctl) */
struct buffers_committed
{
	uint8_t cpu_id;
	uint32_t buffers_consumed;
};

/* Data structure for sharing trace handle/cpu id pair between driver and 
   daemon */
struct cpu_mmap_data
{
	uint8_t cpu_id;
	uint32_t map_size;
};

/* Data structure for sharing architecture-specific info between driver and 
   daemon (via ioctl) */
struct ltt_arch_info
{
	int32_t n_cpus;
	int32_t page_shift;
};

void SigHandler(int pmSignalID);

#endif  /* __TRACE_DAEMON_MAIN_HEADER__ */
