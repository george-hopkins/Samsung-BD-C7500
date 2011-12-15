/*
   EventDB.c : Event database engine for trace toolkit.
   Copyright (C) 2000, 2001, 2002, 2003, 2004 Karim Yaghmour (karim@opersys.com).
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
     K.Y., 05/09/01, Added custom event formatting get/set API.
     K.Y., 19/06/01, Moved event database into libltt.
     JAL,  05/12/00, Added support for cross-platform trace decoding
                     (andy_lowe@mvista.com)
     K.Y., 15/11/00, Custom events support added.
     K.Y., 01/06/00, Added support for RTAI events
     K.Y., 06/02/00, Major modifications in order to support using the trace
                     file as the database rather than reading the file into
                     internals structure, which consumed large amounts of
                     memory.
     K.Y., 11/10/99, Modified to conform with complete kernel instrumentation.
     K.Y., 26/06/99, Initial typing.
*/

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <Tables.h>
#include <LinuxTables.h>
#include <RTAITables.h>
#include <EventDB.h>
#include <RTAIDB.h>

#include "EventDBI.h"

static int junkprint = 0;

/******************************************************************
 * Function :
 *    DBEventGetPosition()
 * Description :
 *    Returns the virtual address at which the event description
 *    is found.
 * Parameters :
 *    pmDB, Event database to which this event belongs.
 *    pmEvent, Pointer to said event.
 * Return values :
 *    Virtual address of event.
 * History :
 *    K.Y., 30/01/2000, Initial typing.
 * Note :
 *    This function is internal and should not be called from the
 *    outside of EventDB.c.
 ******************************************************************/
#if 0
void* DBIEventGetPosition(db* pmDB, event* pmEvent)
{
  void*   pAddress;  /* Address of event */

  /* Get the address of the begining of the buffer */
  pAddress = (void*) (pmEvent->BufID * pmDB->BufferSize);

  /* Add the offset to the event */
  pAddress += pmEvent->EventPos;

  /* Is this address valid */
  if(pAddress > (void*) pmDB->TraceSize)
    return NULL;

  /* Add the start of the trace address */
  pAddress += (uint32_t) pmDB->TraceStart;

  /* Return the address to the caller */
  return pAddress;
}
#endif

/******************************************************************
 * Function :
 *    DBEventID()
 * Description :
 *    Returns the ID of the given event.
 * Parameters :
 *    pmDB, Event database to which this event belongs.
 *    pmEvent, Pointer to said event.
 * Return values :
 *    Event ID.
 * History :
 *    K.Y., 30/01/2000, Initial typing.
 * Note :
 *    This should never be used to get the ID of TRACE_START or
 *    TRACE_BUFFER_START.
 ******************************************************************/
inline uint8_t DBIEventID(db*   pmDB,
			  void* pmEventAddr)
{
  void*     pReadAddr;  /* Address from which to read */
  uint8_t   lEventID;   /* The event ID */

  /* Get the start position of this event */
  pReadAddr = pmEventAddr;
  
  /* Does this trace contain CPUIDs */
  if(pmDB->LogCPUID == TRUE)
    /* Skip the CPUID */
    pReadAddr += sizeof(uint8_t);

  /* Read the event ID */
  lEventID = RFT8(pmDB, *(uint8_t*) pReadAddr);
  
  /* Give the ID to the caller */
  return lEventID;
}
inline uint8_t DBEventID(db* pmDB, event* pmEvent)
{
  void* pReadAddr;  /* Address from which to read */

  /* Get the start position of this event */
  pReadAddr = DBIEventGetPosition(pmDB, pmEvent);
  
  /* Give the ID to the caller */
  return DBIEventID(pmDB, pReadAddr);
}

/******************************************************************
 * Function :
 *    DBEventTime()
 * Description :
 *    Sets the time according to the time of occurence of the event.
 * Parameters :
 *    pmDB, Event database to which this event belongs.
 *    pmEvent, Pointer to said event.
 *    pmTime, Pointe to time that has to be set.
 * Return values :
 *    None.
 * History :
 *    K.Y., 30/01/2000, Initial typing.
 ******************************************************************/
void DBIEventTime(db*             pmDB,
		  uint32_t        pmBufID,
		  void*           pmEventAddr,
		  struct timeval* pmTime)
{
  int                 i;             /* Generic index */
  void*               pReadAddr;     /* Address from which to read */
  void*               pEndEventAddr; /* Address of end of buffer event */
  uint32_t            lSizeLost;     /* Number of bytes lost at the end of the last buffer */
  uint32_t            lTimeDelta;    /* Time delta between this event and the start of the buffer */
  uint32_t*           pBufStartTSC;  /* Pointer to TSC of start of the buffer */
  uint32_t            lBufStartTSC;  /* TSC of start of the buffer */
  uint32_t*           pBufEndTSC;    /* Pointer to TSC of end of the buffer */
  uint32_t            lBufEndTSC;    /* TSC of end of the buffer */
  struct timeval      lBufTotalTime; /* Total time for this buffer */
  uint32_t            lBufTotalUSec; /* Total time for this buffer in usecs */
  double              lBufTotalTSC;  /* Total TSC cycles for this buffer */
  double              lTSCPerUSec;   /* Cycles per usec */
  double              lEventTotalTSC;/* Total cycles from start for event */
  int                 lRollovers;    /* Number of TSC rollovers in this buffer */
  double              lEventUSec;    /* Total usecs from start for event */
  struct timeval      lTimeOffset;   /* Time offset in struct timeval */
  struct timeval*     pBufStartTime; /* Pointer to time at which this buffer started */
  struct timeval      lBufStartTime; /* Time at which this buffer started */
  struct timeval*     pBufEndTime;   /* Pointer to time at which this buffer ended */
  struct timeval      lBufEndTime;   /* Time at which this buffer ended */
  void**              lRolloverAddrs;/* Addresses of events where TSC rollovers occurred */
  int                 lEventRollovers;/* Number of TSC rollovers for this event */

  /* Get the time of start of the buffer to which the event belongs */
  pReadAddr = pmBufID * pmDB->BufferSize + pmDB->TraceStart;

  /* Skip the event ID */
  pReadAddr += sizeof(uint8_t);

  /* Skip the time delta */
  pReadAddr += sizeof(uint32_t);

  /* Get a pointer to the time of begining of this buffer */
  pBufStartTime = &(((trace_buffer_start*) pReadAddr)->Time);

  /* Read the time from the trace */
  lBufStartTime.tv_sec  = RFT32(pmDB, pBufStartTime->tv_sec);
  lBufStartTime.tv_usec = RFT32(pmDB, pBufStartTime->tv_usec);

  if(pmDB->UseTSC == TRUE)
    {
    /* Get a pointer to the TSC of begining of this buffer */
    pBufStartTSC = &(((trace_buffer_start*) pReadAddr)->TSC);
    lBufStartTSC = RFT32(pmDB, *(uint32_t*) pBufStartTSC);

    /* Get the address of the last byte of the buffer containing event */
    pEndEventAddr = pmDB->TraceStart + pmBufID * pmDB->BufferSize 
      + pmDB->BufferSize;

    /* Get the number of bytes lost at the end of the buffer */
    lSizeLost = RFT32(pmDB, *((uint32_t*) (pEndEventAddr - sizeof(uint32_t))));

    /* Get the address of the TRACE_BUFFER_END event */
    pEndEventAddr -= lSizeLost;

    /* Does this trace contain CPUIDs */
    if(pmDB->LogCPUID == TRUE)
      /* Skip the CPUID */
      pEndEventAddr += sizeof(uint8_t);

    /* Skip the event ID */
    pEndEventAddr += sizeof(uint8_t);

    /* Skip the time delta */
    pEndEventAddr += sizeof(uint32_t);

    /* Get a pointer to the time of ending of this buffer */
    pBufEndTime = &(((trace_buffer_end*) pEndEventAddr)->Time);

    /* Read the time from the trace */
    lBufEndTime.tv_sec  = RFT32(pmDB, pBufEndTime->tv_sec);
    lBufEndTime.tv_usec = RFT32(pmDB, pBufEndTime->tv_usec);

    /* Get a pointer to the TSC of ending of this buffer */
    pBufEndTSC = &(((trace_buffer_end*) pEndEventAddr)->TSC);
    lBufEndTSC = RFT32(pmDB, *(uint32_t*) pBufEndTSC);

    /* Set values for flight-recorder */
    if (pmDB->FlightRecorder && !pmDB->Preprocessing)
      {
      if (lBufEndTime.tv_sec == 0)
	{
	lBufEndTime = pmDB->LastBufEndTime;
	lBufEndTSC = pmDB->LastEventTSC;
	}
      }
    }

  /* Get the start position of this event */
  pReadAddr = pmEventAddr;
  
  /* Does this trace contain CPUIDs */
  if(pmDB->LogCPUID == TRUE)
    /* Skip the CPUID */
    pReadAddr += sizeof(uint8_t);

  /* Skip the event ID */
  pReadAddr += sizeof(uint8_t);

  /* Get the time delta */
  lTimeDelta = RFT32(pmDB, *(uint32_t*) pReadAddr);

  /* Compute TSC cycles per USec */
  if((pmDB->FlightRecorder && pmDB->UseTSC == TRUE) && (pmDB->Preprocessing))
    {
    pmDB->LastBufID = pmBufID;
    pmDB->LastBufStartTime = lBufStartTime;
    pmDB->LastBufStartTSC = lBufStartTSC;
    pmDB->LastEventTSC = lTimeDelta;

    if (lBufEndTSC != 0)
      {
      /* Calculate the total time for this buffer */
      DBTimeSub(lBufTotalTime, lBufEndTime, lBufStartTime);

      /* Calculate the total cycles for this bufffer */
      lRollovers = pmDB->TSCRollovers[pmBufID].NRollovers;
      if(lBufEndTSC < lBufStartTSC)
	lBufTotalTSC = (UINT_MAX - lBufStartTSC)
	  + ((double)(lRollovers - 1) * (double)UINT_MAX) + lBufEndTSC;
      else
	lBufTotalTSC = (lBufEndTSC - lBufStartTSC)
	  + ((double)lRollovers * (double)UINT_MAX);

      /* Convert the total time to usecs */
      lBufTotalUSec = lBufTotalTime.tv_sec * 1000000 + lBufTotalTime.tv_usec;

      /* If the total cycles are 0, we have problems */ 
      if((lBufTotalUSec == 0) || (lBufTotalTSC == 0))
	printf("TraceVisualizer: Found buffer with 0 total TSC cycles \n");

      /* How many cycles were there per usec? */
      pmDB->LastGoodTSCPerUSec = lBufTotalTSC/lBufTotalUSec;
      }
    }

  /* If this trace is using TSC */
  if(pmDB->UseTSC == TRUE)
    {
    /* If we're preprocessing TSCs */
    if(pmDB->Preprocessing)
      {
      if(pmDB->CurBufID == pmBufID) /* First event in buf */
	{
	pmDB->PrevTSC = lTimeDelta;
	pmDB->CurBufID++;
	}
      else /* Not first event in buf */
	{
	if(lTimeDelta <= pmDB->PrevTSC) /* rolled over */
	  {
	  uint32_t n;

	  /* If we need (more) memory for rollover buffer data,
	     (re)allocate it */ 
	  if(pmBufID >= pmDB->TSCRolloversSize)
	    if(ReallocRollovers(pmDB))
	      {
	      printf("Memory allocation problem \n");
	      exit(1);
	      }
	  /* If we need (more) memory for rollover addresses for
	     this buffer, (re)allocate it */ 
	  if((pmDB->TSCRollovers[pmBufID].RolloverAddrs == NULL) ||
	     ((pmDB->TSCRollovers[pmBufID].RolloverAddrsSize % ROLLOVER_ADDRS_REALLOC_SIZE) == 0))
	    if(ReallocRolloverAddrs(&(pmDB->TSCRollovers[pmBufID])))
	      {
	      printf("Memory allocation problem \n");
	      exit(1);
	      }
	  /* Save the address of the rollover */
	  n = (pmDB->TSCRollovers[pmBufID]).NRollovers++;
	  (pmDB->TSCRollovers[pmBufID]).RolloverAddrs[n] = pmEventAddr;
	  }
	pmDB->PrevTSC = lTimeDelta;
	}
      return; /* We're preprocessing, no need to do anything else now */
      }

    /* Calculate the total time for this buffer */
    DBTimeSub(lBufTotalTime, lBufEndTime, lBufStartTime);

    /* Calculate the total cycles for this bufffer */
    lRollovers = pmDB->TSCRollovers[pmBufID].NRollovers;
    if(lBufEndTSC < lBufStartTSC)
      lBufTotalTSC = (UINT_MAX - lBufStartTSC)
	+ ((double)(lRollovers - 1) * (double)UINT_MAX) + lBufEndTSC;
    else
      lBufTotalTSC = (lBufEndTSC - lBufStartTSC)
	+ ((double)lRollovers * (double)UINT_MAX);

    /* Convert the total time to usecs */
    lBufTotalUSec = lBufTotalTime.tv_sec * 1000000 + lBufTotalTime.tv_usec;

      /* If the total usecs are 0, we have problems */ 
      if(lBufTotalUSec == 0)
	{
	printf("TraceVisualizer: Found buffer (%u) with total time of 0 \n", pmBufID);
	printf("TraceVisualizer: lBufStartTime.tv_sec = %u\n", lBufStartTime.tv_sec);
	printf("TraceVisualizer: lBufStartTime.tv_usec = %u\n", lBufStartTime.tv_usec);
	printf("TraceVisualizer: lBufEndTime.tv_sec = %u\n", lBufEndTime.tv_sec);
	printf("TraceVisualizer: lBufEndTime.tv_usec = %u\n", lBufEndTime.tv_usec);
	exit(1);
	}

      /* If the total cycles are 0, we have problems */ 
      if((lBufTotalUSec == 0) || (lBufTotalTSC == 0))
	{
	printf("TraceVisualizer: Found buffer with 0 total TSC cycles \n");
	exit(1);
	}

      /* How many cycles were there per usec? */
      lTSCPerUSec = (double)lBufTotalTSC/(double)lBufTotalUSec;

      /* If the cycles per usec is too small, we have problems */ 
      if(lTSCPerUSec == 0)
	{
	printf("TraceVisualizer: Calculated cycles per usec of 0 \n");
	exit(1);
	}

      /* Calculate the total rollovers for this for this event */
      lEventRollovers = 0;
      if(lRollovers) /* There were rollovers in the buffer */
	{
	lRolloverAddrs = pmDB->TSCRollovers[pmBufID].RolloverAddrs;
	for(i = 0; i < lRollovers; i++)
	  if(pmEventAddr >= lRolloverAddrs[i])
	    lEventRollovers++;
	  else
	    break;
	}

      /* Calculate total time in TSCs from start of buffer for this event */
      if(lTimeDelta < lBufStartTSC)
	lEventTotalTSC = (UINT_MAX - lBufStartTSC) 
	  + ((double)(lEventRollovers - 1) * (double)UINT_MAX) + (double)lTimeDelta;
      else
	lEventTotalTSC = (lTimeDelta - lBufStartTSC)
	  + ((double)lEventRollovers * (double)UINT_MAX);

      /* Convert it to usecs */
      lEventUSec = lEventTotalTSC/lTSCPerUSec;

      /* Determine offset in struct timeval */
      lTimeOffset.tv_usec = (long)lEventUSec % 1000000;
      lTimeOffset.tv_sec  = (long)lEventUSec / 1000000;
    }
  else
    {
      /* Determine offset in struct timeval */
      lTimeOffset.tv_usec = lTimeDelta % 1000000;
      lTimeOffset.tv_sec  = lTimeDelta / 1000000;
    }

  /* Add the time and put result in passed timeval */
  DBTimeAdd((*pmTime), lBufStartTime, lTimeOffset);
}
void DBEventTime(db* pmDB, event* pmEvent, struct timeval* pmTime)
{
  void*               pReadAddr;     /* Address from which to read */

  /* Get the begining address of the event */
  pReadAddr = DBIEventGetPosition(pmDB, pmEvent);

  /* Get this event's time */
  DBIEventTime(pmDB,
	       pmEvent->BufID,
	       pReadAddr,
	       pmTime);
}

/******************************************************************
 * Function :
 *    DBEventCPUID()
 * Description :
 *    Returns the CPUID on which this event occured
 * Parameters :
 *    pmDB, Event database to which this event belongs.
 *    pmEvent, Pointer to said event.
 * Return values :
 *    The CPUID on which this event occured.
 *    0 if the CPUID isn't part of the trace.
 * History :
 *    K.Y., 30/01/2000, Initial typing.
 ******************************************************************/
uint8_t DBIEventCPUID(db*   pmDB,
		      void* pmEventAddr)
{
  int   lCPUID;     /* The event CPU ID */

  /* Does this trace contain CPUIDs */
  if(pmDB->LogCPUID == TRUE)
    lCPUID = RFT8(pmDB, *(uint8_t*) pmEventAddr);
  else
    lCPUID = 0;
  
  /* Give the CPU ID to the caller */
  return lCPUID;
}
uint8_t DBEventCPUID(db* pmDB, event* pmEvent)
{
  void* pReadAddr;  /* Address from which to read */

  /* Get the start position of this event */
  pReadAddr = DBIEventGetPosition(pmDB, pmEvent);
  
  /* Give the caller the CPUID */
  return DBIEventCPUID(pmDB, pReadAddr);
}

/******************************************************************
 * Function :
 *    DBEventStruct()
 * Description :
 *    Returns a pointer to the begining of the structure of this
 *    event..
 * Parameters :
 *    pmDB, Event database to which this event belongs.
 *    pmEvent, Pointer to said event.
 * Return values :
 *    Pointer to begining of structure.
 * History :
 *    K.Y., 31/01/2000, Initial typing.
 ******************************************************************/
void* DBIEventStruct(db*   pmDB,
		     void* pmEventAddr)
{
  void* pReadAddr;  /* Address from which to read */
  
  /* Get the address of the event */
  pReadAddr = pmEventAddr;
  
  /* Does this trace contain CPUIDs */
  if(pmDB->LogCPUID == TRUE)
    /* Skip the CPUID */
    pReadAddr += sizeof(uint8_t);

  /* Skip the event ID */
  pReadAddr += sizeof(uint8_t);

  /* Skip the time delta */
  pReadAddr += sizeof(uint32_t);

  /* Give the pointer to the caller */
  return pReadAddr;
}
void* DBEventStruct(db* pmDB, event* pmEvent)
{
  void* pReadAddr;  /* Address from which to read */
  
  /* Get the address of the event */
  pReadAddr = DBIEventGetPosition(pmDB, pmEvent);
  
  /* Give the structure pointer to the caller */
  return DBIEventStruct(pmDB, pReadAddr);
}

/******************************************************************
 * Function :
 *    DBEventString()
 * Description :
 *    Fills the passed string with a text description of the
 *    event.
 * Parameters :
 *    pmDB, Event database to which this event belongs.
 *    pmBufID, ID of buffer to which event belongs
 *    pmEventAddr, Pointer to said event.
 *    pmString, Character string to be filled.
 *    pmStringLen, Length of character string.
 * Return values :
 *    Number of bytes copied to string on success.
 *    Negative number on failure.
 * History :
 *    K.Y., 30/01/2000, Initial typing.
 ******************************************************************/
