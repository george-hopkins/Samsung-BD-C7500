

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


#include <pthread.h>

#include <direct/list.h>
#include <fusion/shmalloc.h>

#include <directfb.h>

#include <core/core.h>
#include <core/coredefs.h>
#include <core/coretypes.h>

#include <core/gfxcard.h>
#include <core/surfaces.h>
#include <core/surfacemanager.h>
#include <core/surfacemanager_internal.h>
#include <core/surfacemanager_buddy.h>
#include <core/system.h>

#include <direct/debug.h>
#include <direct/mem.h>
#include <direct/memcpy.h>
#include <direct/messages.h>
#include <direct/util.h>

D_DEBUG_DOMAIN( Core_SM, "Core/SurfaceMgrBuddy", "DirectFB Surface Manager" );

#define SIZEOF_ARRAY(a) (sizeof(a)/sizeof(a[0]))

//marker for a chunk that was split
#define CHUNK_SPLIT ((SurfaceBuffer*)-1)

//id of the root chunk (yes chunk zero is never used)
#define ROOT_CHUNK 1

#define MINIMUM_PAGE_SIZE 4096

#define CLIST_EMPTY NULL

/*
 * if ALLOCATE_FROM_BOTTOM is defined, the first allocations will be made from
 * the bottom of the pool.  Otherwise, they will be starting from zero.
 */
#define ALLOCATE_FROM_BOTTOM

// initialize a chunk
#define chunk_init(c) memset(c,0,sizeof(*c))

struct _Chunk
{
    SurfaceBuffer *buffer;
    Chunk *next;
};

static void chunk_location(SurfaceManager *manager, Chunk *c, int *offset, int *length);
static void clist_push(Chunk **list, Chunk *item);
static Chunk *clist_pop(Chunk **list);
static Chunk *clist_find_buffer_and_remove(Chunk **list, SurfaceBuffer *buffer);
static bool clist_remove_chunk(Chunk **list, Chunk *item);
static DFBResult split_free_chunk(SurfaceManager *manager, int order);
static bool merge_buddies(SurfaceManager *manager, Chunk *c, int order);

SurfaceManager *
dfb_surfacemanager_buddy_create( unsigned int     offset,
                                 unsigned int     length,
                                 unsigned int     minimum_page_size,
                                 CardLimitations *limits )
{
     SurfaceManager *manager=NULL;

#ifdef DEBUG_ALLOCATION
     dfbbcm_show_allocation = getenv( "dfbbcm_show_allocation" ) ? true : false;
#endif

     D_ASSERT(length!=0);

     //allocate pre-zeroed memory
     manager=SHCALLOC(1,sizeof(SurfaceManager));

     manager->type = DSMT_SURFACEMANAGER_BUDDY;

     manager->heap_offset      = offset;
     manager->byteoffset_align = limits->surface_byteoffset_alignment;
     manager->pixelpitch_align = limits->surface_pixelpitch_alignment;
     manager->bytepitch_align  = limits->surface_bytepitch_alignment;
     manager->max_power_of_two_pixelpitch = limits->surface_max_power_of_two_pixelpitch;
     manager->max_power_of_two_bytepitch  = limits->surface_max_power_of_two_bytepitch;
     manager->max_power_of_two_height     = limits->surface_max_power_of_two_height;

     manager->page_size=MAX(limits->surface_byteoffset_alignment,minimum_page_size ? minimum_page_size : MINIMUM_PAGE_SIZE);

     {
     	int pool_order=direct_log2(length+1)-1;

     	int n_pages=(1<<pool_order)/manager->page_size;

     	manager->order=direct_log2(n_pages+1)-1;

     	manager->page_size=length/n_pages;
     	//manager->page_size=(1<<pool_order)/n_pages;


     	//align with requirements
     	if(manager->byteoffset_align != 0)
     		manager->page_size -= manager->page_size%manager->byteoffset_align;

     	manager->pool_size = manager->page_size * n_pages;
     	manager->available = manager->pool_size;

     	int nchunks=2*(1<<manager->order);

     	//allocate memory without initializing it
     	manager->chunks=SHMALLOC(nchunks * sizeof(*(manager->chunks)));

     	D_ALLOCATION(
            "Core/SurfaceMgrBuddy -> pool info:\n"
     		"total memory   : %d\n"
     		"usable memory  : %d (%d lost)\n"
     		"pages          : %d\n"
     		"page size      : %d\n"
     		"chunks         : %d (order %d)\n",
     		length,
     		manager->pool_size, length - manager->pool_size,
     		n_pages,
     		manager->page_size,
     		nchunks, manager->order);
     }

     {
     	//create the global chunk
     	register Chunk *c=&manager->chunks[ROOT_CHUNK];
     	chunk_init(c);

     	clist_push(&manager->free[manager->order],c);
     	D_DEBUG("pushing root chunk in list of order %d (%d pages)\n",manager->order, 1<<(manager->order));
     }

     fusion_skirmish_init( &manager->lock, "Surface Manager" );

     D_MAGIC_SET( manager, SurfaceManager );

     return manager;
}

