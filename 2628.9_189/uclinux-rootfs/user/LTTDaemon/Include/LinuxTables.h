/*
 * LinuxTables.h
 *
 * Copyright (C) 2000, 2001, 2002 Karim Yaghmour (karym@opersys.com).
 *
 * This is distributed under GPL.
 *
 * Tables for data for the trace toolkit.
 *
 * History : 
 *    K.Y., 31/05/2000, Initial typing.
 */

#ifndef  __TRACE_TOOLKIT_LINUX_TABLES__
#define  __TRACE_TOOLKIT_LINUX_TABLES__

#include <EventDB.h>

/* Event structure size defaults */
extern int LinuxEventStructSizeDefaults[];

/* Event structure sizes */
extern int LinuxEventStructSize
             (db*     /* Event database */,
	      int     /* Event ID */);

/* Event description strings */
extern char* LinuxEventString
             (db*     /* Event database */,
	      int     /* Event ID */,
	      event*  /* Event */);

/* Event strings the user can specify at the command line to omit or trace */
extern char *LinuxEventOT[];

/* System call name according to it's ID for */
extern char* LinuxSyscallString
             (db*  /* Event database */,
	      int  /* Syscall Id */);

/* Trap according to it's ID  for */
extern char *LinuxTrapString
             (db*       /* Event database */,
	      uint64_t  /* Syscall Id */);

#endif /* __TRACE_TOOLKIT_LINUX_TABLES__ */
