/*
   Tables.c : Data tables for the Linux Trace Toolkit.
   Copyright (C) 1999, 2000, 2001 Karim Yaghmour (karim@opersys.com).
 
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
     K.Y., 31/05/2000, Added generalized support for multiple tables
     K.Y., 29/10/1999, Initial typing.
*/

#include <Tables.h>
#include <LinuxEvents.h>
#include <LinuxTables.h>
#if SUPP_RTAI
#include <RTAIEvents.h>
#include <RTAITables.h>
#endif /* SUPP_RTAI */
#include <EventDB.h>

/* Initial values of structures */
int**   EventStructSize = (int**)  &LinuxEventStructSizeDefaults;
char*   (*EventString)(db*, int, event*) = &LinuxEventString;
char*   (*SyscallString)(db*, int);
char*   (*TrapString)(db*, uint64_t);

/* We don't know the type of trace before hand. Therefore, use the table that includes
   the most events possible. */
#if SUPP_RTAI
int     MaxEventID      = TRACE_RTAI_MAX;
char**  EventOT         = (char**)  RTAIEventOT;
#else
int     MaxEventID      = TRACE_MAX;
char**  EventOT         = (char**)  LinuxEventOT;
#endif