int DBILinuxEventString(db*              pmDB,
			uint32_t         pmBufID,
			void*            pmEventAddr,
			char*            pmString,
			int              pmStringLen)
{
  int                 i;            /* Generic index */
  void*               pReadAddr;    /* Address from which to read */
  void*               pEventStruct; /* Pointer to begining of structure of event */
  uint8_t             lEventID;     /* The event's ID */
  struct timeval      lTime;        /* Time at which the event occured */
  customEventDesc*    pCustomType;  /* Pointer to custom event type */

  /* Get the event's ID */
  lEventID = DBIEventID(pmDB, pmEventAddr);

  /* Initialize event string */
  pmString[0] = '\0';

  /* Is there a structure describing this event */
  if((pmDB->EventStructSize(pmDB, lEventID) == 0) || !(ltt_test_bit(lEventID, &(pmDB->DetailsMask))))
    return 0;

  /* Get the structure of this event */
  pEventStruct = pReadAddr = DBIEventStruct(pmDB, pmEventAddr);

  /* Depending on the event's type */
  switch(lEventID)
    {
    /* TRACE_START */
    case TRACE_START :
      /* Get the time of this event */
      DBIEventTime(pmDB, pmBufID, pmEventAddr, &lTime);

      /* Format the string describing the event */
      sprintf(pmString,
	      "START AT : (%ld, %ld)",
	      lTime.tv_sec,
	      lTime.tv_usec);
      break;

    /* TRACE_SYSCALL_ENTRY */
    case TRACE_SYSCALL_ENTRY :
      /* Format the string describing the event */
      if(pmDB->ArchType == TRACE_ARCH_TYPE_I386)
	sprintf(pmString,
		"SYSCALL : %s; EIP : 0x%08X",
		pmDB->SyscallString(pmDB, RFT8(pmDB, SYSCALL_EVENT(pEventStruct)->syscall_id)),
		RFT32(pmDB, SYSCALL_EVENT(pEventStruct)->address));
      else if (pmDB->ArchType == TRACE_ARCH_TYPE_PPC)
	sprintf(pmString,
		"SYSCALL : %s; IP : 0x%08X",
		pmDB->SyscallString(pmDB, RFT8(pmDB, SYSCALL_EVENT(pEventStruct)->syscall_id)),
		RFT32(pmDB, SYSCALL_EVENT(pEventStruct)->address));
      else if (pmDB->ArchType == TRACE_ARCH_TYPE_SH)
	sprintf(pmString,
		"SYSCALL : %s; PC : 0x%08X",
		pmDB->SyscallString(pmDB, RFT8(pmDB, SYSCALL_EVENT(pEventStruct)->syscall_id)),
		RFT32(pmDB, SYSCALL_EVENT(pEventStruct)->address));
      else if (pmDB->ArchType == TRACE_ARCH_TYPE_S390)
        sprintf(pmString,
	        "SYSCALL : %s; PSWA : 0x%08X",
		pmDB->SyscallString(pmDB, RFT8(pmDB, SYSCALL_EVENT(pEventStruct)->syscall_id)),
		RFT32(pmDB, SYSCALL_EVENT(pEventStruct)->address));
      else if (pmDB->ArchType == TRACE_ARCH_TYPE_MIPS)
	sprintf(pmString,
		"SYSCALL : %s; IP : 0x%08X",
		pmDB->SyscallString(pmDB, RFT8(pmDB, SYSCALL_EVENT(pEventStruct)->syscall_id)),
		RFT32(pmDB, SYSCALL_EVENT(pEventStruct)->address));
      else if (pmDB->ArchType == TRACE_ARCH_TYPE_ARM)
	sprintf(pmString,
		"SYSCALL : %s; PC : 0x%08X",
		pmDB->SyscallString(pmDB, RFT8(pmDB, SYSCALL_EVENT(pEventStruct)->syscall_id)),
		RFT32(pmDB, SYSCALL_EVENT(pEventStruct)->address));
      break;

    /* TRACE_TRAP_ENTRY */
    case TRACE_TRAP_ENTRY :
      /* Format the string describing the event */
      if(pmDB->ArchType == TRACE_ARCH_TYPE_I386)
	sprintf(pmString,
		"TRAP : %s; EIP : 0x%08X",
		pmDB->TrapString(pmDB, RFT16(pmDB, TRAP_EVENT(pEventStruct)->trap_id)),
		RFT32(pmDB, TRAP_EVENT(pEventStruct)->address));
      else if (pmDB->ArchType == TRACE_ARCH_TYPE_PPC)
	sprintf(pmString,
		"TRAP : %s; IP : 0x%08X",
		pmDB->TrapString(pmDB, RFT16(pmDB, TRAP_EVENT(pEventStruct)->trap_id)),
		RFT32(pmDB, TRAP_EVENT(pEventStruct)->address));
      else if (pmDB->ArchType == TRACE_ARCH_TYPE_SH)
	sprintf(pmString,
		"TRAP : %s; PC : 0x%08X",
		pmDB->TrapString(pmDB, RFT16(pmDB, TRAP_EVENT(pEventStruct)->trap_id)),
		RFT32(pmDB, TRAP_EVENT(pEventStruct)->address));
      else if (pmDB->ArchType == TRACE_ARCH_TYPE_S390)
	sprintf(pmString,
		"TRAP : %s; PSWA : 0x%08X",
		pmDB->TrapString(pmDB, RFT64(pmDB, TRAP_EVENT_S390(pEventStruct)->trap_id)),
		RFT32(pmDB, TRAP_EVENT_S390(pEventStruct)->address));
      else if (pmDB->ArchType == TRACE_ARCH_TYPE_MIPS)
	sprintf(pmString,
		"TRAP : %s; IP : 0x%08X",
		pmDB->TrapString(pmDB, RFT16(pmDB, TRAP_EVENT(pEventStruct)->trap_id)),
		RFT32(pmDB, TRAP_EVENT(pEventStruct)->address));
      else if (pmDB->ArchType == TRACE_ARCH_TYPE_ARM)
	sprintf(pmString,
		"TRAP : %s; PC : 0x%08X",
 		pmDB->TrapString(pmDB, RFT16(pmDB, TRAP_EVENT(pEventStruct)->trap_id)),
 		RFT32(pmDB, TRAP_EVENT(pEventStruct)->address));
      break;

    /* TRACE_IRQ_ENTRY */
    case TRACE_IRQ_ENTRY :
      /* Are we inside the kernel */
      if(IRQ_EVENT(pEventStruct)->kernel)
	{
	/* Format the string describing the event */
	sprintf(pmString,
		"IRQ : %d, IN-KERNEL",
		RFT8(pmDB, IRQ_EVENT(pEventStruct)->irq_id));
	}
      else
	{
	/* Format the string describing the event */
	sprintf(pmString,
		"IRQ : %d",
		RFT8(pmDB, IRQ_EVENT(pEventStruct)->irq_id));
	}
      break;

    /* TRACE_SCHEDCHANGE */
    case TRACE_SCHEDCHANGE :
      /* Format the string describing the event */
      sprintf(pmString,
	      "IN : %d; OUT : %d; STATE : %d",
	      RFT32(pmDB, SCHED_EVENT(pEventStruct)->in),
	      RFT32(pmDB, SCHED_EVENT(pEventStruct)->out),
	      RFT32(pmDB, SCHED_EVENT(pEventStruct)->out_state));
      break;

    /* TRACE_SOFT_IRQ */
    case TRACE_SOFT_IRQ :
      /* Depending on the type of soft-irq event */
      switch(RFT8(pmDB, SOFT_IRQ_EVENT(pEventStruct)->event_sub_id))
	{
	/* TRACE_SOFT_IRQ_BOTTOM_HALF */
	case TRACE_SOFT_IRQ_BOTTOM_HALF :
	  sprintf(pmString,
		  "BH : %d",
		  RFT32(pmDB, SOFT_IRQ_EVENT(pEventStruct)->event_data));
	  break;

	/* TRACE_SOFT_IRQ_SOFT_IRQ */
	case TRACE_SOFT_IRQ_SOFT_IRQ :
	  sprintf(pmString,
		  "SOFT IRQ : %d",
		  RFT32(pmDB, SOFT_IRQ_EVENT(pEventStruct)->event_data));
	  break;

	/* TRACE_SOFT_IRQ_TASKLET_ACTION */
	case TRACE_SOFT_IRQ_TASKLET_ACTION :
	  sprintf(pmString,
		  "TASKLET ACTION : 0x%08X",
		  RFT32(pmDB, SOFT_IRQ_EVENT(pEventStruct)->event_data));
	  break;

	/* TRACE_SOFT_IRQ_TASKLET_HI_ACTION */
	case TRACE_SOFT_IRQ_TASKLET_HI_ACTION :
	  sprintf(pmString,
		  "TASKLET HI ACTION : 0x%08X",
		  RFT32(pmDB, SOFT_IRQ_EVENT(pEventStruct)->event_data));
	  break;
	}
      break;

    /* TRACE_PROCESS */
    case TRACE_PROCESS :
      /* Depending on the process event */
      switch(RFT8(pmDB, PROC_EVENT(pEventStruct)->event_sub_id))
	{
	/* TRACE_PROCESS_KTHREAD */
	case TRACE_PROCESS_KTHREAD :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "KTHREAD : %d; FUNCTION ADDRESS : 0x%08X",
		  RFT32(pmDB, PROC_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, PROC_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_PROCESS_FORK */
	case TRACE_PROCESS_FORK :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "FORK : %d",
		  RFT32(pmDB, PROC_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_PROCESS_EXIT */
	case TRACE_PROCESS_EXIT :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "EXIT");
	  break;

	/* TRACE_PROCESS_WAIT */
	case TRACE_PROCESS_WAIT :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "WAIT PID : %d" /* The PID waited for */,
		  RFT32(pmDB, PROC_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_PROCESS_SIGNAL */
	case TRACE_PROCESS_SIGNAL :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SIGNAL : %d; PID : %d" /* Signal nbr and destination PID */,
		  RFT32(pmDB, PROC_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, PROC_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_PROCESS_WAKEUP */
	case TRACE_PROCESS_WAKEUP :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "WAKEUP PID : %d; STATE : %d" /* The PID to wake up and it's state before waking */,
		  RFT32(pmDB, PROC_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, PROC_EVENT(pEventStruct)->event_data2));
	  break;
	}
      break;

    /* TRACE_FILE_SYSTEM */
    case TRACE_FILE_SYSTEM :
      /* Depending on the file system event */
      switch(RFT8(pmDB, FS_EVENT(pEventStruct)->event_sub_id))
	{
	/* TRACE_FILE_SYSTEM_BUF_WAIT_START */
	case TRACE_FILE_SYSTEM_BUF_WAIT_START :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "START I/O WAIT");
	  break;

	/* TRACE_FILE_SYSTEM_BUF_WAIT_END */
	case TRACE_FILE_SYSTEM_BUF_WAIT_END :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "END I/O WAIT");
	  break;

	/* TRACE_FILE_SYSTEM_EXEC or TRACE_FILE_SYSTEM_OPEN */
	case TRACE_FILE_SYSTEM_EXEC :
	case TRACE_FILE_SYSTEM_OPEN :
	  /* Skip this structure to get the file name */
	  pReadAddr += pmDB->EventStructSize(pmDB, TRACE_FILE_SYSTEM);

	  /* Format the string describing the event */
	  if(FS_EVENT(pEventStruct)->event_sub_id == TRACE_FILE_SYSTEM_EXEC)
	    sprintf(pmString,
		    "EXEC : %s",
		    (char*) pReadAddr);
	  else
	    sprintf(pmString,
		    "OPEN : %s; FD : %d" /* The opened file and the returned file descriptor */,
		    (char*) pReadAddr,
		    RFT32(pmDB, FS_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_FILE_SYSTEM_CLOSE */
	case TRACE_FILE_SYSTEM_CLOSE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "CLOSE : %d",
		  RFT32(pmDB, FS_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_FILE_SYSTEM_READ */
	case TRACE_FILE_SYSTEM_READ :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "READ : %d; COUNT : %d" /* FD read and how much */,
		  RFT32(pmDB, FS_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, FS_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_FILE_SYSTEM_WRITE */
	case TRACE_FILE_SYSTEM_WRITE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "WRITE : %d; COUNT : %d" /* FD written to and how much */,
		  RFT32(pmDB, FS_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, FS_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_FILE_SYSTEM_SEEK */
	case TRACE_FILE_SYSTEM_SEEK :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SEEK : %d; OFFSET : %d",
		  RFT32(pmDB, FS_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, FS_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_FILE_SYSTEM_IOCTL */
	case TRACE_FILE_SYSTEM_IOCTL :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "IOCTL : %d; COMMAND : 0x%X" /* FD ioctled on and command */,
		  RFT32(pmDB, FS_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, FS_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_FILE_SYSTEM_SELECT */
	case TRACE_FILE_SYSTEM_SELECT :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SELECT : %d; TIMEOUT : %ld" /* FD select on and timeout */,
		  RFT32(pmDB, FS_EVENT(pEventStruct)->event_data1),
		  (signed long) (RFT32(pmDB, FS_EVENT(pEventStruct)->event_data2)));
	  break;

	/* TRACE_FILE_SYSTEM_POLL */
	case TRACE_FILE_SYSTEM_POLL :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "POLL : %d; TIMEOUT : %d" /* FD polled on and timeout */,
		  RFT32(pmDB, FS_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, FS_EVENT(pEventStruct)->event_data2));
	  break;
	}
      break;

    /* TRACE_TIMER */
    case TRACE_TIMER :
      /* Depending on the timer event */
      switch(RFT8(pmDB, TIMER_EVENT(pEventStruct)->event_sub_id))
	{
	/* TRACE_TIMER_EXPIRED */
	case TRACE_TIMER_EXPIRED :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "TIMER EXPIRED");
	  break;

	/* TRACE_TIMER_SETITIMER */
	case TRACE_TIMER_SETITIMER :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SET ITIMER : %d; INTERVAL : %d; VALUE : %d"  /* WHICH, interval, value */,
		  RFT8(pmDB, TIMER_EVENT(pEventStruct)->event_sdata),
		  RFT32(pmDB, TIMER_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, TIMER_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_TIMER_SETTIMEOUT */
	case TRACE_TIMER_SETTIMEOUT :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SET TIMEOUT : %d",
		  RFT32(pmDB, TIMER_EVENT(pEventStruct)->event_data1));
	  break;
	}
      break;

    /* TRACE_MEMORY */
    case TRACE_MEMORY :
      /* Depending on the memory event */
      switch(RFT8(pmDB, MEM_EVENT(pEventStruct)->event_sub_id))
	{
	/* TRACE_MEMORY_PAGE_ALLOC */
	case TRACE_MEMORY_PAGE_ALLOC :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PAGE ALLOC ORDER : %ld",
		  RFT32(pmDB, MEM_EVENT(pEventStruct)->event_data));
	  break;

	/* TRACE_MEMORY_PAGE_FREE */
	case TRACE_MEMORY_PAGE_FREE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PAGE FREE ORDER : %ld" /* The adress of what is freed is useless, it's size is */,
		  RFT32(pmDB, MEM_EVENT(pEventStruct)->event_data));
	  break;

	/* TRACE_MEMORY_SWAP_IN */
	case TRACE_MEMORY_SWAP_IN :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SWAP IN : 0x%08X" /* Page being swapped in */,
		  (uint32_t) RFT32(pmDB, MEM_EVENT(pEventStruct)->event_data));
	  break;

	/* TRACE_MEMORY_SWAP_OUT */
	case TRACE_MEMORY_SWAP_OUT :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SWAP OUT : 0x%08X" /* Page being swapped OUT */,
		  (uint32_t) RFT32(pmDB, MEM_EVENT(pEventStruct)->event_data));
	  break;

	/* TRACE_MEMORY_PAGE_WAIT_START */
	case TRACE_MEMORY_PAGE_WAIT_START :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "START PAGE WAIT");
	  break;

	/* TRACE_MEMORY_PAGE_WAIT_END */
	case TRACE_MEMORY_PAGE_WAIT_END :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "END PAGE WAIT");
	  break;
	}
      break;

    /* TRACE_SOCKET */
    case TRACE_SOCKET :
      switch(RFT8(pmDB, SOCKET_EVENT(pEventStruct)->event_sub_id))
	{
	/* TRACE_SOCKET_CALL */
	case TRACE_SOCKET_CALL :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SO; CALL : %d; FPM(FD) : %d" /* call's id and first parameter */,
		  RFT32(pmDB, SOCKET_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, SOCKET_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_SOCKET_CREATE */
	case TRACE_SOCKET_CREATE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SO CREATE; FD : %d; TYPE : %d",
		  RFT32(pmDB, SOCKET_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, SOCKET_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_SOCKET_SEND */
	case TRACE_SOCKET_SEND :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SO SEND; TYPE : %d; SIZE : %d",
		  RFT32(pmDB, SOCKET_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, SOCKET_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_SOCKET_RECEIVE */
	case TRACE_SOCKET_RECEIVE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SO RECEIVE; TYPE : %d; SIZE : %d",
		  RFT32(pmDB, SOCKET_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, SOCKET_EVENT(pEventStruct)->event_data2));
	  break;

	}
      break;

    /* TRACE_IPC */
    case TRACE_IPC :
      /* Depending on the ipc event */
      switch(RFT8(pmDB, IPC_EVENT(pEventStruct)->event_sub_id))
	{
	/* TRACE_IPC_CALL */
	case TRACE_IPC_CALL :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "IPC; CALL : %d; FPM(ID) : %d" /* call's id and first parameter */,
		  RFT32(pmDB, IPC_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, IPC_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_IPC_IPC_MSG_CREATE */
	case TRACE_IPC_MSG_CREATE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "MSG CREATE; ID : %d; FLAGS : 0x%08X",
		  RFT32(pmDB, IPC_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, IPC_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_IPC_IPC_SEM_CREATE */
	case TRACE_IPC_SEM_CREATE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SEM CREATE; ID : %d; FLAGS : 0x%08X",
		  RFT32(pmDB, IPC_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, IPC_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_IPC_IPC_SHM_CREATE */
	case TRACE_IPC_SHM_CREATE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SHM CREATE; ID : %d; FLAGS : 0x%08X",
		  RFT32(pmDB, IPC_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, IPC_EVENT(pEventStruct)->event_data2));
	  break;
	}
      break;

    /* TRACE_NETWORK */
    case TRACE_NETWORK :
      /* Depending on the network event */
      switch(RFT8(pmDB, NET_EVENT(pEventStruct)->event_sub_id))
	{
	/* TRACE_NETWORK_PACKET_IN */
	case TRACE_NETWORK_PACKET_IN :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PACKET IN; PROTOCOL : %d",
		  RFT32(pmDB, NET_EVENT(pEventStruct)->event_data));
	  break;
	/* TRACE_NETWORK_PACKET_OUT */
	case TRACE_NETWORK_PACKET_OUT :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PACKET OUT; PROTOCOL : %d",
		  RFT32(pmDB, NET_EVENT(pEventStruct)->event_data));
	  break;
	}
      break;

    /* TRACE_NEW_EVENT */
    case TRACE_NEW_EVENT :
      sprintf(pmString,
	      "NEW EVENT TYPE : %s; CUSTOM EVENT ID : %d",
	      NEW_EVENT(pEventStruct)->type,
	      RFT32(pmDB, NEW_EVENT(pEventStruct)->id));
      break;

    /* TRACE_CUSTOM */
    case TRACE_CUSTOM :
      /* Get the custom event description */
      pCustomType = DBIEventGetCustomDescription(pmDB, pmEventAddr);

      /* Just in case */
      if(pCustomType == NULL)
	{
	printf("LibLTT: Unable to find custom event description\n");
	break;
	}
      
      /* Depending on the type of formatting */
      switch(RFT32(pmDB, pCustomType->Event.format_type))
	{
	/* No format */
	case CUSTOM_EVENT_FORMAT_TYPE_NONE:
	  sprintf(pmString, "No format");
	  break;

	/* String format */
	case CUSTOM_EVENT_FORMAT_TYPE_STR:
	  snprintf(pmString,
		   RFT32(pmDB, CUSTOM_EVENT(pEventStruct)->data_size),
		   "%s",
		   (char*) (pEventStruct + sizeof(trace_custom)));
	  break;

	/* Hexadecimal format */
	case CUSTOM_EVENT_FORMAT_TYPE_HEX:
	  /* Print out this event's details in raw format */
	  for(i = 0; i < RFT32(pmDB, CUSTOM_EVENT(pEventStruct)->data_size); i++)
	    sprintf(pmString + i*3,
		    "%02X ",
		    RFT8(pmDB, *((uint8_t*) (pEventStruct + sizeof(trace_custom) + i))));
	  break;

	/* XML format */
	case CUSTOM_EVENT_FORMAT_TYPE_XML:
	  sprintf(pmString, "XML format: %s", pCustomType->Event.form);
	  break;

	/* IBM format */
	case CUSTOM_EVENT_FORMAT_TYPE_IBM:
	  sprintf(pmString, "IBM format");
	  break;
	}
      break;

    /* All other cases */
    default :
      pmString[0] = '\0';
    } /* End of switch */

  /* Give the caller the length of the string printed */
  return strlen(pmString);
}
int DBEventString(db* pmDB, event* pmEvent, char* pmString, int pmStringLen)
{
  void* pReadAddr;  /* Address from which to read */
  
  /* Get the address of the event */
  pReadAddr = DBIEventGetPosition(pmDB, pmEvent);

  /* Is this a Linux kernel event */
  if(DBIEventID(pmDB, pmEvent) <= TRACE_MAX)
    /* Give the caller the length of the string printed */
    return DBILinuxEventString(pmDB,
			       pmEvent->BufID,
			       pReadAddr,
			       pmString,
			       pmStringLen);
  else
#if SUPP_RTAI
    /* Give the caller the length of the string printed */
    return RTAIDBIEventString(pmDB,
			      pmEvent->BufID,
			      pReadAddr,
			      pmString,
			      pmStringLen);
#else
    return 0;
#endif /* SUPP_RTAI */
}