void
dfb_surfacemanager_buddy_destroy( SurfaceManager *manager )
{
     Chunk *c=NULL;
     int memory_leaks=0;
     int i;

     D_ASSERT( manager != NULL );
     D_ASSERT( manager->chunks != NULL );

     D_MAGIC_ASSERT( manager, SurfaceManager );

     //--- automatically free all chunks still allocated ---
     for(i=0;i<manager->order;i++)
     {
     	while(manager->alloc[i])
     	{
     		dfb_surfacemanager_buddy_deallocate(manager,manager->alloc[i]->buffer);
     		memory_leaks++;
     	}
     }

     if(memory_leaks!=0)
     {
     	D_WARN("WARNING: %d memory chunk(s) had not been deallocated when the surface manager was destroyed\n",memory_leaks);
     }

     D_MAGIC_CLEAR( manager );

     //remove the global chunk: it should be the only chunk left in the highest order list
     c=clist_pop(&manager->free[manager->order]);
     D_ASSERT(c);
     D_ASSERT((c-manager->chunks) == ROOT_CHUNK);

     //free the heap of chunks
     SHFREE(manager->chunks);

     /* Destroy manager lock. */
     fusion_skirmish_destroy( &manager->lock );

     /* Deallocate manager struct. */
     SHFREE( manager );
}

DFBResult dfb_surfacemanager_buddy_adjust_heap_offset( SurfaceManager *manager,
                                                       int             offset )
{
     D_ASSERT( offset >= 0 );

     D_MAGIC_ASSERT( manager, SurfaceManager );

     dfb_surfacemanager_lock( manager );

     manager->heap_offset=offset;

     dfb_surfacemanager_unlock( manager );

     return DFB_OK;
}

void
dfb_surfacemanager_buddy_enumerate_chunks( SurfaceManager  *manager,
                                           SMChunkCallback  callback,
                                           void            *ctx )
{
     int i;

     D_ASSERT( manager != NULL );
     D_ASSERT( callback != NULL );

     D_MAGIC_ASSERT( manager, SurfaceManager );

     dfb_surfacemanager_lock( manager );

     for(i=0;i<manager->order;i++)
     {
        Chunk *c=manager->alloc[i];

        while(c)
        {
            int length,offset;
            chunk_location(manager,c,&offset,&length);

            if(callback(
                    c->buffer,  //SurfaceBuffer *buffer,
                    offset,     //int            offset,
                    length,     //int            length,
                    0,          //int            tolerations,
                    ctx         //void            *ctx
                ) !=DFENUM_OK) goto stop;

            c=c->next;
        }
     }


     for(i=0;i<manager->order;i++)
     {
        Chunk *c=manager->free[i];

        while(c)
        {
            int length,offset;
            chunk_location(manager,c,&offset,&length);

            if(callback(
                    c->buffer,  //SurfaceBuffer *buffer,
                    offset,     //int            offset,
                    length,     //int            length,
                    0,          //int            tolerations,
                    ctx         //void            *ctx
                ) !=DFENUM_OK) goto stop;

            c=c->next;
        }
     }

stop:

     dfb_surfacemanager_unlock( manager );
}

/** public functions NOT locking the surfacemanger theirself,
    to be called between lock/unlock of surfacemanager **/

