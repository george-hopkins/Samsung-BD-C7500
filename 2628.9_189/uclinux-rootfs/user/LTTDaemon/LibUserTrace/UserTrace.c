/*
   UserTrace.c : User event tracing libary.
   Copyright (C) 2001-2004 Karim Yaghmour (karim@opersys.com).

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation;
    version 2.1 of the License.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

   History :
     K.Y., 01/12/2001, Initial typing.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#include <LTTTypes.h>
#include <DevCommand.h>
#include <UserTrace.h>

#define USER_TRACE_NULL_HANDLE    -1
#define USER_TRACE_CONTROL_CLOSED -1

 /* Global variables */
static int gTraceHandle = USER_TRACE_NULL_HANDLE;    /* Handle to tracer */
static int gControlFile = USER_TRACE_CONTROL_CLOSED; /* File for ioctls */

/******************************************************************
 * Function :
 *    get_relayfs_mntpt()
 * Description :
 *    Determine whether and where relayfs is mounted.
 * Parameters :
 *    pmMountPoint, receives mount path.
 * Return values :
 *    0, if everything went OK.
 *    -1, if relayfs wasn't mounted.
 * History :
 *    T.R.Z., 18/03/2004. Initial typing.
 * Note :
 ******************************************************************/
int get_relayfs_mntpt(char *pmMountPoint)
{
	char fsname[256];
	char mntpt[PATH_MAX];
	char dummy[256];
  
	FILE *mounts = fopen("/proc/mounts", "r");
	if(mounts == NULL)
		return -1;

	while(fscanf(mounts, "%s%s%s%s%s%s", dummy, mntpt, fsname, dummy, dummy, dummy) == 6) 
	{
		if(!strcmp(fsname, "relayfs")) 
		{
			strcpy(pmMountPoint, mntpt);
			return 0;
		}
	}
	
	return -1;
}

/******************************************************************
 * Function :
 *    open_control_file()
 * Description :
 *    Open /mnt/relay/ltt/control for future ioctls.
 * Parameters :
 *    NONE.
 * Return values :
 *    0, if everything went OK.
 *    -2, if relayfs wasn't mounted.
 *    Error code from "open" operation.
 * History :
 *    T.R.Z., 18/03/2004. Initial typing.
 * Note :
 ******************************************************************/
static int open_control_file(void)
{
	int      lMountRC, lControlFile;
	char     lRelayfsMntPt[PATH_MAX];    /* Relayfs mountpoint path buffer */
	char     lRelayDirPath[PATH_MAX]; /* Relayfs dir path buffer */

	/* Check for relayfs before doing anything else */
	lMountRC = get_relayfs_mntpt(lRelayfsMntPt);
	if(lMountRC) {
		/* relayfs ain't mounted */
		printf("\nUserTrace: relayfs not mounted.\n");
		return -1;
	}

	/* trace files are: relayfs mnt pt/trace/handle/cpu0-cpuX */
	sprintf(lRelayDirPath, "%s/%s/%s", lRelayfsMntPt, TRACE_RELAYFS_ROOT, TRACE_CONTROL_FILE);
	if((lControlFile = open(lRelayDirPath, O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP)) < 0)
		printf("UserTrace: Unable to open control file %s (errno = %d) \n", lRelayDirPath, errno);
	
	return lControlFile;
}

/******************************************************************
 * Function :
 *    close_control_file()
 * Description :
 *    Open /mnt/relay/ltt/control for future ioctls.
 * Parameters :
 *    NONE.
 * Return values :
 *    0, if everything went OK.
 *    -2, if relayfs wasn't mounted.
 *    Error code from "open" operation.
 * History :
 *    T.R.Z., 18/03/2004. Initial typing.
 * Note :
 ******************************************************************/
static int close_control_file(int pmControlFile)
{
	int  lRetValue = 0;     /* Function's return value */

	if(gControlFile != USER_TRACE_CONTROL_CLOSED) {
		lRetValue = close(pmControlFile);
		gControlFile = USER_TRACE_CONTROL_CLOSED;
	}
	
	return lRetValue;
}

