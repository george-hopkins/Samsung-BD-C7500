/*
   TraceReader.c : Main file for alternate trace reader.
   Copyright (C) 2001 Karim Yaghmour (karym@opersys.com).
 
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
     K.Y., 07/09/1999, Initial typing (largely inspired by TraceVisualizer.c).
*/

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#include <Tables.h>
#include <EventDB.h>
#include <RTAIDB.h>
#include <EventOptions.h>

/******************************************************************
 * Function : 
 *    CustomOperations()
 * Description :
 *    Function containing custom operations done on trace.
 * Parameters :
 *    pmSystem, Pointer to system description
 *    pmTraceDB, Pointer to trace database
 *    pmOptions, User-provided options
 * Return values :
 *    NONE
 * History :
 *    K.Y.   07/09/2001, Initial typing.
 * Note :
 ******************************************************************/
void CustomOperations(systemInfo* pmSystem, db* pmTraceDB, options* pmOptions)
{
  int                 lFormatType;       /* Formatting type */
  char                lEventTime[80];    /* String to print event time */
  char*               lFormatStr;        /* Event formatting string */
  event               lEvent;            /* Generic event */
  uint8_t             lEventID;          /* Event ID */
  eventDescription    lEventDesc;        /* Event description */
  customEventDesc*    pCustomDesc;       /* Custom event description */

  /* Position at first event in list */
  lEvent = pmTraceDB->FirstEvent;

  /* Go through event database */
  do
    {
    /* Get the event's ID */
    lEventID = DBEventID(pmTraceDB, &lEvent);

    /* Depending on the type of event */
    switch(lEventID)
      {
      /* Event creation */
      case TRACE_NEW_EVENT:
	/* Get the event's description */
	DBEventDescription(pmTraceDB, &lEvent, TRUE, &lEventDesc);

	/* Format event's time */
	DBFormatTimeInReadableString(lEventTime, lEventDesc.Time.tv_sec, lEventDesc.Time.tv_usec);

	/* Print out some info on this event */
	printf("New event created at: %s; Event type created: %s; Custom event ID: %d \n",
	       lEventTime,
	       NEW_EVENT(lEventDesc.Struct)->type,
	       RFT32(pmTraceDB, NEW_EVENT(lEventDesc.Struct)->id));
	break;

      /* Custom event */
      case TRACE_CUSTOM:
	/* Get the event's description */
	DBEventDescription(pmTraceDB, &lEvent, FALSE, &lEventDesc);

	/* Get the event's custom description */
	pCustomDesc = DBEventGetCustomDescription(pmTraceDB, &lEvent);

	/* Print out info on event */
	printf("Custom Event: %s; Custom event ID: %d; Format type: %d\n",
	       pCustomDesc->Event.type,
	       pCustomDesc->ID,
	       RFT32(pmTraceDB, pCustomDesc->Event.format_type));
	printf("\t Formatting string: %s \n",
	       pCustomDesc->Event.form);	       
	break;

      /* All other events */
      default:
	break;
      }
    } while(DBEventNext(pmTraceDB, &lEvent) == TRUE); /* Continue until there's no more "next" event */

  printf("\n\n");

  /* Find the event fitting the description "omega" */
  /* WARNING: READ note about the following function in LibLTT/EventDB.c before using it */
  if((lFormatStr = DBEventGetFormatByCustomType(pmTraceDB, "Omega", &lFormatType)) != NULL)
    printf("Formatting string for event of type \"Omega\": %s \n", lFormatStr);

  /* Is this event formatted in XML */
  if(lFormatType == CUSTOM_EVENT_FORMAT_TYPE_XML)
    printf("Event formatted in XML \n");

  /* Change the formatting string for the event type "omega" */
  if(DBEventSetFormatByCustomType(pmTraceDB,
				  "Omega",
				  lFormatType,
				  "<event name=\"Omega\" size=\"0\"><var name=\"a_char\" type=\"char\"/></event>") == TRUE)
    {
    printf("Format string for \"Omega\" should have changed now \n");

    /* Read the description again to make sure it changed */
    if((lFormatStr = DBEventGetFormatByCustomType(pmTraceDB, "Omega", &lFormatType)) != NULL)
      printf("Formatting string for event of type \"Gamma\": %s \n", lFormatStr);
    }

  printf("\n\n");

  /* Find the event of custom event ID 31 */
  if((lFormatStr = DBEventGetFormatByCustomID(pmTraceDB, 31, &lFormatType)) != NULL)
    printf("Formatting string for event of custom ID 31: %s \n", lFormatStr);

  /* Is this event formatted in XML */
  if(lFormatType == CUSTOM_EVENT_FORMAT_TYPE_XML)
    printf("Event formatted in XML \n");


  /* Change the formatting string for custom event ID 31 */
  if(DBEventSetFormatByCustomID(pmTraceDB,
				31,
				lFormatType,
				"<event name=\"Delta\" size=\"0\"><var name=\"a_struct\" type=\"u40\"/></event>") == TRUE)
    {
    printf("Format string for custom event ID 31 should have changed now \n");

    /* Read the description again to make sure it changed */
    if((lFormatStr = DBEventGetFormatByCustomID(pmTraceDB, 31, &lFormatType)) != NULL)
      printf("Formatting string for event of custom ID 31: %s \n", lFormatStr);
    }  
}