/******************************************************************
 * Function :
 *    DBEventSize()
 * Description :
 *    Returns the size of the given event.
 * Parameters :
 *    pmDB, Event database to which this event belongs.
 *    pmEvent, Pointer to said event.
 * Return values :
 *    Size of said event.
 * History :
 *    K.Y., 30/01/2000, Initial typing.
 ******************************************************************/
uint16_t DBIEventSize(db*   pmDB,
		      void* pmEventAddr)
{
  void*     pReadAddr;     /* Address from which to read */
  void*     pEventStruct;  /* Pointer to begining of structure of event */
  uint8_t   lEventID;      /* The event ID */

  /* Get the start position of this event */
  pReadAddr = pmEventAddr;
  
  /* Does this trace contain CPUIDs */
  if(pmDB->LogCPUID == TRUE)
    /* Skip the CPUID */
    pReadAddr += sizeof(uint8_t);

  /* Read the event ID */
  lEventID = RFT8(pmDB, *(uint8_t*) pReadAddr);

  /* Skip the event ID */
  pReadAddr += sizeof(uint8_t);

  /* Skip the time delta */
  pReadAddr += sizeof(uint32_t);

  /* Are the event details present? */
  if(ltt_test_bit(lEventID, &(pmDB->DetailsMask)))
    {
    /* Remember the structure's address */
    pEventStruct = pReadAddr;

    /* Skip this event's structure */
    pReadAddr += pmDB->EventStructSize(pmDB, lEventID);

    /* Some events have a variable size */
    switch(lEventID)
      {
      /* TRACE_FILE_SYSTEM */
      case TRACE_FILE_SYSTEM :
	if((RFT8(pmDB, FS_EVENT(pEventStruct)->event_sub_id) == TRACE_FILE_SYSTEM_OPEN)
	 ||(RFT8(pmDB, FS_EVENT(pEventStruct)->event_sub_id) == TRACE_FILE_SYSTEM_EXEC))
	  pReadAddr += RFT32(pmDB, FS_EVENT(pEventStruct)->event_data2) + 1;
	break;

	/* TRACE_CUSTOM */
      case TRACE_CUSTOM :
	pReadAddr += RFT32(pmDB, CUSTOM_EVENT(pEventStruct)->data_size);
	break;
      }
    }

  /* Return the event's size */
  return (RFT16(pmDB, *(uint16_t*) pReadAddr));
}
uint16_t DBEventSize(db* pmDB, event* pmEvent)
{
  void*     pReadAddr;     /* Address from which to read */

  /* Get the start position of this event */
  pReadAddr = DBIEventGetPosition(pmDB, pmEvent);
  
  /* Return the event's size */
  return DBIEventSize(pmDB, pReadAddr);
}

/******************************************************************
 * Function :
 *    DBEventControl()
 * Description :
 *    Determines wether the passed event has generated a change
 *    of control or not.
 * Parameters :
 *    pmDB, Event database to which this event belongs.
 *    pmEvent, Pointer to said event.
 * Return values :
 *    TRUE, Passed event changes control.
 *    FALSE, Passed event doesn't affect control.
 * History :
 *    K.Y., 06/02/2000, Initial typing.
 ******************************************************************/
int DBEventControlEntry(db* pmDB, event* pmEvent, int pmPID)
{
  int                lPrevEventID = pmPID; /* The previous event's ID */
#if 0
  void*              pPrevEventStruct;     /* The previous event's structure */
#endif
  void*              pEventStruct;         /* The event's structure */
  event              lPrevEvent;           /* The previous event */
  uint8_t            lEventID;             /* Event ID */

  /* Get the event's ID */
  lEventID = DBEventID(pmDB, pmEvent);

  /* Get the event's structure */
  pEventStruct = DBEventStruct(pmDB, pmEvent);

  /* Is this event a valid entry */
  if((lEventID != TRACE_SYSCALL_ENTRY)
   &&(lEventID != TRACE_TRAP_ENTRY)
   &&((lEventID != TRACE_IRQ_ENTRY)
   ||((lEventID == TRACE_IRQ_ENTRY) && (RFT8(pmDB, IRQ_EVENT(pEventStruct)->kernel) == 1))))
    return FALSE;

#if 0
  /* Go backwards in the event database */
  for(lPrevEvent = *pmEvent;
      DBEventPrev(pmDB, &lPrevEvent) == TRUE;)
    {
#endif

  /* Get the last event */
  lPrevEvent = *pmEvent;
  if(DBEventPrev(pmDB, &lPrevEvent) == FALSE)
    return FALSE;

  /* Get the last event's ID */
  lPrevEventID = DBEventID(pmDB, &lPrevEvent);

  /* Is this an exiting event */
  if(((lPrevEventID == TRACE_SYSCALL_EXIT)
    ||(lPrevEventID == TRACE_TRAP_EXIT)
    ||(lPrevEventID == TRACE_IRQ_EXIT)
    ||(lPrevEventID == TRACE_SCHEDCHANGE)
    ||(lPrevEventID == TRACE_KERNEL_TIMER)
    ||(lPrevEventID == TRACE_SOFT_IRQ)
    ||(lPrevEventID == TRACE_PROCESS)
    ||(lPrevEventID == TRACE_NETWORK))
   &&(DBEventControlExit(pmDB, &lPrevEvent, pmPID) == TRUE))
    return TRUE;

#if 0
  /* Keep the PID of the outgoing process if it's a scheduling change */
  if(lPrevEventID == TRACE_SCHEDCHANGE)
    {
    pPrevEventStruct = DBEventStruct(pmDB, &lPrevEvent);
    pmPID = RFT32(pmDB, SCHED_EVENT(pPrevEventStruct)->out);
    }
    }
#endif
  
  /* We haven't found anything */
  return FALSE;
}
int DBEventControlExit(db* pmDB, event* pmEvent, int pmPID)
{
  void*             pNextEventStruct;         /* Next event's struct */
  event             lNextEvent = *pmEvent;    /* The next event */
  uint8_t           lEventID;                 /* The event's ID */
  uint8_t           lNextEventID;             /* Next event's ID */

  /* Is this process the idle task */
  if(pmPID == 0)
    return FALSE;

  /* Get the event's ID */
  lEventID = DBEventID(pmDB, pmEvent);

  /* Get the next event */
  if(DBEventNext(pmDB, &lNextEvent) == FALSE)
    return FALSE;

  /* Get the next event's ID */
  lNextEventID = DBEventID(pmDB, &lNextEvent);

  /* Get the next event's structure */
  pNextEventStruct = DBEventStruct(pmDB, &lNextEvent);

  /* Is this event a possible exit from the kernel */
  if((lEventID == TRACE_SYSCALL_EXIT)
   ||(lEventID == TRACE_TRAP_EXIT)
   ||(lEventID == TRACE_IRQ_EXIT)
   ||(lEventID == TRACE_SCHEDCHANGE)
   ||(lEventID == TRACE_KERNEL_TIMER)
   ||(lEventID == TRACE_SOFT_IRQ)
   ||(lEventID == TRACE_PROCESS)
   ||(lEventID == TRACE_NETWORK))
    /* Is the next event an entry into the kernel */
    if((lNextEventID == TRACE_SYSCALL_ENTRY)
     ||(lNextEventID == TRACE_TRAP_ENTRY)
     ||((lNextEventID == TRACE_IRQ_ENTRY)
      &&(RFT8(pmDB, IRQ_EVENT(pNextEventStruct)->kernel) != 1)))
      return TRUE;

  return FALSE;
}
int DBEventControl(db* pmDB, event* pmEvent, systemInfo* pmSystem)
{
  int               lPID;         /* Process PID */
  uint8_t           lEventID;     /* The event's ID */
  process*          pProcess;     /* Pointer to process */             

  /* Get the event's ID */
  lEventID = DBEventID(pmDB, pmEvent);

  /* Get the process to which this event belongs */
  pProcess = DBEventProcess(pmDB, pmEvent, pmSystem, FALSE);

  /* Does this event belong to a process */
  if(pProcess != NULL)
    /* Get the process' PID */
    lPID = pProcess->PID;
  else
    return FALSE;
  
  /* Is this an entry event */
  if((lEventID == TRACE_SYSCALL_ENTRY)
   ||(lEventID == TRACE_TRAP_ENTRY)
   ||(lEventID == TRACE_IRQ_ENTRY))
    return DBEventControlEntry(pmDB, pmEvent, lPID);
  /* Is this an exit event */
  else if((lEventID == TRACE_SYSCALL_EXIT)
	||(lEventID == TRACE_TRAP_EXIT)
	||(lEventID == TRACE_IRQ_EXIT)
	||(lEventID == TRACE_SCHEDCHANGE)
	||(lEventID == TRACE_KERNEL_TIMER)
	||(lEventID == TRACE_SOFT_IRQ)
	||(lEventID == TRACE_PROCESS)
	||(lEventID == TRACE_NETWORK))
    return DBEventControlExit(pmDB, pmEvent, lPID);
  /* Is this something else */
  else
    return FALSE;
}

/******************************************************************
 * Function :
 *    DBEventGetCustomDescription()
 * Description :
 *    Returns a pointer to the custom event description.
 * Parameters :
 *    pmDB, Event database to which this event belongs.
 *    pmEvent, Pointer to said event.
 * Return values :
 *    Pointer to found description.
 *    NULL otherwise.
 * History :
 *    K.Y., 05/09/2001, Initial typing.
 ******************************************************************/
customEventDesc* DBIEventGetCustomDescription(db*   pmDB,
					      void* pmEventAddr)
{
  void*                pEventStruct;    /* Event structure */
  uint32_t             lCustomID;       /* Custom event ID */
  customEventDesc*     pCustomType;     /* Description of custom event */

  /* Check the event's ID */
  if(DBIEventID(pmDB, pmEventAddr) != TRACE_CUSTOM)
    /* No custom description for non-custom events */
    return NULL;

  /* Has the details of this event been recorded? */
  if(ltt_test_bit(TRACE_CUSTOM, &(pmDB->DetailsMask)))
    {
    /* Get the event's structure */
    pEventStruct = DBIEventStruct(pmDB, pmEventAddr);

    /* Get the custom event ID */
    lCustomID = RFT32(pmDB, CUSTOM_EVENT(pEventStruct)->id);

    /* Go through custom event hash table */
    for(pCustomType = pmDB->CustomHashTable[lCustomID % CUSTOM_EVENT_HASH_TABLE_SIZE].NextHash;
	pCustomType != NULL;
	pCustomType = pCustomType->NextHash)
      /* Is this the one we're looking for */
      if(pCustomType->ID == lCustomID)
	/* Give it to the caller */
	return pCustomType;
    }

  /* We haven't found anything */
  return NULL;
}

customEventDesc* DBEventGetCustomDescription(db* pmDB, event* pmEvent)
{  
  void* pReadAddr;  /* Address from which to read */
  
  /* Get the address of the event */
  pReadAddr = DBIEventGetPosition(pmDB, pmEvent);
  
  /* Give the description pointer to the caller */
  return DBIEventGetCustomDescription(pmDB, pReadAddr);
}

/******************************************************************
 * Function :
 *    DBEventGetFormat()
 * Description :
 *    Returns a pointer to a string which contains the formatting
 *    data of that type of event.
 * Parameters :
 *    pmDB, Event database to which this event belongs.
 *    pmEvent, Pointer to said event.
 *    pmFormatType, Pointer to integer where the formatting type
 *                  will be stored.
 * Return values :
 *    Pointer to found string.
 *    NULL otherwise.
 * History :
 *    K.Y., 05/09/2001, Initial typing.
 ******************************************************************/
char* DBEventGetFormat(db* pmDB, event* pmEvent, int* pmFormatType)
{
  customEventDesc*   pCustomType;     /* Description of custom event */

  /* Is there a custom event description for the given event */
  if((pCustomType = DBEventGetCustomDescription(pmDB, pmEvent)) != NULL)
    {
    /* Set the format type */
    *pmFormatType = (int) (RFT32(pmDB, pCustomType->Event.format_type));

    /* Return the string describing this event's format */
    return pCustomType->Event.form;
    }
  else
    /* No format for this event (for now) */
    return NULL;
}

/******************************************************************
 * Function :
 *    DBEventGetFormatByCustomID()
 * Description :
 *    Returns a pointer to a string which contains the formatting
 *    data of events identified by the given custom ID.
 * Parameters :
 *    pmDB, Event database containing the custom events.
 *    pmID, The custom ID of the event.
 *    pmFormatType, Pointer to integer where the formatting type
 *                  will be stored.
 * Return values :
 *    Pointer to found string.
 *    NULL otherwise.
 * History :
 *    K.Y., 05/09/2001, Initial typing.
 ******************************************************************/
char* DBEventGetFormatByCustomID(db* pmDB, int pmID, int* pmFormatType)
{
  customEventDesc*   pCustomType;     /* Description of custom event */

  /* Go through custom event hash table entries */
  for(pCustomType = pmDB->CustomHashTable[pmID % CUSTOM_EVENT_HASH_TABLE_SIZE].NextHash;
      pCustomType != NULL;
      pCustomType = pCustomType->NextHash)
    {
    /* Is this the custom event type we're looking for */
    if(pCustomType->ID == pmID)
      {
      /* Set the format type */
      *pmFormatType = (int) (RFT32(pmDB, pCustomType->Event.format_type));

      /* Return the string describing this event's format */
      return pCustomType->Event.form;
      }
    }

  /* We haven't found anything */
  return NULL;
}

/******************************************************************
 * Function :
 *    DBEventGetFormatByCustomType()
 * Description :
 *    Returns a pointer to a string which contains the formatting
 *    data of events identified by the given custom type string.
 * Parameters :
 *    pmDB, Event database containing the custom events.
 *    pmTypeStr, The string identifying the type of custom events.
 *    pmFormatType, Pointer to integer where the formatting type
 *                  will be stored.
 * Return values :
 *    Pointer to found string.
 *    NULL otherwise.
 * History :
 *    K.Y., 05/09/2001, Initial typing.
 * NOTE :
 *    BE CAREFUL in using this function as it will only read the
 *    first event description it finds fitting the description.
 *    In other words, if different instances of custom events are
 *    created using the same type string over and over again, only
 *    the first one will be read.
 ******************************************************************/
char* DBEventGetFormatByCustomType(db* pmDB, char* pmTypeStr, int* pmFormatType)
{
  customEventDesc*   pCustomType;     /* Description of custom event */

  /* Go through custom event list */
  for(pCustomType = pmDB->CustomEvents.Next;
      pCustomType != &(pmDB->CustomEvents);
      pCustomType = pCustomType->Next)
    {
    /* Is this the custom event type we're looking for */
    if(!strcmp(pCustomType->Event.type, pmTypeStr))
      {
      /* Set the format type */
      *pmFormatType = (int) (RFT32(pmDB, pCustomType->Event.format_type));

      /* Return the string describing this event's format */
      return pCustomType->Event.form;
      }
    }

  /* We haven't found anything */
  return NULL;
}

/******************************************************************
 * Function :
 *    DBEventSetFormat()
 * Description :
 *    Sets the formatting string and formatting type of all events
 *    of the same type as the one provided.
 * Parameters :
 *    pmDB, Event database to which this event belongs.
 *    pmEvent, Pointer to said event.
 *    pmFormatType, Type of formatting (none, str, hex, xml, ibm).
 *    pmFormatStr, Format string
 * Return values :
 *    FALSE, operation failed
 *    TRUE, operation succeeded
 * History :
 *    K.Y., 05/09/2001, Initial typing.
 ******************************************************************/
int DBEventSetFormat(db* pmDB, event* pmEvent, int pmFormatType, char* pmFormatStr)
{
  customEventDesc*   pCustomType;     /* Description of custom event */

  /* Is there a custom event description for the given event */
  if((pCustomType = DBEventGetCustomDescription(pmDB, pmEvent)) != NULL)
    {
    /* Set the format type (write the format type in reverse if needed) */
    if(pmDB->ByteRev)
      pCustomType->Event.format_type = BREV32((uint32_t) (pmFormatType));
    else
      pCustomType->Event.format_type = pmFormatType;

    /* Copy the formatting string to the event description */
    strncpy(pCustomType->Event.form, pmFormatStr, CUSTOM_EVENT_FORM_STR_LEN);

    /* Insure null termination */
    pCustomType->Event.form[CUSTOM_EVENT_FORM_STR_LEN - 1] = '\0';

    /* Tell the caller everything went well */
    return TRUE;
    }
  else
    /* This type of event isn't recognized */
    return FALSE;
}

/******************************************************************
 * Function :
 *    DBEventSetFormatByCustomID()
 * Description :
 *    Sets the formatting string and formatting type of all events
 *    of the given custom ID type.
 * Parameters :
 *    pmDB, Event database containing the custom events.
 *    pmID, The custom ID of the event.
 *    pmFormatType, Type of formatting (none, str, hex, xml, ibm).
 *    pmFormatStr, Format string
 * Return values :
 *    FALSE, operation failed
 *    TRUE, operation succeeded
 * History :
 *    K.Y., 05/09/2001, Initial typing.
 ******************************************************************/