DFBResult dfb_surfacemanager_buddy_allocate( SurfaceManager *manager,
                                             SurfaceBuffer  *buffer )
{
     DFBResult result=DFB_OK;
     int length, order;
     int height;
     int width;
     Chunk *c=NULL;
     CoreSurface *surface=NULL;

     D_MAGIC_ASSERT( manager, SurfaceManager );

     if (manager->suspended)
          return DFB_NOVIDEOMEMORY;

     D_ASSERT(buffer);
     D_ASSERT(buffer->surface);

     surface = buffer->surface;

     width = surface->width;
     height = surface->height;

     /* Align width and height before malloc */
     if (surface->caps & DSCAPS_ALLOC_ALIGNED_WIDTH_4)
          width = (width + 3) & ~3;
     else if (surface->caps & DSCAPS_ALLOC_ALIGNED_WIDTH_8)
          width = (width + 7) & ~7;
     else if (surface->caps & DSCAPS_ALLOC_ALIGNED_WIDTH_16)
          width = (width + 15) & ~15;

     if (surface->caps & DSCAPS_ALLOC_ALIGNED_HEIGHT_4)
          height = (height + 3) & ~3;
     else if (surface->caps & DSCAPS_ALLOC_ALIGNED_HEIGHT_8)
          height = (height + 7) & ~7;
     else if (surface->caps & DSCAPS_ALLOC_ALIGNED_HEIGHT_16)
          height = (height + 15) & ~15;

     /* calculate the required length depending on limitations */
     unsigned int pitch = MAX( width, surface->min_width );

     if (pitch < manager->max_power_of_two_pixelpitch &&
         surface->height < manager->max_power_of_two_height)
          pitch = 1 << direct_log2( pitch );

     if (manager->pixelpitch_align > 1) {
          pitch += manager->pixelpitch_align - 1;
          pitch -= pitch % manager->pixelpitch_align;
     }

     pitch = DFB_BYTES_PER_LINE( buffer->format, pitch );

     if (pitch < manager->max_power_of_two_bytepitch &&
         height < manager->max_power_of_two_height)
          pitch = 1 << direct_log2( pitch );

     if (manager->bytepitch_align > 1) {
          pitch += manager->bytepitch_align - 1;
          pitch -= pitch % manager->bytepitch_align;
     }

     length = DFB_PLANE_MULTIPLY( buffer->format, MAX( height, surface->min_height ) * pitch );

     order=direct_log2(((length-1)/manager->page_size) + 1);

     D_DEBUG("requiring %d bytes (order %d : %d)\n",length,order, 1<<order);

    //take a chunk from the list
     c=clist_pop(&manager->free[order]);
     if(c==NULL)
     {
     	//if list was empty, take a bigger chunk and split it...
     	result=split_free_chunk(manager,order+1);

     	//cant do?
     	if(result!=DFB_OK) {
     		D_ALLOCATION("Core/SurfaceMgrBuddy -> policy %d allocating %d requires locking %d but...\n",buffer->policy, length,(1<<order)*manager->page_size);
     		D_ERROR("CANT ALLOCATE ANY MORE VIDEO MEMORY!!\n");
     		return result;
     	}

     	//... otherwise, try again, as there is now a chunk of the right size
     	c=clist_pop(&manager->free[order]);
     	D_ASSERT(c);
     }

     //mark the chunk as allocated
     c->buffer=buffer;

     //put the chunk in an allocated list, according its size
     clist_push(&manager->alloc[order],c);

     {
     	int size;
     	int offset;

     	chunk_location(manager,c,&offset,&size);

     	buffer->video.offset = offset;
     	buffer->video.pitch  = pitch;
     	buffer->video.health = CSH_STORED;
     	buffer->video.chunk  = (Chunk*)c;

     	manager->available -= size;

     	D_ALLOCATION(
     		"Core/SurfaceMgrBuddy -> policy %d reserved %d at 0x%08X for %d (%d wasted) %d avail.\n",buffer->policy, size,offset,length,size-length,manager->available);
     }


     if (manager->funcs->AllocateSurfaceBuffer) {
        if (manager->funcs->AllocateSurfaceBuffer(manager, buffer) != DFB_OK) {
             dfb_surfacemanager_buddy_deallocate( manager, buffer );
             return DFB_NOVIDEOMEMORY;
        }
     }

     return result;
}

