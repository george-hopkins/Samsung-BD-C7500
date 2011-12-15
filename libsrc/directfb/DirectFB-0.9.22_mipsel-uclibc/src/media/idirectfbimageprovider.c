/*
   (c) Copyright 2000-2002  convergence integrated media GmbH.
   (c) Copyright 2002       convergence GmbH.

   All rights reserved.

   Written by Denis Oliver Kropp <dok@directfb.org>,
              Andreas Hundt <andi@fischlustig.de> and
              Sven Neumann <sven@convergence.de>.

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

#include <config.h>

#include <stddef.h>

#include <directfb.h>

#include <direct/interface.h>
#include <direct/mem.h>

#include <media/idirectfbimageprovider.h>
#include <media/idirectfbdatabuffer.h>
#include <display/idirectfbsurface.h>

#include <core/surfaces.h>

#include <fusion/arena.h>
#include <fusion/lock.h>

#include <core/core.h>
#include <core/core_parts.h>

#include <stdlib.h>

typedef struct {
    FusionSkirmish lock;
    bool enabled;
    bool force_premult;
    IDirectFBSurfaceManager * surface_manager;
    IDirectFBSurface * intermediate_surface;
    bool isMemoryMode256MB;
} ConverterSingleton;

static ConverterSingleton *converter_singleton = NULL;

extern IDirectFB *idirectfb_singleton;

SID_SurfaceOutputBufferInfo sid_info;

DFB_CORE_PART( converter, 0, sizeof(ConverterSingleton) )

static DFBResult
IDirectFBImageProvider_AddRef( IDirectFBImageProvider *thiz )
{
     return DFB_UNIMPLEMENTED;
}

static DFBResult
IDirectFBImageProvider_Release( IDirectFBImageProvider *thiz )
{
     return DFB_UNIMPLEMENTED;
}

static DFBResult
IDirectFBImageProvider_GetSurfaceDescription( IDirectFBImageProvider *thiz,
                                              DFBSurfaceDescription  *ret_dsc )
{
     DFBResult ret = DFB_OK;

     if (!ret_dsc)
          return DFB_INVARG;

     if ((ret = thiz->GetSurfaceDescriptionUnaltered(thiz, ret_dsc)) != DFB_OK) {
          D_DERROR(ret, "Media/ImageProvider: GetSurfaceDescriptionUnaltered failed\n");
          goto out;
     }

     ret_dsc->flags |= (DFBSurfaceDescriptionFlags)(DSDESC_PIXELFORMAT);
     ret_dsc->pixelformat = DSPF_ARGB;

out:

     if (ret != DFB_OK) ret_dsc->flags = 0;

     return ret;
}

static DFBResult
IDirectFBImageProvider_GetImageDescription( IDirectFBImageProvider *thiz,
                                            DFBImageDescription    *ret_dsc )
{
     if (!ret_dsc)
          return DFB_INVARG;

     ret_dsc->caps = DICAPS_NONE;

     return DFB_UNIMPLEMENTED;
}

static DFBResult
IDirectFBImageProvider_RenderTo( IDirectFBImageProvider *thiz,
                                 IDirectFBSurface       *destination,
                                 const DFBRectangle     *destination_rect )
{
    DFBResult ret = DFB_OK;
	DFBResult ret1 = DFB_OK;

    IDirectFBSurface_data *dst_data;
    CoreSurface           *dst_surface;

    DFBSurfaceDescription image_surface_desc;

	int height_temp;
	int num_segments;

    int destination_width;
    int destination_height;

    bool destination_differs = false;
    DFBSurfaceBlittingFlags old_blittingflags = DSBLIT_NOFX;
    bool need_only_alphapremultiply = false;



	DFBRectangle my_srect;


	DFBRectangle my_drect;





    D_ASSERT(thiz != NULL);
    D_ASSERT(destination != NULL);

    if (converter_singleton->enabled) {
        dst_data = (IDirectFBSurface_data*) destination->priv;
        if (!dst_data)
             return DFB_DEAD;

        dst_surface = dst_data->surface;
        if (!dst_surface)
             return DFB_DESTROYED;

        if ((ret = thiz->GetSurfaceDescriptionUnaltered(thiz, &image_surface_desc)) != DFB_OK) {
            D_DERROR(ret, "Media/ImageProvider: GetSurfaceDescriptionUnaltered failed\n");
            return ret;
        }

        /* GetSurfaceDescriptionUnaltered should at least have width, height and pixel format */
        if (!(image_surface_desc.flags & DSDESC_WIDTH) ||
            !(image_surface_desc.flags & DSDESC_HEIGHT) ||
            !(image_surface_desc.flags & DSDESC_PIXELFORMAT)) {
            D_ERROR("missing DFBSurfaceDescriptionFlags\n");
            return DFB_FAILURE;
        }

        if ((ret = destination->GetSize( destination, &destination_width, &destination_height )) != DFB_OK) {
            D_DERROR(ret, "Media/ImageProvider: GetSize failed\n");
            return ret;
        }


		/*Ensure that the segment height is large enough to allow at least 4 lines of scaled destination segment*/

		if (thiz->PipelineImageDecodingAndScaling.imageFormat == JPEG)
		{

			if (destination_rect)
			{
				if (destination_rect->h > destination_height)
				{
					D_DERROR(DFB_INVAREA, "\n\nDestination rectangle bigger than destination surface\n\n");
				}
				thiz->PipelineImageDecodingAndScaling.segment_height = ((4*(image_surface_desc.height/destination_rect->h + 1)) & 0xfffffff0) | 0x10;
			}
			else
			{
				thiz->PipelineImageDecodingAndScaling.segment_height = ((4*(image_surface_desc.height/destination_height + 1)) & 0xfffffff0) | 0x10;
			}

			if (thiz->PipelineImageDecodingAndScaling.segment_height < 64)
			{
				thiz->PipelineImageDecodingAndScaling.segment_height = 64;
			}

			num_segments = (image_surface_desc.height + thiz->PipelineImageDecodingAndScaling.segment_height - 1)/thiz->PipelineImageDecodingAndScaling.segment_height;


			my_srect.h = thiz->PipelineImageDecodingAndScaling.segment_height;
			my_srect.w = image_surface_desc.width;
			my_srect.x = 0;
			my_srect.y = 0;



			if (destination_rect)
			{
				my_drect.h = (thiz->PipelineImageDecodingAndScaling.segment_height * destination_rect->h)/image_surface_desc.height;
				my_drect.w = destination_rect->w;
				my_drect.x = 0;
				my_drect.y = 0;
			}
			else
			{
				my_drect.h = (thiz->PipelineImageDecodingAndScaling.segment_height * destination_height)/image_surface_desc.height;
				my_drect.w = destination_width;
				my_drect.x = 0;
				my_drect.y = 0;
			}


		}


        if (!destination_differs) destination_differs = ((destination_width != image_surface_desc.width) || (destination_height != image_surface_desc.height));

		/*PR12701*/
		if ((dst_surface->format != DSPF_ARGB) || (getenv("sid_use_intermediate_buffer")))
		{
        	if (!destination_differs)
        	{
				destination_differs = (dst_surface->format != image_surface_desc.pixelformat);
			}
		}

        /* Always force surfaces in video memory to avoid cost of the initial
         * swapping from system to video or the use of the software rasterizer.
         * Check if the ImageProvider is trying to force capabilities,
         * such as video only.
         */
        if (image_surface_desc.flags & DSDESC_CAPS) {
            void * bogus_pointer;
            int pitch;
            int desired_pitch;
            int destination_aligned_width = destination_width;

            image_surface_desc.caps |= (DFBSurfaceCapabilities)(DSCAPS_VIDEOONLY);

            /* Check the surface caps, and verify if video only was already set */
            if (!destination_differs) destination_differs = ((dst_surface->caps & image_surface_desc.caps) != image_surface_desc.caps);

            if ((ret = destination->Lock( destination, DSLF_READ, &bogus_pointer, &pitch )) != DFB_OK) {
                D_DERROR(ret, "Media/ImageProvider: Lock failed\n");
                return ret;
            }

            if ((ret = destination->Unlock( destination )) != DFB_OK) {
                D_DERROR(ret, "Media/ImageProvider: Unlock failed\n");
                return ret;
            }

            if (image_surface_desc.caps & DSCAPS_ALLOC_ALIGNED_WIDTH_4)
                destination_aligned_width = (destination_aligned_width + 3) & ~3;
            else if (image_surface_desc.caps & DSCAPS_ALLOC_ALIGNED_WIDTH_8)
                destination_aligned_width = (destination_aligned_width + 7) & ~7;
            else if (image_surface_desc.caps & DSCAPS_ALLOC_ALIGNED_WIDTH_16)
                destination_aligned_width = (destination_aligned_width + 15) & ~15;

            desired_pitch = destination_aligned_width*DFB_BYTES_PER_PIXEL(image_surface_desc.pixelformat);
            /* Pitch should always be at least 32-bit aligned */
            desired_pitch = ((desired_pitch + 3) & ~3);

 /*PR12701*/ if (getenv("sid_use_intermediate_buffer"))
 			 {
            	if (!destination_differs)
            	{
					destination_differs = (desired_pitch != pitch);
				}
        	 }
        }
        else {
            image_surface_desc.flags |= (DFBSurfaceDescriptionFlags)(DSDESC_CAPS);
            image_surface_desc.caps = DSCAPS_VIDEOONLY;
        }

		if ((!destination_differs)&&(thiz->PipelineImageDecodingAndScaling.imageFormat == JPEG)&&(destination_rect))
		{
			destination_differs = ((destination_rect->h != image_surface_desc.height) || (destination_rect->w != image_surface_desc.width)) ? true : false;
		}


        /* We will proceed in doing the premultiplication */
        if (!destination_differs)
        {
            destination_differs = ((dst_surface->caps & DSCAPS_PREMULTIPLIED) || converter_singleton->force_premult);
            need_only_alphapremultiply = ((destination_differs == true) && (dst_surface->format == DSPF_ARGB)) ? true : false;
        }

        /* If the destination surface does not match the expected ImageProvider
         * description, we convert, else with skip to RenderToUnaltered.
         */
        if (destination_differs) {
            int intermediate_pitch;
            int intermediate_aligned_width = image_surface_desc.width;
            int intermediate_aligned_height = image_surface_desc.height;
            DFBSurfaceBlittingFlags new_blittingflags = DSBLIT_NOFX;

            fusion_skirmish_prevail( &converter_singleton->lock );

            old_blittingflags = dst_data->state.blittingflags;

            if (!need_only_alphapremultiply)
            {
                if (image_surface_desc.flags & DSDESC_CAPS)
                {
                    if (image_surface_desc.caps & DSCAPS_ALLOC_ALIGNED_HEIGHT_4)
                        intermediate_aligned_height = (intermediate_aligned_height + 3) & ~3;
                    else if (image_surface_desc.caps & DSCAPS_ALLOC_ALIGNED_HEIGHT_8)
                        intermediate_aligned_height = (intermediate_aligned_height + 7) & ~7;
                    else if (image_surface_desc.caps & DSCAPS_ALLOC_ALIGNED_HEIGHT_16)
                        intermediate_aligned_height = (intermediate_aligned_height + 15) & ~15;

                    if (image_surface_desc.caps & DSCAPS_ALLOC_ALIGNED_WIDTH_4)
                        intermediate_aligned_width = (intermediate_aligned_width + 3) & ~3;
                    else if (image_surface_desc.caps & DSCAPS_ALLOC_ALIGNED_WIDTH_8)
                        intermediate_aligned_width = (intermediate_aligned_width + 7) & ~7;
                    else if (image_surface_desc.caps & DSCAPS_ALLOC_ALIGNED_WIDTH_16)
                        intermediate_aligned_width = (intermediate_aligned_width + 15) & ~15;
                }

                intermediate_pitch = intermediate_aligned_width*DFB_BYTES_PER_PIXEL(image_surface_desc.pixelformat);
                /* Pitch should always be at least 32-bit aligned */
                intermediate_pitch = ((intermediate_pitch + 3) & ~3);

                /* This goes beyond the supported max size for the surface manager, let's create
                 * out of the regular pool
                 */
                if ((intermediate_pitch * intermediate_aligned_height) > sid_info.buffer_size) {

					if (thiz->PipelineImageDecodingAndScaling.imageFormat == JPEG)
					{
						height_temp = image_surface_desc.height;
						image_surface_desc.height = thiz->PipelineImageDecodingAndScaling.segment_height;
					}

                    if ((ret = idirectfb_singleton->CreateSurface(idirectfb_singleton, &image_surface_desc, &converter_singleton->intermediate_surface)) != DFB_OK) {
                        D_DERROR(ret, "Media/ImageProvider: CreateSurface failed\n");
                        goto out;
                    }

					if (thiz->PipelineImageDecodingAndScaling.imageFormat == JPEG)
					{
						image_surface_desc.height = height_temp;
					}




                }
                else {
                    D_ASSERT(converter_singleton->surface_manager != NULL);

                    /* If the intermediate surface is no larger than ~8MB (size of a 1080i/p), then a
                     * special surface manager is used. This to avoid video memory fragmentation. If surfaces
                     * were allocated then deallocated, this would create a lot of fragmentation. A 8MB buffer is
                     * kept and released only when render_convert_release is called.
                     */
					if (thiz->PipelineImageDecodingAndScaling.imageFormat == JPEG)
					{
						height_temp = image_surface_desc.height;
						image_surface_desc.height = thiz->PipelineImageDecodingAndScaling.segment_height;
					}

                    if ((ret = converter_singleton->surface_manager->CreateSurface(converter_singleton->surface_manager, &image_surface_desc, &converter_singleton->intermediate_surface)) != DFB_OK) {
                        D_DERROR(ret, "Media/ImageProvider: CreateSurface failed\n");
                        goto out;
                    }

					if (thiz->PipelineImageDecodingAndScaling.imageFormat == JPEG)
					{
						image_surface_desc.height = height_temp;
					}
                }



				if (thiz->PipelineImageDecodingAndScaling.imageFormat == JPEG)
				{
					thiz->PipelineImageDecodingAndScaling.segmentedDecodingOn = true;
					for (;num_segments;num_segments--)
					{
                		if ((ret = thiz->RenderToUnaltered(thiz, converter_singleton->intermediate_surface, NULL)) != DFB_OK) {
                    		D_DERROR(ret, "Media/ImageProvider: RenderToUnaltered failed\n");
                    		goto out;
               			 }

						if (DFB_PIXELFORMAT_IS_INDEXED(image_surface_desc.pixelformat) && DFB_PIXELFORMAT_IS_INDEXED(dst_surface->format)) {
                    		IDirectFBPalette * palette;

                    		if ((ret = converter_singleton->intermediate_surface->GetPalette( converter_singleton->intermediate_surface, &palette )) != DFB_OK) {
                        		D_DERROR(ret, "Media/ImageProvider: GetPalette failed\n");
                        		goto out;
                    		}

                    		if ((ret = destination->SetPalette( destination, palette )) != DFB_OK) {
                        		D_DERROR(ret, "Media/ImageProvider: SetPalette failed\n");
                        		goto out;
                    		}
                		}

                		/* If caps premultiplied, store as pre-multiplied. Also scaling without premult source causes
                 				* bleeding, must work around this.
                 				* Should we backup ?
                 				*/
                		if (((dst_surface->caps & DSCAPS_PREMULTIPLIED) || converter_singleton->force_premult)) {
                    		new_blittingflags = DSBLIT_SRC_PREMULTIPLY;
                		}

                		if ((ret = destination->SetBlittingFlags( destination, new_blittingflags )) != DFB_OK) {
                    		D_DERROR(ret, "Media/ImageProvider: SetBlittingFlags failed\n");
                    		goto out;
                		}

                		if ((ret = destination->StretchBlit(destination, converter_singleton->intermediate_surface, &my_srect, &my_drect)) != DFB_OK) {
                    		D_DERROR(ret, "Media/ImageProvider: StretchBlit failed\n");
                    		goto out;
                		}

						thiz->PipelineImageDecodingAndScaling.first_segment = false;
						my_drect.y += my_drect.h;
					}

				}
				else
				{
                if ((ret = thiz->RenderToUnaltered(thiz, converter_singleton->intermediate_surface, NULL)) != DFB_OK) {
                    D_DERROR(ret, "Media/ImageProvider: RenderToUnaltered failed\n");
                    goto out;
                }

                if (DFB_PIXELFORMAT_IS_INDEXED(image_surface_desc.pixelformat) && DFB_PIXELFORMAT_IS_INDEXED(dst_surface->format)) {
                    IDirectFBPalette * palette;

                    if ((ret = converter_singleton->intermediate_surface->GetPalette( converter_singleton->intermediate_surface, &palette )) != DFB_OK) {
                        D_DERROR(ret, "Media/ImageProvider: GetPalette failed\n");
                        goto out;
                    }

                    if ((ret = destination->SetPalette( destination, palette )) != DFB_OK) {
                        D_DERROR(ret, "Media/ImageProvider: SetPalette failed\n");
                        goto out;
                    }
                }

                /* If caps premultiplied, store as pre-multiplied. Also scaling without premult source causes
                 * bleeding, must work around this.
                 * Should we backup ?
                 */
                if (((dst_surface->caps & DSCAPS_PREMULTIPLIED) || converter_singleton->force_premult)) {
                    new_blittingflags = DSBLIT_SRC_PREMULTIPLY;
                }

                if ((ret = destination->SetBlittingFlags( destination, new_blittingflags )) != DFB_OK) {
                    D_DERROR(ret, "Media/ImageProvider: SetBlittingFlags failed\n");
                    goto out;
                }

                if ((ret = destination->StretchBlit(destination, converter_singleton->intermediate_surface, NULL, destination_rect)) != DFB_OK) {
                    D_DERROR(ret, "Media/ImageProvider: StretchBlit failed\n");
                    goto out;
                }
            }

            }
            else
            {
                if ((ret = thiz->RenderToUnaltered(thiz, destination, NULL)) != DFB_OK) {
                    D_DERROR(ret, "Media/ImageProvider: RenderToUnaltered failed\n");
                    goto out;
                }

                /* If caps premultiplied, store as pre-multiplied. Also scaling without premult source causes
                 * bleeding, must work around this.
                 * Should we backup ?
                 */
                if (((dst_surface->caps & DSCAPS_PREMULTIPLIED) || converter_singleton->force_premult)) {
                    new_blittingflags = DSBLIT_SRC_PREMULTIPLY;
                }

                if ((ret = destination->SetBlittingFlags( destination, new_blittingflags )) != DFB_OK) {
                    D_DERROR(ret, "Media/ImageProvider: SetBlittingFlags failed\n");
                    goto out;
                }

                if ((ret = destination->StretchBlit(destination, destination, NULL, destination_rect)) != DFB_OK) {
                    D_DERROR(ret, "Media/ImageProvider: StretchBlit failed\n");
                    goto out;
                }
            }
        }
        else {
            if ((ret = thiz->RenderToUnaltered(thiz, destination, destination_rect)) != DFB_OK) {
                D_DERROR(ret, "Media/ImageProvider: RenderToUnaltered failed\n");
                goto out;
            }
        }
    }
    else {
        if ((ret = thiz->RenderToUnaltered(thiz, destination, destination_rect)) != DFB_OK) {
            D_DERROR(ret, "Media/ImageProvider: RenderToUnaltered failed\n");
            goto out;
        }
    }