int DBEventSetFormatByCustomID(db* pmDB, int pmID, int pmFormatType, char* pmFormatStr)
{
  customEventDesc*   pCustomType;     /* Description of custom event */

  /* Go through custom event hash table entries */
  for(pCustomType = pmDB->CustomHashTable[pmID % CUSTOM_EVENT_HASH_TABLE_SIZE].NextHash;
      pCustomType != NULL;
      pCustomType = pCustomType->NextHash)
    {
    /* Is this the custom event type we're looking for */
    if(pCustomType->ID == pmID)
      {
      /* Set the format type (write the format type in reverse if needed) */
      if(pmDB->ByteRev)
	pCustomType->Event.format_type = BREV32((uint32_t) (pmFormatType));
      else
	pCustomType->Event.format_type = pmFormatType;

      /* Copy the formatting string to the event description */
      strncpy(pCustomType->Event.form, pmFormatStr, CUSTOM_EVENT_FORM_STR_LEN);

      /* Insure null termination */
      pCustomType->Event.form[CUSTOM_EVENT_FORM_STR_LEN - 1] = '\0';

      /* Tell the caller everything went well */
      return TRUE;
      }
    }

  /* This type of event isn't recognized */
  return FALSE;
}

/******************************************************************
 * Function :
 *    DBEventSetFormatByCustomType()
 * Description :
 *    Sets the formatting string and formatting type of all events
 *    having the given custom type string.
 * Parameters :
 *    pmDB, Event database containing the custom events.
 *    pmTypeStr, The string identifying the type of custom events.
 *    pmFormatType, Type of formatting (none, str, hex, xml, ibm).
 *    pmFormatStr, Format string
 * Return values :
 *    FALSE, operation failed
 *    TRUE, operation succeeded
 * History :
 *    K.Y., 05/09/2001, Initial typing.
 * NOTE :
 *    BE CAREFUL in using this function as it will only write to the
 *    first event description it finds fitting the description.
 *    In other words, if different instances of custom events are
 *    created using the same type string over and over again, only
 *    the first one will be modified.
 ******************************************************************/
int DBEventSetFormatByCustomType(db* pmDB, char* pmTypeStr, int pmFormatType, char* pmFormatStr)
{
  customEventDesc*   pCustomType;     /* Description of custom event */

  /* Go through custom event list */
  for(pCustomType = pmDB->CustomEvents.Next;
      pCustomType != &(pmDB->CustomEvents);
      pCustomType = pCustomType->Next)
    {
    /* Is this the custom event type we're looking for */
    if(!strcmp(pCustomType->Event.type, pmTypeStr))
      {
      /* Set the format type (write the format type in reverse if needed) */
      if(pmDB->ByteRev)
	pCustomType->Event.format_type = BREV32((uint32_t) (pmFormatType));
      else
	pCustomType->Event.format_type = pmFormatType;

      /* Copy the formatting string to the event description */
      strncpy(pCustomType->Event.form, pmFormatStr, CUSTOM_EVENT_FORM_STR_LEN);

      /* Insure null termination */
      pCustomType->Event.form[CUSTOM_EVENT_FORM_STR_LEN - 1] = '\0';

      /* Tell the caller everything went well */
      return TRUE;
      }
    }

  /* This type of event isn't recognized */
  return FALSE;
}

/******************************************************************
 * Function :
 *    DBEventProcess()
 * Description :
 *    Returns a pointer to the process to which this event belongs.
 * Parameters :
 *    pmDB, Event database to which this event belongs.
 *    pmEvent, Pointer to said event.
 *    pmSystem, Pointer to system description.
 *    pmSetSwitchTime, Set the time at which this process was last
 *                     switched in starting from the current event.
 * Return values :
 *    Pointer to found process.
 *    NULL otherwise.
 * History :
 *    K.Y., 22/08/2000, Added switch-time memory.
 *    K.Y., 30/01/2000, Initial typing.
 ******************************************************************/
process* DBEventProcess(db* pmDB, event* pmEvent, systemInfo* pmSystem, int pmSetSwitchTime)
{
  int      lFound = FALSE;    /* Did we find the last scheduling change */
  int      lPID;              /* PID of process */
  int      lCPUID;            /* The ID of the CPU on which we should be looking for a sched change */
  void*    pEventStruct;      /* Event structure */
  event    lEvent;            /* Event used to backup */
  process* pProcess;          /* Pointer to the process found */

  /* Get the processor on which the event occured */
  lCPUID = DBEventCPUID(pmDB, pmEvent);

  /* Use copy of event */
  lEvent = *pmEvent;

  /* Backup to the last scheduling change */
  do
    {
    /* Is this event a scheduling change */
    if((DBEventCPUID(pmDB, &lEvent) == lCPUID) && (DBEventID(pmDB, &lEvent) == TRACE_SCHEDCHANGE))
      {
      lFound = TRUE;
      break;
      }
    } while(DBEventPrev(pmDB, &lEvent) == TRUE);
       
  /* Did we find anything */
  if(lFound == TRUE)
    {
    /* Get this event's structure */
    pEventStruct = DBEventStruct(pmDB, &lEvent);

    /* Get the PID of the process that just got scheduled in */
    lPID = RFT32(pmDB, SCHED_EVENT(pEventStruct)->in);

    /* Get the process corresponding to this PID */
    pProcess = DBGetProcByPID(lPID, pmSystem);

    /* Should we remember the time of occurence of this process switch */
    if(pmSetSwitchTime == TRUE)
      DBEventTime(pmDB, &lEvent, &(pProcess->ReportedSwitchTime));

    /* Return the process corresponding to this PID */
    return pProcess;
    }
  
  return NULL;
}

/******************************************************************
 * Function :
 *    DBEventDescription()
 * Description :
 *    Sets the internals of passed event description to represent
 *    the pointed event
 * Parameters :
 *    pmDB, Event database to which this event belongs.
 *    pmEvent, Pointer to said event.
 *    pmStringRequired, Is the event string required
 *    pmEventDescription, event to be filled
 * Return values :
 *    None
 * History :
 *    K.Y., 03/02/2000, Initial typing.
 ******************************************************************/
void DBEventDescription(db* pmDB, event* pmEvent, int pmStringRequired, eventDescription* pmEventDescription)
{
  void*     pReadAddr;     /* Address from which to read */

  /* Get the start position of this event */
  pReadAddr = DBIEventGetPosition(pmDB, pmEvent);

  /* Get the event ID */
  pmEventDescription->ID = DBIEventID(pmDB, pReadAddr);

  /* Get the event's time */
  DBIEventTime(pmDB, pmEvent->BufID, pReadAddr, &(pmEventDescription->Time));

  /* Get the CPUID of this event */
  pmEventDescription->CPUID = DBIEventCPUID(pmDB, pReadAddr);

  /* Get the event's structure */
  pmEventDescription->Struct = DBIEventStruct(pmDB, pReadAddr);

  /* Get the event's string */
  if(pmStringRequired == TRUE)
    {
    /* Is this a linux kernel event */
    if(pmEventDescription->ID <= TRACE_MAX)
      DBILinuxEventString(pmDB, pmEvent->BufID, pReadAddr, pmEventDescription->String, EVENT_STRING_MAX_SIZE);
#if SUPP_RTAI
    else
      RTAIDBIEventString(pmDB, pmEvent->BufID, pReadAddr, pmEventDescription->String, EVENT_STRING_MAX_SIZE);      
#endif /* SUPP_RTAI */
    }
  else
    pmEventDescription->String[0] = '\0';

  /* Get the event's size */
  pmEventDescription->Size = DBIEventSize(pmDB, pReadAddr);  
}

/******************************************************************
 * Function :
 *    DBEventNext()
 * Description :
 *    Sets the internals of passed event to point to the next event
 *    in the trace.
 * Parameters :
 *    pmDB, Event database to which this event belongs.
 *    pmEvent, Pointer to said event.
 * Return values :
 *    FALSE, If it was the last event.
 *    TRUE, Otherwise.
 * History :
 *    K.Y., 30/01/2000, Initial typing.
 ******************************************************************/
int DBEventNext(db* pmDB, event* pmEvent)
{
  void*     pInitAddr;     /* Initially retrieved address */
  void*     pReadAddr;     /* Address from which to read */
  void*     pNextEvent;    /* Pointer to next event */
  void*     pEventStruct;  /* Pointer to begining of structure of event */
  uint8_t   lEventID;      /* The event ID */
#if 0
  uint16_t  lEventSize;    /* Size of event */
#endif

  /* Get the start position of this event */
  pInitAddr = pReadAddr = DBIEventGetPosition(pmDB, pmEvent);

  /* Does this trace contain CPUIDs */
  if(pmDB->LogCPUID == TRUE)
    /* Skip the CPUID */
    pReadAddr += sizeof(uint8_t);

  /* Read the event ID */
  lEventID = RFT8(pmDB, *(uint8_t*) pReadAddr);

  /* Skip the event ID */
  pReadAddr += sizeof(uint8_t);

  /* Skip the time delta */
  pReadAddr += sizeof(uint32_t);

  /* Have this event's details been recorded? */
  if(ltt_test_bit(lEventID, &(pmDB->DetailsMask)))
    {
    /* Remember the structure's address */
    pEventStruct = pReadAddr;

    /* Skip this event's structure */
    pReadAddr += pmDB->EventStructSize(pmDB, lEventID);

    /* Some events have a variable size */
    switch(lEventID)
      {
      /* TRACE_FILE_SYSTEM */
      case TRACE_FILE_SYSTEM :
	if((RFT8(pmDB, FS_EVENT(pEventStruct)->event_sub_id) == TRACE_FILE_SYSTEM_OPEN)
	 ||(RFT8(pmDB, FS_EVENT(pEventStruct)->event_sub_id) == TRACE_FILE_SYSTEM_EXEC))
	  pReadAddr += RFT32(pmDB, FS_EVENT(pEventStruct)->event_data2) + 1;
	break;

	/* TRACE_CUSTOM */
      case TRACE_CUSTOM :
	pReadAddr += RFT32(pmDB, CUSTOM_EVENT(pEventStruct)->data_size);
	break;
      }
    }

#if 0
  /* Read the event size */
  lEventSize = RFT16(pmDB, *(uint16_t*) pReadAddr);
#endif

  /* Skip the size of the event */
  pReadAddr += sizeof(uint16_t);

#if 0
  /* Does the real event size match the size written in the trace */
  if((pReadAddr - pInitAddr) != lEventSize)
    {
    printf("LibLTT: Real size of Event \"%s\" different from size recorded in trace \n", pmDB->EventString(pmDB, lEventID));
    printf("LibLTT: Real size = %d; Recorded size = %d \n", (pReadAddr - pInitAddr), lEventSize);
    }
#endif

  /* Keep pointer to next event */
  pNextEvent = pReadAddr;

  /* Does this trace contain CPUIDs */
  if(pmDB->LogCPUID == TRUE)
    /* Skip the CPUID */
    pReadAddr += sizeof(uint8_t);

  /* Read the event ID */
  lEventID = RFT8(pmDB, *(uint8_t*) pReadAddr);

  /* Is this a valid event ID */
  if(lEventID > MaxEventID)
    {
    printf("Unknown event ID %d \n", lEventID);
    exit(1);
    }

  /* Is this the end of the buffer */
  if(lEventID == TRACE_BUFFER_END)
    {
    /* Are we at the end of the trace */
    if((pmEvent->BufID + 1) * pmDB->BufferSize >= pmDB->TraceSize)
      return FALSE;

    /* Go to the next buffer */
    pmEvent->BufID++;

    /* Skip the buffer start event */
    pmEvent->EventPos = sizeof(uint8_t);                       /* Skip event ID of first event in buffer */
    pmEvent->EventPos += sizeof(uint32_t);                     /* Skip time delta */
    pmEvent->EventPos += pmDB->EventStructSize(pmDB, TRACE_BUFFER_START);  /* Skip buffer start structure */
    pmEvent->EventPos += sizeof(uint16_t);                     /* Skip event size */
    }
  else
    /* Point to the next event */
    pmEvent->EventPos += (pNextEvent - pInitAddr);

  /* Tell the user that we successfully forwarded */
  return TRUE;
}

/******************************************************************
 * Function :
 *    DBEventPrev()
 * Description :
 *    Sets the internals of passed event to point to the previous
 *    event in the trace.
 * Parameters :
 *    pmDB, Event database to which this event belongs.
 *    pmEvent, Pointer to said event.
 * Return values :
 *    FALSE, If it was the first event.
 *    TRUE, Otherwise.
 * History :
 *    K.Y., 30/01/2000, Initial typing.
 ******************************************************************/
int DBEventPrev(db* pmDB, event* pmEvent)
{
  void*     pInitAddr;     /* Initially retrieved address */
  void*     pBufStart;     /* Buffer start */
  void*     pReadAddr;     /* Address from which to read */
  uint16_t  lEventSize;    /* Size of event */
  uint32_t  lSizeLost;     /* Number of bytes lost at the end of the last buffer */

  /* Get the buffer start */
  pBufStart = DBIEventGetBufStart(pmDB, pmEvent);

  /* Get the start position of this event */
  pInitAddr = pReadAddr = pBufStart + pmEvent->EventPos;

  /* Backup t'il the event's size */
  pReadAddr -= sizeof(uint16_t);

  /* Read the event size */
  lEventSize = RFT16(pmDB, *(uint16_t*) pReadAddr);

  /* Safety check */
  if(lEventSize == 0)
    {
    /* Not normal */
    printf("TraceVisualizer: Found event size 0 (zero) in DBEventPrev() \n");
    exit(1);
    }

  /* Backup to the begining of the previous event */
  pReadAddr = (void*)((uint32_t)pInitAddr - lEventSize);

  /* Is this the start of the buffer */
  if(pReadAddr == pBufStart)
    {
    /* Are we at the begining of the trace */
    if(pReadAddr == pmDB->TraceStart)
      /* Don't move */
      return FALSE;

    /* Decrement the buffer ID */
    pmEvent->BufID--;

    /* Get the number of bytes lost at the end of the previous buffer */
    lSizeLost = RFT32(pmDB, *((uint32_t*) (pReadAddr - sizeof(uint32_t))));

    /* Set the position of the event */
    pmEvent->EventPos = pmDB->BufferSize - lSizeLost;

    /* Rewind to the event before TRACE_BUFFER_END */
    DBEventPrev(pmDB, pmEvent);
    }
  else
    pmEvent->EventPos -= lEventSize;

  /* Tell the user that we successfully rewinded */
  return TRUE;
}

/******************************************************************
 * Function :
 *    DBISetTraceType()
 * Description :
 *    Sets the type of trace being analyzed so that the event related
 *    tables match the type of trace analyzed.
 * Parameters :
 *    pmDB, Database who's type is to be set
 *    pmArchType, Type of architecture.
 *    pmSystemType, System type.
 * Return values :
 *    NONE
 * History :
 *    K.Y., 31/05/2000, Initial typing.
 * Note :
 ******************************************************************/
void DBISetTraceType(db* pmDB, int pmArchType, int pmSystemType)
{
  /* Depending on the type of system */
  switch(pmSystemType)
    {
    case TRACE_SYS_TYPE_VANILLA_LINUX:
      /* Initialize value common to all architectures running linux */
      pmDB->MaxEventID      = TRACE_MAX;
      pmDB->EventStructSize = &LinuxEventStructSize;
      pmDB->EventString     = &LinuxEventString;
      pmDB->EventOT         = (char**) LinuxEventOT;

      /* Set everything to reflect Linux's tables on a i386 */
      pmDB->SyscallString   = &LinuxSyscallString;
      pmDB->TrapString      = &LinuxTrapString;
      break;

#if SUPP_RTAI
    case TRACE_SYS_TYPE_RTAI_LINUX:
      /* Use RTAI values */
      pmDB->MaxEventID      = TRACE_RTAI_MAX;
      pmDB->EventStructSize = &RTAIEventStructSize;
      pmDB->EventString     = &RTAIEventString;
      pmDB->EventOT         = (char**) RTAIEventOT;

      /* Set everything to reflect Linux's tables on a i386 */
      pmDB->SyscallString   = &LinuxSyscallString;
      pmDB->TrapString      = &LinuxTrapString;
      break;
#endif /* SUPP_RTAI */

    default:
#if 1
      printf("TraceVisualizer: Unknown system type %d \n", pmSystemType);
#endif
      exit(1);
      break;
    }
}


/******************************************************************
 * Function :
 *    DBReadTrace()
 * Description :
 *    Fills the trace data-base using the raw data file and
 *    according to the options provided at the command line.
 * Parameters :
 *    pmTraceDataFile, File containing the raw trace data.
 *    pmTraceDB, The event database to be built.
 * Return values :
 *    0, The event database was correctly read.
 *    1, The input file doesn't contain a valid trace.
 *    2, The input file trace format version is not supported by
 *       this version of the reader.
 * History :
 *    K.Y., 20/05/99, Initial typing.
 * Note :
 *    The file is memory mapped, so we can deal with much larger
 *    file than if we read the file into internal structures.
 *    Though, for this to handle as large files as can be, the
 *    file should only be mapped in read-only. Otherwise, files
 *    larger than the system available memory will not be allowed.
 ******************************************************************/