DFBResult dfb_surfacemanager_buddy_deallocate( SurfaceManager *manager,
                                               SurfaceBuffer  *buffer )
{
     DFBResult result=DFB_OK;
     int length, order;
     int height;
     int width;
     Chunk *c=NULL;
     int loops = 0;

     CoreSurface *surface = buffer->surface;

     width = surface->width;
     height = surface->height;

     /* Align width and height before malloc */
     if (surface->caps & DSCAPS_ALLOC_ALIGNED_WIDTH_4)
          width = (width + 3) & ~3;
     else if (surface->caps & DSCAPS_ALLOC_ALIGNED_WIDTH_8)
          width = (width + 7) & ~7;
     else if (surface->caps & DSCAPS_ALLOC_ALIGNED_WIDTH_16)
          width = (width + 15) & ~15;

     if (surface->caps & DSCAPS_ALLOC_ALIGNED_HEIGHT_4)
          height = (height + 3) & ~3;
     else if (surface->caps & DSCAPS_ALLOC_ALIGNED_HEIGHT_8)
          height = (height + 7) & ~7;
     else if (surface->caps & DSCAPS_ALLOC_ALIGNED_HEIGHT_16)
          height = (height + 15) & ~15;

     D_ASSERT( buffer->surface );

     D_MAGIC_ASSERT( manager, SurfaceManager );

     if (manager->funcs->DeallocateSurfaceBuffer) {
         /* Should check DFBResult */
         manager->funcs->DeallocateSurfaceBuffer(manager, buffer);
     }

     if (buffer->video.health == CSH_INVALID)
          return DFB_OK;

     if (buffer->video.access & VAF_SOFTWARE_WRITE) {
          dfb_gfxcard_flush_texture_cache(buffer);
          buffer->video.access &= ~VAF_SOFTWARE_WRITE;
     }

     if (buffer->video.access & VAF_HARDWARE_READ) {
          dfb_gfxcard_sync();
          buffer->video.access &= ~(VAF_HARDWARE_READ | VAF_HARDWARE_WRITE);
     }

     if (buffer->video.access & VAF_HARDWARE_WRITE) {
          dfb_gfxcard_wait_serial( &buffer->video.serial );
          buffer->video.access &= ~VAF_HARDWARE_WRITE;
     }

     buffer->video.health = CSH_INVALID;
     buffer->video.chunk = NULL;

     dfb_surface_notify_listeners( buffer->surface, CSNF_VIDEO );

     while (buffer->video.locked) {
          if (++loops > 1000)
               break;

          sched_yield();
     }

     if (buffer->video.locked)
          D_WARN( "Freeing chunk with a non-zero lock counter" );

     /* calculate the required length depending on limitations */
     unsigned int pitch = MAX( width, surface->min_width );

     if (pitch < manager->max_power_of_two_pixelpitch &&
     	height < manager->max_power_of_two_height)
     	pitch = 1 << direct_log2( pitch );

     if (manager->pixelpitch_align > 1) {
     	pitch += manager->pixelpitch_align - 1;
     	pitch -= pitch % manager->pixelpitch_align;
     }

     pitch = DFB_BYTES_PER_LINE( buffer->format, pitch );

     if (pitch < manager->max_power_of_two_bytepitch &&
     	height < manager->max_power_of_two_height)
     	pitch = 1 << direct_log2( pitch );

     if (manager->bytepitch_align > 1) {
     	pitch += manager->bytepitch_align - 1;
     	pitch -= pitch % manager->bytepitch_align;
     }

     length = DFB_PLANE_MULTIPLY( buffer->format, MAX( height, surface->min_height ) * pitch );

     order=direct_log2(((length-1)/manager->page_size) + 1);

     /* use buffer->video->chunk as a shortcut ?? */
     c=clist_find_buffer_and_remove(&manager->alloc[order],buffer);

     if(c)
     {
     	int size;

     	chunk_location(manager,c,NULL,&size);

     	manager->available += size;

     	D_ALLOCATION("Core/SurfaceMgrBuddy -> freed %d bytes, %d now available.\n",size,manager->available);

     	//mark the chunk as free
     	c->buffer=NULL;

     	//try to merge buddies; if it didn't merge, put it back in a free list
     	if(!merge_buddies(manager,c,order))
     		clist_push(&manager->free[order],c);
     }
     else
     {
     	D_ERROR("surfacemanager could not find specified buffer to deallocate!\n");
     	result=DFB_INVARG;
     }

     return result;
}