/******************************************************************
 * Function : 
 *    main()
 * Description :
 *    Main entry point into data decoder.
 * Parameters :
 *    argc, The number of command line arguments
 *    argv, Array of strings containing the command line arguments
 * Return values :
 *    NONE
 * History :
 *    K.Y.   07/09/2001, Initial typing.
 * Note :
 ******************************************************************/
int main(int argc, char** argv)
{
  db*               pTraceDB;             /* The trace database */
  int               lReadResult;          /* Result of reading the trace file */
  int               lDecodedDataFile = 0; /* File containing the human readable data */
  int               lTraceDataFile = 0;   /* The file to which tracing data WAS dumped */
  systemInfo*       pSystem;              /* Full description of system during trace */
  options*          pOptions;             /* Parsed command line options */
  FILE*             lProcDataFile = NULL; /* File containing /proc information */

  /* Initialize variables used */
  pTraceDB = NULL;
  pSystem  = NULL;
  pOptions = NULL;

  /* Allocate space for options */
  pOptions = CreateOptions();

  /* How many parameters do we have? (reader tracefile procfile dumpfile) */
  if((argc < 3) || (argc > 4))
    {
    printf("TraceReader: Wrong number of command-line arguments \n");
    printf("TraceReader: Usage => tracereader tracefile procfile [dumpfile] \n");
    exit(1);
    }

  /* Copy the trace file name */
  pOptions->InputFileName = (char*) malloc(strlen(argv[1]) + 1);
  strcpy(pOptions->InputFileName, argv[1]);

  /* Copy the proc file name */
  pOptions->ProcFileName = (char*) malloc(strlen(argv[2]) + 1);
  strcpy(pOptions->ProcFileName, argv[2]);

  /* Are we dumping to an output file? */
  if(argc == 4)
    {
    /* Copy the output file name */
    pOptions->OutputFileName = (char*) malloc(strlen(argv[3]) + 1);
    strcpy(pOptions->OutputFileName, argv[3]);
    }

  /* Open the input file */
  if((lTraceDataFile = open(pOptions->InputFileName, O_RDONLY, 0)) < 0)
    {
    /* File ain't good */
    printf("Unable to open input data file \n");
    exit(1);
    }
  
  /* Open the /proc information file */
  if((lProcDataFile = fopen(pOptions->ProcFileName, "r")) == NULL)
    {
    /* File ain't good */
    printf("Unable to open /proc information file \n");
    close(lTraceDataFile);
    exit(1);
    }

  /* Are we dumping to an output file? */
  if(argc == 4)
    {
    /* Open the output file */
    if((lDecodedDataFile = creat(pOptions->OutputFileName, S_IRUSR| S_IWUSR | S_IRGRP | S_IROTH)) < 0)
      {
      /* File ain't good */
      printf("Unable to open output file \n");
      close(lTraceDataFile);
      fclose(lProcDataFile);
      exit(1);
      }
    }

  /* Create database and system structures */
  pTraceDB = DBCreateDB();
  pSystem  = DBCreateSystem();

  /* Do the read proper */
  lReadResult = DBReadTrace(lTraceDataFile, pTraceDB);

  /* Was there any problem reading the trace */
  if(lReadResult != 0)
    {
    /* Depending on the type of error */
    switch(lReadResult)
      {
      case 1:
      default:
	/* Something's wrong with the input file */
	printf("\nInput file isn't a valid trace file \n");
	break;

      case 2:
	/* Trace file format version not supported */
	printf("\nTrace file format version %d.%d not supported by visualizer \n",
	       pTraceDB->MajorVersion,
	       pTraceDB->MinorVersion);
	break;
      }

    /* Stop here */
    exit(1);
    }

  /* Process the information in the /proc information file */
  DBProcessProcInfo(lProcDataFile, pSystem);

  /* Close the /proc info file */
  fclose(lProcDataFile);

  /* Process the database according to selected options */
  DBPreprocessTrace(pSystem, pTraceDB, pOptions);

  /* Process the database according to selected options */
  DBProcessTrace(pSystem, pTraceDB, pOptions);

#if 1 /* Toggle this binary flag to enable or disable custom code */
  /* Call custom operations routine */
  CustomOperations(pSystem, pTraceDB, pOptions);
#endif

  /* Are we dumping to an output file? */
  if(argc == 4)
    {
    /* Depending on the type of system */
    switch(pTraceDB->SystemType)
      {
      /* Plain Linux */
      case TRACE_SYS_TYPE_VANILLA_LINUX:
	/* Print out the data-base and add details requested at the command-line */
	DBPrintTrace(lDecodedDataFile, pSystem, pTraceDB, pOptions);
	break;

#if SUPP_RTAI
      /* RTAI/Linux system */
      case TRACE_SYS_TYPE_RTAI_LINUX:
	/* Print out the data-base and add details requested at the command-line */
	RTAIDBPrintTrace(lDecodedDataFile, pSystem, pTraceDB, pOptions);
#endif /* SUPP_RTAI */
      }

    /* Close the output */
    close(lDecodedDataFile);
    printf("Output has been written to %s \n", pOptions->OutputFileName);
    }

  /* Close the input file */
  if(pOptions->InputFileName != NULL)
    close(lTraceDataFile);

  /* We're outa here */
  exit(0);
}

