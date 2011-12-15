/*
   RTAIDB.c : Event database engine for RTAI traces.
   Copyright (C) 2000, 2001, 2002 Karim Yaghmour (karim@opersys.com).
 
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
     K.Y., 16/03/2002, Added RTAI cross-platform reading capabilities.
     K.Y., 19/06/2001, Moved event database into libltt.
     K.Y., 12/06/2000, Initial typing.
*/

#include <malloc.h>
#include <unistd.h>

#include <Tables.h>
#include <EventDB.h>
#include <RTAIDB.h>

#include "EventDBI.h"

/******************************************************************
 * Function :
 *    RTAIDBIEventString()
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
 *    K.Y., 28/05/2000, Initial typing.
 ******************************************************************/
int RTAIDBIEventString(db*              pmDB,
		       uint32_t         pmBufID,
		       void*            pmEventAddr,
		       char*            pmString,
		       int              pmStringLen)
{
  void*          pReadAddr;     /* Address from which to read */
  void*          pEventStruct;  /* Pointer to begining of structure of event */
  uint8_t        lEventID;      /* The event's ID */

  /* Get the event's ID */
  lEventID = DBIEventID(pmDB, pmEventAddr);

  /* Is there a structure describing this event */
  if((pmDB->EventStructSize(pmDB, lEventID) == 0) || !(ltt_test_bit(lEventID, &(pmDB->DetailsMask))))
    return 0;

  /* Get the structure of this event */
  pEventStruct = pReadAddr = DBIEventStruct(pmDB, pmEventAddr);

  /* Depending on the event's type */
  switch(lEventID)
    {
    /* TRACE_RTAI_MOUNT */
    case TRACE_RTAI_MOUNT :
      /* Format the string describing the event */
      pmString[0] = '\0';
      break;

    /* TRACE_RTAI_UMOUNT */
    case TRACE_RTAI_UMOUNT :
      /* Format the string describing the event */
      pmString[0] = '\0';
      break;

    /* TRACE_RTAI_GLOBAL_IRQ_ENTRY */
    case TRACE_RTAI_GLOBAL_IRQ_ENTRY :
      /* Are we inside the kernel */
      if(RFT8(pmDB, RTAI_GLOBAL_IRQ_ENTRY_EVENT(pEventStruct)->kernel) == 1)
	/* Format the string describing the event */
	sprintf(pmString,
		"IRQ : %d, IN-KERNEL",
		RFT8(pmDB, RTAI_GLOBAL_IRQ_ENTRY_EVENT(pEventStruct)->irq_id));
      else
	/* Format the string describing the event */
	sprintf(pmString,
		"IRQ : %d",
		RFT8(pmDB, RTAI_GLOBAL_IRQ_ENTRY_EVENT(pEventStruct)->irq_id));
      break;

    /* TRACE_RTAI_GLOBAL_IRQ_EXIT */
    case TRACE_RTAI_GLOBAL_IRQ_EXIT :
      /* Format the string describing the event */
      pmString[0] = '\0';
      break;

    /* TRACE_RTAI_OWN_IRQ_ENTRY */
    case TRACE_RTAI_OWN_IRQ_ENTRY :
      /* Are we inside the kernel */
      if(RFT8(pmDB, RTAI_OWN_IRQ_ENTRY_EVENT(pEventStruct)->kernel) == 1)
	/* Format the string describing the event */
	sprintf(pmString,
		"IRQ : %d, IN-KERNEL",
		RFT8(pmDB, RTAI_OWN_IRQ_ENTRY_EVENT(pEventStruct)->irq_id));
      else
	/* Format the string describing the event */
	sprintf(pmString,
		"IRQ : %d",
		RFT8(pmDB, RTAI_OWN_IRQ_ENTRY_EVENT(pEventStruct)->irq_id));
      break;

    /* TRACE_RTAI_OWN_IRQ_EXIT */
    case TRACE_RTAI_OWN_IRQ_EXIT :
      /* Format the string describing the event */
      pmString[0] = '\0';
      break;

    /* TRACE_RTAI_TRAP_ENTRY */
    case TRACE_RTAI_TRAP_ENTRY :
      /* Format the string describing the event */
      sprintf(pmString,
	      "TRAP : %s; EIP : 0x%08X",
	      pmDB->TrapString(pmDB, RFT8(pmDB, RTAI_TRAP_ENTRY_EVENT(pEventStruct)->trap_id)),
	      RFT32(pmDB, RTAI_TRAP_ENTRY_EVENT(pEventStruct)->address));
      break;

    /* TRACE_RTAI_TRAP_EXIT */
    case TRACE_RTAI_TRAP_EXIT :
      /* Format the string describing the event */
      pmString[0] = '\0';
      break;

    /* TRACE_RTAI_SRQ_ENTRY */
    case TRACE_RTAI_SRQ_ENTRY :
      /* Are we inside the kernel */
      if(RFT8(pmDB, RTAI_SRQ_ENTRY_EVENT(pEventStruct)->kernel) == 1)
	/* Format the string describing the event */
	sprintf(pmString,
		"SRQ : %d, IN-KERNEL",
		RFT8(pmDB, RTAI_SRQ_ENTRY_EVENT(pEventStruct)->srq_id));
      else
	/* Format the string describing the event */
	sprintf(pmString,
		"SRQ : %d",
		RFT8(pmDB, RTAI_SRQ_ENTRY_EVENT(pEventStruct)->srq_id));
      break;

    /* TRACE_RTAI_SRQ_EXIT */
    case TRACE_RTAI_SRQ_EXIT :
      /* Format the string describing the event */
      pmString[0] = '\0';
      break;

    /* TRACE_RTAI_SWITCHTO_LINUX */
    case TRACE_RTAI_SWITCHTO_LINUX :
      /* Format the string describing the event */
      sprintf(pmString,
	      "CPU-ID : %d",
	      RFT8(pmDB, RTAI_SWITCHTO_LINUX_EVENT(pEventStruct)->cpu_id));
      break;

    /* TRACE_RTAI_SWITCHTO_RT */
    case TRACE_RTAI_SWITCHTO_RT :
      /* Format the string describing the event */
      sprintf(pmString,
	      "CPU-ID : %d",
	      RFT8(pmDB, RTAI_SWITCHTO_RT_EVENT(pEventStruct)->cpu_id));
      break;

    /* TRACE_RTAI_SCHED_CHANGE */
    case TRACE_RTAI_SCHED_CHANGE :
      /* Format the string describing the event */
      sprintf(pmString,
	      "IN : %d; OUT : %d; STATE : %d",
	      RFT32(pmDB, RTAI_SCHED_CHANGE_EVENT(pEventStruct)->in),
	      RFT32(pmDB, RTAI_SCHED_CHANGE_EVENT(pEventStruct)->out),
	      RFT32(pmDB, RTAI_SCHED_CHANGE_EVENT(pEventStruct)->out_state));
      break;

    /* TRACE_RTAI_TASK */
    case TRACE_RTAI_TASK :
      /* Depending on the task event */
      switch(RFT8(pmDB, RTAI_TASK_EVENT(pEventStruct)->event_sub_id))
	{
	/* TRACE_RTAI_INIT */
	case TRACE_RTAI_TASK_INIT :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "INIT => TASK : %d, THREAD : %08LX, PRIORITY : %Ld",
		  RFT32(pmDB, RTAI_TASK_EVENT(pEventStruct)->event_data1),
		  RFT64(pmDB, RTAI_TASK_EVENT(pEventStruct)->event_data2),
		  RFT64(pmDB, RTAI_TASK_EVENT(pEventStruct)->event_data3));
	  break;

	/* TRACE_RTAI_TASK_DELETE */
	case TRACE_RTAI_TASK_DELETE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "DELETE => TASK : %d",
		  RFT32(pmDB, RTAI_TASK_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_TASK_SIG_HANDLER */
	case TRACE_RTAI_TASK_SIG_HANDLER :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SIG HANDLER => TASK : %d; HANDLER : %08LX",
		  RFT32(pmDB, RTAI_TASK_EVENT(pEventStruct)->event_data1),
		  RFT64(pmDB, RTAI_TASK_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_TASK_YIELD */
	case TRACE_RTAI_TASK_YIELD :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "YIELD");
	  break;

	/* TRACE_RTAI_TASK_SUSPEND */
	case TRACE_RTAI_TASK_SUSPEND :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SUSPEND => TASK : %d",
		  RFT32(pmDB, RTAI_TASK_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_TASK_RESUME */
	case TRACE_RTAI_TASK_RESUME :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "RESUME => TASK : %d",
		  RFT32(pmDB, RTAI_TASK_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_TASK_MAKE_PERIOD_RELATIVE */
	case TRACE_RTAI_TASK_MAKE_PERIOD_RELATIVE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "MAKE PERIODIC RELATIVE => TASK : %d; START DELAY : %Ld; PERIOD : %Ld",
		  RFT32(pmDB, RTAI_TASK_EVENT(pEventStruct)->event_data1),
		  RFT64(pmDB, RTAI_TASK_EVENT(pEventStruct)->event_data2),
		  RFT64(pmDB, RTAI_TASK_EVENT(pEventStruct)->event_data3));
	  break;

	/* TRACE_RTAI_TASK_MAKE_PERIOD */
	case TRACE_RTAI_TASK_MAKE_PERIOD :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "MAKE PERIODIC => TASK : %d; START DELAY : %Ld; PERIOD : %Ld",
		  RFT32(pmDB, RTAI_TASK_EVENT(pEventStruct)->event_data1),
		  RFT64(pmDB, RTAI_TASK_EVENT(pEventStruct)->event_data2),
		  RFT64(pmDB, RTAI_TASK_EVENT(pEventStruct)->event_data3));
	  break;

	/* TRACE_RTAI_TASK_WAIT_PERIOD */
	case TRACE_RTAI_TASK_WAIT_PERIOD :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "WAIT PERIOD");
	  break;

	/* TRACE_RTAI_TASK_BUSY_SLEEP */
	case TRACE_RTAI_TASK_BUSY_SLEEP :
	  /* Format the string describing the event */
	  pmString[0] = '\0';
	  break;

	/* TRACE_RTAI_TASK_SLEEP */
	case TRACE_RTAI_TASK_SLEEP :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "BUSY SLEEP => TIME(ns) : %d",
		  RFT32(pmDB, RTAI_TASK_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_TASK_SLEEP_UNTIL */
	case TRACE_RTAI_TASK_SLEEP_UNTIL :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SLEEP UNTIL => TIME : %Ld",
		  RFT64(pmDB, RTAI_TASK_EVENT(pEventStruct)->event_data2));
	  break;
	}
      break;

    /* TRACE_RTAI_TIMER */
    case TRACE_RTAI_TIMER :
      /* Depending on the timer event */
      switch(RFT8(pmDB, RTAI_TIMER_EVENT(pEventStruct)->event_sub_id))
	{
	/* TRACE_RTAI_TIMER_REQUEST */
	case TRACE_RTAI_TIMER_REQUEST :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "REQUEST => HANDLER : %08X; TICKS : %d",
		  RFT32(pmDB, RTAI_TIMER_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_TIMER_EVENT(pEventStruct)->event_data2));
	  break;
	/* TRACE_RTAI_TIMER_FREE */
	case TRACE_RTAI_TIMER_FREE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "FREE");
	  break;
	/* TRACE_RTAI_TIMER_REQUEST_APIC */
	case TRACE_RTAI_TIMER_REQUEST_APIC :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "REQUEST APIC TIMERS => HANDLER : %08X",
		  RFT32(pmDB, RTAI_TIMER_EVENT(pEventStruct)->event_data1));
	  break;
	/* TRACE_RTAI_TIMER_APIC_FREE */
	case TRACE_RTAI_TIMER_APIC_FREE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "FREE APIC TIMERS");
	  break;
	/* TRACE_RTAI_TIMER_HANDLE_EXPIRY */
	case TRACE_RTAI_TIMER_HANDLE_EXPIRY :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "TIMER EXPIRY");
	  break;
	}
      break;

    /* TRACE_RTAI_SEM */
    case TRACE_RTAI_SEM :
      /* Depending on the semaphore event */
      switch(RFT8(pmDB, RTAI_SEM_EVENT(pEventStruct)->event_sub_id))
	{
	/* TRACE_RTAI_SEM_INIT */
	case TRACE_RTAI_SEM_INIT :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "INIT => SEM : %08X; VALUE : %Ld",
		  RFT32(pmDB, RTAI_SEM_EVENT(pEventStruct)->event_data1),
		  RFT64(pmDB, RTAI_SEM_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_SEM_DELETE */
	case TRACE_RTAI_SEM_DELETE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "DELETE => SEM : %08X",
		  RFT32(pmDB, RTAI_SEM_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_SEM_SIGNAL */
	case TRACE_RTAI_SEM_SIGNAL :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SIGNAL => SEM : %08X",
		  RFT32(pmDB, RTAI_SEM_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_SEM_WAIT */
	case TRACE_RTAI_SEM_WAIT :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "WAIT => SEM : %08X",
		  RFT32(pmDB, RTAI_SEM_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_SEM_WAIT_IF */
	case TRACE_RTAI_SEM_WAIT_IF :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "WAIT IF => SEM : %08X",
		  RFT32(pmDB, RTAI_SEM_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_SEM_WAIT_UNTIL */
	case TRACE_RTAI_SEM_WAIT_UNTIL :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "WAIT UNTIL => SEM : %08X; DEADLINE : %Ld",
		  RFT32(pmDB, RTAI_SEM_EVENT(pEventStruct)->event_data1),
		  RFT64(pmDB, RTAI_SEM_EVENT(pEventStruct)->event_data2));
	  break;
	}
      break;

    /* TRACE_RTAI_MSG */
    case TRACE_RTAI_MSG :
      /* Depending on the message event */
      switch(RFT8(pmDB, RTAI_MSG_EVENT(pEventStruct)->event_sub_id))
	{
	/* TRACE_RTAI_MSG_SEND */
	case TRACE_RTAI_MSG_SEND :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SEND => TASK : %d; MSG : %ud",
		  RFT32(pmDB, RTAI_MSG_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_MSG_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_MSG_SEND_IF */
	case TRACE_RTAI_MSG_SEND_IF :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SEND IF => TASK : %d; MSG : %ud",
		  RFT32(pmDB, RTAI_MSG_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_MSG_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_MSG_SEND_UNTIL */
	case TRACE_RTAI_MSG_SEND_UNTIL :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SEND UNTIL => TASK : %d; MSG : %ud; DEADLINE : %Ld",
		  RFT32(pmDB, RTAI_MSG_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_MSG_EVENT(pEventStruct)->event_data2),
		  RFT64(pmDB, RTAI_MSG_EVENT(pEventStruct)->event_data3));
	  break;

	/* TRACE_RTAI_MSG_RECV */
	case TRACE_RTAI_MSG_RECV :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "RECEIVE => TASK : %d",
		  RFT32(pmDB, RTAI_MSG_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_MSG_RECV_IF */
	case TRACE_RTAI_MSG_RECV_IF :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "RECEIVE IF => TASK : %d",
		  RFT32(pmDB, RTAI_MSG_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_MSG_RECV_UNTIL */
	case TRACE_RTAI_MSG_RECV_UNTIL :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "RECEIVE UNTIL => TASK : %d; DEADLINE : %d",
		  RFT32(pmDB, RTAI_MSG_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_MSG_EVENT(pEventStruct)->event_data2));
	  break;
	}
      break;

    /* TRACE_RTAI_RPC */
    case TRACE_RTAI_RPC :
      /* Depending on the RPC event */
      switch(RFT8(pmDB, RTAI_RPC_EVENT(pEventStruct)->event_sub_id))
	{
	/* TRACE_RTAI_RPC_MAKE */
	case TRACE_RTAI_RPC_MAKE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "MAKE : TASK : %d; TO DO : %d",
		  RFT32(pmDB, RTAI_RPC_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_RPC_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_RPC_MAKE_IF */
	case TRACE_RTAI_RPC_MAKE_IF :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "MAKE IF : TASK : %d; TO DO : %d",
		  RFT32(pmDB, RTAI_RPC_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_RPC_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_RPC_MAKE_UNTIL */
	case TRACE_RTAI_RPC_MAKE_UNTIL :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "MAKE UNTIL : TASK : %d; TO DO : %d; DEADLINE : %Ld",
		  RFT32(pmDB, RTAI_RPC_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_RPC_EVENT(pEventStruct)->event_data2),
		  RFT64(pmDB, RTAI_RPC_EVENT(pEventStruct)->event_data3));
	  break;

	/* TRACE_RTAI_RPC_RETURN */
	case TRACE_RTAI_RPC_RETURN :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "RETURN : TASK : %d; RESULT : %d",
		  RFT32(pmDB, RTAI_RPC_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_RPC_EVENT(pEventStruct)->event_data2));
	  break;
	}
      break;

    /* TRACE_RTAI_MBX */
    case TRACE_RTAI_MBX :
      /* Depending on the mailbox event */
      switch(RFT8(pmDB, RTAI_MBX_EVENT(pEventStruct)->event_sub_id))
	{
	/* TRACE_RTAI_MBX_INIT */
	case TRACE_RTAI_MBX_INIT :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "INIT => MBX : %08X; SIZE : %d",
		  RFT32(pmDB, RTAI_MBX_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_MBX_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_MBX_DELETE */
	case TRACE_RTAI_MBX_DELETE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "DELETE => MBX : %08X",
		  RFT32(pmDB, RTAI_MBX_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_MBX_SEND */
	case TRACE_RTAI_MBX_SEND :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SEND => MBX : %08X; SIZE : %d",
		  RFT32(pmDB, RTAI_MBX_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_MBX_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_MBX_SEND_WP */
	case TRACE_RTAI_MBX_SEND_WP :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SEND WP => MBX : %08X; SIZE : %d",
		  RFT32(pmDB, RTAI_MBX_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_MBX_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_MBX_SEND_IF */
	case TRACE_RTAI_MBX_SEND_IF :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SEND IF => MBX : %08X; SIZE : %d",
		  RFT32(pmDB, RTAI_MBX_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_MBX_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_MBX_SEND_UNTIL */
	case TRACE_RTAI_MBX_SEND_UNTIL :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SEND UNTIL => MBX : %08X; SIZE : %d; DEADLINE : %qu",
		  RFT32(pmDB, RTAI_MBX_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_MBX_EVENT(pEventStruct)->event_data2),
		  RFT64(pmDB, RTAI_MBX_EVENT(pEventStruct)->event_data3));
	  break;

	/* TRACE_RTAI_MBX_RECV */
	case TRACE_RTAI_MBX_RECV :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "RECEIVE => MBX : %08X; RCV SIZE : %d",
		  RFT32(pmDB, RTAI_MBX_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_MBX_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_MBX_RECV_WP */
	case TRACE_RTAI_MBX_RECV_WP :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "RECEIVE WP => MBX : %08X; RCV SIZE : %d",
		  RFT32(pmDB, RTAI_MBX_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_MBX_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_MBX_RECV_IF */
	case TRACE_RTAI_MBX_RECV_IF :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "RECEIVE IF => MBX : %08X; RCV SIZE : %d",
		  RFT32(pmDB, RTAI_MBX_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_MBX_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_MBX_RECV_UNTIL */
	case TRACE_RTAI_MBX_RECV_UNTIL :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "RECEIVE UNTIL => MBX : %08X; RCV SIZE : %d; DEADLINE : %qu",
		  RFT32(pmDB, RTAI_MBX_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_MBX_EVENT(pEventStruct)->event_data2),
		  RFT64(pmDB, RTAI_MBX_EVENT(pEventStruct)->event_data3));
	  break;
	}
      break;

    /* TRACE_RTAI_FIFO */
    case TRACE_RTAI_FIFO :
      /* Depending on the FIFO event */
      switch(RFT8(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_sub_id))
	{
	/* TRACE_RTAI_FIFO_CREATE */
	case TRACE_RTAI_FIFO_CREATE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "CREATE => MINOR NB : %d; SIZE : %d",
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_FIFO_DESTOY */
	case TRACE_RTAI_FIFO_DESTROY :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "DESTROY => MINOR NB : %d",
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_FIFO_RESET */
	case TRACE_RTAI_FIFO_RESET :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "RESET => MINOR NB : %d",
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_FIFO_RESIZE */
	case TRACE_RTAI_FIFO_RESIZE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "RESIZE => MINOR NB : %d; SIZE : %d",
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_FIFO_PUT */
	case TRACE_RTAI_FIFO_PUT :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PUT => MINOR NB : %d; COUNT : %d",
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_FIFO_GET */
	case TRACE_RTAI_FIFO_GET :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "GET => MINOR NB : %d; COUNT : %d",
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_FIFO_CREATE_HANDLER */
	case TRACE_RTAI_FIFO_CREATE_HANDLER :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "CREATE HANDLER => MINOR NB : %d; HANDLER : %08X",
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_FIFO_OPEN */
	case TRACE_RTAI_FIFO_OPEN :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "OPEN => MINOR NB : %d; SIZE : %d",
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_FIFO_RELEASE */
	case TRACE_RTAI_FIFO_RELEASE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "RELEASE => MINOR NB : %d",
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_FIFO_READ */
	case TRACE_RTAI_FIFO_READ :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "READ => MINOR NB : %d; COUNT : %d",
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_FIFO_WRITE */
	case TRACE_RTAI_FIFO_WRITE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "WRITE => MINOR NB : %d; COUNT : %d",
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_FIFO_READ_TIMED */
	case TRACE_RTAI_FIFO_READ_TIMED :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "READ TIMED => MINOR NB : %d; DELAY : %d",
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_FIFO_WRITE_TIMED */
	case TRACE_RTAI_FIFO_WRITE_TIMED :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "WRITE TIMED => MINOR NB : %d; DELAY : %d",
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_FIFO_READ_ALLATONCE */
	case TRACE_RTAI_FIFO_READ_ALLATONCE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "READ ALL AT ONCE");
	  break;

	/* TRACE_RTAI_FIFO_LLSEEK */
	case TRACE_RTAI_FIFO_LLSEEK :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "LLSEEK => MINOR NB : %d; OFFSET : %d",
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_FIFO_FASYNC */
	case TRACE_RTAI_FIFO_FASYNC :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "FASYNC => MINOR NB : %d; FD : %d",
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_FIFO_IOCTL */
	case TRACE_RTAI_FIFO_IOCTL :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "IOCTL => MINOR NB : %d; COMMAND : %d",
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_FIFO_POLL */
	case TRACE_RTAI_FIFO_POLL :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "POLL => MINOR NB : %d",
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_FIFO_SUSPEND_TIMED */
	case TRACE_RTAI_FIFO_SUSPEND_TIMED :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SUSPEND TIMED => MINOR NB : %d; DELAY : %d",
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_FIFO_SET_ASYNC_SIG */
	case TRACE_RTAI_FIFO_SET_ASYNC_SIG :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SET ASYNC SIG => ASYNC SIG : %d",
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_FIFO_SEM_INIT */
	case TRACE_RTAI_FIFO_SEM_INIT :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SEM INIT => MINOR NB : %d; VALUE : %d",
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_FIFO_SEM_POST */
	case TRACE_RTAI_FIFO_SEM_POST :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SEM POST => MINOR NB : %d",
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_FIFO_SEM_WAIT */
	case TRACE_RTAI_FIFO_SEM_WAIT :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SEM WAIT => MINOR NB : %d",
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_FIFO_SEM_TRY_WAIT */
	case TRACE_RTAI_FIFO_SEM_TRY_WAIT :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SEM TRY WAIT => MINOR NB : %d",
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_FIFO_SEM_TIMED_WAIT */
	case TRACE_RTAI_FIFO_SEM_TIMED_WAIT :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SEM TIMED WAIT => MINOR NB : %d; DELAY : %d",
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_FIFO_SEM_DESTROY */
	case TRACE_RTAI_FIFO_SEM_DESTROY :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SEM DESTROY => MINOR NB : %d",
		  RFT32(pmDB, RTAI_FIFO_EVENT(pEventStruct)->event_data1));
	  break;
	}
      break;

    /* TRACE_RTAI_SHM */
    case TRACE_RTAI_SHM :
      /* Depending on the shared memory event */
      switch(RFT8(pmDB, RTAI_SHM_EVENT(pEventStruct)->event_sub_id))
	{
	/* TRACE_RTAI_SHM_MALLOC */
	case TRACE_RTAI_SHM_MALLOC :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "MALLOC => NAME : %d; SRQ: %d; PID : %d",
		  RFT32(pmDB, RTAI_SHM_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_SHM_EVENT(pEventStruct)->event_data2),
		  RFT32(pmDB, RTAI_SHM_EVENT(pEventStruct)->event_data3));
	  break;

	/* TRACE_RTAI_SHM_KMALLOC */
	case TRACE_RTAI_SHM_KMALLOC :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "KMALLOC => NAME : %d; SIZE: %d; PID : %d",
		  RFT32(pmDB, RTAI_SHM_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_SHM_EVENT(pEventStruct)->event_data2),
		  RFT32(pmDB, RTAI_SHM_EVENT(pEventStruct)->event_data3));
	  break;

	/* TRACE_RTAI_SHM_GET_SIZE */
	case TRACE_RTAI_SHM_GET_SIZE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "GET SIZE => NAME : %d; SRQ: %d; PID : %d",
		  RFT32(pmDB, RTAI_SHM_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_SHM_EVENT(pEventStruct)->event_data2),
		  RFT32(pmDB, RTAI_SHM_EVENT(pEventStruct)->event_data3));
	  break;

	/* TRACE_RTAI_SHM_FREE */
	case TRACE_RTAI_SHM_FREE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "FREE => NAME : %d; SRQ: %d; PID : %d",
		  RFT32(pmDB, RTAI_SHM_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_SHM_EVENT(pEventStruct)->event_data2),
		  RFT32(pmDB, RTAI_SHM_EVENT(pEventStruct)->event_data3));
	  break;

	/* TRACE_RTAI_SHM_KFREE */
	case TRACE_RTAI_SHM_KFREE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "KFREE => NAME : %d; PID : %d",
		  RFT32(pmDB, RTAI_SHM_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_SHM_EVENT(pEventStruct)->event_data3));
	  break;
	}
      break;

    /* TRACE_RTAI_POSIX */
    case TRACE_RTAI_POSIX :
      /* Depending on the POSIX event */
      switch(RFT8(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_sub_id))
	{
	/* TRACE_RTAI_POSIX_MQ_OPEN */
	case TRACE_RTAI_POSIX_MQ_OPEN :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "MQ OPEN => QUEUE ID : %d",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_POSIX_MQ_CLOSE */
	case TRACE_RTAI_POSIX_MQ_CLOSE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "MQ CLOSE => QUEUE ID : %u",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_POSIX_MQ_SEND */
	case TRACE_RTAI_POSIX_MQ_SEND :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "MQ SEND => QUEUE ID : %u; MSGLEN : %u; PRIORITY : %u",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data2),
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data3));
	  break;

	/* TRACE_RTAI_POSIX_MQ_RECV */
	case TRACE_RTAI_POSIX_MQ_RECV :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "MQ RECEIVE => QUEUE ID : %u; BUFLEN : %d",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_POSIX_MQ_GET_ATTR */
	case TRACE_RTAI_POSIX_MQ_GET_ATTR :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "MQ GET ATTR => QUEUE ID : %u",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_POSIX_MQ_SET_ATTR */
	case TRACE_RTAI_POSIX_MQ_SET_ATTR :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "MQ SET ATTR => QUEUE ID : %u",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_POSIX_MQ_NOTIFY */
	case TRACE_RTAI_POSIX_MQ_NOTIFY :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "MQ NOTIFY => QUEUE ID : %u",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_POSIX_MQ_UNLINK */
	case TRACE_RTAI_POSIX_MQ_UNLINK :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "MQ UNLINK => QUEUE ID : %u",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_CREATE */
	case TRACE_RTAI_POSIX_PTHREAD_CREATE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD CREATE => ROUTINE : %08X; TID : %d",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_EXIT */
	case TRACE_RTAI_POSIX_PTHREAD_EXIT :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD EXIT");
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_SELF */
	case TRACE_RTAI_POSIX_PTHREAD_SELF :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD SELF => TID : %d",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_ATTR_INIT */
	case TRACE_RTAI_POSIX_PTHREAD_ATTR_INIT :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD ATTRIBUTE INIT => ATTRIBUTE : %08X",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_ATTR_DESTROY */
	case TRACE_RTAI_POSIX_PTHREAD_ATTR_DESTROY :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD ATTRIBUTE DESTROY => ATTRIBUTE : %08X",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_ATTR_SETDETACHSTATE */
	case TRACE_RTAI_POSIX_PTHREAD_ATTR_SETDETACHSTATE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD ATTRIBUTE SET DETACH STATE => ATTRIBUTE : %08X; STATE : %d",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_ATTR_GETDETACHSTATE */
	case TRACE_RTAI_POSIX_PTHREAD_ATTR_GETDETACHSTATE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD ATTRIBUTE GET DETACH STATE => ATTRIBUTE : %08X; STATE : %d",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_ATTR_SETSCHEDPARAM */
	case TRACE_RTAI_POSIX_PTHREAD_ATTR_SETSCHEDPARAM :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD ATTRIBUTE SET SCHED PARAM => ATTRIBUTE : %08X",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_ATTR_GETSCHEDPARAM */
	case TRACE_RTAI_POSIX_PTHREAD_ATTR_GETSCHEDPARAM :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD ATTRIBUTE GET SCHED PARAM => ATTRIBUTE : %08X",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_ATTR_SETSCHEDPOLICY */
	case TRACE_RTAI_POSIX_PTHREAD_ATTR_SETSCHEDPOLICY :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD ATTRIBUTE SET SCHED POLICY => ATTRIBUTE : %08X; POLICY : %d",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_ATTR_GETSCHEDPOLICY */
	case TRACE_RTAI_POSIX_PTHREAD_ATTR_GETSCHEDPOLICY :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD ATTRIBUTE GET SCHED POLICY => ATTRIBUTE : %08X; POLICY : %d",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_ATTR_SETINHERITSCHED */
	case TRACE_RTAI_POSIX_PTHREAD_ATTR_SETINHERITSCHED :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD ATTRIBUTE SET INHERIT SCHED => ATTRIBUTE : %08X; INHERIT : %d",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_ATTR_GETINHERITSCHED */
	case TRACE_RTAI_POSIX_PTHREAD_ATTR_GETINHERITSCHED :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD ATTRIBUTE GET INHERIT SCHED => ATTRIBUTE : %08X; INHERIT : %d",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_ATTR_SETSCOPE */
	case TRACE_RTAI_POSIX_PTHREAD_ATTR_SETSCOPE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD ATTRIBUTE SET SCOPE => ATTRIBUTE : %08X; SCOPE : %d",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_ATTR_GETSCOPE */
	case TRACE_RTAI_POSIX_PTHREAD_ATTR_GETSCOPE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD ATTRIBUTE GET SCOPE => ATTRIBUTE : %08X; SCOPE : %d",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_SCHED_YIELD */
	case TRACE_RTAI_POSIX_PTHREAD_SCHED_YIELD :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD SCHED YIELD");
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_CLOCK_GETTIME */
	case TRACE_RTAI_POSIX_PTHREAD_CLOCK_GETTIME :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD CLOCK GET TIME => CLOCK ID : %d",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_MUTEX_INIT */
	case TRACE_RTAI_POSIX_PTHREAD_MUTEX_INIT :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD MUTEX INIT => MUTEX : %08X",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_MUTEX_DESTROY */
	case TRACE_RTAI_POSIX_PTHREAD_MUTEX_DESTROY :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD MUTEX DESTROY => MUTEX : %08X",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_MUTEXATTR_INIT */
	case TRACE_RTAI_POSIX_PTHREAD_MUTEXATTR_INIT :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD MUTEX ATTR INIT => MUTEX : %08X",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_MUTEXATTR_DESTROY */
	case TRACE_RTAI_POSIX_PTHREAD_MUTEXATTR_DESTROY :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD MUTEX ATTR DESTROY => MUTEX : %08X",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_MUTEXATTR_SETKIND_NP */
	case TRACE_RTAI_POSIX_PTHREAD_MUTEXATTR_SETKIND_NP :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD MUTEX SET KIND NP => MUTEX : %08X; KIND : %d",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_MUTEXATTR_GETKIND_NP */
	case TRACE_RTAI_POSIX_PTHREAD_MUTEXATTR_GETKIND_NP :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD MUTEX GET KIND NP => MUTEX : %08X; KIND : %d",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_SETSCHEDPARAM */
	case TRACE_RTAI_POSIX_PTHREAD_SETSCHEDPARAM :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD SET SCHED PARAM => TID : %d; POLICY : %d",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_GETSCHEDPARAM */
	case TRACE_RTAI_POSIX_PTHREAD_GETSCHEDPARAM :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD GET SCHED PARAM => TID : %d; POLICY : %d",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_MUTEX_TRY_LOCK */
	case TRACE_RTAI_POSIX_PTHREAD_MUTEX_TRY_LOCK :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD MUTEX TRY LOCK => MUTEX : %08X",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_MUTEX_LOCK */
	case TRACE_RTAI_POSIX_PTHREAD_MUTEX_LOCK :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD MUTEX LOCK => MUTEX : %08X",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_MUTEX_UNLOCK */
	case TRACE_RTAI_POSIX_PTHREAD_MUTEX_UNLOCK :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD MUTEX UNLOCK => MUTEX : %08X",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_COND_INIT */
	case TRACE_RTAI_POSIX_PTHREAD_COND_INIT :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD COND INIT => COND : %08X",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_COND_DESTROY */
	case TRACE_RTAI_POSIX_PTHREAD_COND_DESTROY :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD COND DESTROY => COND : %08X",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_CONDATTR_INIT */
	case TRACE_RTAI_POSIX_PTHREAD_CONDATTR_INIT :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD COND ATTR INIT => COND ATTR : %08X",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_CONDATTR_DESTROY */
	case TRACE_RTAI_POSIX_PTHREAD_CONDATTR_DESTROY :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD COND ATTR DESTROY => COND ATTR : %08X",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_COND_WAIT */
	case TRACE_RTAI_POSIX_PTHREAD_COND_WAIT :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD COND WAIT => COND : %08X; MUTEX : %08X",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_COND_TIMEDWAIT */
	case TRACE_RTAI_POSIX_PTHREAD_COND_TIMEDWAIT :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD COND TIMED WAIT => COND : %08X; MUTEX : %08X",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_COND_SIGNAL */
	case TRACE_RTAI_POSIX_PTHREAD_COND_SIGNAL :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD COND SIGNAL => COND : %08X",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_POSIX_PTHREAD_COND_BROADCAST */
	case TRACE_RTAI_POSIX_PTHREAD_COND_BROADCAST :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PTHREAD COND BROADCAST => COND : %08X",
		  RFT32(pmDB, RTAI_POSIX_EVENT(pEventStruct)->event_data1));
	  break;
	}
      break;

    /* TRACE_RTAI_LXRT */
    case TRACE_RTAI_LXRT :
      /* Depending on the LXRT event */
      switch(RFT8(pmDB, RTAI_LXRT_EVENT(pEventStruct)->event_sub_id))
	{
	/* TRACE_RTAI_LXRT_RTAI_SYSCALL_ENTRY */
	case TRACE_RTAI_LXRT_RTAI_SYSCALL_ENTRY :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "RTAI SYSCALL ENTRY");
	  break;

	/* TRACE_RTAI_LXRT_RTAI_SYSCALL_EXIT */
	case TRACE_RTAI_LXRT_RTAI_SYSCALL_EXIT :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "RTAI SYSCALL EXIT");
	  break;

	/* TRACE_RTAI_LXRT_SCHED_CHANGE */
	case TRACE_RTAI_LXRT_SCHED_CHANGE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SCHED CHANGE => IN : %d; OUT : %d; STATE : %d",
		  RFT32(pmDB, RTAI_LXRT_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_LXRT_EVENT(pEventStruct)->event_data2),
		  RFT32(pmDB, RTAI_LXRT_EVENT(pEventStruct)->event_data3));
	  break;

	/* TRACE_RTAI_LXRT_STEAL_TASK */
	case TRACE_RTAI_LXRT_STEAL_TASK :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "STEAL TASK => TASK : %d",
		  RFT32(pmDB, RTAI_LXRT_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_LXRT_GIVE_BACK_TASK */
	case TRACE_RTAI_LXRT_GIVE_BACK_TASK :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "GIVE BACK TASK => TASK : %d",
		  RFT32(pmDB, RTAI_LXRT_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_LXRT_SUSPEND */
	case TRACE_RTAI_LXRT_SUSPEND :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SUSPEND => TASK : %d",
		  RFT32(pmDB, RTAI_LXRT_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_LXRT_RESUME */
	case TRACE_RTAI_LXRT_RESUME :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "RESUME => TASK : %d; TYPE : %d",
		  RFT32(pmDB, RTAI_LXRT_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_LXRT_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_LXRT_HANDLE */
	case TRACE_RTAI_LXRT_HANDLE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "HANDLE => SRQ : %d",
		  RFT32(pmDB, RTAI_LXRT_EVENT(pEventStruct)->event_data1));
	  break;
	}
      break;

    /* TRACE_RTAI_LXRTI */
    case TRACE_RTAI_LXRTI :
      /* Depending on the LXRTI event */
      switch(RFT8(pmDB, RTAI_LXRTI_EVENT(pEventStruct)->event_sub_id))
	{
	/* TRACE_RTAI_LXRTI_NAME_ATTACH */
	case TRACE_RTAI_LXRTI_NAME_ATTACH :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "NAME ATTACH => PID : %u; NAME : %d",
		  RFT32(pmDB, RTAI_LXRTI_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_LXRTI_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_LXRTI_NAME_LOCATE */
	case TRACE_RTAI_LXRTI_NAME_LOCATE :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "NAME LOCATE => NAME : %d",
		  RFT32(pmDB, RTAI_LXRTI_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_LXRTI_NAME_DETACH */
	case TRACE_RTAI_LXRTI_NAME_DETACH :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "NAME DETACH => PID : %u",
		  RFT32(pmDB, RTAI_LXRTI_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_LXRTI_SEND */
	case TRACE_RTAI_LXRTI_SEND :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "SEND => PID : %u; SEND SIZE : %d; RECEIVE SIZE : %Ld",
		  RFT32(pmDB, RTAI_LXRTI_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_LXRTI_EVENT(pEventStruct)->event_data2),
		  RFT64(pmDB, RTAI_LXRTI_EVENT(pEventStruct)->event_data3));
	  break;

	/* TRACE_RTAI_LXRTI_RECV */
	case TRACE_RTAI_LXRTI_RECV :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "RECEIVE => PID : %u; MAX SIZE : %d",
		  RFT32(pmDB, RTAI_LXRTI_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_LXRTI_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_LXRTI_CRECV */
	case TRACE_RTAI_LXRTI_CRECV :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "C-RECEIVE => PID : %u; MAX SIZE : %d; DELAY : %Ld",
		  RFT32(pmDB, RTAI_LXRTI_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_LXRTI_EVENT(pEventStruct)->event_data2),
		  RFT64(pmDB, RTAI_LXRTI_EVENT(pEventStruct)->event_data3));
	  break;

	/* TRACE_RTAI_LXRTI_REPLY */
	case TRACE_RTAI_LXRTI_REPLY :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "REPLY => PID : %u; SIZE : %d",
		  RFT32(pmDB, RTAI_LXRTI_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_LXRTI_EVENT(pEventStruct)->event_data2));
	  break;

	/* TRACE_RTAI_LXRTI_PROXY_ATTACH */
	case TRACE_RTAI_LXRTI_PROXY_ATTACH :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PROXY ATTACH => PID : %u; NB BYTES : %d; PRIORITY : %Ld",
		  RFT32(pmDB, RTAI_LXRTI_EVENT(pEventStruct)->event_data1),
		  RFT32(pmDB, RTAI_LXRTI_EVENT(pEventStruct)->event_data2),
		  RFT64(pmDB, RTAI_LXRTI_EVENT(pEventStruct)->event_data3));
	  break;

	/* TRACE_RTAI_LXRTI_PROXY_DETACH */
	case TRACE_RTAI_LXRTI_PROXY_DETACH :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "PROXY DETACH => PID : %u",
		  RFT32(pmDB, RTAI_LXRTI_EVENT(pEventStruct)->event_data1));
	  break;

	/* TRACE_RTAI_LXRTI_TRIGGER */
	case TRACE_RTAI_LXRTI_TRIGGER :
	  /* Format the string describing the event */
	  sprintf(pmString,
		  "TRIGGER => PID : %u",
		  RFT32(pmDB, RTAI_LXRTI_EVENT(pEventStruct)->event_data1));
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

/******************************************************************
 * Function :
 *    RTAIDBEventTask()
 * Description :
 *    Returns a pointer to the task to which this event belongs.
 * Parameters :
 *    pmDB, Event database to which this event belongs.
 *    pmEvent, Pointer to said event.
 *    pmSystem, Pointer to system description.
 *    pmSetSwitchTime, Set the time at which this task was last
 *                     switched in starting from the current event.
 * Return values :
 *    Pointer to found task.
 *    NULL otherwise.
 * History :
 *    K.Y., 22/08/2000, Added switch-time memory.
 *    K.Y., 14/06/2000, This is largely inspired by DBEventProcess.
 ******************************************************************/
RTAItask* RTAIDBEventTask(db* pmDB, event* pmEvent, systemInfo* pmSystem, int pmSetSwitchTime)
{
  int              lFound = FALSE;    /* Did we find the last scheduling change */
  int              lTID;              /* TID of task */
  void*            pEventStruct;      /* Event structure */
  event            lEvent;            /* Event used to backup */
  RTAItask*        pTask;             /* Pointer to the task found */

  /* Use copy of event */
  lEvent = *pmEvent;

  /* Backup to the last scheduling change */
  do
    {
    /* Is this event a RTAI scheduling change */
    if(DBEventID(pmDB, &lEvent) == TRACE_RTAI_SCHED_CHANGE)
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

    /* Get the TID of the task that just got scheduled in */
    if((RFT32(pmDB, RTAI_SCHED_CHANGE_EVENT(pEventStruct)->out_state) == 0)
     &&(RFT32(pmDB, RTAI_SCHED_CHANGE_EVENT(pEventStruct)->in) == 0))
      lTID = RFT32(pmDB, RTAI_SCHED_CHANGE_EVENT(pEventStruct)->out);
    else
      lTID = RFT32(pmDB, RTAI_SCHED_CHANGE_EVENT(pEventStruct)->in);

    /* Get the task corresponding to this TID */
#if 0
    pTask = RTAIDBGetTaskByTID(lTID, pmSystem);
#else /* Make things simple for now ... */
    pTask = RTAIDBFindTaskInTree(lTID, RTAI_SYSTEM(pmSystem->SystemSpec)->TaskTree);
#endif

    /* Should we remember the time of occurence of this task switch */
    if(pmSetSwitchTime == TRUE)
      DBEventTime(pmDB, &lEvent, &(pTask->ReportedSwitchTime));

    /* Return the task corresponding to this TID */
    return pTask;
    }
  
  return NULL;
}

/******************************************************************
 * Function :
 *    RTAIDBGetCurrentState()
 * Description :
 *    Determines the current system state given the current position
 *    in the trace (marked by the current event).
 * Parameters :
 *    pmDB, Event database to which this event belongs.
 *    pmEvent, Pointer to said event.
 *    pmSystem, The system description to which this event belongs.
 * Return values :
 *    System state __BEFORE__ event's occurence.
 * History :
 *    K.Y., 29/06/2000, Initial typing.
 * Note :
 *    This is much more expensive to use than RTAIDBEventState. This
 *    should only be used to get the state of the system for the
 *    first event to be drawn.
 ******************************************************************/
RTAIsystemState RTAIDBGetCurrentState(db* pmEventDB, event* pmEvent, systemInfo* pmSystem)
{
  void*              pEventStruct;           /* The event's structure */
  void*              pPrevEventStruct;       /* The previous event's structure */
  event              lPrevEvent = *pmEvent;  /* The previous event */
  event              lCurrEvent;             /* The current event */
  uint8_t            lEventID;               /* Event ID */
  uint8_t            lPrevEventID;           /* Previous event's ID */
  process*           pProcess;               /* Generic process */
  RTAItask*          pTask;                  /* Generic RTAI task */
  RTAIsystemState    lSysState = NO_STATE;   /* Resulting system state */

  /* Get the event's ID */
  lEventID = DBEventID(pmEventDB, pmEvent);

  /* Get the event's structure */
  pEventStruct = DBEventStruct(pmEventDB, pmEvent);

  /* Loop until we find a significant state transition */
  do
    {
    /* Get the previous event */
    if(DBEventPrev(pmEventDB, &lPrevEvent) == FALSE)
      /* If we are at the begining of the trace, then we're in RTAI's core */
      lSysState = LINUX_KERNEL;

    /* Get the previous event's ID */
    lPrevEventID = DBEventID(pmEventDB, &lPrevEvent);

    /* Get the previous event's structure */
    pPrevEventStruct = DBEventStruct(pmEventDB, &lPrevEvent);

#if 0
    /* Was the last event a possible exit */
    if((lPrevEventID == TRACE_SYSCALL_EXIT)
     ||(lPrevEventID == TRACE_TRAP_EXIT)
     ||(lPrevEventID == TRACE_IRQ_EXIT)
     ||((lPrevEventID == TRACE_SCHEDCHANGE)
      &&(RFT32(pmEventDB, SCHED_EVENT(pPrevEventStruct)->in) != 0))
     ||(lPrevEventID == TRACE_KERNEL_TIMER)
     ||(lPrevEventID == TRACE_BOTTOM_HALF)
     ||(lPrevEventID == TRACE_PROCESS)
     ||(lPrevEventID == TRACE_NETWORK)
     ||(lPrevEventID == TRACE_RTAI_GLOBAL_IRQ_EXIT)
     ||(lPrevEventID == TRACE_RTAI_OWN_IRQ_EXIT))
      {
      /* Is the current event an entry into the kernel or RTAI through LXRT */
      if((lEventID == TRACE_SYSCALL_ENTRY)
       ||(lEventID == TRACE_TRAP_ENTRY)
       ||((lEventID == TRACE_IRQ_ENTRY)
        &&(RFT8(pmEventDB, IRQ_EVENT(pEventStruct))->kernel != 1))
       ||((lEventID == TRACE_RTAI_LXRT)
	&&(RFT8(pmEventDB, RTAI_LXRT_EVENT(pEventStruct)->event_sub_id) == TRACE_RTAI_LXRT_RTAI_SYSCALL_ENTRY)))
	lSysState = LINUX_PROCESS;
      }
#endif
    /* Was the last event a definitive entry into Linux */
    if((lPrevEventID == TRACE_IRQ_ENTRY)
     ||(lPrevEventID == TRACE_TRAP_ENTRY)
     ||(lPrevEventID == TRACE_SYSCALL_ENTRY))
      lSysState = LINUX_KERNEL;
    /* Was the last event a switch to an RTAI task */
    else if(lPrevEventID == TRACE_RTAI_SCHED_CHANGE)
      {
      /* Is the Linux kernel the incoming task */
      if(RFT32(pmEventDB, RTAI_SCHED_CHANGE_EVENT(pPrevEventStruct)->in) == 0)
	/* We're going into the linux kernel */
	lSysState = LINUX_KERNEL;
      else
	/* We're going into an RT task */
	lSysState = RTAI_TASK;
      }
    /* Was the last event a definitive entry into RTAI */
    else if((lPrevEventID == TRACE_RTAI_GLOBAL_IRQ_ENTRY)
          ||(lPrevEventID == TRACE_RTAI_OWN_IRQ_ENTRY))
      lSysState = RTAI_CORE;
    /* Was the last event a possible exit from something and the next and entry into RTAI through LXRT */
    else if((lEventID == TRACE_SYSCALL_ENTRY)
#if 0 /* On PPC traps don't have the same meaning as on the i386 */
          ||((lEventID == TRACE_TRAP_ENTRY)
#endif
	  ||((lEventID == TRACE_IRQ_ENTRY)
	   &&(RFT8(pmEventDB, IRQ_EVENT(pEventStruct)->kernel) != 1)))
      lSysState = LINUX_PROCESS;
    else if((lEventID == TRACE_RTAI_LXRT)
	   &&(RFT8(pmDB, RTAI_LXRT_EVENT(pEventStruct)->event_sub_id) == TRACE_RTAI_LXRT_RTAI_SYSCALL_ENTRY))
      {
      /* Get the currently running process and currently running task */
      pTask    = RTAIDBEventTask(pmEventDB, &lPrevEvent, pmSystem, FALSE);
      pProcess = DBEventProcess(pmEventDB, &lPrevEvent, pmSystem, FALSE);

      /* Is the currently running task a buddy of the currently running process */
      if((pProcess       != NULL)
       &&(pProcess->Hook != NULL))
	lSysState = LINUX_PROCESS;
      }

    /* Update the current event */
    lEventID     = lPrevEventID;
    pEventStruct = pPrevEventStruct;
    }
  while(lSysState == NO_STATE);

  /* Start at the last current event */
  lCurrEvent = lPrevEvent;
  DBEventNext(pmEventDB, &lCurrEvent);
  
  /* Forward until we get to the event we started at */
  while(!DBEventsEqual(lCurrEvent, (*pmEvent)))
    {
    /* Get the currently running process and currently running task */
    /* N.B.: These are expensive calls to make in a loop but we can afford doing them
             since we don't plan to loop around often */
    pTask    = RTAIDBEventTask(pmEventDB, &lCurrEvent, pmSystem, FALSE);
    pProcess = DBEventProcess(pmEventDB, &lCurrEvent, pmSystem, FALSE);

    /* Get the current state */
    lSysState = RTAIDBEventState(pmEventDB,
				 &lCurrEvent,
				 pProcess,
				 lSysState,
				 pTask);

    /* Move on forward */
    DBEventNext(pmEventDB, &lCurrEvent);
    }

  /* Return the state found to the caller */
  return lSysState;
}

/******************************************************************
 * Function :
 *    RTAIDBEventState()
 * Description :
 *    Determines wether the passed event has generated a change
 *    in the system's sate.
 * Parameters :
 *    pmDB, Event database to which this event belongs.
 *    pmEvent, Pointer to said event.
 *    pmProcess, The currently running process
 *    pmCurSysState, Current state before event's occurence.
 *    pmTask, The currently running task
 * Return values :
 *    System state __AFTER__ event's occurence.
 * History :
 *    K.Y., 24/06/2000, Initial typing.
 ******************************************************************/
RTAIsystemState RTAIDBEventState(db*             pmDB,
				 event*          pmEvent,
				 process*        pmProcess,
				 RTAIsystemState pmCurSysState,
				 RTAItask*       pmTask)
{
  void*              pEventStruct;           /* The event's structure */
  void*              pPrevEventStruct;       /* The previous event's structure */
  void*              pNextEventStruct;       /* The next event's structure */
  event              lPrevEvent = *pmEvent;  /* The previous event */
  event              lNextEvent = *pmEvent;  /* The next event */
  uint8_t            lEventID;               /* Event ID */
  uint8_t            lPrevEventID;           /* Previous event's ID */
  uint8_t            lNextEventID;           /* Next event's ID */
  RTAIsystemState    lSysState;              /* Resulting system state */

  /* By default we remain in the current state */
  lSysState = pmCurSysState;

  /* Get the event's ID */
  lEventID = DBEventID(pmDB, pmEvent);

  /* Get the event's structure */
  pEventStruct = DBEventStruct(pmDB, pmEvent);

  /* Get the previous event */
  if(DBEventPrev(pmDB, &lPrevEvent) == FALSE)
    return lSysState;

  /* Get the previous event's ID */
  lPrevEventID = DBEventID(pmDB, &lPrevEvent);

  /* Get the previous event's structure */
  pPrevEventStruct = DBEventStruct(pmDB, &lPrevEvent);

  /* Get the next event */
  if(DBEventNext(pmDB, &lNextEvent) == FALSE)
    return lSysState;

  /* Get the next event's ID */
  lNextEventID = DBEventID(pmDB, &lNextEvent);

  /* Get the next event's structure */
  pNextEventStruct = DBEventStruct(pmDB, &lNextEvent);

  /* Depending on the current state */
  switch(pmCurSysState)
    {
    /* We are in RTAI */
    case RTAI_CORE:
      /* Depending on the current event */
      switch(lEventID)
	{
	/* Scheduling change */
	case TRACE_RTAI_SCHED_CHANGE:
	  /* Is the Linux kernel the incoming task */
	  if(RFT32(pmDB, RTAI_SCHED_CHANGE_EVENT(pEventStruct)->in)  == 0)
	    /* We're going into the linux kernel */
	    lSysState = LINUX_KERNEL;
	  else
	    /* We're going into an RT task */
	    lSysState = RTAI_TASK;
	  break;

	/* Linux IRQ entry */
	case TRACE_IRQ_ENTRY:
#if 0
	case TRACE_SCHEDCHANGE:
#endif
	  lSysState = LINUX_KERNEL;
	  break;

	/* Linux scheduling change */
	case TRACE_SCHEDCHANGE:

	/* An exit from RTAI's handlers */
	case TRACE_RTAI_GLOBAL_IRQ_EXIT:
	case TRACE_RTAI_OWN_IRQ_EXIT:
	case TRACE_RTAI_TRAP_EXIT:
	case TRACE_RTAI_SRQ_EXIT:
	  /* Is the next event an entry */
	  switch(lNextEventID)
	    {
	    /* A definite entry */
	    case TRACE_IRQ_ENTRY:
	    case TRACE_TRAP_ENTRY:
	    case TRACE_SYSCALL_ENTRY:
	      /* Is this an IRQ entry that happened inside the kernel */
	      if(((lNextEventID == TRACE_IRQ_ENTRY)
                  &&(RFT8(pmDB, IRQ_EVENT(pNextEventStruct)->kernel) == 1))
	        ||((lNextEventID == TRACE_RTAI_GLOBAL_IRQ_ENTRY)
		  &&(RFT8(pmDB, RTAI_GLOBAL_IRQ_ENTRY_EVENT(pNextEventStruct)->kernel) == 1))
		||((lNextEventID == TRACE_RTAI_OWN_IRQ_ENTRY)
		  &&(RFT8(pmDB, RTAI_OWN_IRQ_ENTRY_EVENT(pNextEventStruct)->kernel) == 1))
		||((lNextEventID == TRACE_TRAP_ENTRY)
                  &&(pmProcess->PID == 0))
		||((lEventID == TRACE_SCHEDCHANGE)
		   &&(RFT32(pmDB, SCHED_EVENT(pEventStruct)->in) == 0)))
		/* We're going in the Linux kernel */
		lSysState = LINUX_KERNEL;
	      else
		/* We are going in a Linux process */
		lSysState = LINUX_PROCESS;
	      break;

	    /* A certain entry into RTAI */
	    case TRACE_RTAI_GLOBAL_IRQ_ENTRY:
	    case TRACE_RTAI_OWN_IRQ_ENTRY:
	      lSysState = RTAI_CORE;
	      break;

	    /* An SRQ entry into RTAI */
	    case TRACE_RTAI_SRQ_ENTRY:
	      if(RFT8(pmDB, RTAI_SRQ_ENTRY_EVENT(pEventStruct)->kernel) == 1)
		lSysState = RTAI_TASK;
	      else
		lSysState = LINUX_PROCESS;
	      break;
	      

	    /* A possible entry into RTAI through LXRT */
	    case TRACE_RTAI_LXRT:
	      /* Is this an entry into LXRT */
	      if(RFT8(pmDB, RTAI_LXRT_EVENT(pNextEventStruct)->event_sub_id) == TRACE_RTAI_LXRT_RTAI_SYSCALL_ENTRY)
		{
		/* Are we going into a linux process */
		if((pmTask != NULL)
                 &&((pmTask->TID == 0)
                  ||(pmTask->IsProcessBuddy == TRUE)
		  ||(pmTask->IsProcessShadow == TRUE)))
		  lSysState = LINUX_PROCESS;
		else
		  lSysState = RTAI_TASK;
		}
	      break;

	    /* Anything else */
	    default:
	      if(lNextEventID <= TRACE_MAX)
		lSysState = LINUX_KERNEL;
	      break;
	    } /* Switch next event ID */
	  

	/* A possible exit from an LXRT call */
	case TRACE_RTAI_LXRT:
	  /* Is this event an exit from an LXRT system call */
	  if(RFT8(pmDB, RTAI_LXRT_EVENT(pEventStruct)->event_sub_id) == TRACE_RTAI_LXRT_RTAI_SYSCALL_EXIT)
	    {
	    /* Is the next event an entry */
	    switch(lNextEventID)
	      {
	      /* A definite entry */
	      case TRACE_TRAP_ENTRY:
	      case TRACE_SYSCALL_ENTRY:
		/* We are going in a Linux process */
		lSysState = LINUX_PROCESS;
		break;

	      /* A possible entry into RTAI through LXRT */
	      case TRACE_RTAI_LXRT:
		/* Is this an entry into LXRT */
		if(RFT8(pmDB, RTAI_LXRT_EVENT(pNextEventStruct)->event_sub_id) == TRACE_RTAI_LXRT_RTAI_SYSCALL_ENTRY)
		  {
		  /* Is this currently running task a linux process buddy */
		  if((pmTask                  != NULL)
	           &&(pmTask->TID             != 0)
	           &&(pmTask->IsProcessBuddy  == FALSE)
                   &&(pmTask->IsProcessShadow == FALSE))
		    lSysState = RTAI_TASK;
		  else
		    lSysState = LINUX_PROCESS;
		  }
		break;
	      } /* Switch next event ID */
	    }
	  break;
	}
      break;

    /* We are in an RTAI task */
    case RTAI_TASK:
      /* Depending on the current event */
      switch(lEventID)
	{
	/* Scheduling change */
	case TRACE_RTAI_SCHED_CHANGE:
	  /* Is the Linux kernel the incoming task */
	  if(RFT32(pmDB, RTAI_SCHED_CHANGE_EVENT(pEventStruct)->in) == 0)
	    /* We're going into the linux kernel */
	    lSysState = LINUX_KERNEL;
	  else
	    /* We're going into an RT task */
	    lSysState = RTAI_TASK;
	  break;

	/* IRQ entry */
	case TRACE_RTAI_GLOBAL_IRQ_ENTRY:
	case TRACE_RTAI_OWN_IRQ_ENTRY:
	case TRACE_RTAI_SRQ_ENTRY:
	  lSysState = RTAI_CORE;
	  break;

	/* Linux IRQ entry */
	case TRACE_IRQ_ENTRY:
	  lSysState = LINUX_KERNEL;
	  break;

	/* A possible entry into an LXRT call */
	case TRACE_RTAI_LXRT:
	  /* Is this event an entry to an LXRT system call */
	  if(RFT8(pmDB, RTAI_LXRT_EVENT(pEventStruct)->event_sub_id) == TRACE_RTAI_LXRT_RTAI_SYSCALL_ENTRY)
	    lSysState = RTAI_CORE;
	  break;
	}
      break;
    
    /* We are in the Linux kernel */
    case LINUX_KERNEL:
      /* Depending on the current event */
      switch(lEventID)
	{
	/* Scheduling change */
	case TRACE_RTAI_SCHED_CHANGE:
	  /* Is the Linux kernel the incoming task */
	  if(RFT32(pmDB, RTAI_SCHED_CHANGE_EVENT(pEventStruct)->in) == 0)
	    /* We're going into the linux kernel */
	    lSysState = LINUX_KERNEL;
	  else
	    /* We're going into an RT task */
	    lSysState = RTAI_TASK;
	  break;

	/* IRQ entry */
	case TRACE_RTAI_GLOBAL_IRQ_ENTRY:
	case TRACE_RTAI_OWN_IRQ_ENTRY:
	  lSysState = RTAI_CORE;
	  break;

	/* A possible exit */
#if 0
        case TRACE_SYSCALL_EXIT:
	case TRACE_TRAP_EXIT:
	case TRACE_IRQ_EXIT:
	case TRACE_SCHEDCHANGE:
	case TRACE_KERNEL_TIMER:
	case TRACE_BOTTOM_HALF:
	case TRACE_PROCESS:
	case TRACE_NETWORK:
	case TRACE_RTAI_GLOBAL_IRQ_EXIT:
	case TRACE_RTAI_OWN_IRQ_EXIT:
#else
	default:
#endif
	  /* Is this a scheduling change with the idle task as incoming task */
	  if((lEventID == TRACE_SCHEDCHANGE)
           &&(RFT32(pmDB, SCHED_EVENT(pEventStruct)->in) == 0))
	    /* This event doesn't mark an exit */
	    break;

	  /* Is the next event an entry */
	  switch(lNextEventID)
	    {
	    /* A definite entry */
	    case TRACE_IRQ_ENTRY:
	    case TRACE_TRAP_ENTRY:
	    case TRACE_SYSCALL_ENTRY:
	    case TRACE_RTAI_GLOBAL_IRQ_ENTRY:
	    case TRACE_RTAI_OWN_IRQ_ENTRY:
	    case TRACE_RTAI_SRQ_ENTRY:
	      /* Is this an IRQ entry that happened inside the kernel */
	      if(((lNextEventID == TRACE_IRQ_ENTRY)
                  &&(RFT8(pmDB, IRQ_EVENT(pNextEventStruct)->kernel) == 1))
	        ||((lNextEventID == TRACE_RTAI_GLOBAL_IRQ_ENTRY)
		  &&(RFT8(pmDB, RTAI_GLOBAL_IRQ_ENTRY_EVENT(pNextEventStruct)->kernel) == 1))
		||((lNextEventID == TRACE_RTAI_OWN_IRQ_ENTRY)
		  &&(RFT8(pmDB, RTAI_OWN_IRQ_ENTRY_EVENT(pNextEventStruct)->kernel) == 1))
		||((lNextEventID == TRACE_TRAP_ENTRY)
                  &&(pmProcess->PID == 0)))
		/* We're still in the Linux kernel */
		lSysState = LINUX_KERNEL;
	      else
		/* We are going in a Linux process */
		lSysState = LINUX_PROCESS;
	      break;

	    /* A possible entry into RTAI through LXRT */
	    case TRACE_RTAI_LXRT:
	      /* Is this an entry into LXRT */
	      if(RFT8(pmDB, RTAI_LXRT_EVENT(pNextEventStruct)->event_sub_id) == TRACE_RTAI_LXRT_RTAI_SYSCALL_ENTRY)
		lSysState = LINUX_PROCESS;
	      break;
	    } /* Switch next event ID */
	  break;
	} /* Switch on event ID */
      break;

    /* We are in a Linux process */
    case LINUX_PROCESS:
      /* Depending on the current event */
      switch(lEventID)
	{
	/* Entering Linux kernel */
	case TRACE_IRQ_ENTRY:
	case TRACE_TRAP_ENTRY:
	case TRACE_SYSCALL_ENTRY:
	  lSysState = LINUX_KERNEL;
	  break;

	/* Entering RTAI */
	case TRACE_RTAI_GLOBAL_IRQ_ENTRY:
	case TRACE_RTAI_OWN_IRQ_ENTRY:
	case TRACE_RTAI_SRQ_ENTRY:
	  lSysState = RTAI_CORE;
	  break;

	/* A possible entry into RTAI through LXRT */
	case TRACE_RTAI_LXRT:
	  /* Is this an entry into LXRT */
	  if(RFT8(pmDB, RTAI_LXRT_EVENT(pEventStruct)->event_sub_id) == TRACE_RTAI_LXRT_RTAI_SYSCALL_ENTRY)
	    lSysState = RTAI_CORE;
	  break;
	} /* Switch on event ID */
      break;

    default:
      break;
    } /* Switch on current state */

#if 0 /* For debug only */
  printf("Current state: %d; Event: %s; Next Event: %s Result: %d \n",
	 pmCurSysState,
	 EVENT_STR(lEventID),
	 EVENT_STR(lNextEventID),
	 lSysState);
#endif

  /* Return the resuling system state */
  return lSysState;
}

/******************************************************************
 * Function :
 *    RTAIDBFindTaskInTree()
 * Description :
 *    Finds a task with given TID in a task tree.
 * Parameters :
 *    pmTID, TID of task to be found.
 *    pmTree, Tree to be searched.
 * Return values :
 *    Valid pointer to task, if task found.
 *    NULL, otherwise
 * History :
 *    K.Y., 14/06/2000, This is largely inspired by DBFindProcInTree
 * Note :
 *    This function is recursive.
 ******************************************************************/
RTAItask* RTAIDBFindTaskInTree(int pmTID, RTAItask* pmTree)
{
  RTAItask* pTaskJ;    /* Generic task pointer */
  RTAItask* pTask;     /* Task found */

  /* For all the children in the same level */
  for(pTaskJ = pmTree; pTaskJ != NULL; pTaskJ = pTaskJ->NextChild)
    {
    /* Is this the task we're looking for */
    if(pTaskJ->TID == pmTID)
      return pTaskJ;

    /* Does this task have children of his own  */
    if(pTaskJ->Children != NULL)
      /* Give this part of the tree to a recursive copy of this function */
      if((pTask = RTAIDBFindTaskInTree(pmTID, pTaskJ->Children)) != NULL)
	return pTask;
    }

  /* We didn't find anything */
  return NULL;
}

/******************************************************************
 * Function :
 *    RTAIDBBuildTaskTree()
 * Description :
 *    Builds the task tree for a RTAI system description.
 * Parameters :
 *    pmSystem, The system who's task tree has to be built.
 * Return values :
 *    NONE
 * History :
 *    K.Y., 14/06/2000, This is largely inspired by DBBuildProcTree.
 * Note :
 ******************************************************************/
void RTAIDBBuildTaskTree(systemInfo* pmSystem)
{
  RTAItask*    pTaskI;         /* Generic task pointer */
  RTAItask*    pTaskJ;         /* Generic task pointer */
  RTAItask*    pPrevTask;      /* Previous task in task list */
  RTAItask*    pTask;          /* Task found in task tree */
  RTAItask*    pTaskTemp;      /* Temporary task pointer */
  RTAIsystem*  pRTAISystem;    /* RTAI system description */

  /* Get the RTAI system description */
  pRTAISystem = RTAI_SYSTEM(pmSystem->SystemSpec);

  /* Go through task list */
  for(pTaskI = pRTAISystem->Tasks; pTaskI != NULL; pTaskI = pTaskI->Next)
    {
    /* Is this the first task */
    if(pRTAISystem->TaskTree == NULL)
      {
      /* Set the first element in tree */
      pRTAISystem->TaskTree = pTaskI;
      
      /* Go on to the rest of the tasks */
      continue;
      }

    /* Is this task parent any of the topmost tasks */
    pPrevTask = NULL;
    for(pTaskJ = pRTAISystem->TaskTree; pTaskJ != NULL;)
      {
      /* Is this a child of the current task */
      if(pTaskI->TID == pTaskJ->PTID)
	{
	/* Remember next element */
	pTaskTemp = pTaskJ->NextChild;

	/* Remove this child from the topmost list */
	if(pPrevTask == NULL)
	  pRTAISystem->TaskTree = pTaskJ->NextChild;
	else
	  pPrevTask->NextChild = pTaskJ->NextChild;

	/* Insert this in front of this parent's children list */
	pTaskJ->NextChild = pTaskI->Children;
	pTaskI->Children  = pTaskJ;

	/* Update task pointer */
	pTaskJ = pTaskTemp;
	}
      else
	{
	/* Remember current position and update to next position */
	pPrevTask = pTaskJ;
	pTaskJ = pTaskJ->NextChild;
	}
      }

    /* Is the parent of this task already in the tree */
    if((pTask = RTAIDBFindTaskInTree(pTaskI->PTID, pRTAISystem->TaskTree)) != NULL)
      {
      /* Place the new child in front of the children list */
      pTaskI->NextChild  = pTask->Children;
      pTask->Children = pTaskI;
      }
    else
      {
      /* Place this task as the first child at the topmost level */
      pTaskI->NextChild  = pRTAISystem->TaskTree;
      pRTAISystem->TaskTree = pTaskI;
      }
    }
}

/******************************************************************
 * Function :
 *    RTAIDBProcessTrace()
 * Description :
 *    Analyze the RTAI event database, cumulate statistics and build
 *    system description accordingly.
 * Parameters :
 *    pmSystem, Description of the system to be filled
 *    pmTraceDB, The event database to be processed.
 *    pmOptions, The user given options.
 * Return values :
 *    0, If everything went as planned
 *    Error code otherwise
 * History :
 *    K.Y., 06/06/2000. This is largely inspired by what
 *                      DBProcessTrace() used to be.
 * Note :
 ******************************************************************/
int RTAIDBProcessTrace(systemInfo* pmSystem, db* pmTraceDB, options* pmOptions)
{
  int               lOPID;                      /* Outgoing process's PID */
  int               lPID = -1;                  /* Process ID */
  int               lOTID;                      /* Outgoing task's TID */
  int               lTID = -1;                  /* Task ID */
  int               lEventID;                   /* The ID of the event */
  int               lNextEventID;               /* The ID of the next event */
  int               lShadowPending = FALSE;     /* A shadow is pending creation */
  int               lFirstSchedChange = TRUE;   /* There hasn't been any scheduling changes yet */
  int               lHashPosition;              /* Position of event type in hash table */
#if 0
  char              lFirstTime[TIME_STR_LEN];   /* Time string */
  char              lSecondTime[TIME_STR_LEN];  /* Time string */
#endif
  void*             pEventStruct;               /* The event's description structure */
  void*             pNextEventStruct;           /* The next event's description structure */
  void*             pTempStruct;                /* The event's description structure */
  event             lEvent;                     /* Generic event */
  event             lNextEvent;                 /* Next event */
  event             lLastCtrlEvent;             /* Last system-wide control event */
  syscallInfo*      pSyscall;                   /* System call pointer */
  syscallInfo*      pPrevSyscall;               /* Previous system call in syscall list */
  syscallInfo*      pSyscallP;                  /* Processed: System call pointer */
  process*          pProcess = NULL;            /* Current process */
  RTAItask*         pTask = NULL;               /* RTAI task */
  RTAItask*         pNewTask;                   /* Newly created RTAI task */
  RTAIsystem*       pRTAISystem;                /* RTAI-specific system info. */
  struct timeval    lTempTime;                  /* Temporary time variable */
  struct timeval    lTime;                      /* Local time variable */
  struct timeval    lEventTime;                 /* Time at which the event occured */
  struct timeval    lLastCtrlEventTime;         /* Time of last control event */
  customEventDesc*  pCustom;                    /* Custom event */

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

  /* Allocate RTAI-specific system description */
  pmSystem->SystemSpec   = (void*) malloc(sizeof(RTAIsystem));

  /* Initialize RTAI-specific system description */
  pRTAISystem = RTAI_SYSTEM(pmSystem->SystemSpec);
  pRTAISystem->Tasks           = NULL;
  pRTAISystem->TaskTree        = NULL;
  pRTAISystem->NbRTEvents      = 0;
  pRTAISystem->Mount           = 0;
  pRTAISystem->UMount          = 0;
  pRTAISystem->GlobalIRQEntry  = 0;
  pRTAISystem->GlobalIRQExit   = 0;
  pRTAISystem->CPUOwnIRQEntry  = 0;
  pRTAISystem->CPUOwnIRQExit   = 0;
  pRTAISystem->TrapEntry       = 0;
  pRTAISystem->TrapExit        = 0;
  pRTAISystem->SRQEntry        = 0;
  pRTAISystem->SRQExit         = 0;
  pRTAISystem->SwitchToLinux   = 0;
  pRTAISystem->SwitchToRT      = 0;
  pRTAISystem->SchedChange     = 0;
  pRTAISystem->LXRTSchedChange = 0;

  /* Initialize last control event */
  lLastCtrlEvent.BufID    = 0;
  lLastCtrlEvent.EventPos = 0;

  /* Always insert task ID 0 into task list */
  pTask = RTAIDBCreateTask(pmSystem, 0, -1);

  /* Go through the trace database and fill system description accordingly */
  lEvent = pmTraceDB->FirstEvent;
  do
    {
    /* Increment the number of events */
    pmTraceDB->Nb++;

    /* Get the event's ID */
    lEventID   = DBEventID(pmTraceDB, &lEvent);

    /* Is this trace empty */
    if(lEventID == TRACE_BUFFER_END)
      return ERR_EMPTY_TRACE;

    /* Is this one more RT event */
    if(lEventID > TRACE_MAX)
      pRTAISystem->NbRTEvents++;

    /* TEMPORARY MEASURE UNTIL RTAI IS FIXED */
    /* Remember the last event's time of occurence */
    lTempTime = lEventTime;

    /* Get the event's time */
    DBEventTime(pmTraceDB, &lEvent, &lEventTime);

    /* TEMPORARY MEASURE UNTIL RTAI IS FIXED */
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

    /* Forget what the trace daemon does right after opening the trace device */
    if((lPID == -1)
     &&(lEventID >= TRACE_START)  && (lEventID <= TRACE_MAX)
     &&(lEventID != TRACE_SCHEDCHANGE)
     &&(lEventID != TRACE_NEW_EVENT))
      continue;

    /* Don't deal with RTAI events until a scheduling change */
    if((lTID == -1)
     &&(lEventID >= TRACE_MAX) && (lEventID <= TRACE_RTAI_MAX)
     &&(lEventID != TRACE_RTAI_SCHED_CHANGE))
      continue;

    /* Are the details of the event present */
    if(!ltt_test_bit(lEventID, &(pmTraceDB->DetailsMask)))
      continue;

    /* DEBUG */
#if 0
    printf("%d ", lEventID);
#endif

    /* Depending on the type of event */
    switch(lEventID)
      {
      /* Entry in a given system call */
      case TRACE_SYSCALL_ENTRY :
	/* Increment number of system call entries */
	pProcess->NbSyscalls++;
	pmSystem->SyscallEntry++;

	/* Set system call depth */
	pProcess->Depth++;

	/* Create new pending system call */
	if((pSyscall = (syscallInfo*) malloc(sizeof(syscallInfo))) == NULL)
	  {
	  printf("Memory allocation problem \n");
	  exit(1);
	  }

	/* Set new system call */
	pSyscall->ID    = RFT8(pmTraceDB, SYSCALL_EVENT(pEventStruct)->syscall_id);
	pSyscall->Nb    = 0;
	pSyscall->Depth = pProcess->Depth;
	memcpy(&(pSyscall->Time), &lEventTime, sizeof(struct timeval));

	/* Link this syscall to the others pending */
	pSyscall->Next = pProcess->Pending;
	pProcess->Pending = pSyscall;
	break;
      
      /* Exit from a given system call */
      case TRACE_SYSCALL_EXIT :
	/* Increment number of system call exits */
	pmSystem->SyscallExit++;

	/* Find the system call in the list of pending system calls */
	pPrevSyscall = pProcess->Pending;
	for(pSyscall = pProcess->Pending; pSyscall != NULL; pSyscall = pSyscall->Next)
	  {
	  if(pSyscall->Depth == pProcess->Depth)
	    break;
	  pPrevSyscall = pSyscall;
	  }

	/* The first syscall_exit probably doesn't include an entry */
	if(pSyscall == NULL)
	  goto IsEventControl;

	/* Remove syscall from pending syscalls */
	pPrevSyscall->Next = pSyscall->Next;

	/* Find syscall in already processed syscalls list */
	for(pSyscallP = pProcess->Syscalls; pSyscallP != NULL; pSyscallP = pSyscallP->Next)
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
	  pSyscallP->Next = pProcess->Syscalls;
	  pProcess->Syscalls = pSyscallP;
	  }

	/* Increment number of times syscall was called */
	pSyscallP->Nb++;

	/* Add time spent in this syscall */
	DBTimeSub(lTime, lEventTime, pSyscall->Time);
	DBTimeAdd(pSyscallP->Time, pSyscallP->Time, lTime);

	/* Decrement syscall depth */
	pProcess->Depth--;
	if(pSyscall == pProcess->Pending) pProcess->Pending = NULL;
	free(pSyscall);
	pSyscall = NULL;
	break;

      /* Entry into a trap */
      case TRACE_TRAP_ENTRY :
	pProcess->NbTraps++;
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
	lPID  = RFT32(pmTraceDB, SCHED_EVENT(pEventStruct)->in);

	/* Was the currently runing process ever scheduled in */
	if((pProcess != NULL)
	 &&(pProcess->LastSchedIn.EventPos != 0))
	  {
          /* Get the time of occurence of the last scheduling change */
	  DBEventTime(pmTraceDB, &(pProcess->LastSchedIn), &lTime);
	    
	  /* During this period we were runing */
	  DBTimeSub(lTime, lEventTime, lTime);
	  DBTimeAdd(pProcess->TimeRuning, pProcess->TimeRuning, lTime);
	  }

	/* Find the process in the list of already known processes */
	for(pProcess = pmSystem->Processes; pProcess != NULL; pProcess = pProcess->Next)
	  if(pProcess->PID == lPID)
	    break;
	
	/* Did we find anything */
	if(pProcess == NULL)
	  /* Create a new process */
	  pProcess = DBCreateProcess(pmSystem, lPID, -1);

	/* We just got scheduled in */
	pProcess->LastSchedIn = lEvent;

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

      /* Going to run a soft IRQ */
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
	lPID = RFT32(pmTraceDB, PROC_EVENT(pEventStruct)->event_data1);

	/* Create a new process */
	DBCreateProcess(pmSystem, lPID, pProcess->PID);
	break;

      /* Hit key part of file system */
      case TRACE_FILE_SYSTEM :
	/* Depending of the event subtype */
	switch(RFT8(pmTraceDB, FS_EVENT(pEventStruct)->event_sub_id))
	  {
	  /* TRACE_FILE_SYSTEM_BUF_WAIT_START */
	  case TRACE_FILE_SYSTEM_BUF_WAIT_START :
	    /* If we hadn't already started an I/O wait, then we're starting a new one now */
	    if((pProcess->LastIOEvent.BufID    == 0)
	     &&(pProcess->LastIOEvent.EventPos == 0))
	      pProcess->LastIOEvent = lEvent;
	    else
	      {
	      /* Get the event's structure */
	      pTempStruct = DBEventStruct(pmTraceDB, &(pProcess->LastIOEvent));

	      /* Was the last event something else than a wait start */
	      if(RFT8(pmTraceDB, FS_EVENT(pTempStruct)->event_sub_id) != TRACE_FILE_SYSTEM_BUF_WAIT_START)
		pProcess->LastIOEvent = lEvent;
	      }
	    break;

	  /* TRACE_FILE_SYSTEM_BUF_WAIT_END */
	  case TRACE_FILE_SYSTEM_BUF_WAIT_END :
 	    /* Does the last IO event exist */
	    if(/*(pProcess->LastIOEvent.BufID    != 0)
		 &&*/(pProcess->LastIOEvent.EventPos != 0))
	      {
	      /* Get the event's structure */
	      pTempStruct = DBEventStruct(pmTraceDB, &(pProcess->LastIOEvent));

	      /* Was the last event a wait start */
	      if(RFT8(pmTraceDB, FS_EVENT(pTempStruct)->event_sub_id) == TRACE_FILE_SYSTEM_BUF_WAIT_START)
		{
		/* Get the event's time */
		DBEventTime(pmTraceDB, &(pProcess->LastIOEvent), &lTime);

		/* Add the time to time spent waiting for I/O */
		DBTimeSub(lTime, lEventTime, lTime);
		DBTimeAdd(pProcess->TimeIO, pProcess->TimeIO, lTime);

		/* This is the next LastIOEvent */
		pProcess->LastIOEvent = lEvent;
		}
	      }
	    break;

	  /* TRACE_FILE_SYSTEM_EXEC */
	  case TRACE_FILE_SYSTEM_EXEC :
	    /* Retain only the first exec */
	    if(pProcess->Command != NULL)
	      continue;

	    /* Allocate space for command and copy it */
	    pProcess->Command = (char*) malloc(strlen(FS_EVENT_FILENAME(pEventStruct)) + 1);
	    strcpy(pProcess->Command, FS_EVENT_FILENAME(pEventStruct));
	    break;

          /* TRACE_FILE_SYSTEM_OPEN */
	  case TRACE_FILE_SYSTEM_OPEN :
	    break;

	  /* TRACE_FILE_SYSTEM_READ */
	  case TRACE_FILE_SYSTEM_READ :
	    pProcess->QFileRead += RFT32(pmTraceDB, FS_EVENT(pEventStruct)->event_data2);
	    break;

	  /* TRACE_FILE_SYSTEM_WRITE */
	  case TRACE_FILE_SYSTEM_WRITE :
	    pProcess->QFileWrite += RFT32(pmTraceDB, FS_EVENT(pEventStruct)->event_data2);
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

      /* RTAI has been mounted */
      case TRACE_RTAI_MOUNT :
	pRTAISystem->Mount++;
	break;

      /* RTAI has been unmounted */
      case TRACE_RTAI_UMOUNT :
	pRTAISystem->UMount++;
	break;

      /* Global IRQ entry */
      case TRACE_RTAI_GLOBAL_IRQ_ENTRY :
	pRTAISystem->GlobalIRQEntry++;
	break;

      /* Global IRQ exit */
      case TRACE_RTAI_GLOBAL_IRQ_EXIT :
	pRTAISystem->GlobalIRQExit++;
	break;

      /* CPU own IRQ entry */
      case TRACE_RTAI_OWN_IRQ_ENTRY :
	pRTAISystem->CPUOwnIRQEntry++;
	break;

      /* CPU own IRQ exit */
      case TRACE_RTAI_OWN_IRQ_EXIT :
	pRTAISystem->CPUOwnIRQExit++;
	break;

      /* Trap entry */
      case TRACE_RTAI_TRAP_ENTRY :
	pRTAISystem->TrapEntry++;
	break;

      /* Trap exit */
      case TRACE_RTAI_TRAP_EXIT :
	pRTAISystem->TrapExit++;
	break;

      /* System ReQuest entry */
      case TRACE_RTAI_SRQ_ENTRY :
	pRTAISystem->SRQEntry++;
	break;

      /* System ReQuest exit */
      case TRACE_RTAI_SRQ_EXIT :
	pRTAISystem->SRQExit++;
	break;

      /* Switch to Linux */
      case TRACE_RTAI_SWITCHTO_LINUX :
	pRTAISystem->SwitchToLinux++;
	break;

      /* Switch to RTAI */
      case TRACE_RTAI_SWITCHTO_RT :
	pRTAISystem->SwitchToRT++;
	break;

      /* Scheduling change */
      case TRACE_RTAI_SCHED_CHANGE :
	/* Which side is the task change */
	if((RFT32(pmTraceDB, RTAI_SCHED_CHANGE_EVENT(pEventStruct)->out_state) == 0)
         &&(RFT32(pmTraceDB, RTAI_SCHED_CHANGE_EVENT(pEventStruct)->in)        == 0))
	  {
	  /* Read PID of incoming task */
	  lOTID = RFT32(pmTraceDB, RTAI_SCHED_CHANGE_EVENT(pEventStruct)->in);
	  lTID  = RFT32(pmTraceDB, RTAI_SCHED_CHANGE_EVENT(pEventStruct)->out);
	  }
	else
	  {
	  /* Read PID of incoming task */
	  lOTID = RFT32(pmTraceDB, RTAI_SCHED_CHANGE_EVENT(pEventStruct)->out);
	  lTID  = RFT32(pmTraceDB, RTAI_SCHED_CHANGE_EVENT(pEventStruct)->in);
	  }

	/* Did we really do a scheduling change */
	if((pTask      != NULL)
	 &&(pTask->TID == lTID))
	  /* We haven't really changed anything, get outa here */
	  break;

	/* Was the currently running task ever scheduled in */
	if((pTask != NULL)
	 &&(pTask->NbTimesScheduled != 0))
	  {
          /* Get the time of occurence of the last scheduling change */
	  DBEventTime(pmTraceDB, &(pTask->LastSchedIn), &lTime);
	    
	  /* During this period we were running */
	  DBTimeSub(lTime, lEventTime, lTime);
	  DBTimeAdd(pTask->TimeRunning, pTask->TimeRunning, lTime);

	  /* We've just been scheduled out */
	  pTask->LastSchedOut = lEvent;
	  }
	
	/* Find the task in the list of already known tasks */
	for(pTask = RTAI_SYSTEM(pmSystem->SystemSpec)->Tasks;
	    pTask != NULL;
	    pTask = pTask->Next)
	  if(pTask->TID == lTID)
	    break;

	/* Did we find anything */
	if(pTask == NULL)
	  /* Create a new task with Linux as parent */
	  pTask = RTAIDBCreateTask(pmSystem, lTID, 0);

	/* Is this NOT the first time we are scheduled in */
	if(pTask->NbTimesScheduled > 0)
	  {
          /* Get the time of occurence of the last scheduling change */
	  DBEventTime(pmTraceDB, &(pTask->LastSchedOut), &lTime);
	    
	  /* During this period we were waiting to run */
	  DBTimeSub(lTime, lEventTime, lTime);
	  DBTimeAdd(pTask->TimeBetweenRuns, pTask->TimeBetweenRuns, lTime);
	  }

	/* We just got scheduled in */
	pTask->LastSchedIn = lEvent;
	pTask->NbTimesScheduled++;

	/* One more scheduling change */
	pRTAISystem->SchedChange++;
	break;

      /* Hit key part of task services */
      case TRACE_RTAI_TASK :
	/* Don't analyze anything but task creating for now */
	if(RFT8(pmTraceDB, RTAI_TASK_EVENT(pEventStruct)->event_sub_id) != TRACE_RTAI_TASK_INIT)
	  continue;

	/* We have created a task, get the child's TID */
	lTID = RFT32(pmTraceDB, RTAI_TASK_EVENT(pEventStruct)->event_data1);

	/* Is a any task currently runing */
	if(pTask != NULL)
	  /* Create a new task with the current task as parent */
	  pNewTask = RTAIDBCreateTask(pmSystem, lTID, pTask->TID);
	else
	  /* Create a new task with Linux as parent */
	  pNewTask = RTAIDBCreateTask(pmSystem, lTID, 0);

	/* This is the creation time of the newly create task */
	pNewTask->TimeOfBirth = lEventTime;

	/* Was there a shadow creation pending */
	if(lShadowPending == TRUE)
	  {
	  /* No more shadow is pending */
	  lShadowPending = FALSE;

	  /* The currently running task is trying to become a real-time task */
	  pNewTask->IsProcessShadow = TRUE;
	  pNewTask->ShadowProcess   = pProcess;

	  /* Hook the current task onto the process structure */
	  pProcess->Hook = (void*) pNewTask;
	  }
	break;

      /* Hit key part of timer services */
      case TRACE_RTAI_TIMER :
	break;

      /* Hit key part of semaphore services */
      case TRACE_RTAI_SEM :
	break;

      /* Hit key part of message services */
      case TRACE_RTAI_MSG :
	break;

      /* Hit key part of RPC services */
      case TRACE_RTAI_RPC :
	break;

      /* Hit key part of mail box services */
      case TRACE_RTAI_MBX :
	break;

      /* Hit key part of FIFO services */
      case TRACE_RTAI_FIFO :
	/* Depending on the event sub-ID */
	switch(RFT8(pmTraceDB, RTAI_FIFO_EVENT(pEventStruct)->event_sub_id))
	  {
	  /* TRACE_RTAI_FIFO_PUT */
	  case TRACE_RTAI_FIFO_PUT:
	    pTask->QFIFOPut += RFT32(pmTraceDB, RTAI_FIFO_EVENT(pEventStruct)->event_data2);
	    break;

	  /* TRACE_RTAI_FIFO_GET */
	  case TRACE_RTAI_FIFO_GET:
	    pTask->QFIFOGet += RFT32(pmTraceDB, RTAI_FIFO_EVENT(pEventStruct)->event_data2);
	    break;
	  }
	break;

      /* Hit key part of shared memory services */
      case TRACE_RTAI_SHM :
	break;

      /* Hit key part of Posix services */
      case TRACE_RTAI_POSIX :
	break;

      /* Hit key part of LXRT services */
      case TRACE_RTAI_LXRT :
	/* Depending on the event sub-ID */
	switch(RFT8(pmTraceDB, RTAI_LXRT_EVENT(pEventStruct)->event_sub_id))
	  {
	  /* TRACE_RTAI_LXRT_RTAI_SYSCALL_ENTRY */
	  case TRACE_RTAI_LXRT_RTAI_SYSCALL_ENTRY:
	    break;

	  /* TRACE_RTAI_LXRT_SCHED_CHANGE */
	  case TRACE_RTAI_LXRT_SCHED_CHANGE:
	    pRTAISystem->LXRTSchedChange++;
	    break;

	  /* TRACE_RTAI_LXRT_HANDLE */
	  case TRACE_RTAI_LXRT_HANDLE:
	    /* Depending on the SRQ */
	    switch(RFT32(pmTraceDB, RTAI_LXRT_EVENT(pEventStruct)->event_data1))
	      {
	      /* TASK_INIT */
	      case 1002:
		/* Get the next event */
		lNextEvent = lEvent;
		if(DBEventNext(pmTraceDB, &lNextEvent) == TRUE)
		  {
		  /* Get the event's ID */
		  lNextEventID = DBEventID(pmTraceDB, &lNextEvent);

		  /* Get the next event's structure */
		  pNextEventStruct = DBEventStruct(pmTraceDB, &lNextEvent);

		  /* Does the next event mark a successfull creation of an RT task */
		  if((lNextEventID == TRACE_RTAI_TASK)
		   &&(RFT8(pmTraceDB, RTAI_TASK_EVENT(pNextEventStruct)->event_sub_id) == TRACE_RTAI_TASK_INIT))
		    lShadowPending = TRUE;
		  }
		break;

	      /* MAKE_HARD_RT */
	      case 1009:
		/* The currently running task calls to be a hard-real-time task */
		pTask->IsProcessBuddy = TRUE;
		pTask->BuddyProcess   = pProcess;		  
		break;
	      }
	    break;
	  }
	break;

      /* Hit key part of LXRT-Informed services */
      case TRACE_RTAI_LXRTI :
	break;

      default :
	printf("Internal error : Unknown event (%d) while processing trace \n", lEventID);
	printf("Processed %ld events before failing \n", pmTraceDB->Nb);
	DBEventPrev(pmTraceDB, &lEvent);
	printf("Previous event is %s \n", EVENT_STR(pmTraceDB, DBEventID(pmTraceDB, &lEvent), &lEvent));
	DBEventPrev(pmTraceDB, &lEvent);
	printf("Previous Previous event is %s \n", EVENT_STR(pmTraceDB, DBEventID(pmTraceDB, &lEvent), &lEvent));
	exit(1);
	break;
      }

    /* We may not have seen a process yet, just RTAI tasks, so we may not want to continue assuming
       we do have a valid process*/
    if(pProcess == NULL)
      continue;

IsEventControl:
    /* Is this an entry control event */
    if(DBEventControlEntry(pmTraceDB, &lEvent, pProcess->PID) == TRUE)
      {
      /* Accumulate the time that passed since the last control event */
      DBTimeSub(lTime, lEventTime, lLastCtrlEventTime);
      DBTimeAdd(pProcess->TimeProcCode, pProcess->TimeProcCode, lTime);
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
    if(DBEventControlExit(pmTraceDB, &lEvent, pProcess->PID) == TRUE)
      {
      lLastCtrlEvent     = lEvent;
      lLastCtrlEventTime = lEventTime;
      }
    } while(DBEventNext(pmTraceDB, &lEvent) == TRUE);

  /* Remember the last event and it's time */
  pmTraceDB->LastEvent = lEvent;
  pmTraceDB->EndTime   = lEventTime;

  /* Sum up syscall time for all the system */
  for(pProcess = pmSystem->Processes; pProcess != NULL; pProcess = pProcess->Next)
    /* Go through his syscall list */
    for(pSyscall = pProcess->Syscalls; pSyscall != NULL; pSyscall = pSyscall->Next)
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

  /* Build task tree */
  RTAIDBBuildTaskTree(pmSystem);

#if 0  /* Debugging only */ 
  do
  {
   int i;
  for(i = 0; i <= pmTraceDB->LastEvent.BufID; i++)
    {
  void*               pReadAddr;     /* Address from which to read */
  struct timeval*     pBufStartTime; /* Time at which this buffer started */
  uint32_t            lTimeDelta;

  /* Get the time of start of the buffer to which the event belongs */
  pReadAddr = i * pmTraceDB->BufferSize + pmTraceDB->TraceStart;

  /* Skip the event ID */
  pReadAddr += sizeof(uint8_t);

  /* Skip the time delta */
  pReadAddr += sizeof(uint32_t);

  /* Get a pointer to the time of begining of this buffer */
  pBufStartTime = &(((trace_buffer_start*) pReadAddr)->Time);

  pReadAddr += sizeof(trace_buffer_start);
  pReadAddr += sizeof(uint8_t);
  
  /* Does this trace contain CPUIDs */
  if(pmTraceDB->LogCPUID == TRUE)
    /* Skip the CPUID */
    pReadAddr += sizeof(uint8_t);

  /* Skip the event ID */
  pReadAddr += sizeof(uint8_t);

  /* Get the time delta */
  lTimeDelta = *(uint32_t*) pReadAddr;

  printf("Buffer %i starts at (%ld, %ld). Delta is %d. \n", i, pBufStartTime->tv_sec, pBufStartTime->tv_usec, lTimeDelta);
    }
  } while(0);
#endif

  /* Everything is OK */
  return 0;
}

/******************************************************************
 * Function :
 *    RTAIDBPrintEvent()
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
 *    pmeventTID, The TID of the task to which this event belongs
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
void RTAIDBPrintEvent(int              pmDecodedDataFile,
		      event*           pmEvent,
		      int              pmEventCPUID,
		      int              pmEventID,
		      struct timeval*  pmEventTime,
		      int              pmEventPID,
		      int              pmEventTID,
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

  /*  An extra space */
  if((pmEventID == TRACE_TIMER)
   ||(pmEventID == TRACE_MEMORY)
   ||(pmEventID == TRACE_SOCKET)
   ||(pmEventID == TRACE_IPC)
   ||(pmEventID == TRACE_RTAI_MOUNT)
   ||(pmEventID == TRACE_RTAI_UMOUNT)
   ||(pmEventID == TRACE_RTAI_TASK)
   ||(pmEventID == TRACE_RTAI_TIMER)
   ||(pmEventID == TRACE_RTAI_SEM)
   ||(pmEventID == TRACE_RTAI_MBX)
   ||(pmEventID == TRACE_RTAI_LXRT)
   ||(pmEventID == TRACE_RTAI_LXRTI))
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
    /* Is this an RT event */
    if(pmEventID <= TRACE_MAX)
      {
      /*  The PID of the process to which the event belongs */
      if(pmEventPID != -1)
	DBPrintData(pmDecodedDataFile, "%d", pmEventPID);
      else
	DBPrintData(pmDecodedDataFile, "N/A");
      }
    else
      {
      /*  The TID of the task to which the event belongs */
      if(pmEventTID != -1)
	DBPrintData(pmDecodedDataFile, "RT:%d", pmEventTID);
      else
	DBPrintData(pmDecodedDataFile, "RT:N/A");
      }
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
 *    RTAIDBPrintTrace()
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
void RTAIDBPrintTrace(int pmDecodedDataFile, systemInfo* pmSystem, db* pmTraceDB, options* pmOptions)
{
  int              lPID;                  /* PID of process to which event belongs */
  int              lTID;                  /* TID of task to which event belongs */
  int              lPrevEventSet = FALSE; /* Has the previous event been set */
  char             lTabing[10];           /* Tab distance between syscall name and time */
  event            lEvent;                /* Generic event */
  syscallInfo*     pSyscall;              /* Generic syscall pointer */
  process*         pProcess;              /* Generic process pointer */
  process*         pProcessIdle = NULL;   /* Pointer to the idle process */
  process*         pPrevProcess = NULL;   /* Pointer to the process of the last event */
  RTAItask*        pTask;                 /* Generic task pointer */
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

    /* Get the event's task */
    pTask = RTAIDBEventTask(pmTraceDB, &lEvent, pmSystem, FALSE);

    /* Does this process belong to anyone */
    if(pProcess == NULL)
      lPID = -1;
    else
      lPID = pProcess->PID;

    /* Does this task belong to anyone */
    if(pTask == NULL)
      lTID = -1;
    else
      lTID = pTask->TID;

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
      RTAIDBPrintEvent(pmDecodedDataFile,
		       &lEvent,
		       lEventDesc.CPUID,
		       lEventDesc.ID,
		       &(lEventDesc.Time),
		       lPID,
		       lTID,
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
 *    RTAIDBCreateTask()
 * Description :
 *    Creates a RTAI task with the fields initialized
 * Parameters :
 *    pmSystem, The system to which the task belongs.
 *    pmTID, The task's ID.
 *    pmPTID, The task's parent ID.
 * Return values :
 *    Pointer to the created task.
 * History :
 *    K.Y., 13/06/2000. Initial typing.
 * Note :
 ******************************************************************/
RTAItask* RTAIDBCreateTask(systemInfo* pmSystem, int pmTID, int pmPTID)
{
  RTAItask* pTask;          /* Generic task pointer */

  /* Allocate space for a new task */
  if((pTask = (RTAItask*) malloc(sizeof(RTAItask))) == NULL)
    {
    printf("Memory allocation problem \n");
    exit(1);
    }
	  
  /* Set new task's properties */
  pTask->Type             = RTAI_TASK_OBJECT;
  pTask->TID              = pmTID;
  pTask->PTID             = pmPTID;
  pTask->IsProcessBuddy   = FALSE;
  pTask->IsProcessShadow  = FALSE;
  pTask->BuddyProcess     = NULL;
  pTask->ShadowProcess    = NULL;
  pTask->System           = pmSystem;
  pTask->NbTimesScheduled = 0;

  /* Initialize struct members, ahe@sysgo.de 07-03-2002 */
  pTask->TimeRunning.tv_sec         = 0;
  pTask->TimeRunning.tv_usec        = 0;
  pTask->TimeBetweenRuns.tv_sec     = 0;
  pTask->TimeBetweenRuns.tv_usec    = 0;
  pTask->TimeOfBirth.tv_sec         = 0;
  pTask->TimeOfBirth.tv_usec        = 0;
  pTask->QFIFOPut = pTask->QFIFOGet = 0;

  /* List linking */
  pTask->Next             = NULL;
  pTask->Children         = NULL;
  pTask->NextChild        = NULL;
  pTask->NextHash         = NULL;

  /* Insert new task in task list */
  pTask->Next = RTAI_SYSTEM(pmSystem->SystemSpec)->Tasks;
  RTAI_SYSTEM(pmSystem->SystemSpec)->Tasks = pTask;

  /* Give the newly created task to the caller */
  return pTask;

}

/******************************************************************
 * Function :
 *    RTAIDBDestroySystem()
 * Description :
 *    Destroys the RTAI-specific info. of a system description.
 * Parameters :
 *    pmSystem, The system for which the RTAI-specifc info. must
 *              be destroyed.
 * Return values :
 *    NONE
 * History :
 *    K.Y., 12/06/2000. Initial typing.
 * Note :
 ******************************************************************/
void RTAIDBDestroySystem(systemInfo* pmSystem)
{
  RTAItask*    pTask;        /* Generic task pointer */
  RTAItask*    pTaskTemp;    /* Temporary task pointer */
  RTAIsystem*  pRTAISystem;  /* RTAI system description */

  /* Get the RTAI system description */
  pRTAISystem = RTAI_SYSTEM(pmSystem->SystemSpec);

  /* Go through task list */
  for(pTask = pRTAISystem->Tasks; pTask != NULL;)
    {
    /* Remember next task in list */
    pTaskTemp = pTask->Next;

    /* Free memory used by task */
    free(pTask);

    /* Go to next task */
    pTask = pTaskTemp;
    }

  /* Free space used by RTAI-specific structure */
  free(pmSystem->SystemSpec);
}