DFBResult dfb_surfacemanager_buddy_assure_video( SurfaceManager *manager,
                                                 SurfaceBuffer  *buffer )
{
     DFBResult    ret;
     CoreSurface *surface = buffer->surface;

     D_MAGIC_ASSERT( manager, SurfaceManager );

     if (manager->suspended)
          return DFB_NOVIDEOMEMORY;

     switch (buffer->video.health) {
          case CSH_STORED:

               return DFB_OK;

          case CSH_INVALID:

               ret = dfb_surfacemanager_buddy_allocate( manager, buffer );
               if (ret == DFB_NOVIDEOMEMORY) {
                    D_WARN( "ran out of video memory" );
#ifdef DEBUG_ALLOCATION
                    dfb_surfacemanager_enumerate_chunks( manager,
                    surfacemanager_chunk_callback,
                    NULL );
#endif
               }
               if (ret)
                    return ret;

               /* FALL THROUGH, because after successful allocation
                  the surface health is CSH_RESTORE */

          case CSH_RESTORE:
               if (buffer->system.health != CSH_STORED)
                    D_BUG( "system/video instances both not stored!" );

               if (buffer->flags & SBF_WRITTEN) {
                    int   i;
                    char *src = buffer->system.addr;
                    char *dst = dfb_system_video_memory_virtual( buffer->video.offset);

                    dfb_surface_video_access_by_software( buffer, DSLF_WRITE );

                    for (i=0; i<surface->height; i++) {
                         direct_memcpy( dst, src,
                                        DFB_BYTES_PER_LINE(buffer->format, surface->width) );
                         src += buffer->system.pitch;
                         dst += buffer->video.pitch;
                    }

                    if (buffer->format == DSPF_YV12 || buffer->format == DSPF_I420) {
                         for (i=0; i<surface->height; i++) {
                              direct_memcpy( dst, src, DFB_BYTES_PER_LINE(buffer->format,
                                                                          surface->width / 2) );
                              src += buffer->system.pitch / 2;
                              dst += buffer->video.pitch  / 2;
                         }
                    }
                    else if (buffer->format == DSPF_NV12 || buffer->format == DSPF_NV21) {
                         for (i=0; i<surface->height/2; i++) {
                              direct_memcpy( dst, src, DFB_BYTES_PER_LINE(buffer->format,
                                                                          surface->width) );
                              src += buffer->system.pitch;
                              dst += buffer->video.pitch;
                         }
                    }
                    else if (buffer->format == DSPF_NV16) {
                         for (i=0; i<surface->height; i++) {
                              direct_memcpy( dst, src, DFB_BYTES_PER_LINE(buffer->format,
                                                                          surface->width) );
                              src += buffer->system.pitch;
                              dst += buffer->video.pitch;
                         }
                    }
               }

               buffer->video.health             = CSH_STORED;

               dfb_surface_notify_listeners( surface, CSNF_VIDEO );

               return DFB_OK;

          default:
               break;
     }

     D_BUG( "unknown video instance health" );
     return DFB_BUG;
}


/** internal functions NOT locking the surfacemanager **/

// provides the offset and length of a chunk
static void
chunk_location(SurfaceManager *manager, Chunk *c, int *offset, int *length)
{
    register int cid=(int)(c-manager->chunks);
    register int order=manager->order - (direct_log2(cid+1)-1);
    register int l=(1<<order)*manager->page_size;

    if(length)
        *length = l;

    if(offset)
        *offset = (cid % (1<<(manager->order - order))) * l + manager->heap_offset;
}

// push an item on the top of a list
static void
clist_push(Chunk **list, Chunk *item)
{
    D_ASSERT(item->next==NULL);
    item->next=*list;
    *list=item;
}

// pop an item from the top of a list
static Chunk *
clist_pop(Chunk **list)
{
    Chunk *c=*list;
    if(*list)
        *list=(*list)->next;

    if(c)
        c->next=NULL;

    return c;
}

/*
 * search a list for a chunk that is associated to the specified buffer and
 * remove it.
 */