/******************************************************************
 * Function :
 *    trace_attach()
 * Description :
 *    Attach to trace device.
 * Parameters :
 *    NONE.
 * Return values :
 *    0, if everything went OK.
 *    Error code from "open" operation.
 * History :
 *    K.Y., 04/12/2001. Initial typing.
 * Note :
 ******************************************************************/
int trace_attach(void)
{
  int                   lRetValue = 0;     /* Function's return value */
  struct control_data   lControlData;

  if((gControlFile = open_control_file()) < 0)
  {
	  printf("\nUserTrace: exiting.\n");
	  exit(1);
  }
  
  /* Open the tracing device */
  lControlData.tracer_handle = 2;
  if((gTraceHandle = ioctl(gControlFile, TRACER_ALLOC_HANDLE, &lControlData)) < 0)
    {
    lRetValue = gTraceHandle;
    gTraceHandle = USER_TRACE_NULL_HANDLE;
    }

  /* Everything is OK */
  return lRetValue;
}

/******************************************************************
 * Function :
 *    trace_detach()
 * Description :
 *    Detach from trace device.
 * Parameters :
 *    NONE.
 * Return values :
 *    0, if everything went OK.
 *    -1, Device wasn't attached to begin with.
 * History :
 *    K.Y., 04/12/2001. Initial typing.
 * Note :
 ******************************************************************/
int trace_detach(void)
{
  int lRetValue = 0;     /* Function's return value */
  struct control_data   lControlData;

  /* Is the tracer open? */
  if(gTraceHandle == USER_TRACE_NULL_HANDLE)
    return -1;

  /* Free the user handle in the tracer */
  lControlData.tracer_handle = gTraceHandle;
  lRetValue = ioctl(gControlFile, TRACER_FREE_HANDLE, &lControlData);

  /* Tracer has now been closed */
  gTraceHandle = USER_TRACE_NULL_HANDLE;

  close_control_file(gControlFile);
  
  /* Everything is OK */
  return lRetValue;
}

/******************************************************************
 * Function :
 *    trace_create_event()
 * Description :
 *    Create a new event.
 * Parameters :
 *    pmEventType, string describing event type
 *    pmEventDesc, string used for standard formatting
 *    pmFormatType, type of formatting used to log event
 *                  data
 *    pmFormatData, data specific to format
 *    pmEventID, the ID of the event created.
 * Return values :
 *    Event ID on success,
 *    Device error code otherwise (< 0).
 * History :
 *    K.Y., 04/12/2001. Initial typing.
 * Note :
 ******************************************************************/
int trace_create_event(char*            pmEventType,
		       char*            pmEventDesc,
		       int              pmFormatType,
		       char*            pmFormatData)
{
  int                         lRetValue = 0;     /* Function's return value */
  tracer_create_user_event    lCreateUserEvent;  /* Create user event information */
  struct control_data         lControlData;

  /* Is the trace device open? */
  if(gTraceHandle == USER_TRACE_NULL_HANDLE)
    return -1;

  /* Set basic event properties */
  if(pmEventType != NULL)
    strncpy(lCreateUserEvent.type, pmEventType, CUSTOM_EVENT_TYPE_STR_LEN);
  if(pmEventDesc != NULL)
    strncpy(lCreateUserEvent.desc, pmEventDesc, CUSTOM_EVENT_DESC_STR_LEN);
  if(pmFormatData != NULL)
    strncpy(lCreateUserEvent.form, pmFormatData, CUSTOM_EVENT_FORM_STR_LEN);

  /* Ensure that strings are bound */
  lCreateUserEvent.type[CUSTOM_EVENT_TYPE_STR_LEN - 1] = '\0';
  lCreateUserEvent.desc[CUSTOM_EVENT_DESC_STR_LEN - 1] = '\0';
  lCreateUserEvent.form[CUSTOM_EVENT_FORM_STR_LEN - 1] = '\0';

  /* Set format type */
  lCreateUserEvent.format_type = pmFormatType;

  /* Send the command to the device */
  lControlData.tracer_handle = gTraceHandle;
  lControlData.command_arg = (unsigned long)&lCreateUserEvent;
  lRetValue = ioctl(gControlFile, TRACER_CREATE_USER_EVENT, &lControlData);

  /* Set the event ID if operation was successful*/
  if(lRetValue == 0)
    return lCreateUserEvent.id;

  /* Tell the caller about his operation's status */
  return lRetValue;
}

