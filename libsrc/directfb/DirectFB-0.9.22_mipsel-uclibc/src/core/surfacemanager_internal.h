#ifndef __SURFACEMANAGER_INTERNAL_H__
#define __SURFACEMANAGER_INTERNAL_H__

#include <core/surfacemanager.h>

#define MAX_ORDER 32
#define MAX_PARTITIONS 2
#define MAX_NUM_OBJ 4096

#ifdef DIRECT_BUILD_TEXT
#define D_INFO_IF(cond, x...)  do { if(cond) { D_INFO(x); } } while (0)
#else
#define D_INFO_IF(cond, x...)  do { } while (0)
#endif

/*
 * for D_ALLOCATION() messages to print, DEBUG_ALLOCATION must
 * be #defined and the environment variable "dfbbcm_show_allocation" must
 * be set at runtime.
 */

/****************************************************************************/
#ifdef DEBUG_ALLOCATION
static bool dfbbcm_show_allocation = false;
#define D_ALLOCATION(x...) do { D_INFO_IF(dfbbcm_show_allocation,x); } while (0)
#else
#define D_ALLOCATION(x...) do { } while (0)
#endif
/****************************************************************************/

typedef struct {
    Chunk        *chunks;
    int           length;
    int           available;
    unsigned int  heap_offset;
} SurfaceManagerPartition;

typedef struct SeqChunk {
     bool           is_used;
     int            offset;      /* offset in video memory, is greater or equal to the heap offset */
     int            length;      /* length of this chunk in bytes */
     SurfaceBuffer *buffer;      /* pointer to surface buffer occupying */
} SeqChunk;

struct _SurfaceManager {
     DFBSurfaceManagerTypes type;

     int             magic;
     FusionSkirmish  lock;

     Chunk          *chunks;
     int             length;
     int             available;

     bool            suspended;

     /* offset of the surface heap */
     unsigned int    heap_offset;

     /* card limitations for surface offsets and their pitch */
     unsigned int    byteoffset_align;
     unsigned int    pixelpitch_align;
     unsigned int    bytepitch_align;

     unsigned int    max_power_of_two_pixelpitch;
     unsigned int    max_power_of_two_bytepitch;
     unsigned int    max_power_of_two_height;

     /* If available, these functions replace DirectFB surface manager */
     SurfaceManagerFuncs *funcs;

     /* Buddy specific fields */
     int             order;
     int             page_size;
     int             pool_size;

     /* list of free and allocated chunks, for each 'power of two' size */
     Chunk          *free[MAX_ORDER];
     Chunk          *alloc[MAX_ORDER];

	 /* extension for multiple partitions */
     SurfaceManagerPartition partition[MAX_PARTITIONS];
     unsigned char           partitions_number;

     /* Sequential specific fields */
     unsigned int    cum_offset;
     unsigned int    next_free_obj;
     SeqChunk        obj[MAX_NUM_OBJ];
};

#endif