static Chunk *
clist_find_buffer_and_remove(Chunk **list, SurfaceBuffer *buffer)
{
    Chunk *c=NULL;

    //empty list?
    if(*list==CLIST_EMPTY) return NULL;

    c=*list;

    if(c->buffer==buffer)
    {
        //cut the top of the list
        *list=c->next;
        c->next=NULL;
    }
    else
    {
        Chunk *prev=c;
        c=c->next;

        for(;;)
        {
            //end of list
            if(c==NULL) break;

            if(c->buffer==buffer)
            {
                //shortcut the list
                prev->next=c->next;
                c->next=NULL;
                break;
            }

            prev=c;
            c=c->next;
        }
    }
    return c;
}

// search a list for the specified chunk and remove it.  return true if found.
static bool
clist_remove_chunk(Chunk **list, Chunk *item)
{
    Chunk *c=NULL;

    //empty list?
    if(*list==CLIST_EMPTY) return false;

    c=*list;

    if(c==item)
    {
        //cut the top of the list
        *list=c->next;
        c->next=NULL;
        return true;
    }
    else
    {
        Chunk *prev=c;
        c=c->next;

        for(;;)
        {
            //end of list
            if(c==NULL) break;

            if(c==item)
            {
                //shortcut the list
                prev->next=c->next;
                c->next=NULL;
                break;
            }

            prev=c;
            c=c->next;
        }

        if(c==NULL)
        {
            D_DEBUG("couldnt remove chunk from list ???\n");
            return false;
        }
        else
            return true;
    }
}

/* take a free chunk of the specified order and split it in two chunk of a
 * lower order.  If no free chunks of this order are available, split a chunk
 * of an higher order then try again.
 */
static DFBResult
split_free_chunk(SurfaceManager *manager, int order)
{
    DFBResult result=DFB_OK;
    Chunk *parent=NULL;
    int i;
    int parent_id;

    D_ASSERT(manager);
    D_ASSERT(manager->chunks);
    D_ASSERT(order!=0);
    //D_ASSERT(order <= manager->order);

    // try taking a chunk
    parent=clist_pop(&manager->free[order]);

    //if there's no available chunk of that order...
    if(parent==NULL)
    {
        //impossible to split chunks, return an error code
        if(order >= manager->order)
            return DFB_NOVIDEOMEMORY;

        //split a chunk of an higher order
        result=split_free_chunk(manager,order+1);
        if(result!=DFB_OK) return result;

        //try again
        parent=clist_pop(&manager->free[order]);
    }

    D_DEBUG("split_free_chunk, order %d  parent: %08X chunks: %08X\n",order,parent,manager->chunks);

    //mark chunk as split
    parent->buffer=CHUNK_SPLIT;

    //find parent chunk id
    parent_id=(int)(parent - manager->chunks);

    //push back the two new chunks in the proper free list
    for(i=0;i<2;i++)
    {
        register Chunk *child=NULL;

        #ifdef ALLOCATE_FROM_BOTTOM
            child = &manager->chunks[parent_id*2 + i];
        #else
            child = &manager->chunks[parent_id*2 + 1-i];
        #endif

        chunk_init(child);
        clist_push(&manager->free[order-1],child);
    }

    return result;
}

// try to merge a chunk with its buddy. return true if successful.
static bool
merge_buddies(SurfaceManager *manager, Chunk *c, int order)
{
    bool result;
    bool merged;
    Chunk *parent=NULL;
    Chunk *buddy=NULL;
    int cid = (int)(c - manager->chunks);

    //the root cannot be split
    if(cid==ROOT_CHUNK)
        return false;

    //find buddy according to the chunk id
    buddy  = &manager->chunks[(cid&1)?cid-1:cid+1];

    //chunks with busy buddies (either allocated or split) cannot be merged
    if(buddy->buffer!=NULL)
        return false;

    result=clist_remove_chunk(&manager->free[order],buddy);
    D_ASSERT(result==true);

    //find parent according to the chunk id
    parent = &manager->chunks[cid>>1];
    D_ASSERT(parent->buffer==CHUNK_SPLIT);
    parent->buffer=NULL;

    merged=merge_buddies(manager,parent,order+1);

    //if the parent couldn't merge, put it back in a free list
    if(!merged)
        clist_push(&manager->free[order+1],parent);

    return true;
}