/******************************************************************
 * Function :
 *    trace_destroy_event()
 * Description :
 *    Destroy an event.
 * Parameters :
 *    pmEventID, the Id returned by trace_create_event()
 * Return values :
 *    0, if everything went OK.
 *    Device error code otherwise.
 * History :
 *    K.Y., 04/12/2001. Initial typing.
 * Note :
 ******************************************************************/
int trace_destroy_event(int pmEventID)
{
  int                   lRetValue = 0;     /* Function's return value */
  struct control_data   lControlData;

  /* Is the trace device open? */
  if(gTraceHandle == USER_TRACE_NULL_HANDLE)
    return -1;

  /* Tell the device to destroy the event */
  lControlData.tracer_handle = gTraceHandle;
  lControlData.command_arg = (unsigned long)pmEventID;
  lRetValue = ioctl(gControlFile, TRACER_DESTROY_USER_EVENT, &lControlData);

  /* Tell the caller about his operation's status */
  return lRetValue;
}

/******************************************************************
 * Function :
 *    trace_user_event()
 * Description :
 *    Trace a user event.
 * Parameters :
 *    pmEventID, the event ID provided upon creation.
 *    pmEventSize, the size of the data provided.
 *    pmEventData, data buffer describing event.
 * Return values :
 *    0, if everything went OK.
 *    Device error code otherwise.
 * History :
 *    K.Y., 04/12/2001. Initial typing.
 * Note :
 ******************************************************************/
int trace_user_event(int pmEventID, int pmEventSize, void* pmEventData)
{
  int                       lRetValue = 0; /* Function's return value */
  tracer_trace_user_event   lUserEvent;    /* The user's event */
  struct control_data       lControlData;

  /* Is the trace device open? */
  if(gTraceHandle == USER_TRACE_NULL_HANDLE)
    return -1;

  /* Set custom event Id */
  lUserEvent.id = pmEventID;

  /* Set the data size */
  if(pmEventSize <= CUSTOM_EVENT_MAX_SIZE)
    lUserEvent.data_size = (uint32_t) pmEventSize;
  else
    lUserEvent.data_size = (uint32_t) CUSTOM_EVENT_MAX_SIZE;

  /* Set the pointer to the event data */
  lUserEvent.data = pmEventData;
  
  /* Send the command to the device */
  lControlData.tracer_handle = gTraceHandle;
  lControlData.command_arg = (unsigned long)&lUserEvent;
  lRetValue = ioctl(gControlFile, TRACER_TRACE_USER_EVENT, &lControlData);

  /* Tell the caller about his operation's status */
  return lRetValue;
}

/******************************************************************
 * Function :
 *    trace_set_event_mask()
 * Description :
 *    Set the event mask.
 * Parameters :
 *    pmEventMask, Event mask to be set.
 * Return values :
 *    0, if everything went OK.
 *    Device error code otherwise.
 * History :
 *    K.Y., 04/12/2001. Initial typing.
 * Note :
 ******************************************************************/
int trace_set_event_mask(trace_event_mask pmEventMask)
{
  int                   lRetValue = 0;     /* Function's return value */
  struct control_data   lControlData;

  /* Is the trace device open? */
  if(gTraceHandle == USER_TRACE_NULL_HANDLE)
    return -1;

  /* Send the command to the device */
  lControlData.tracer_handle = gTraceHandle;
  lControlData.command_arg = (unsigned long)&pmEventMask;
  lRetValue = ioctl(gControlFile, TRACER_SET_EVENT_MASK, &lControlData);

  /* Tell the caller about his operation's status */
  return lRetValue;
}