out:

    if (destination_differs) {
        if ((ret1 = destination->SetBlittingFlags( destination, old_blittingflags )) != DFB_OK) {
            D_DERROR(ret1, "Media/ImageProvider: SetBlittingFlags failed\n");
        }

        dfb_state_set_source(&dst_data->state, NULL);

        /* Release is supposed to cause an EngineSync, so no need to WaitIdle */
        if (converter_singleton->intermediate_surface) {
            converter_singleton->intermediate_surface->Release(converter_singleton->intermediate_surface);
            converter_singleton->intermediate_surface = NULL;
        }

        fusion_skirmish_dismiss( &converter_singleton->lock );
    }



if (ret)
    return ret;
else
	return ret1;

}

static DFBResult
IDirectFBImageProvider_SetRenderCallback( IDirectFBImageProvider *thiz,
                                          DIRenderCallback        callback,
                                          void                   *callback_data )
{
     return DFB_UNIMPLEMENTED;
}

static void
IDirectFBImageProvider_Construct( IDirectFBImageProvider *thiz )
{
     thiz->AddRef                = IDirectFBImageProvider_AddRef;
     thiz->Release               = IDirectFBImageProvider_Release;
     thiz->GetSurfaceDescription = IDirectFBImageProvider_GetSurfaceDescription;
     thiz->GetImageDescription   = IDirectFBImageProvider_GetImageDescription;
     thiz->RenderTo              = IDirectFBImageProvider_RenderTo;
     thiz->SetRenderCallback     = IDirectFBImageProvider_SetRenderCallback;
}


