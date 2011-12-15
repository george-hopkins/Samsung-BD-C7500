
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <sys/time.h>
#include <directfb.h>
#include <cairo.h>
#include <cairo-directfb.h>
#include <bdvd.h>

#include <direct/debug.h>

#define TRACE(x) { /*fprintf(stderr,x);*/ }

#define DFBCHECK(x...)                                         \
  {                                                            \
    DFBResult err = x;                                         \
                                                               \
    if (err != DFB_OK)                                         \
      {                                                        \
        fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ ); \
        DirectFBErrorFatal( #x, err );                         \
      }                                                        \
  }
  
#define PI 3.14159265

void cairo_ellipse(cairo_t* cairo, double x, double y, double xradius, double yradius)
{
	cairo_save (cairo);
	cairo_translate (cairo, x, y);
	cairo_scale (cairo, xradius, yradius);
	cairo_arc (cairo, 0., 0., 1., 0., 2 * PI);
	cairo_restore(cairo);
}

void draw_screen(cairo_t* cairo, int width, int height)
{
	int i;
	
	//draw a few concentric ellipses
	for(i=0;i<5;i++)
	{
		cairo_new_path(cairo);
		cairo_ellipse(cairo,width/2,height/2,200-i*10,200);
		cairo_set_source_rgba(cairo,(double)(i+1)/5.,(double)(i+1)/5.,1,1);
		cairo_fill(cairo);
	}
	
	//draw a star in the center
	cairo_save(cairo);
	cairo_new_path(cairo);
	cairo_translate(cairo,width/2,height/2);
	cairo_rotate(cairo,PI/5.);
	cairo_move_to(cairo,0,100);
	for(i=0;i<5;i++)
	{
		cairo_rotate(cairo,PI/5.);
		cairo_line_to(cairo,0,40);
		cairo_rotate(cairo,PI/5.);
		cairo_line_to(cairo,0,100);
	}
	cairo_restore(cairo);
	cairo_set_source_rgba(cairo,0.7,0.5,0.5,1);
	cairo_fill_preserve(cairo);
	cairo_set_source_rgba(cairo,0,0,0,1);
	cairo_stroke(cairo);
	
	//draw a semi-transparent green rectangle on top
	cairo_new_path(cairo);
	cairo_rectangle(cairo,width/3,height/2-25,width/3,50);
	cairo_set_source_rgba(cairo,0.2,0.7,0.3,0.5);
	cairo_fill_preserve(cairo);
	cairo_set_source_rgba(cairo,0.2,0.7,0.3,1);
	cairo_stroke(cairo);
}

int main(int argc, char* argv[])
{
    int ret = EXIT_SUCCESS;

	DFBSurfaceDescription dsc;
	IDirectFB *dfb = NULL;
	IDirectFBSurface *surface = NULL;
	IDirectFBDisplayLayer* layer=NULL;
	cairo_surface_t* cairo_surface=NULL;
	cairo_t* cairo=NULL;
	bdvd_dfb_ext_h dfb_ext;
	bdvd_dfb_ext_layer_settings_t dfb_ext_settings;
	int i;
	int width;
	int height;

    /* bdvd_init begin */
    bdvd_display_h bdisplay = NULL;
    if (bdvd_init(BDVD_VERSION) != bdvd_ok) {
        D_ERROR("Could not init bdvd\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    if ((bdisplay = bdvd_display_open(B_ID(0))) == NULL) {
        printf("bdvd_display_open failed\n");
        ret = EXIT_FAILURE;
        goto out;
    }
    /* bdvd_init end */

	dfb=bdvd_dfb_ext_open(2);

	dfb_ext=bdvd_dfb_ext_layer_open(bdvd_dfb_ext_bd_j_graphics_layer);
	bdvd_dfb_ext_layer_get(dfb_ext, &dfb_ext_settings);
	DFBCHECK ( dfb->GetDisplayLayer(dfb,dfb_ext_settings.id,&layer) );
	DFBCHECK ( layer->SetCooperativeLevel(layer,DLSCL_ADMINISTRATIVE ));
	DFBCHECK ( layer->SetLevel(layer, 1));
	DFBCHECK ( layer->GetSurface(layer,&surface) );
	DFBCHECK ( surface->Clear(surface, 0, 0, 0, 0) );

	surface->GetSize(surface,&width,&height);

	DFBSurfacePixelFormat pixfmt;
	surface->GetPixelFormat(surface,&pixfmt);
	
	switch(pixfmt)
	{
	case DSPF_ARGB:
		fprintf(stderr,"surface is DSPF_ARGB\n");
		break;
	default:
		fprintf(stderr,"surface is %d\n",pixfmt);
		break;
	}
	
	cairo_surface=cairo_directfb_surface_create(dfb,surface);
	cairo=cairo_create(cairo_surface);
	
	//-------------------------------------------------
	
	draw_screen(cairo,width,height);
	surface->Flip(surface,0,DSFLIP_NONE);
	
	fprintf(stderr,"Press Enter to switch anti-aliasing off then redraw...");
	getchar();
	
	//-------------------------------------------------
	
	cairo_set_antialias(cairo,CAIRO_ANTIALIAS_NONE);
	draw_screen(cairo,width,height);
	surface->Flip(surface,0,DSFLIP_NONE);
	
	fprintf(stderr,"Press Enter to exit...");
	getchar();
	
	//-------------------------------------------------

	cairo_destroy(cairo);
    cairo_surface_destroy (cairo_surface);

out:

	if (surface) surface->Release(surface);
	if (layer) layer->Release(layer);

	if (dfb_ext) bdvd_dfb_ext_layer_close(dfb_ext);
	bdvd_dfb_ext_close();

    /* bdvd_uninit begin */
    if (bdisplay) bdvd_display_close(bdisplay);
    bdvd_uninit();
    /* bdvd_uninit end */

    return ret;
}