int DBReadTrace(int pmTraceDataFile, db* pmTraceDB)
{
  void*               pReadAddr;              /* Pointer to read address */
  trace_start         lTraceStartEvent;       /* The header of the first buffer */
  trace_buffer_start  lBufferBegEvent;        /* The header of the first buffer */
  struct stat         lTDFStat;               /* Trace data file status */

  /* Initialize database */
  pmTraceDB->Nb = 0;
  memset(&(pmTraceDB->StartTime), 0, sizeof(struct timeval));
  memset(&(pmTraceDB->EndTime), 0, sizeof(struct timeval));

  /* Get the file's status */
  if(fstat(pmTraceDataFile, &lTDFStat) < 0)
    return 1;

  /* Is the file large enough to contain a trace */
  if(lTDFStat.st_size <
     (EVENT_STRUCT_SIZE(TRACE_BUFFER_START)
      + EVENT_STRUCT_SIZE(TRACE_START)
      + 2 * (sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint8_t))))
    return 1;

  /* Set the size of the trace */
  pmTraceDB->TraceSize = lTDFStat.st_size;

  /* Map the file into the process's memory */
  pmTraceDB->TraceStart = mmap(NULL                     /* Start address asked for */,
			       pmTraceDB->TraceSize     /* Length of file being mapped */,
 			       PROT_READ                /* Memory protection */,
			       MAP_PRIVATE              /* Type of the mapped object */,
			       pmTraceDataFile          /* File descriptor */,
			       0                        /* Offset from start of file */);

  /* Was the file mmaped correctly */
  if(((int) pmTraceDB->TraceStart) == -1)
    return 1;

  /* Initialize read address */
  pReadAddr  = pmTraceDB->TraceStart;

  /* Position read address to read start event */
  pReadAddr += sizeof(uint8_t);                       /* Skip event ID of first event in buffer */
  pReadAddr += sizeof(uint32_t);                      /* Skip time delta */
  pReadAddr += EVENT_STRUCT_SIZE(TRACE_BUFFER_START); /* Skip buffer start structure */
  pReadAddr += sizeof(uint16_t);                      /* Skip event size */
  pReadAddr += sizeof(uint8_t);                       /* Skip event ID of start event */
  pReadAddr += sizeof(uint32_t);                      /* Skip time delta of start event */

  /* Read the start event from the trace buffer */
  memcpy(&lTraceStartEvent,
	 pReadAddr,
	 EVENT_STRUCT_SIZE(TRACE_START));

  /* Determine whether this is a trace file and which byte order does its data come in */
  /* We are using the byte order of the trace file magic number to determine
   * whether or not any byte order translation needs to be done between the
   * byte order of the data in the trace file and the native byte order of
   * the machine running the trace decoder (this code).  For the most general
   * case, we could use the pair consisting of the ArchType value from the
   * trace file and the ArchType of the host running the trace decoder as an
   * index into a table of data conversions for each individual data type
   * in the trace data file.  In practice, all we really need to do (at least
   * for the architectures currently supported) is to determine whether the
   * byte order of integers in the trace data file is reversed with respect to
   * the native integer byte ordering.  For this purpose, examining the byte
   * order of the trace file magic number is sufficient.
   */
  /* Is the trace file in the same byte order as this machine? */
  if (lTraceStartEvent.MagicNumber == TRACER_MAGIC_NUMBER)
    pmTraceDB->ByteRev = 0;
  /* Is the trace file have an inverted byte order? */
  else if (BREV32(lTraceStartEvent.MagicNumber) == TRACER_MAGIC_NUMBER)
    pmTraceDB->ByteRev = 1;
  /* This trace file isn't valid */
  else
    {
    /* Unmapp the mapped file */
    munmap(pmTraceDB->TraceStart, pmTraceDB->TraceSize);
    return 1;
    }

  /* Read trace file versions */
  pmTraceDB->MajorVersion = RFT8(pmTraceDB, lTraceStartEvent.MajorVersion);
  pmTraceDB->MinorVersion = RFT8(pmTraceDB, lTraceStartEvent.MinorVersion);

  /* Does the trace file have a valid trace version */
  if((pmTraceDB->MajorVersion != TRACER_SUP_VERSION_MAJOR)
   ||(pmTraceDB->MinorVersion != TRACER_SUP_VERSION_MINOR))
    {
    /* Unmapp the mapped file */
    munmap(pmTraceDB->TraceStart, pmTraceDB->TraceSize);
    return 2;
    }

  /* Set the trace type */
  DBISetTraceType(pmTraceDB,
		  RFT32(pmTraceDB, lTraceStartEvent.ArchType),
		  RFT32(pmTraceDB, lTraceStartEvent.SystemType));

  /* Initailize read address */
  pReadAddr  = pmTraceDB->TraceStart;

  /* Position read address to read the begining of buffer event */
  pReadAddr += sizeof(uint8_t);                      /* Skip event ID of first event in buffer */
  pReadAddr += sizeof(uint32_t);                     /* Skip time delta */

  /* Read the begining of the first buffer */
  memcpy(&lBufferBegEvent,
	 pReadAddr,
	 EVENT_STRUCT_SIZE(TRACE_BUFFER_START));

  /* Set the properties of the trace */
  pmTraceDB->BufferSize        = RFT32(pmTraceDB, lTraceStartEvent.BufferSize);
  pmTraceDB->EventMask         = lTraceStartEvent.EventMask;
  pmTraceDB->DetailsMask       = lTraceStartEvent.DetailsMask;
  pmTraceDB->LogCPUID          = RFT8(pmTraceDB, lTraceStartEvent.LogCPUID);
  pmTraceDB->UseTSC            = RFT8(pmTraceDB, lTraceStartEvent.UseTSC);
  pmTraceDB->FlightRecorder    = RFT8(pmTraceDB, lTraceStartEvent.FlightRecorder);
  pmTraceDB->ArchType          = RFT32(pmTraceDB, lTraceStartEvent.ArchType);
  pmTraceDB->ArchVariant       = RFT32(pmTraceDB, lTraceStartEvent.ArchVariant);
  pmTraceDB->SystemType        = RFT32(pmTraceDB, lTraceStartEvent.SystemType);
  pmTraceDB->StartTime.tv_sec  = RFT32(pmTraceDB, lBufferBegEvent.Time.tv_sec);
  pmTraceDB->StartTime.tv_usec = RFT32(pmTraceDB, lBufferBegEvent.Time.tv_usec);
    
  /* DEBUG */
#if 0
  printf("TraceVisualizer: Details mask is %X \n", lTraceStartEvent.DetailsMask);
#endif

  /* Start out after the begining of the first buffer */
  pmTraceDB->FirstEvent.BufID    = 0;
  pmTraceDB->FirstEvent.EventPos = 0;
  pmTraceDB->FirstEvent.EventPos += sizeof(uint8_t);                        /* Skip event ID of first event in buffer */
  pmTraceDB->FirstEvent.EventPos += sizeof(uint32_t);                       /* Skip time delta */
  pmTraceDB->FirstEvent.EventPos += EVENT_STRUCT_SIZE(TRACE_BUFFER_START);  /* Skip buffer start structure */
  pmTraceDB->FirstEvent.EventPos += sizeof(uint16_t);                       /* Skip event size */
  pmTraceDB->FirstEvent.EventPos += sizeof(uint8_t);                        /* Skip event ID of start event */
  pmTraceDB->FirstEvent.EventPos += sizeof(uint32_t);                       /* Skip time delta of start event */
  pmTraceDB->FirstEvent.EventPos += EVENT_STRUCT_SIZE(TRACE_START);         /* Skip start structure */
  pmTraceDB->FirstEvent.EventPos += sizeof(uint16_t);                       /* Skip event size */

  pmTraceDB->LastGoodTSCPerUSec = 0.0;

  /* Tell the caller that everything is OK */
  return 0;
}

/******************************************************************
 * Function :
 *    DBProcessProcInfo()
 * Description :
 *    Reads the /proc info file and fills the system process list
 *    accordingly.
 * Parameters :
 *    pmProcDataFile, File pointer to /proc info file.
 *    pmSystem, Description of the system to be filled.
 * Return values :
 *    NONE
 * History :
 *    K.Y., 27/05/99, Initial typing.
 * Note :
 ******************************************************************/
void DBProcessProcInfo(FILE* pmProcDataFile, systemInfo* pmSystem)
{
  int        lIntID;                 /* Interrupt ID */
  int        lPID, lPPID;            /* Process PID and Parent PID */
  char       lName[256];             /* Process name */
  process*   pProcess;               /* Generic process pointer */

  /* Add the Idle task to all system descriptions */
  DBCreateProcess(pmSystem, 0, -1);

  /* Read it through */
  while(fscanf(pmProcDataFile, "PID: %d; PPID: %d; NAME: %s\n", &lPID, &lPPID, lName) > 0)
    {
    /* Create a new process */
    pProcess = DBCreateProcess(pmSystem, lPID, lPPID);

    /* Set the command line */
    pProcess->Command   = (char*) malloc(strlen(lName) + 1);
    strcpy(pProcess->Command, lName);
    }

  /* ROC - Grab the interrupts quick and dirty; should work ok on old files.
     Scan for known fields and white space before the description (that
     trailing space on the format is important!).  Finally fgets the rest 
     (which may have lots of white space). */

  /* Read the interrupt information */
  while(fscanf(pmProcDataFile, "IRQ: %d; NAME: ", &lIntID) > 0)
    {
    /* Read 'til the end of the line */
    fgets(lName, 200, pmProcDataFile);

    /* Does this interrupt ID make sense */
    if(lIntID >= 0 && lIntID < MAX_NB_INTERRUPTS)
      {
      /* Make sure we don't cause any mem leakage */
      if(pmSystem->Interrupts[lIntID] != NULL)
	free(pmSystem->Interrupts[lIntID]);

      /* Create space to store the description of this new interrupt */
      pmSystem->Interrupts[lIntID] = malloc(strlen(lName) + 1);

      /* Overwrite \n from fgets */
      lName[strlen(lName) - 1] = '\0';

      /* Copy the description of the interrupt into its designated entry */
      strcpy(pmSystem->Interrupts[lIntID], lName);
      }
    else
      /* This entry doesn't make sense */
      printf("Interrupt %d:%s out of range in proc file\n", lPID, lName);
    }
}

/******************************************************************
 * Function :
 *    DBIGetPIDHashIndex()
 * Description :
 *    Finds a hash index corresponding to a PID.
 * Parameters :
 *    pmPID, PID of process for which index is to be found.
 * Return values :
 *    Hash index value.
 * History :
 *    K.Y., 11/02/2000, Initial typing.
 ******************************************************************/
inline proc_hash DBIGetPIDHashIndex(int pmPID)
{
  uint8_t*      pPID;   /* Pointer to begining of 4 bytes */
  proc_hash     lHash;  /* Hash index in process hash table */

  /* Get the begining address of the PID */
  pPID = (uint8_t*) &pmPID;

  /* Get the hash index */
  lHash = pPID[0] + pPID[1] + pPID[2] + pPID[3];

  /* Return hash index found */
  return lHash;
}

/******************************************************************
 * Function :
 *    DBGetProcByPID()
 * Description :
 *    Finds a process with given PID in a system.
 * Parameters :
 *    pmPID, PID of process to be found.
 *    pmSystem, System to which process belongs.
 * Return values :
 *    Valid pointer to process, if process found.
 *    NULL, otherwise
 * History :
 *    K.Y., 11/02/2000, Initial typing.
 ******************************************************************/
process* DBGetProcByPID(int pmPID, systemInfo* pmSystem)
{
  process*      pProc;  /* Process pointer */

  /* Get the first entry in the hash table */
  pProc = pmSystem->ProcHash[DBIGetPIDHashIndex(pmPID)].NextHash;

  /* Loop around until the process is found */
  for(; pProc != NULL; pProc = pProc->NextHash)
    /* Is this the process we are looking for */
    if(pProc->PID == pmPID)
      return pProc;

  /* We haven't found anything */
  return NULL;
}

/******************************************************************
 * Function :
 *    DBFindProcInTree()
 * Description :
 *    Finds a process with given PID in a process tree.
 * Parameters :
 *    pmPID, PID of process to be found.
 *    pmTree, Tree to be searched.
 * Return values :
 *    Valid pointer to process, if process found.
 *    NULL, otherwise
 * History :
 *    K.Y., 17/06/99, Initial typing.
 * Note :
 *    This function is recursive.
 ******************************************************************/
process* DBFindProcInTree(int pmPID, process* pmTree)
{
  process* pProcJ;    /* Generic process pointers */
  process* pProcess;  /* Process found */

  /* For all the children in the same level */
  for(pProcJ = pmTree; pProcJ != NULL; pProcJ = pProcJ->NextChild)
    {
    /* Is this the process we're looking for */
    if(pProcJ->PID == pmPID)
      return pProcJ;

    /* Does this process have children of his own  */
    if(pProcJ->Children != NULL)
      /* Give this part of the tree to a recursive copy of this function */
      if((pProcess = DBFindProcInTree(pmPID, pProcJ->Children)) != NULL)
	return pProcess;
    }

  /* We didn't find anything */
  return NULL;
}

/******************************************************************
 * Function :
 *    DBBuildProcTree()
 * Description :
 *    Builds the process tree for a system description.
 * Parameters :
 *    pmSystem, The system who's proc tree has to be built.
 * Return values :
 *    NONE
 * History :
 *    K.Y., 17/06/99, Initial typing.
 * Note :
 ******************************************************************/
void DBBuildProcTree(systemInfo* pmSystem)
{
  process*    pProcI;         /* Generic process pointer */
  process*    pProcJ;         /* Generic process pointer */
  process*    pPrevProc;      /* Previous process in process list */
  process*    pProcess;       /* Process found in process tree */
  process*    pProcTemp;      /* Temporary process pointer */

  /* Go through process list */
  for(pProcI = pmSystem->Processes; pProcI != NULL; pProcI = pProcI->Next)
    {
    /* Is this the first process */
    if(pmSystem->ProcTree == NULL)
      {
      /* Set the first element in tree */
      pmSystem->ProcTree = pProcI;
      
      /* Go on to the rest of the processes */
      continue;
      }

    /* Is this process parent any of the topmost process */
    pPrevProc = NULL;
    for(pProcJ = pmSystem->ProcTree; pProcJ != NULL;)
      {
      /* Is this a child of the current process */
      if(pProcI->PID == pProcJ->PPID)
	{
	/* Remember next element */
	pProcTemp = pProcJ->NextChild;

	/* Remove this child from the topmost list */
	if(pPrevProc == NULL)
	  pmSystem->ProcTree = pProcJ->NextChild;
	else
	  pPrevProc->NextChild = pProcJ->NextChild;

	/* Insert this in front of this parent's children list */
	pProcJ->NextChild = pProcI->Children;
	pProcI->Children  = pProcJ;

	/* Update process pointer */
	pProcJ = pProcTemp;
	}
      else
	{
	/* Remember current position and update to next position */
	pPrevProc = pProcJ;
	pProcJ = pProcJ->NextChild;
	}
      }

    /* Is the parent of this process already in the tree */
    if((pProcess = DBFindProcInTree(pProcI->PPID, pmSystem->ProcTree)) != NULL)
      {
      /* Place the new child in front of the children list */
      pProcI->NextChild  = pProcess->Children;
      pProcess->Children = pProcI;
      }
    else
      {
      /* Place this process as the first child at the topmost level */
      pProcI->NextChild  = pmSystem->ProcTree;
      pmSystem->ProcTree = pProcI;
      }
    }
}

/******************************************************************
 * Function :
 *    DBBuildProcHash()
 * Description :
 *    Builds the process hash table for a system description.
 * Parameters :
 *    pmSystem, The system who's proc hash table has to be built.
 * Return values :
 *    NONE
 * History :
 *    K.Y., 11/02/2000, Initial typing.
 * Note :
 ******************************************************************/
void DBBuildProcHash(systemInfo* pmSystem)
{
  int         i;         /* Generic index */
  process*    pProcI;    /* Generic process pointer */
  process*    pProcJ;    /* Generic process pointer */
  proc_hash   lHash;     /* Hash index in process hash table */

  /* Initialize process hash table */
  for(i = 0; i < PROC_HASH_SIZE; i++)
    pmSystem->ProcHash[i].NextHash = NULL;

  /* Go through process list */
  for(pProcI = pmSystem->Processes;
      pProcI != NULL;
      pProcI  = pProcI->Next)
    {
    /* Get the position where this process should be put */
    lHash = DBIGetPIDHashIndex(pProcI->PID);

    /* Loop around this hash table entry list t'ill the end */
    for(pProcJ = &(pmSystem->ProcHash[lHash]);
	pProcJ->NextHash != NULL;
	pProcJ = pProcJ->NextHash);

    /* Append this process */
    pProcJ->NextHash = pProcI;

    /* There is no one after this process */
    pProcI->NextHash = NULL;
    }
}

/******************************************************************
 * Function :
 *    ReallocRollovers()
 * Description :
 *    Allocate/reallocate and initialize TSC rollover data stuctures.
 * Parameters :
 *    pmTraceDB, The event database.
 * Return values :
 *    0, If everything went as planned
 *    Error code otherwise
 * History :
 *    T.R.Z., 12/06/2002.
 * Note :
 ******************************************************************/
int ReallocRollovers(db* pmTraceDB)
{
  int i, end_idx;
  
  pmTraceDB->TSCRolloversSize += ROLLOVER_REALLOC_SIZE;
  pmTraceDB->TSCRollovers = (TSCRollover *)realloc(pmTraceDB->TSCRollovers, 
			   sizeof(TSCRollover)*pmTraceDB->TSCRolloversSize);

  if(pmTraceDB->TSCRollovers == NULL)
    return -1;

  end_idx = (pmTraceDB->TSCRolloversSize - 1);

  /* Initialize only the newly allocated structs */
  for(i = 0; i < ROLLOVER_REALLOC_SIZE; i++) 
    {
    pmTraceDB->TSCRollovers[end_idx - i].NRollovers = 0;
    pmTraceDB->TSCRollovers[end_idx - i].RolloverAddrsSize = 0;
    pmTraceDB->TSCRollovers[end_idx - i].RolloverAddrs = NULL;
    }
  
  return 0;
}

/******************************************************************
 * Function :
 *    ReallocRolloverAddrs()
 * Description :
 *    Allocate/reallocate memory for the TSC rollover address data.
 * Parameters :
 *    pmRollover, The TSCRollover struct for the buffer being processed.
 * Return values :
 *    0, If everything went as planned
 *    Error code otherwise
 * History :
 *    T.R.Z., 12/06/2002.
 * Note :
 ******************************************************************/
int ReallocRolloverAddrs(TSCRollover* pmRollover)
{
  int i, end_idx;
  
  pmRollover->RolloverAddrsSize += ROLLOVER_ADDRS_REALLOC_SIZE;
  pmRollover->RolloverAddrs = (void **)realloc(pmRollover->RolloverAddrs, 
			  sizeof(void *)*pmRollover->RolloverAddrsSize);

  if(pmRollover->RolloverAddrs == NULL)
    return -1;

  return 0;
}

/******************************************************************
 * Function :
 *    DBPreprocessLinuxTrace()
 * Description :
 *    Preprocess the the Linux event database, accumulating information
 *    needed for later TSC processing.
 * Parameters :
 *    pmSystem, Description of the system to be filled
 *    pmTraceDB, The event database to be processed.
 *    pmOptions, The user given options.
 * Return values :
 *    0, If everything went as planned
 *    Error code otherwise
 * History :
 *    T.R.Z., 12/06/2002.
 * Note :
 ******************************************************************/
int DBPreprocessLinuxTrace(systemInfo* pmSystem, db* pmTraceDB, options* pmOptions)
{
  int                lEventID;                   /* The ID of the event */
  event              lEvent;                     /* Generic event */
  struct timeval     lEventTime;                 /* Time at which the event occured */

  /* If this DB isn't using TSC timestamping, nothing to do */
  if(pmTraceDB->UseTSC == FALSE)
    return 0;

  /* Allocate and initialize data for rollover counts */
  pmTraceDB->Preprocessing = TRUE;
  pmTraceDB->CurBufID = 0;
  pmTraceDB->TSCRolloversSize = 0;
  pmTraceDB->TSCRollovers = NULL;
  if(ReallocRollovers(pmTraceDB))
    {
    printf("Memory allocation problem \n");
    exit(1);
    }

  /* Go through the trace database and accumulate TSC timestamp info */
  lEvent = pmTraceDB->FirstEvent;

  do
    {
    /* Get the event's ID */
    lEventID   = DBEventID(pmTraceDB, &lEvent);

    /* Is this trace empty */
    if(lEventID == TRACE_BUFFER_END && !pmTraceDB->FlightRecorder)
      return ERR_EMPTY_TRACE;
    else if (lEventID == TRACE_BUFFER_END && pmTraceDB->FlightRecorder)
      {
      /* Is this the end of the buffer */
      /* Are we at the end of the trace */
      if((lEvent.BufID + 1) * pmTraceDB->BufferSize >= pmTraceDB->TraceSize)
	return FALSE;
	    
      /* Go to the next buffer */
      pmTraceDB->FirstEvent.BufID++;

      pmTraceDB->CurBufID++;

      /* Skip the buffer start event */
      pmTraceDB->FirstEvent.EventPos = sizeof(uint8_t);
      /* Skip event ID of first event in buffer */
      pmTraceDB->FirstEvent.EventPos += sizeof(uint32_t);
      /* Skip time delta */
      pmTraceDB->FirstEvent.EventPos += pmTraceDB->EventStructSize(pmTraceDB, TRACE_BUFFER_START);  /* Skip buffer start structure */
      pmTraceDB->FirstEvent.EventPos += sizeof(uint16_t);
      /* Skip event size */

      lEvent = pmTraceDB->FirstEvent;
      /* Get the event's ID */
      lEventID   = DBEventID(pmTraceDB, &lEvent);

      if (lEventID == TRACE_START)
	{
	  /* Skip the start event, if present */
	  pmTraceDB->FirstEvent.EventPos += sizeof(uint8_t);
	  /* Skip event ID of first event in buffer */
	  pmTraceDB->FirstEvent.EventPos += sizeof(uint32_t);
	  /* Skip time delta */
	  pmTraceDB->FirstEvent.EventPos += pmTraceDB->EventStructSize(pmTraceDB, TRACE_START);  /* Skip start structure */
	  pmTraceDB->FirstEvent.EventPos += sizeof(uint16_t);
	  /* Skip event size */
	  lEvent = pmTraceDB->FirstEvent;
	  /* Get the event's ID */
	  lEventID   = DBEventID(pmTraceDB, &lEvent);
	}
      }

    /* Preprocess the event's time */
    DBEventTime(pmTraceDB, &lEvent, &lEventTime);

    /* DEBUG */
#if 0
    printf("%d ", lEventID);
#endif

    } while(DBEventNext(pmTraceDB, &lEvent) == TRUE);

  pmTraceDB->Preprocessing = FALSE;

  if((pmTraceDB->FlightRecorder && pmTraceDB->UseTSC == TRUE))
    {
    uint32_t            lRollovers;    /* Number of TSC rollovers in this buffer */
    double              lLastBufTotalTSC;  /* Total TSC cycles for this buffer */
    uint32_t            lLastBufTotalUSec; /* Total time for this buffer in usecs */

    /* Calculate the total cycles for this bufffer */
    lRollovers = pmTraceDB->TSCRollovers[pmTraceDB->LastBufID].NRollovers;
    if(pmTraceDB->LastEventTSC < pmTraceDB->LastBufStartTSC)
      lLastBufTotalTSC = (UINT_MAX - pmTraceDB->LastBufStartTSC)
	+ ((double)(lRollovers - 1) * (double)UINT_MAX) + pmTraceDB->LastEventTSC;
    else
      lLastBufTotalTSC = (pmTraceDB->LastEventTSC - pmTraceDB->LastBufStartTSC) + ((double)lRollovers * (double)UINT_MAX);

    /* If there's only 1 buffer, we won't have a number, use 400 MHz */
    if (pmTraceDB->LastGoodTSCPerUSec == 0.0)
      pmTraceDB->LastGoodTSCPerUSec = 400.0;
	  
    lLastBufTotalUSec = lLastBufTotalTSC / pmTraceDB->LastGoodTSCPerUSec;
    pmTraceDB->LastBufEndTime.tv_sec = lLastBufTotalUSec / 1000000;
    pmTraceDB->LastBufEndTime.tv_usec = lLastBufTotalUSec % 1000000;
    DBTimeAdd(pmTraceDB->LastBufEndTime, pmTraceDB->LastBufEndTime, pmTraceDB->LastBufStartTime);
    }
  
  /* Everything is OK */
  return 0;
}