DFBResult
IDirectFBImageProvider_CreateFromBuffer( IDirectFBDataBuffer     *buffer,
                                         IDirectFBImageProvider **interface )
{
     DFBResult                            ret;
     DirectInterfaceFuncs                *funcs = NULL;
     IDirectFBDataBuffer_data            *buffer_data;
     IDirectFBImageProvider              *imageprovider;
     IDirectFBImageProvider_ProbeContext  ctx;

     /* Get the private information of the data buffer. */
     buffer_data = (IDirectFBDataBuffer_data*) buffer->priv;
     if (!buffer_data)
          return DFB_DEAD;

     /* Provide a fallback for image providers without data buffer support. */
     ctx.filename = buffer_data->filename;

     /* Wait until 32 bytes are available. */
     ret = buffer->WaitForData( buffer, 32 );
     if (ret)
          return ret;

     /* Read the first 32 bytes. */
     ret = buffer->PeekData( buffer, 32, 0, ctx.header, NULL );
     if (ret)
          return ret;

     /* Find a suitable implementation. */
     ret = DirectGetInterface( &funcs, "IDirectFBImageProvider", NULL, DirectProbeInterface, &ctx );
     if (ret)
          return ret;

     DIRECT_ALLOCATE_INTERFACE( imageprovider, IDirectFBImageProvider );

     /* Initialize interface pointers. */
     IDirectFBImageProvider_Construct( imageprovider );

     fusion_skirmish_prevail( &converter_singleton->lock );

     if (converter_singleton->enabled && converter_singleton->surface_manager == NULL)
     {
         converter_singleton->surface_manager = (IDirectFBSurfaceManager *)sid_info.surface_manager;
     }

     fusion_skirmish_dismiss( &converter_singleton->lock );

     /* Construct the interface. */
     ret = funcs->Construct( imageprovider, buffer );
     if (ret)
          return ret;

     *interface = imageprovider;

     return DFB_OK;
}