/******************************************************************
 * Function :
 *    trace_get_event_mask()
 * Description :
 *    Get the event mask.
 * Parameters :
 *    pmEventMask, Pointer to variable receiving the event mask.
 * Return values :
 *    0, if everything went OK.
 *    Device error code otherwise.
 * History :
 *    K.Y., 04/12/2001. Initial typing.
 * Note :
 ******************************************************************/
int trace_get_event_mask(trace_event_mask* pmEventMask)
{
  int                   lRetValue = 0;     /* Function's return value */
  struct control_data   lControlData;

  /* Is the trace device open? */
  if(gTraceHandle == USER_TRACE_NULL_HANDLE)
    return -1;

  /* Send the command to the device */
  lControlData.tracer_handle = gTraceHandle;
  lControlData.command_arg = (unsigned long)pmEventMask;
  lRetValue = ioctl(gControlFile, TRACER_GET_EVENT_MASK, &lControlData);

  /* Tell the caller about his operation's status */
  return lRetValue;
}

/******************************************************************
 * Function :
 *    trace_enable_event_trace()
 * Description :
 *    Enable the tracing of a certain event.
 * Parameters :
 *    pmEventID, Event ID who's tracing is to be enabled.
 * Return values :
 *    0, if everything went OK.
 *    Device error code otherwise.
 * History :
 *    K.Y., 04/12/2001. Initial typing.
 * Note :
 ******************************************************************/
int trace_enable_event_trace(int pmEventID)
{
  int               lRetValue = 0;     /* Function's return value */
  trace_event_mask  lEventMask;        /* The event trace mask */

  /* Get the current event mask */
  if(trace_get_event_mask(&lEventMask) < 0)
    return -1;

  /* Set the event's bit to enable tracing */
  ltt_set_bit(pmEventID, &lEventMask);

  /* Set the event mask */
  lRetValue = trace_set_event_mask(lEventMask);

  /* Tell the caller about his operation's status */
  return lRetValue;
}

/******************************************************************
 * Function :
 *    trace_disable_event_trace()
 * Description :
 *    Disable the tracing of a certain event.
 * Parameters :
 *    pmEventID, Event ID who's tracing is to be disabled.
 * Return values :
 *    0, if everything went OK.
 *    Device error code otherwise.
 * History :
 *    K.Y., 04/12/2001. Initial typing.
 * Note :
 ******************************************************************/
int trace_disable_event_trace(int pmEventID)
{
  int               lRetValue = 0;     /* Function's return value */
  trace_event_mask  lEventMask;        /* The event trace mask */

  /* Get the current event mask */
  if(trace_get_event_mask(&lEventMask) < 0)
    return -1;

  /* Set the event's bit to enable tracing */
  ltt_clear_bit(pmEventID, &lEventMask);

  /* Set the event mask */
  lRetValue = trace_set_event_mask(lEventMask);

  /* Tell the caller about his operation's status */
  return lRetValue;
}

/******************************************************************
 * Function :
 *    trace_is_event_traced()
 * Description :
 *    Check if a certain event is being traced.
 * Parameters :
 *    pmEventID, Event ID to be checked for tracing.
 * Return values :
 *    1, if event is being traced.
 *    0, if event is not being traced.
 * History :
 *    K.Y., 04/12/2001. Initial typing.
 * Note :
 ******************************************************************/
int trace_is_event_traced(int pmEventID)
{
  int               lRetValue = 0;     /* Function's return value */
  trace_event_mask  lEventMask;        /* The event trace mask */

  /* Get the current event mask */
  if(trace_get_event_mask(&lEventMask) < 0)
    return -1;

  /* Set the event's bit to enable tracing */
  lRetValue = ltt_test_bit(pmEventID, &lEventMask);

  /* Tell the caller about his operation's status */
  return lRetValue;
}