/******************************************************************
 * Function :
 *    DBProcessLinuxTrace()
 * Description :
 *    Analyze the Linux event database, cumulate statistics and build
 *    system description accordingly.
 * Parameters :
 *    pmSystem, Description of the system to be filled
 *    pmTraceDB, The event database to be processed.
 *    pmOptions, The user given options.
 * Return values :
 *    0, If everything went as planned
 *    Error code otherwise
 * History :
 *    K.Y., 06/06/2000. This is largely what DBProcessTrace() used to be.
 * Note :
 ******************************************************************/
int DBProcessLinuxTrace(systemInfo* pmSystem, db* pmTraceDB, options* pmOptions)
{
  int                k;                          /* Loop counter */
  int                lCPUID;                     /* CPU on which the event occurred */
  int                lOPID;                      /* Outgoing process's PID */
  int                lPID[MAX_CPUID];            /* Process ID */
  int                lEventID;                   /* The ID of the event */
  int                lFirstSchedChange = TRUE;   /* There hasn't been any scheduling changes yet */
  int                lHashPosition;              /* Position of event type in hash table */
#if 0
  char               lFirstTime[TIME_STR_LEN];   /* Time string */
  char               lSecondTime[TIME_STR_LEN];  /* Time string */
#endif
  void*              pEventStruct;               /* The event's description structure */
  void*              pTempStruct;                /* The event's description structure */
  event              lEvent;                     /* Generic event */
  event              lLastCtrlEvent;             /* Last system-wide control event */
  syscallInfo*       pSyscall;                   /* System call pointer */
  syscallInfo*       pPrevSyscall;               /* Previous system call in syscall list */
  syscallInfo*       pSyscallP;                  /* Processed: System call pointer */
  process*           pProcess[MAX_CPUID];        /* Current process */
  struct timeval     lTempTime;                  /* Temporary time variable */
  struct timeval     lTime;                      /* Local time variable */
  struct timeval     lEventTime;                 /* Time at which the event occured */
  struct timeval     lLastCtrlEventTime;         /* Time of last control event */
  customEventDesc*   pCustom;                    /* Custom event */

  /* Initialize per-CPU process ID and pointer */
  for (k = 0; k < MAX_CPUID; k++)
    {
    lPID[k] = -1;
    pProcess[k] = NULL;
    }

  /* Initialize system description */
  pmSystem->SyscallEntry = 0;
  pmSystem->SyscallExit  = 0;
  pmSystem->TrapEntry    = 0;
  pmSystem->TrapExit     = 0;
  pmSystem->IRQEntry     = 0;
  pmSystem->IRQExit      = 0;
  pmSystem->SchedChange  = 0;
  pmSystem->KernelTimer  = 0;
  pmSystem->BH           = 0;
  pmSystem->TimerExpire  = 0;
  pmSystem->PageAlloc    = 0;
  pmSystem->PageFree     = 0;
  pmSystem->PacketOut    = 0;
  pmSystem->PacketIn     = 0;

  /* Initialize last control event */
  lLastCtrlEvent.BufID    = 0;
  lLastCtrlEvent.EventPos = 0;

  /* Go through the trace database and fill system description accordingly */
  lEvent = pmTraceDB->FirstEvent;
  do
    {
    /* Increment the number of events */
    pmTraceDB->Nb++;

    /* Get the event's ID */
    lEventID   = DBEventID(pmTraceDB, &lEvent);

    /* Get the event's CPUID */
    lCPUID = DBEventCPUID(pmTraceDB, &lEvent);

    /* Is this trace empty */
    if(lEventID == TRACE_BUFFER_END && !pmTraceDB->FlightRecorder)
      return ERR_EMPTY_TRACE;

    /* TEST FOR PROBLEMS WITH do_gettimeofday() */
    /* Remember the last event's time of occurence */
    lTempTime = lEventTime;

    /* Get the event's time */
    DBEventTime(pmTraceDB, &lEvent, &lEventTime);

    /* TEST FOR PROBLEMS WITH do_gettimeofday() */
#if 0 /* Enable this to check for any wrong RTAI timing */
    /* Is there a time inversion problem */
    if(DBGetTimeInDouble(lEventTime) < DBGetTimeInDouble(lTempTime))
      {
      printf("TraceVisualizer: Consecutive events with inverted times ... \n");
      DBFormatTimeInReadableString(lFirstTime, lEventTime.tv_sec, lEventTime.tv_usec);
      DBFormatTimeInReadableString(lSecondTime, lTempTime.tv_sec, lTempTime.tv_usec);
      printf("TraceVisualizer: %s is after %s \n", lFirstTime, lSecondTime);
      }
#endif

    /* Get the event's structure */
    pEventStruct = DBEventStruct(pmTraceDB, &lEvent);

    /* DEBUG */
#if 0
    printf("%d ", lEventID);
#endif

    /* Ignore events until the PID running on this processor is known */
    if((lPID[lCPUID] == -1) && (lEventID != TRACE_SCHEDCHANGE) && (lEventID != TRACE_NEW_EVENT))
      continue;

    /* Are the details of the event present */
    if(!ltt_test_bit(lEventID, &(pmTraceDB->DetailsMask)))
      continue;

    /* Depending on the type of event */
    switch(lEventID)
      {
      /* Entry in a given system call */
      case TRACE_SYSCALL_ENTRY :
	/* Increment number of system call entries */
	pProcess[lCPUID]->NbSyscalls++;
	pmSystem->SyscallEntry++;

	/* Set system call depth */
	pProcess[lCPUID]->Depth++;

	/* Create new pending system call */
	if((pSyscall = (syscallInfo*) malloc(sizeof(syscallInfo))) == NULL)
	  {
	  printf("Memory allocation problem \n");
	  exit(1);
	  }

	/* Set new system call */
	pSyscall->ID    = RFT8(pmTraceDB, SYSCALL_EVENT(pEventStruct)->syscall_id);
	pSyscall->Nb    = 0;
	pSyscall->Depth = pProcess[lCPUID]->Depth;
	memcpy(&(pSyscall->Time), &lEventTime, sizeof(struct timeval));

	/* Link this syscall to the others pending */
	pSyscall->Next = pProcess[lCPUID]->Pending;
	pProcess[lCPUID]->Pending = pSyscall;
	break;
      
      /* Exit from a given system call */
      case TRACE_SYSCALL_EXIT :
	/* Increment number of system call exits */
	pmSystem->SyscallExit++;

	/* Find the system call in the list of pending system calls */
	pPrevSyscall = pProcess[lCPUID]->Pending;
	for(pSyscall = pProcess[lCPUID]->Pending; pSyscall != NULL; pSyscall = pSyscall->Next)
	  {
	  if(pSyscall->Depth == pProcess[lCPUID]->Depth)
	    break;
	  pPrevSyscall = pSyscall;
	  }

	/* The first syscall_exit probably doesn't include an entry */
	if(pSyscall == NULL)
	  goto IsEventControl;

	/* Remove syscall from pending syscalls */
	pPrevSyscall->Next = pSyscall->Next;

	/* Find syscall in already processed syscalls list */
	for(pSyscallP = pProcess[lCPUID]->Syscalls; pSyscallP != NULL; pSyscallP = pSyscallP->Next)
	  if(pSyscallP->ID == pSyscall->ID)
	    break;

	/* Did we find anything */
	if(pSyscallP == NULL)
	  {
	  /* Allocate space for new syscall */
	  if((pSyscallP = (syscallInfo*) malloc(sizeof(syscallInfo))) == NULL)
	    {
	    printf("Memory allocation problem \n");
	    exit(1);
	    }

	  /* Set and link new syscall */
	  pSyscallP->ID    = pSyscall->ID;
	  pSyscallP->Nb    = 0;
	  pSyscallP->Depth = 0;
	  memset(&(pSyscallP->Time), 0, sizeof(struct timeval));
	  pSyscallP->Next = pProcess[lCPUID]->Syscalls;
	  pProcess[lCPUID]->Syscalls = pSyscallP;
	  }

	/* Increment number of times syscall was called */
	pSyscallP->Nb++;

	/* Add time spent in this syscall */
	DBTimeSub(lTime, lEventTime, pSyscall->Time);
	DBTimeAdd(pSyscallP->Time, pSyscallP->Time, lTime);

	/* Decrement syscall depth */
	pProcess[lCPUID]->Depth--;
	if(pSyscall == pProcess[lCPUID]->Pending) pProcess[lCPUID]->Pending = NULL;
	free(pSyscall);
	pSyscall = NULL;
	break;

      /* Entry into a trap */
      case TRACE_TRAP_ENTRY :
	pProcess[lCPUID]->NbTraps++;
	pmSystem->TrapEntry++;
	break;

      /* Exit from a trap */
      case TRACE_TRAP_EXIT :
	pmSystem->TrapExit++;
	break;

      /* Entry into an IRQ */
      case TRACE_IRQ_ENTRY :
	pmSystem->IRQEntry++;
	break;

      /* Exit from an IRQ */
      case TRACE_IRQ_EXIT :
	pmSystem->IRQExit++;
	break;

      /* Task scheduling change */
      case TRACE_SCHEDCHANGE :
	/* Read PID of incoming task */
	lOPID = RFT32(pmTraceDB, SCHED_EVENT(pEventStruct)->out);
	lPID[lCPUID]  = RFT32(pmTraceDB, SCHED_EVENT(pEventStruct)->in);

	/* Was the currently runing process ever scheduled in */
	if((pProcess[lCPUID] != NULL)
	 &&(pProcess[lCPUID]->LastSchedIn.EventPos != 0))
	  {
          /* Get the time of occurence of the last scheduling change */
	  DBEventTime(pmTraceDB, &(pProcess[lCPUID]->LastSchedIn), &lTime);
	    
	  /* During this period we were runing */
	  DBTimeSub(lTime, lEventTime, lTime);
	  DBTimeAdd(pProcess[lCPUID]->TimeRuning, pProcess[lCPUID]->TimeRuning, lTime);
	  }

	/* Find the process in the list of already known processes */
	for(pProcess[lCPUID] = pmSystem->Processes;
	    pProcess[lCPUID] != NULL;
	    pProcess[lCPUID] = pProcess[lCPUID]->Next)
	  if(pProcess[lCPUID]->PID == lPID[lCPUID])
	    break;
	
	/* Did we find anything */
	if(pProcess[lCPUID] == NULL)
	  /* Create a new process */
	  pProcess[lCPUID] = DBCreateProcess(pmSystem, lPID[lCPUID], -1);

	/* We just got scheduled in */
	pProcess[lCPUID]->LastSchedIn = lEvent;

	/* Update the number of scheduling changes */
	pmSystem->SchedChange++;

	/* Is this the first scheduling change */
	if(lFirstSchedChange == TRUE)
	  {
	  /* Don't come here again */
	  lFirstSchedChange = FALSE;

	  /* There is at least one scheduling change */
	  pmTraceDB->SchedEventsExist = TRUE;

	  /* Remember this scheduling change */
	  pmTraceDB->FirstEventWithProc = lEvent;

	  /* This is the first event from which we can start drawing */
	  pmTraceDB->DrawStartTime = lEventTime;
	  }
	break;

      /* The kernel timer routine has been called */
      case TRACE_KERNEL_TIMER :
	pmSystem->KernelTimer++;
	break;

      /* Going to run a bottom half */
      case TRACE_SOFT_IRQ :
	if(RFT8(pmTraceDB, SOFT_IRQ_EVENT(pEventStruct)->event_sub_id) == TRACE_SOFT_IRQ_BOTTOM_HALF)
	  pmSystem->BH++;
	break;

      /* Hit key part of process management */
      case TRACE_PROCESS :
	/* Don't analyze anything but forking for now */
	if(RFT8(pmTraceDB, PROC_EVENT(pEventStruct)->event_sub_id) != TRACE_PROCESS_FORK)
	  continue;

	/* We have forked, get the child's PID */
	lPID[lCPUID] = RFT32(pmTraceDB, PROC_EVENT(pEventStruct)->event_data1);

	/* Create a new process */
	DBCreateProcess(pmSystem, lPID[lCPUID], pProcess[lCPUID]->PID);
	break;

      /* Hit key part of file system */
      case TRACE_FILE_SYSTEM :
	/* Depending of the event subtype */
	switch(RFT8(pmTraceDB, FS_EVENT(pEventStruct)->event_sub_id))
	  {
	  /* TRACE_FILE_SYSTEM_BUF_WAIT_START */
	  case TRACE_FILE_SYSTEM_BUF_WAIT_START :
	    /* If we hadn't already started an I/O wait, then we're starting a new one now */
	    if((pProcess[lCPUID]->LastIOEvent.BufID    == 0)
	     &&(pProcess[lCPUID]->LastIOEvent.EventPos == 0))
	      pProcess[lCPUID]->LastIOEvent = lEvent;
	    else
	      {
	      /* Get the event's structure */
	      pTempStruct = DBEventStruct(pmTraceDB, &(pProcess[lCPUID]->LastIOEvent));

	      /* Was the last event something else than a wait start */
	      if(RFT8(pmTraceDB, FS_EVENT(pTempStruct)->event_sub_id) != TRACE_FILE_SYSTEM_BUF_WAIT_START)
		pProcess[lCPUID]->LastIOEvent = lEvent;
	      }
	    break;

	  /* TRACE_FILE_SYSTEM_BUF_WAIT_END */
	  case TRACE_FILE_SYSTEM_BUF_WAIT_END :
 	    /* Does the last IO event exist */
	    if(/*(pProcess[lCPUID]->LastIOEvent.BufID    != 0)
		 &&*/(pProcess[lCPUID]->LastIOEvent.EventPos != 0))
	      {
	      /* Get the event's structure */
	      pTempStruct = DBEventStruct(pmTraceDB, &(pProcess[lCPUID]->LastIOEvent));

	      /* Was the last event a wait start */
	      if(RFT8(pmTraceDB, FS_EVENT(pTempStruct)->event_sub_id) == TRACE_FILE_SYSTEM_BUF_WAIT_START)
		{
		/* Get the event's time */
		DBEventTime(pmTraceDB, &(pProcess[lCPUID]->LastIOEvent), &lTime);

		/* Add the time to time spent waiting for I/O */
		DBTimeSub(lTime, lEventTime, lTime);
		DBTimeAdd(pProcess[lCPUID]->TimeIO, pProcess[lCPUID]->TimeIO, lTime);

		/* This is the next LastIOEvent */
		pProcess[lCPUID]->LastIOEvent = lEvent;
		}
	      }
	    break;

	  /* TRACE_FILE_SYSTEM_EXEC */
	  case TRACE_FILE_SYSTEM_EXEC :
	    /* Retain only the first exec */
	    if(pProcess[lCPUID]->Command != NULL)
	      continue;

	    /* Allocate space for command and copy it */
	    pProcess[lCPUID]->Command = (char*) malloc(strlen(FS_EVENT_FILENAME(pEventStruct)) + 1);
	    strcpy(pProcess[lCPUID]->Command, FS_EVENT_FILENAME(pEventStruct));
	    break;

          /* TRACE_FILE_SYSTEM_OPEN */
	  case TRACE_FILE_SYSTEM_OPEN :
	    break;

	  /* TRACE_FILE_SYSTEM_READ */
	  case TRACE_FILE_SYSTEM_READ :
	    pProcess[lCPUID]->QFileRead += RFT32(pmTraceDB, FS_EVENT(pEventStruct)->event_data2);
	    break;

	  /* TRACE_FILE_SYSTEM_WRITE */
	  case TRACE_FILE_SYSTEM_WRITE :
	    pProcess[lCPUID]->QFileWrite += RFT32(pmTraceDB, FS_EVENT(pEventStruct)->event_data2);
	    break;
	  }
	break;

      /* Hit key part of timer management */
      case TRACE_TIMER :
	/* Did a timer expire */
	if(RFT8(pmTraceDB, TIMER_EVENT(pEventStruct)->event_sub_id) == TRACE_TIMER_EXPIRED)
	  pmSystem->TimerExpire++;
	break;

      /* Hit key part of memory management */
      case TRACE_MEMORY :
	/* Depending on the event sub id */
	switch(RFT8(pmTraceDB, MEM_EVENT(pEventStruct)->event_sub_id))
	  {
	  /* TRACE_MEMORY_PAGE_ALLOC */
	  case TRACE_MEMORY_PAGE_ALLOC :
	    pmSystem->PageAlloc++;
	    break;

	  /* TRACE_MEMORY_PAGE_FREE */
	  case TRACE_MEMORY_PAGE_FREE :
	    pmSystem->PageFree++;
	    break;
	  }
	break;

      /* Hit key part of socket communication */
      case TRACE_SOCKET :
	break;

      /* Hit key part of inter-process communication */
      case TRACE_IPC :
	break;

      /* Hit key part of network communication */
      case TRACE_NETWORK :
	if(RFT8(pmTraceDB, NET_EVENT(pEventStruct)->event_sub_id) == TRACE_NETWORK_PACKET_OUT)
	  pmSystem->PacketOut++;
	else
	  pmSystem->PacketIn++;
	break;

      /* A new event is declared */
      case TRACE_NEW_EVENT :
	/* Allocate space for new event */
	if((pCustom = (customEventDesc*) malloc(sizeof(customEventDesc))) == NULL)
	  {
	  printf("Memory allocation problem \n");
	  exit(1);
	  }

	/* Initialize event description */
	pCustom->ID    = RFT32(pmTraceDB, NEW_EVENT(pEventStruct)->id);
	memcpy(&(pCustom->Event), pEventStruct, sizeof(trace_new_event));

	/* Insert new event in custom event list */
	pCustom->Next = &(pmTraceDB->CustomEvents);
	pCustom->Prev = pmTraceDB->CustomEvents.Prev;
	pmTraceDB->CustomEvents.Prev->Next = pCustom;
	pmTraceDB->CustomEvents.Prev = pCustom;

	/* Calculate the hash position of the event type */
	lHashPosition = (int) (pCustom->ID % CUSTOM_EVENT_HASH_TABLE_SIZE);

	/* Place custom event description at the head of the hash table */
	pCustom->NextHash = pmTraceDB->CustomHashTable[lHashPosition].NextHash;
	pmTraceDB->CustomHashTable[lHashPosition].NextHash = pCustom;

	/* Increment number of custom events */
	pmTraceDB->NbCustom++;
	continue;
	break;

      /* A custom event has occured */
      case TRACE_CUSTOM :
	break;

      /* A trace mask change */
      case TRACE_CHANGE_MASK :
	/* The resulting mask is made of only those events that match all the masks */
	pmTraceDB->EventMask &= CHMASK_EVENT(pEventStruct)->mask;
	break;

      /* A heartbeat event has occured */
      case TRACE_HEARTBEAT :
	break;

      default :
	printf("Internal error : Unknown event ID %d while processing trace \n", lEventID);
	exit(1);
	break;
      }

IsEventControl:
    /* Is this an entry control event */
    if(DBEventControlEntry(pmTraceDB, &lEvent, pProcess[lCPUID]->PID) == TRUE)
      {
      /* Accumulate the time that passed since the last control event */
      DBTimeSub(lTime, lEventTime, lLastCtrlEventTime);
      DBTimeAdd(pProcess[lCPUID]->TimeProcCode, pProcess[lCPUID]->TimeProcCode, lTime);
#if 0
      if((lTime.tv_sec < 0) || (lTime.tv_usec < 0) || (lLastCtrlEventTime.tv_sec == 0) || (lLastCtrlEventTime.tv_usec == 0)
        || (lTime.tv_sec > 10000))
	{
	printf("BUG!!! \n");
	printf("lEventTime         : (%ld, %ld) \n", lEventTime.tv_sec, lEventTime.tv_usec);
	printf("lLastCtrlEventTime : (%ld, %ld) \n", lLastCtrlEventTime.tv_sec, lLastCtrlEventTime.tv_usec);
	}
#endif
      }

    /* Is this an exiting control event */
    if(DBEventControlExit(pmTraceDB, &lEvent, pProcess[lCPUID]->PID) == TRUE)
      {
      lLastCtrlEvent     = lEvent;
      lLastCtrlEventTime = lEventTime;
      }
    } while(DBEventNext(pmTraceDB, &lEvent) == TRUE);

  /* Remember the last event and its time */
  pmTraceDB->LastEvent = lEvent;
  pmTraceDB->EndTime   = lEventTime;

  /* Sum up syscall time for all the system */
  for(pProcess[lCPUID] = pmSystem->Processes;
      pProcess[lCPUID] != NULL;
      pProcess[lCPUID] = pProcess[lCPUID]->Next)
    /* Go through his syscall list */
    for(pSyscall = pProcess[lCPUID]->Syscalls; pSyscall != NULL; pSyscall = pSyscall->Next)
      {
      /* Find this syscall in the system-wide list */
      for(pSyscallP = pmSystem->Syscalls; pSyscallP != NULL; pSyscallP = pSyscallP->Next)
	if(pSyscallP->ID == pSyscall->ID)
	  break;

      /* Did we find anything */
      if(pSyscallP == NULL)
	{
	/* Allocate space for new syscall */
	if((pSyscallP = (syscallInfo*) malloc(sizeof(syscallInfo))) == NULL)
	  {
	  printf("Memory allocation problem \n");
	  exit(1);
	  }

	/* Set and link new syscall */
	pSyscallP->ID    = pSyscall->ID;
	pSyscallP->Nb    = 0;
	pSyscallP->Depth = 0;
	memset(&(pSyscallP->Time), 0, sizeof(struct timeval));
	pSyscallP->Next = pmSystem->Syscalls;
	pmSystem->Syscalls = pSyscallP;
	}

      /* Set number of times system call was called */
      pSyscallP->Nb += pSyscall->Nb;
      
      /* Add time spent in this syscall */
      pSyscallP->Time.tv_sec += pSyscall->Time.tv_sec;
      pSyscallP->Time.tv_usec += pSyscall->Time.tv_usec;

      /* Have we more than a million microseconds */
      if(pSyscallP->Time.tv_usec > 1000000)
	{
	pSyscallP->Time.tv_sec += pSyscallP->Time.tv_usec / 1000000;
	pSyscallP->Time.tv_usec = pSyscallP->Time.tv_usec % 1000000;
	}
      }

  /* Build process tree */
  DBBuildProcTree(pmSystem);

  /* Build process hash table */
  DBBuildProcHash(pmSystem);

#if 0  /* Debugging only */ 
  do
  {
   int i;
  for(i = 0; i <= pmTraceDB->LastEvent.BufID; i++)
    {
  void*               pReadAddr;     /* Address from which to read */
  struct timeval*     pBufStartTime; /* Pointer to time at which this buffer started */
  struct timeval      lBufStartTime; /* Time at which this buffer started */
  uint32_t            lTimeDelta;

  /* Get the time of start of the buffer to which the event belongs */
  pReadAddr = i * pmTraceDB->BufferSize + pmTraceDB->TraceStart;

  /* Skip the event ID */
  pReadAddr += sizeof(uint8_t);

  /* Skip the time delta */
  pReadAddr += sizeof(uint32_t);

  /* Get a pointer to the time of begining of this buffer */
  pBufStartTime = &(((trace_buffer_start*) pReadAddr)->Time);

  /* Read the time */
  lBufStartTime.tv_sec  = RFT32(pmTraceDB, pBufStartTime->tv_sec);
  lBufStartTime.tv_usec = RFT32(pmTraceDB, pBufStartTime->tv_usec);

  pReadAddr += sizeof(trace_buffer_start);
  pReadAddr += sizeof(uint8_t);
  
  /* Does this trace contain CPUIDs */
  if(pmTraceDB->LogCPUID == TRUE)
    /* Skip the CPUID */
    pReadAddr += sizeof(uint8_t);

  /* Skip the event ID */
  pReadAddr += sizeof(uint8_t);

  /* Get the time delta */
  lTimeDelta = RFT32(pmTraceDB, *(uint32_t*) pReadAddr);

  printf("Buffer %i starts at (%ld, %ld). Delta is %d. \n", i, lBufStartTime.tv_sec, lBufStartTime.tv_usec, lTimeDelta);
    }
  } while(0);
#endif

  /* Everything is OK */
  return 0;
}