DFBResult static
dfb_converter_initialize( CoreDFB  *core,
                          void *data_local,
                          void *data_shared )
{
     D_ASSERT( converter_singleton == NULL );

     converter_singleton = data_shared;

     fusion_skirmish_init( &converter_singleton->lock, "Converter Core" );

     if (getenv("force_no_sid") || getenv("no_sid"))
     {
         converter_singleton->enabled = (dfb_gfxcard_memory_length() > 136*1024*1024);
     }
     else
     {
         converter_singleton->enabled = true;
     }

     sid_info.enabled         = converter_singleton->enabled;
     sid_info.buffer_size     = 0;
     sid_info.surface_manager = NULL;

     if (getenv("converter_force_premult")) {
         D_INFO("converter: forcing alpha premultiplication\n");
         converter_singleton->force_premult = true;
     }
     else {
         converter_singleton->force_premult = false;
     }

     converter_singleton->isMemoryMode256MB = (getenv("MemoryMode256MB") != NULL) ? true : false;

     return DFB_OK;
}

DFBResult static
dfb_converter_join( CoreDFB  *core,
                    void     *data_local,
                    void     *data_shared )
{
     D_ASSERT( converter_singleton == NULL );

     converter_singleton = data_shared;

     return DFB_OK;
}

static DFBResult
dfb_converter_shutdown( CoreDFB  *core,
                        bool      emergency )
{
     D_ASSERT( converter_singleton != NULL );

     converter_singleton = NULL;

     return DFB_OK;
}

static DFBResult
dfb_converter_leave( CoreDFB  *core,
                     bool      emergency )
{
     D_ASSERT( converter_singleton != NULL );

     fusion_skirmish_destroy( &converter_singleton->lock );

     converter_singleton = NULL;

     return DFB_OK;
}

static DFBResult
dfb_converter_suspend( CoreDFB *core )
{
     D_ASSERT( converter_singleton != NULL );

     return DFB_OK;
}

static DFBResult
dfb_converter_resume( CoreDFB *core )
{
     D_ASSERT( converter_singleton != NULL );

     return DFB_OK;
}
