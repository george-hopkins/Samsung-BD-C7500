/*
   (c) Copyright 2000-2002  convergence integrated media GmbH.
   (c) Copyright 2002-2004  convergence GmbH.

   All rights reserved.

   Written by Denis Oliver Kropp <dok@directfb.org>,
              Andreas Hundt <andi@fischlustig.de>,
              Sven Neumann <neo@directfb.org> and
              Ville Syrjl <syrjala@sci.fi>.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __DFB__CORE__CORE_SYSTEM_H__
#define __DFB__CORE__CORE_SYSTEM_H__

#include <core/system.h>

static void
system_get_info( CoreSystemInfo *info );

static DFBResult
system_initialize( CoreDFB *core, void **data );

static DFBResult
system_join( CoreDFB *core, void **data );

static DFBResult
system_shutdown( bool emergency );

static DFBResult
system_leave( bool emergency );

static DFBResult
system_suspend();

static DFBResult
system_resume();

static VideoMode*
system_get_modes();

static VideoMode*
system_get_current_mode();

static DFBResult
system_thread_init();

static bool
system_input_filter( CoreInputDevice *device,
                     DFBInputEvent   *event );

static volatile void*
system_map_mmio( unsigned int    offset,
                 int             length );

static void
system_unmap_mmio( volatile void  *addr,
                   int             length );

static int
system_get_accelerator();

static unsigned long
system_video_memory_physical( unsigned int offset );

static void*
system_video_memory_virtual( unsigned int offset );

static unsigned int
system_get_partition_type( unsigned int policy);

static unsigned int
system_get_num_of_partitions( );

static unsigned int
system_videoram_length(unsigned int partition);


static CoreSystemFuncs system_funcs = {
     GetSystemInfo:       system_get_info,
     Initialize:          system_initialize,
     Join:                system_join,
     Shutdown:            system_shutdown,
     Leave:               system_leave,
     Suspend:             system_suspend,
     Resume:              system_resume,
     GetModes:            system_get_modes,
     GetCurrentMode:      system_get_current_mode,
     ThreadInit:          system_thread_init,
     InputFilter:         system_input_filter,
     MapMMIO:             system_map_mmio,
     UnmapMMIO:           system_unmap_mmio,
     GetAccelerator:      system_get_accelerator,
     VideoMemoryPhysical: system_video_memory_physical,
     VideoMemoryVirtual:  system_video_memory_virtual,
     GetPartitionType:    system_get_partition_type,
     GetNumOfPartitions:  system_get_num_of_partitions,
     VideoRamLength:      system_videoram_length
};

#define DFB_CORE_SYSTEM(shortname)                          \
__attribute__((constructor)) void directfb_##shortname();   \
                                                            \
void                                                        \
directfb_##shortname()                                      \
{                                                           \
     direct_modules_register( &dfb_core_systems,            \
                              DFB_CORE_SYSTEM_ABI_VERSION,  \
                              #shortname, &system_funcs );  \
}

#endif

