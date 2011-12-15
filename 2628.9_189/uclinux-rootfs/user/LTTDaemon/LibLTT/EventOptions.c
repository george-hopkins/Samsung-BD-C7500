/*
   EventOptions.c : File containing event options code.
   Copyright (C) 2001 Karim Yaghmour (karim@opersys.com).
 
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
     K.Y., 19/06/2001, Initial typing.
*/

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <EventOptions.h>

/******************************************************************
 * Function :
 *    CreateOptions()
 * Description :
 * Parameters :
 * Return values :
 * History :
 * Note :
 ******************************************************************/
options* CreateOptions(void)
{
  options* pOptions;    /* Options to be created */

  /* Allocate initialized space for new options */
  pOptions = (options*) calloc(1, sizeof(options));

#if 0
  /* Allocate space for new options */
  pOptions = (options*) malloc(sizeof(options));

  /* Initialize options */
  pOptions->Graphic        = FALSE;
  pOptions->Dump           = FALSE;
  pOptions->Omit           = FALSE;
  pOptions->OmitMask       = 0;
  pOptions->Trace          = FALSE;
  pOptions->TraceMask      = 0;
  pOptions->TraceCPUID     = FALSE;
  pOptions->CPUID          = 0;
  pOptions->TracePID       = FALSE;
  pOptions->PID            = 0;
  pOptions->Summarize      = FALSE;
  pOptions->AcctSysCall    = FALSE;
  pOptions->ForgetCPUID    = FALSE;
  pOptions->ForgetTime     = FALSE;
  pOptions->ForgetPID      = FALSE;
  pOptions->ForgetDataLen  = FALSE;
  pOptions->ForgetString   = FALSE;
  pOptions->Benchmark      = FALSE;
  pOptions->FlightRecorder = FALSE;
  pOptions->InputFileName  = NULL;
  pOptions->OutputFileName = NULL;
  pOptions->ProcFileName   = NULL;
#endif

  /* Give the options to the caller */
  return pOptions;
}

/******************************************************************
 * Function :
 *    DestroyOptions()
 * Description :
 * Parameters :
 * Return values :
 * History :
 * Note :
 *    Don't need to verify if pointers are NULL since free will
 *    simply ignore them in that case.
 ******************************************************************/
void DestroyOptions(options* pmOptions)
{
  /* Free space taken by file names */
  free(pmOptions->InputFileName);
  free(pmOptions->OutputFileName);
  free(pmOptions->ProcFileName);

  /* Free space taken by options structure */
  free(pmOptions);
}