/******************************************************************
 * Function :
 *    DBPreprocessTrace()
 * Description :
 *    Summons trace processing according to the type of trace.
 * Parameters :
 *    pmSystem, Description of the system to be filled
 *    pmTraceDB, The event database to be processed.
 *    pmOptions, The user given options.
 * Return values :
 *    0, Everything went fine
 *    Error code otherwise
 * History :
 *    K.Y., 06/06/2000, Moved most of the stuff to functions
 *                      that deal with specific types of traces.
 *    K.Y., 20/05/99, Initial typing.
 * Note :
 ******************************************************************/
int DBPreprocessTrace(systemInfo* pmSystem, db* pmTraceDB, options* pmOptions)
{
  /* Depending on the type of trace */
  switch(pmTraceDB->SystemType)
    {
    /* Vanilla Linux */
    case TRACE_SYS_TYPE_VANILLA_LINUX :
      return DBPreprocessLinuxTrace(pmSystem, pmTraceDB, pmOptions);
      break;

#if SUPP_RTAI
    /* RTAI modified Linux */
    case TRACE_SYS_TYPE_RTAI_LINUX :
      return 0;
      break;
#endif /* SUPP_RTAI */
    
    /* Just in case */
    default :
      return ERR_UNKNOWN_TRACE_TYPE;
      break;
    }

  /* Just in case */
  return ERR_UNKNOWN_TRACE_TYPE;
}

/******************************************************************
 * Function :
 *    DBProcessTrace()
 * Description :
 *    Summons trace processing according to the type of trace.
 * Parameters :
 *    pmSystem, Description of the system to be filled
 *    pmTraceDB, The event database to be processed.
 *    pmOptions, The user given options.
 * Return values :
 *    0, Everything went fine
 *    Error code otherwise
 * History :
 *    K.Y., 06/06/2000, Moved most of the stuff to functions
 *                      that deal with specific types of traces.
 *    K.Y., 20/05/99, Initial typing.
 * Note :
 ******************************************************************/
int DBProcessTrace(systemInfo* pmSystem, db* pmTraceDB, options* pmOptions)
{
  /* Depending on the type of trace */
  switch(pmTraceDB->SystemType)
    {
    /* Vanilla Linux */
    case TRACE_SYS_TYPE_VANILLA_LINUX :
      return DBProcessLinuxTrace(pmSystem, pmTraceDB, pmOptions);
      break;

#if SUPP_RTAI
    /* RTAI modified Linux */
    case TRACE_SYS_TYPE_RTAI_LINUX :
      return RTAIDBProcessTrace(pmSystem, pmTraceDB, pmOptions);
      break;
#endif /* SUPP_RTAI */
    
    /* Just in case */
    default :
      return ERR_UNKNOWN_TRACE_TYPE;
      break;
    }

  /* Just in case */
  return ERR_UNKNOWN_TRACE_TYPE;
}

/******************************************************************
 * Function :
 *     DBFormatTimeInReadableString()
 * Description :
 *     Takes a time value and inserts commas between every 3 digit
 *     in order to ease readability.
 * Parameters :
 *     pmFormatedString, String where the formatted time should be
 *                       written.
 *     pmSeconds, The number of seconds.
 *     pmMicroSecs, The number of micro-seconds.
 * Return values :
 *     NONE
 * History :
 *     K.Y., 29/05/2000, Initial typing.
 * Note :
 *     This intentionally resembles WDIFormatTimeInReadableString().
 *     Except that this one takes seconds/micro-second pairs rather
 *     than a double value.
 ******************************************************************/
void DBFormatTimeInReadableString(char* pmFormatedString, long pmSeconds, long pmMicroSecs)
{
  char            lTempStr[TIME_STR_LEN];   /* Temporary string */
  unsigned int    i, j;                     /* Generic indexes */
  unsigned int    lStrLen;                  /* String length */
  unsigned int    lExtraChars;              /* Characters more that the last triplet */
  unsigned int    lFormatedStrStart;        /* Start of copying position into formated str */

  /* Print the time into the temporary string */
  sprintf(lTempStr, "%ld%06ld", pmSeconds, pmMicroSecs);

  /* How long is the string */
  lStrLen = strlen(lTempStr);

  /* How many digits are left after the last triplet */
  lFormatedStrStart = lExtraChars = lStrLen % 3;

  /* Are there any trailing characters */
  if(lExtraChars != 0)
    {
    /* Copy the trailing characters right away */
    for(i = 0; i < lExtraChars; i++)
      pmFormatedString[i] = lTempStr[i];

    /* Insert a comma */
    pmFormatedString[i] = ',';

    /* Start copying past the added comma */
    lFormatedStrStart += 1;
    }

  /* Copy the characters into the final string in groups of three */
  for(i = lFormatedStrStart, j = lExtraChars; j < lStrLen; i += 4, j += 3)
    {
    pmFormatedString[i]     = lTempStr[j];
    pmFormatedString[i + 1] = lTempStr[j + 1];
    pmFormatedString[i + 2] = lTempStr[j + 2];
    pmFormatedString[i + 3] = ',';
    }

  /* Insert the end of string marker */
  pmFormatedString[i - 1] = '\0';
}

/******************************************************************
 * Function :
 *    DBPrintHeader()
 * Description :
 *    Prints the output file header according to the user options.
 * Parameters :
 *    pmDecodedDataFile, File to which the header is written.
 *    pmDB, The database to which this event belongs.
 *    pmOptions, The user given options.
 * Return values :
 *    NONE
 * History :
 *    K.Y., 06/02/00, Removed from FileIO.c and put in EventDB.c.
 *    K.Y., 17/10/99, Added support for the trace properties
 *    K.Y., 25/05/99, Initial typing.
 * Note :
 ******************************************************************/
void DBPrintHeader(int       pmDecodedDataFile,
		   db*       pmDB,
		   options*  pmOptions)
{
  /* Write a header to the output file */
  /*  The baner */
  DBPrintData(pmDecodedDataFile, HEADER_BANER);

  /*  The CPUID */
  if((pmOptions->ForgetCPUID != TRUE) && (pmDB->LogCPUID == TRUE))
    DBPrintData(pmDecodedDataFile, "%s \t", HEADER_CPUID); 

  /*  The Event ID */
  DBPrintData(pmDecodedDataFile, HEADER_EVENT);

  /*  The time */
  if(pmOptions->ForgetTime != TRUE)
    DBPrintData(pmDecodedDataFile, HEADER_TIME);

  /*  The PID */
  if(pmOptions->ForgetPID != TRUE)
    DBPrintData(pmDecodedDataFile, HEADER_PID);

  /*  The data length */
  if(pmOptions->ForgetDataLen != TRUE)
    DBPrintData(pmDecodedDataFile, HEADER_LENGTH);

  /*  The description string */
  if(pmOptions->ForgetString != TRUE)
    DBPrintData(pmDecodedDataFile, HEADER_STRING);

  /*  Jump a line */
  DBPrintData(pmDecodedDataFile, "\n");

  /*  The baner */
  DBPrintData(pmDecodedDataFile, HEADER_BANER);
}

/******************************************************************
 * Function :
 *    DBPrintEvent()
 * Description :
 *    Prints an event to the given output file according to the
 *    options.
 * Parameters :
 *    pmDecodedDataFile, File to which the event is print.
 *    pmEvent, Raw event
 *    pmEventCPUID, The CPU-ID of the cpu on which event took place
 *    pmEventID, The event's ID.
 *    pmEventTime, The time at which the event occured.
 *    pmEventPID, The PID of the process to which this event belongs
 *    pmEventDataSize, The size of the tracing driver entry.
 *    pmEventString, String describing the event.
 *    pmDB, The database to which this event belongs.
 *    pmOptions, The user given options.
 * Return values :
 *    NONE
 * History :
 *    K.Y., 06/02/00, Removed from FileIO.c and put in EventDB.c.
 *    K.Y., 17/10/99, Added support for the trace properties and
 *                    extend list of parameters.
 *    K.Y., 25/05/99, Initial typing.
 * Note :
 ******************************************************************/
void DBPrintEvent(int              pmDecodedDataFile,
		  event*           pmEvent,
		  int              pmEventCPUID,
		  int              pmEventID,
		  struct timeval*  pmEventTime,
		  int              pmEventPID,
		  int              pmEventDataSize,
		  char*            pmEventString,
		  db*              pmDB,
		  options*         pmOptions)
{
  char              lTimeStr[TIME_STR_LEN]; /* Time of event */

  /* Write it all in a readable format according to the user's options */
  /*  The CPUID */
  if((pmOptions->ForgetCPUID != TRUE) && (pmDB->LogCPUID == TRUE))
    DBPrintData(pmDecodedDataFile, "%d \t", pmEventCPUID);

#if 0
  /*  The event ID */
  DBPrintData(pmDecodedDataFile, "%s \t", pmDB->EventString(pmDB, pmEventID, pmEvent));

  /* If we have RTAI, this will make sure that RT and normal Linux events are aligned OK */
  if(pmEventID <= TRACE_MAX)
    DBPrintData(pmDecodedDataFile, "\t");

  /*  An extra space for anything shorter than 6 characters */
  if(strlen(pmDB->EventString(pmDB, pmEventID, pmEvent)) <= 6)
    DBPrintData(pmDecodedDataFile, "\t");
#else
  /*  The event ID */
  DBPrintData(pmDecodedDataFile, "%-23s", pmDB->EventString(pmDB, pmEventID, pmEvent));
#endif

  /*  The time */
  if(pmOptions->ForgetTime != TRUE)
    {
    DBFormatTimeInReadableString(lTimeStr,
				 pmEventTime->tv_sec,
				 pmEventTime->tv_usec);    
    DBPrintData(pmDecodedDataFile, "%s \t", lTimeStr);
    }

  /*  The PID */
  if(pmOptions->ForgetPID != TRUE)
    {
    /*  The PID of the process to which the event belongs */
    if(pmEventPID != -1)
      DBPrintData(pmDecodedDataFile, "%d", pmEventPID);
    else
      DBPrintData(pmDecodedDataFile, "N/A");
    }

  /* Put a space */
  DBPrintData(pmDecodedDataFile, " \t");

  /*  The data size */
  if(pmOptions->ForgetDataLen != TRUE)
    DBPrintData(pmDecodedDataFile, "%d \t", pmEventDataSize);

  /*  The description string */
  if((pmEventString != NULL) && (pmOptions->ForgetString != TRUE))
    DBPrintData(pmDecodedDataFile, "%s", pmEventString);

  /*  A carriage return line-feed */
  DBPrintData(pmDecodedDataFile, "\n");
}

/******************************************************************
 * Function :
 *    DBPrintTrace()
 * Description :
 *    Print out the data-base and add details requested at the command-line
 * Parameters :
 *    pDecodedDataFile, File to which the data is to be printed.
 *    pmSystem, Description of the system.
 *    pmTraceDB, The event database to be processed.
 *    pmOptions, The user given options.
 * Return values :
 *    NONE
 * History :
 *    K.Y., 06/02/00, Modified so it would handle the new event
 *                    format.
 *    K.Y., 20/05/99, Initial typing.
 * Note :
 ******************************************************************/
