/*
 * EventDBI.h
 *
 * Copyright (C) 2000 Karim Yaghmour (karym@opersys.com).
 *
 * This is distributed under GPL.
 *
 * Header for internal functions of the event database engine.
 * WARNING: This is the private interface of EventDB. Are you
 *          sure you shouldn't be looking at EventDB.h instead???
 *
 * History : 
 *    K.Y., 12/06/2000, Initial typing.
 */

#ifndef __TRACE_TOOLKIT_EVENT_DBI__
#define __TRACE_TOOLKIT_EVENT_DBI__

inline uint8_t DBIEventID
                   (db*             /* Pointer to event's database */,
		    void*           /* Event's start address */);
void           DBIEventTime
                   (db*             /* Pointer to event's database */,
		    uint32_t        /* ID of buffer to which event belongs */,
		    void*           /* Event's start address */,
		    struct timeval* /* Time value to be filled */);
uint8_t        DBIEventCPUID
                   (db*             /* Pointer to event's database */,
		    void*           /* Event's start address */);
void*          DBIEventStruct
                   (db*             /* Pointer to event's database */,
		    void*           /* Event's start address */);
int            DBILinuxEventString
                   (db*             /* Pointer to event's database */,
		    uint32_t        /* ID of buffer to which event belongs */,
		    void*           /* Event's start address */,
                    char*           /* Buffer where event string is to be put */,
		    int             /* Destination buffer's size */);
uint16_t       DBIEventSize
                   (db*             /* Pointer to event's database */,
		    void*           /* Event's start address */);
customEventDesc*
               DBIEventGetCustomDescription
                   (db*             /* Pointer to event's database */,
		    void*           /* Event's start address */);

#endif /* __TRACE_TOOLKIT_EVENT_DBI__ */