void DBPrintTrace(int pmDecodedDataFile, systemInfo* pmSystem, db* pmTraceDB, options* pmOptions)
{
  int              lPID;                  /* PID of process to which event belongs */
  int              lPrevEventSet = FALSE; /* Has the previous event been set */
  char             lTabing[10];           /* Tab distance between syscall name and time */
  event            lEvent;                /* Generic event */
  syscallInfo*     pSyscall;              /* Generic syscall pointer */
  process*         pProcess;              /* Generic process pointer */
  process*         pProcessIdle = NULL;   /* Pointer to the idle process */
  process*         pPrevProcess = NULL;   /* Pointer to the process of the last event */
  struct timeval   lTraceDuration;        /* The time the trace lasted */
  struct timeval   lTimeProcesses={0,0};  /* Total amount of time during which processes had CPU control */
  struct timeval   lTimeSystem={0,0};     /* Total amount of time during which the system had CPU control */
  eventDescription lEventDesc;            /* Event's decription */

  /* Print some information about the trace  */
  /*  Start time*/
  DBPrintData(pmDecodedDataFile,
	    "Trace start time: (%ld, %ld) \n",
	    pmTraceDB->StartTime.tv_sec,
	    pmTraceDB->StartTime.tv_usec);
  /*  End time */
  DBPrintData(pmDecodedDataFile,
	    "Trace end time: (%ld, %ld) \n",
	    pmTraceDB->EndTime.tv_sec,
	    pmTraceDB->EndTime.tv_usec);
  /*  Duration */
  DBTimeSub(lTraceDuration, pmTraceDB->EndTime, pmTraceDB->StartTime);

  DBPrintData(pmDecodedDataFile,
	    "Trace duration: (%ld, %ld) \n\n",
	    lTraceDuration.tv_sec,
	    lTraceDuration.tv_usec);
  /*  Number of occurences of some events */
  DBPrintData(pmDecodedDataFile,
	    "Number of occurences of:\n");
  DBPrintData(pmDecodedDataFile,
	    "\tEvents: %ld \n",
	    pmTraceDB->Nb);
  DBPrintData(pmDecodedDataFile,
	    "\tScheduling changes: %ld \n",
	    pmSystem->SchedChange);
  DBPrintData(pmDecodedDataFile,
	    "\tKernel timer tics: %ld \n",
	    pmSystem->KernelTimer);
  DBPrintData(pmDecodedDataFile,
	    "\tSystem call entries: %ld \n",
	    pmSystem->SyscallEntry);
  DBPrintData(pmDecodedDataFile,
	    "\tSystem call exits: %ld \n",
	    pmSystem->SyscallExit);
  DBPrintData(pmDecodedDataFile,
	    "\tTrap entries: %ld \n",
	    pmSystem->TrapEntry);
  DBPrintData(pmDecodedDataFile,
	    "\tTrap exits: %ld \n",
	    pmSystem->TrapExit);
  DBPrintData(pmDecodedDataFile,
	    "\tIRQ entries: %ld \n",
	    pmSystem->IRQEntry);
  DBPrintData(pmDecodedDataFile,
	    "\tIRQ exits: %ld \n",
	    pmSystem->IRQExit);
  DBPrintData(pmDecodedDataFile,
	    "\tBottom halves: %ld \n",
	    pmSystem->BH);
  DBPrintData(pmDecodedDataFile,
	    "\tTimer expiries: %ld \n",
	    pmSystem->TimerExpire);
  DBPrintData(pmDecodedDataFile,
	    "\tPage allocations: %ld \n",
	    pmSystem->PageAlloc);
  DBPrintData(pmDecodedDataFile,
	    "\tPage frees: %ld \n",
	    pmSystem->PageFree);
  DBPrintData(pmDecodedDataFile,
	    "\tPackets Out: %ld \n",
	    pmSystem->PacketOut);
  DBPrintData(pmDecodedDataFile,
	    "\tPackets In: %ld \n\n",
	    pmSystem->PacketIn);

  /* Has the user requested to know the time spent in system calls */
  if(pmOptions->AcctSysCall)
    {
    /* Print summary header */
    DBPrintData(pmDecodedDataFile,
	      "Analysis details: \n");

    /* Go through the process list */
    for(pProcess = pmSystem->Processes; pProcess != NULL; pProcess = pProcess->Next)
      {
      /* Print basic process information */
      DBPrintData(pmDecodedDataFile,
		"\tProcess (%d, %d)",
		pProcess->PID,
		pProcess->PPID);
      if(pProcess->Command != NULL)
	DBPrintData(pmDecodedDataFile,
		  ": %s: \n",
		  pProcess->Command);
      else
	DBPrintData(pmDecodedDataFile,
		  ": \n");

      /* Is this the idle process */
      if(pProcess->PID == 0)
	pProcessIdle = pProcess;

      /* Print detailed process information */
      DBPrintData(pmDecodedDataFile,
		"\t\tNumber of system calls: %d \n",
		pProcess->NbSyscalls);
      DBPrintData(pmDecodedDataFile,
		"\t\tNumber of traps: %d \n",
		pProcess->NbTraps);
      DBPrintData(pmDecodedDataFile,
		"\t\tQuantity of data read from files: %d \n",
		pProcess->QFileRead);
      DBPrintData(pmDecodedDataFile,
		"\t\tQuantity of data written to files: %d \n",
		pProcess->QFileWrite);
      DBPrintData(pmDecodedDataFile,
		"\t\tTime executing process code: (%ld, %ld) => %2.2f %% \n",
		pProcess->TimeProcCode.tv_sec,
		pProcess->TimeProcCode.tv_usec,
		(float) 100 * (DBGetTimeInDouble(pProcess->TimeProcCode) / DBGetTimeInDouble(lTraceDuration)));
      DBPrintData(pmDecodedDataFile,
		"\t\tTime running: (%ld, %ld) => %2.2f %% \n",
		pProcess->TimeRuning.tv_sec,
		pProcess->TimeRuning.tv_usec,
		(float) 100 * (DBGetTimeInDouble(pProcess->TimeRuning) / DBGetTimeInDouble(lTraceDuration)));
      DBPrintData(pmDecodedDataFile,
		"\t\tTime waiting for I/O: (%ld, %ld) => %2.2f %% \n",
		pProcess->TimeIO.tv_sec,
		pProcess->TimeIO.tv_usec,
		(float) 100 * (DBGetTimeInDouble(pProcess->TimeIO) / DBGetTimeInDouble(lTraceDuration)));

      /* Add the time of this process to the amount of time the CPU was controled by a process */
      DBTimeAdd(lTimeProcesses, lTimeProcesses, pProcess->TimeProcCode);

      /* Print out system call detailed information */
      DBPrintData(pmDecodedDataFile,
		"\t\tSystem call usage: \n");

      /* Go through the list of system calls called by this process */
      for(pSyscall = pProcess->Syscalls; pSyscall != NULL; pSyscall = pSyscall->Next)
	{
	/* Compute tabbing according to syscall's name length */
	if(strlen(pmTraceDB->SyscallString(pmTraceDB, pSyscall->ID)) < 6)
	  sprintf(lTabing, "\t\t");
	else if (strlen(pmTraceDB->SyscallString(pmTraceDB, pSyscall->ID)) < 13)
	  sprintf(lTabing, "\t");
	else
	  lTabing[0] = '\0';
	
	/* Print system call information */
	DBPrintData(pmDecodedDataFile,
		  "\t\t%s: %s %d \t (%ld, %ld)\n",
		  pmTraceDB->SyscallString(pmTraceDB, pSyscall->ID),
		  lTabing,
		  pSyscall->Nb,
		  pSyscall->Time.tv_sec,
		  pSyscall->Time.tv_usec);
	}
      }

    /* System wide information */
    DBPrintData(pmDecodedDataFile,
	      "\tSystem-wide:\n");

    /* Print out the total amount of time processes had the control of the cpu */
    DBPrintData(pmDecodedDataFile,
	      "\t\tTotal process time: (%ld, %ld) => %2.2f %% \n",
	      lTimeProcesses.tv_sec,
	      lTimeProcesses.tv_usec,
	      (float) 100 * (DBGetTimeInDouble(lTimeProcesses) / DBGetTimeInDouble(lTraceDuration)));

    /* Print out the total amount of time linux had the control of the cpu */
    DBTimeSub(lTimeSystem, lTraceDuration, lTimeProcesses);
    DBPrintData(pmDecodedDataFile,
	      "\t\tTotal system time: (%ld, %ld) => %2.2f %% \n",
	      lTimeSystem.tv_sec,
	      lTimeSystem.tv_usec,
	      (float) 100 * (DBGetTimeInDouble(lTimeSystem) / DBGetTimeInDouble(lTraceDuration)));

    /* Print out the total amount of time idle was runing, if it ever ran */
    if(pProcessIdle != NULL)
      DBPrintData(pmDecodedDataFile,
		"\t\tSystem idle: (%ld, %ld) => %2.2f %% \n",
		pProcessIdle->TimeRuning.tv_sec,
		pProcessIdle->TimeRuning.tv_usec,
		(float) 100 * (DBGetTimeInDouble(pProcessIdle->TimeRuning) / DBGetTimeInDouble(lTraceDuration)));

    /* Print out system call detailed information */
    DBPrintData(pmDecodedDataFile,
	      "\t\tSystem call usage: \n");

    /* Go through the system-wide syscall accouting list */
    for(pSyscall = pmSystem->Syscalls; pSyscall != NULL; pSyscall = pSyscall->Next)
      {
      /* Compute tabbing according to syscall's name length */
      if(strlen(pmTraceDB->SyscallString(pmTraceDB, pSyscall->ID)) < 6)
	sprintf(lTabing, "\t\t");
      else if (strlen(pmTraceDB->SyscallString(pmTraceDB, pSyscall->ID)) < 13)
	sprintf(lTabing, "\t");
      else
	lTabing[0] = '\0';
      
      /* Print system call information */
      DBPrintData(pmDecodedDataFile,
		"\t\t%s: %s %d \t (%ld, %ld)\n",
		pmTraceDB->SyscallString(pmTraceDB, pSyscall->ID),
		lTabing,
		pSyscall->Nb,
		pSyscall->Time.tv_sec,
		pSyscall->Time.tv_usec);
      }
    }

  DBPrintData(pmDecodedDataFile,
	    "\n");

  /* Are we tracing only one CPUID */
  if(pmOptions->TraceCPUID == TRUE)
    DBPrintData(pmDecodedDataFile,
	      "Tracing CPU %d only \n\n",
	      pmOptions->CPUID);

  /* Are we tracing only one PID */
  if(pmOptions->TracePID == TRUE)
    DBPrintData(pmDecodedDataFile,
	      "Tracing process %d only \n\n",
	      pmOptions->PID);

  /* Was it only a summary that we wanted */
  if(pmOptions->Summarize == TRUE)
    return;

  /* Print the output file's event header */
  DBPrintHeader(pmDecodedDataFile, pmTraceDB, pmOptions);

  /* Go through the events list */
  lEvent = pmTraceDB->FirstEvent;
  do
    {
    /* Get the event's description */
    DBEventDescription(pmTraceDB, &lEvent, TRUE, &lEventDesc);

    /* Get the event's process */
    pProcess = DBEventProcess(pmTraceDB, &lEvent, pmSystem, FALSE);

    /* Does this process belong to anyone */
    if(pProcess == NULL)
      lPID = -1;
    else
      lPID = pProcess->PID;

    /* Did the user request this event to be printed */
    if((((pmOptions->Omit == TRUE) && (!ltt_test_bit(lEventDesc.ID, &(pmOptions->OmitMask))))
       ||((pmOptions->Trace == TRUE) && (ltt_test_bit(lEventDesc.ID, &(pmOptions->TraceMask))))
       ||((pmOptions->Trace != TRUE) && (pmOptions->Omit != TRUE)))
     &&(((pmTraceDB->LogCPUID == TRUE)
        &&(((pmOptions->TraceCPUID == TRUE) && (pmOptions->CPUID == lEventDesc.CPUID))
          ||(pmOptions->TraceCPUID != TRUE)))
       ||(pmTraceDB->LogCPUID != TRUE))
     &&(((pmOptions->TracePID == TRUE) && (pProcess != NULL) && (pmOptions->PID == pProcess->PID))
       ||(pmOptions->TracePID != TRUE)
       ||((pmOptions->TracePID == TRUE)
         &&(lPrevEventSet == TRUE)
         &&(pPrevProcess != NULL)
         &&(lEventDesc.ID == TRACE_SCHEDCHANGE)
         &&(pPrevProcess->PID == pmOptions->PID))))
      /* Output this event */
      DBPrintEvent(pmDecodedDataFile,
	           &lEvent,
		   lEventDesc.CPUID,
		   lEventDesc.ID,
		   &(lEventDesc.Time),
		   lPID,
		   lEventDesc.Size,
		   lEventDesc.String,
		   pmTraceDB,
		   pmOptions);

    /* Set the previous event's data */
    lPrevEventSet = TRUE;
    pPrevProcess  = pProcess;
    } while(DBEventNext(pmTraceDB, &lEvent) == TRUE);
}

/******************************************************************
 * Function :
 *    DBCreateProcess()
 * Description :
 *    Creates a process, initializes it, inserts it into the list
 *    of processes belonging to the given system and returns it
 *    to the caller.
 * Parameters :
 *    pmSystem, the system to which the process belongs.
 *    pmPID, the PID of the process being created.
 *    pmPPID, the PID of the process' parent.
 * Return values :
 *    The newly created process.
 * History :
 *    K.Y. 03/11/99. Intial typing.
 ******************************************************************/
process* DBCreateProcess(systemInfo* pmSystem, int pmPID, int pmPPID)
{
  process*  pProcess;             /* Generic process pointer */
  
  /* Allocate space for a new process */
  if((pProcess = (process*) malloc(sizeof(process))) == NULL)
    {
    printf("Memory allocation problem \n");
    exit(1);
    }
	  
  /* Set new process's properties */
  pProcess->Type       = LINUX_PROCESS_OBJECT;
  pProcess->PID        = pmPID;
  pProcess->PPID       = pmPPID;
  pProcess->Command    = NULL;
  pProcess->NbSyscalls = 0;
  pProcess->NbTraps    = 0;
  pProcess->QFileRead  = 0;
  pProcess->QFileWrite = 0;

  pProcess->TimeProcCode.tv_sec  = 0;
  pProcess->TimeProcCode.tv_usec = 0;
  pProcess->TimeRuning.tv_sec    = 0;
  pProcess->TimeRuning.tv_usec   = 0;
  pProcess->TimeIO.tv_sec        = 0;
  pProcess->TimeIO.tv_usec       = 0;

  pProcess->Depth         = 0;
  pProcess->Pending       = NULL;
  pProcess->CtrlDepth     = 0;

  memset(&(pProcess->LastIOEvent), 0, sizeof(pProcess->LastIOEvent));
  memset(&(pProcess->LastSchedIn), 0, sizeof(pProcess->LastSchedIn));

  pProcess->Syscalls   = NULL;
  pProcess->System     = pmSystem;

  pProcess->Next       = NULL;
  pProcess->Children   = NULL;
  pProcess->NextChild  = NULL;

  pProcess->Hook       = NULL;

  /* Insert new process in processes list */
  pProcess->Next = pmSystem->Processes;
  pmSystem->Processes = pProcess;    

  /* Give the newly created process to the caller */
  return pProcess;
}

/******************************************************************
 * Function :
 *    DBCreateDB()
 * Description :
 *    Creates a database of events, initializes it and returns it
 *    to the caller.
 * Parameters :
 *    None.
 * Return values :
 *    The newly created database.
 * History :
 *    K.Y. 21/10/99. Intial typing.
 ******************************************************************/
db* DBCreateDB(void)
{
  db*     pDB;             /* The newly create database */
  int     i;               /* Generic index */

  /* Allocate space for database */
  if((pDB = (db*) malloc(sizeof(db))) == NULL)
    {
    printf("Memory allocation problem \n");
    exit(1);
    }

  /* Initialize database */
  pDB->TraceStart = NULL;
  pDB->Nb         = 0;
  pDB->NbCustom   = 0;

  /* Initialize custom event circular list head */
  pDB->CustomEvents.Next = &(pDB->CustomEvents);
  pDB->CustomEvents.Prev = &(pDB->CustomEvents);

  /* Initialize scheduling events existence */
  pDB->SchedEventsExist = FALSE;

  /* Initialize custom event hash table */
  for(i = 0; i < CUSTOM_EVENT_HASH_TABLE_SIZE; i++)
    pDB->CustomHashTable[i].NextHash = NULL;

  /* Give the new database to the caller */
  return pDB;
}

/******************************************************************
 * Function :
 *    DBCreateSystem()
 * Description :
 *    Creates a system, initializes it and returns it to the caller.
 * Parameters :
 *    None.
 * Return values :
 *    The newly created system.
 * History :
 *    K.Y. 21/10/99. Intial typing.
 ******************************************************************/
systemInfo* DBCreateSystem(void)
{
  int          i;        /* Generic index */
  systemInfo*  pSystem;  /* The newly created system */

  /* Allocate space for database */
  if((pSystem = (systemInfo*) malloc(sizeof(systemInfo))) == NULL)
    {
    printf("Memory allocation problem \n");
    exit(1);
    }

  /* Initialize the system */
  pSystem->Processes  = NULL;
  pSystem->ProcTree   = NULL;
  pSystem->Syscalls   = NULL;
  pSystem->SystemSpec = NULL;

  /* For every entry in the interrupt description table */
  for (i = 0; i < MAX_NB_INTERRUPTS; i++)
#if 0
    {
    /* Allocate space for this entry */
    pSystem->Interrupts[i] = malloc(4);	/* plus the null char */

    /* Did we allocated anything successfully */
    if(pSystem->Interrupts[i] == NULL)
      {
      printf("Memory allocation problem: malloc of Interrupt name table failed");
      exit(1);
      }

    /* Fill the entry with the default "N/A" */
    strcpy(pSystem->Interrupts[i], "N/A");
    }
#else
  pSystem->Interrupts[i] = NULL;
#endif

  /* Give the new system to the caller */
  return pSystem;
}

/******************************************************************
 * Function :
 *    DBDestroyTrace()
 * Description :
 * Parameters :
 * Return values :
 * History :
 * Note :
 ******************************************************************/
void DBDestroyTrace(systemInfo* pmSystem, db* pmEventDB)
{
  int                 i;            /* Generic index */
  syscallInfo*        pSysCall;     /* Generic syscall pointer */
  syscallInfo*        pSysCallTemp; /* Temporary syscall pointer */
  process*            pProc;        /* Generic process pointer */
  process*            pProcTemp;    /* Temporary process pointer */
  customEventDesc*    pCustom;      /* Generic custom event pointer */
  customEventDesc*    pCustomTemp;  /* Temporyra custom event pointer */

  /* Unmap the event file, if it was mapped */
  if(pmEventDB->TraceStart != NULL)
    munmap(pmEventDB->TraceStart, pmEventDB->TraceSize);

#if SUPP_RTAI
  /* Does this trace contain RTAI-specific information */
  if(pmEventDB->SystemType == TRACE_SYS_TYPE_RTAI_LINUX)
    /* Destroy RTAI-secific info. */
    RTAIDBDestroySystem(pmSystem);
#endif /* endif */

  /* Go through custom events list */
  for(pCustom = pmEventDB->CustomEvents.Next;
      pCustom != &(pmEventDB->CustomEvents);)
    {
    /* Remember next custom event */
    pCustomTemp = pCustom->Next;

    /* Free space taken by custom event */
    free(pCustom);

    /* Go to next custom event */
    pCustom = pCustomTemp;
    }

  /* Free memory used by event database */
  free(pmEventDB);

  /* Go through process list */
  for(pProc = pmSystem->Processes; pProc != NULL;)
    {
    /* Remember next process in list */
    pProcTemp = pProc->Next;

    /* Free memory used by command, if any */
    if(pProc->Command != NULL)
      free(pProc->Command);

    /* Go through list of syscalls for this process */
    for(pSysCall = pProc->Syscalls; pSysCall != NULL;)
      {
      /* Remember next syscall in syscall list */
      pSysCallTemp = pSysCall->Next;

      /* Free space taken by syscall */
      free(pSysCall);

      /* Go to next syscall */
      pSysCall = pSysCallTemp;
      }

    /* Go through list of pending  syscalls for this process (this is PARANOIA) */
    for(pSysCall = pProc->Pending; pSysCall != NULL;)
      {
      /* Remember next syscall in syscall list */
      pSysCallTemp = pSysCall->Next;

      /* Free space taken by syscall */
      free(pSysCall);

      /* Go to next syscall */
      pSysCall = pSysCallTemp;
      }

    /* Free memory used by process */
    free(pProc);

    /* Go to next process */
    pProc = pProcTemp;
    }

  /* Go through list of system-wide system call accouting */
  for(pSysCall = pmSystem->Syscalls; pSysCall != NULL;)
    {
    /* Remember next syscall in syscall list */
    pSysCallTemp = pSysCall->Next;

    /* Free space taken by syscall */
    free(pSysCall);
    
    /* Go to next syscall */
    pSysCall = pSysCallTemp;
    }

  /* Free the entries allocated for interrupt descriptions */
  for(i = 0; i < MAX_NB_INTERRUPTS; i++)
    free(pmSystem->Interrupts[i]);

  /* Free memory used by system */
  free(pmSystem);
}
