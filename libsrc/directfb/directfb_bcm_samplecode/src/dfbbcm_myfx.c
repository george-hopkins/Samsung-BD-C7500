
#include <directfb.h>
#include <direct/util.h>
#include <direct/debug.h>
#include <bdvd.h>
#include <math.h>

#include "dfbbcm_utils.h"

/* macro for a safe call to DirectFB functions */
#define DFBCHECK(x...)                                                    \
     {                                                                    \
          err = x;                                                        \
          if (err != DFB_OK) {                                            \
               fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ );     \
               DirectFBErrorFatal( #x, err );                             \
          }                                                               \
     }

static IDirectFB               *dfb;
static IDirectFBSurface        *primary;
static IDirectFBImageProvider  *provider;


int N_ITEMS=1;
//#define N_ITEMS 5//22
#define PI 3.14159

typedef struct vector
{
	float x;
	float y;
} vector_t;

void RenderSprite(IDirectFBSurface* s, IDirectFBSurface* temp, int x, int y, float scale)
{
	int w,h;
	s->GetSize(s,&w,&h);
	
	/*if(scale>1)
		printf("scaling upwards!!\n");*/
	
	DFBRectangle dest={0,0,w*scale,h*scale};
	
	temp->SetBlittingFlags(temp,DSBLIT_NOFX);
	temp->Clear(temp,0,0,0,0);
	temp->StretchBlit(temp,s,NULL,&dest);
	
	primary->SetBlittingFlags(primary,DSBLIT_BLEND_ALPHACHANNEL);
	primary->Blit(primary,temp,&dest,x-dest.w/2,y-dest.h/2);
}


float direction=1;
float stop_at=0;
float angle=0;
//float t=0;


void planets()
{
	N_ITEMS=5;
	stop_at=2*PI/N_ITEMS + 2*PI/4;
	
	DFBResult     err;
	IDirectFBSurface * planet=NULL;
	IDirectFBSurface * nebulae=NULL;
	IDirectFBSurface * small=NULL;
	
		DFBSurfaceDescription dsc; 
	
	 dsc.flags = DSDESC_CAPS;
     dsc.caps = DSCAPS_PRIMARY|DSCAPS_TRIPLE;

     err = dfb->CreateSurface( dfb, &dsc, &primary );
	
	 DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/planet.png",
                                        &provider ));

     DFBCHECK (provider->GetSurfaceDescription (provider, &dsc));
     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &planet ));
     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &small ));

     DFBCHECK(provider->RenderTo( provider, planet, NULL ));
     provider->Release( provider );
     
     
     DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/nebulae.jpg",
                                        &provider ));

     DFBCHECK (provider->GetSurfaceDescription (provider, &dsc));
     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &nebulae ));

     DFBCHECK(provider->RenderTo( provider, nebulae, NULL ));
     provider->Release( provider );
     
     
	for(;;)
	{
	
     	const vector_t dial_center={720/2,480/2-25};
		const int h=480/4;
		const int w=720/3;
		
		float increment=0.05*direction * (0.02+fabs(angle-stop_at));
	
		if(direction==1 && angle < stop_at && angle+increment>=stop_at)
		{
			angle=stop_at;
			direction=0;
		}
		else if(direction==-1 && angle > stop_at && angle+increment<=stop_at)
		{
			angle=stop_at;
			direction=0;
		}
		else
		{
			angle+=increment;
		}
	
	
		if(direction!=0)
		{
			int i;
			primary->SetBlittingFlags(primary,DSBLIT_NOFX);
			primary->StretchBlit(primary,nebulae,NULL,NULL);
			
			for(i=0;i<N_ITEMS;i++)
			{
				if(planet)
				{
					float y=dial_center.y + h*sin(angle+2*PI*i/N_ITEMS);
					RenderSprite(
						planet,small,
						dial_center.x + w*cos(angle+2*PI*i/N_ITEMS),
						y,
						(0.4+0.6*(y-(dial_center.y-h))/(2*h)));
				}
			}
			primary->Flip(primary,NULL,0);
		}
        
        mymsleep(10);
	}
}

void encentrus_sliders()
{
	N_ITEMS=22;
	stop_at=2*PI/N_ITEMS + 2*PI/4;
	DFBResult     err;
	IDirectFBSurface        *E=NULL;
	IDirectFBSurface        *n=NULL;
	IDirectFBSurface        *C=NULL;
	IDirectFBSurface        *e=NULL;
	//IDirectFBSurface      *n=NULL;
	IDirectFBSurface        *t=NULL;
	IDirectFBSurface        *r=NULL;
	IDirectFBSurface        *u=NULL;
	IDirectFBSurface        *s=NULL;
	
	IDirectFBSurface * small=NULL;
	DFBSurfaceDescription dsc; 
	
	 dsc.flags = DSDESC_CAPS;
     dsc.caps = DSCAPS_PRIMARY|DSCAPS_TRIPLE;

     err = dfb->CreateSurface( dfb, &dsc, &primary );
	
	 DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/E.png",
                                        &provider ));

     DFBCHECK (provider->GetSurfaceDescription (provider, &dsc));
     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &E ));
     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &small ));

     DFBCHECK(provider->RenderTo( provider, E, NULL ));
     provider->Release( provider );
     
     DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/n.png",
                                        &provider ));

     DFBCHECK (provider->GetSurfaceDescription (provider, &dsc));
     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &n ));

     DFBCHECK(provider->RenderTo( provider, n, NULL ));
     provider->Release( provider );
     
     DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/C.png",
                                        &provider ));

     DFBCHECK (provider->GetSurfaceDescription (provider, &dsc));
     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &C ));

     DFBCHECK(provider->RenderTo( provider, C, NULL ));
     provider->Release( provider );
     
     DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/e.png",
                                        &provider ));

     DFBCHECK (provider->GetSurfaceDescription (provider, &dsc));
     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &e ));

     DFBCHECK(provider->RenderTo( provider, e, NULL ));
     provider->Release( provider );
     
     DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/t.png",
                                        &provider ));

     DFBCHECK (provider->GetSurfaceDescription (provider, &dsc));
     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &t ));

     DFBCHECK(provider->RenderTo( provider, t, NULL ));
     provider->Release( provider );
     
     DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/r.png",
                                        &provider ));

     DFBCHECK (provider->GetSurfaceDescription (provider, &dsc));
     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &r ));

     DFBCHECK(provider->RenderTo( provider, r, NULL ));
     provider->Release( provider );
     
     DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/u.png",
                                        &provider ));

     DFBCHECK (provider->GetSurfaceDescription (provider, &dsc));
     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &u ));

     DFBCHECK(provider->RenderTo( provider, u, NULL ));
     provider->Release( provider );
     
     DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/s.png",
                                        &provider ));

     DFBCHECK (provider->GetSurfaceDescription (provider, &dsc));
     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &s ));

     DFBCHECK(provider->RenderTo( provider, s, NULL ));
     provider->Release( provider );
    
    
     for(;;)
     {
     	const vector_t dial_center={720/2,480/2-25};
		const int h=480/4;
		const int w=720/3;
		
		float increment=0.1*direction * (0.02+fabs(angle-stop_at));
	
		if(direction==1 && angle < stop_at && angle+increment>=stop_at)
		{
			angle=stop_at;
			direction=0;
		}
		else if(direction==-1 && angle > stop_at && angle+increment<=stop_at)
		{
			angle=stop_at;
			direction=0;
		}
		else
		{
			angle+=increment;
		}


		if(direction!=0)
		{
			primary->Clear(primary,0,0,0,0);
			int i;
			for(i=0;i<N_ITEMS;i++)
			{
				IDirectFBSurface* tex=NULL;
				
				switch(10-i%11)
				{
				case 0:
					tex=E; break;
				case 1:
					tex=n; break;
				case 2:
					tex=C; break;
				case 3:
					tex=e; break;
				case 4:
					tex=n; break;
				case 5:
					tex=t; break;
				case 6:
					tex=r; break;
				case 7:
					tex=u; break;
				case 8:
					tex=s; break;
				case 9:
				case 10:
					tex=NULL; break;
				}
		
            	if(tex)
				{
					float y=dial_center.y + h*sin(angle+2*PI*i/N_ITEMS);
					RenderSprite(
						tex,small,
						dial_center.x + w*cos(angle+2*PI*i/N_ITEMS),
						y,
						0.4+0.6*(y-(dial_center.y-h))/(2*h));
				}
			}
			primary->Flip(primary,NULL,0);
		}
        
        /* Yield */
        mymsleep(10);
	}
}

void blend()
{
	DFBResult     err;
	IDirectFBSurface        *image[2];
	
	DFBSurfaceDescription dsc; 

	 dsc.flags = DSDESC_CAPS;
     dsc.caps = DSCAPS_PRIMARY;

     err = dfb->CreateSurface( dfb, &dsc, &primary );
	
	DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/desktop.png",
                                        &provider ));

     DFBCHECK (provider->GetSurfaceDescription (provider, &dsc));
     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &image[0] ));

     DFBCHECK(provider->RenderTo( provider, image[0], NULL ));
     provider->Release( provider );

	 DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/smokey_light.jpg",
                                        &provider ));

     DFBCHECK (provider->GetSurfaceDescription (provider, &dsc));
     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &image[1] ));

     DFBCHECK(provider->RenderTo( provider, image[1], NULL ));
     provider->Release( provider );
     
     float t=0;
     for(;;)
     {
     	t+=0.5;
		primary->SetBlittingFlags(primary,DSBLIT_NOFX);
		primary->Blit( primary, image[0], NULL, 0, 0 );
			
		primary->SetBlittingFlags(primary,DSBLIT_BLEND_COLORALPHA);
		primary->SetColor(primary,0,0,0,128+120*sin((float)t));
			
		primary->Blit( primary, image[1], NULL, 0, 0 );
     }
     
	image[0]->Release(image[0]);
	image[1]->Release(image[1]);
}

void pan_and_scan()
{
	DFBResult     err;
	IDirectFBSurface        *image[2];
	
	DFBSurfaceDescription dsc; 
	dsc.flags = DSDESC_CAPS;
    dsc.caps = DSCAPS_PRIMARY;

     err = dfb->CreateSurface( dfb, &dsc, &primary );
	
	 DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/face-big.jpg",
                                        &provider ));

     DFBCHECK (provider->GetSurfaceDescription (provider, &dsc));
     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &image[1] ));

     DFBCHECK(provider->RenderTo( provider, image[1], NULL ));
     provider->Release( provider );
	
	float t=0;
	for(;;)
	{
		t+=0.5;
		int ow,oh;
		image[1]->GetSize(image[1],&ow,&oh);
				
		int w=720*4/5 - 72*2 + 72*2*sin(t/75);
		int h=480*4/5 - 48*2 + 48*2*sin(t/75);
		
		DFBRectangle src={
			ow/4 + ow/5*cos(t/100),
			oh/4 + oh/5*sin(t/200),
			w,
			h
			};
		
		//weirdest bugfix i've ever seen
		if(src.w==471 || src.w==472) continue;
		
		//printf("%d %d %d %d\n",src.x,src.y,src.w,src.h);
		
		primary->SetBlittingFlags(primary,DSBLIT_NOFX);
		
		primary->StretchBlit(primary,image[1],&src,NULL);
		
		dfb->WaitIdle(primary);
	}
	
	
	image[1]->Release(image[1]);
}

void load_image(char* filename)
{
	DFBResult     err;
	IDirectFBSurface*        image;
	
	DFBSurfaceDescription dsc; 
	dsc.flags = DSDESC_CAPS;
    dsc.caps = DSCAPS_PRIMARY;//|DSCAPS_DOUBLE;

    err = dfb->CreateSurface( dfb, &dsc, &primary );
    
	DFBCHECK(dfb->CreateImageProvider( dfb, filename,
                                        &provider ));

    DFBCHECK (provider->GetSurfaceDescription (provider, &dsc));
    DFBCHECK(dfb->CreateSurface( dfb, &dsc, &image ));

    DFBCHECK(provider->RenderTo( provider, image, NULL ));
    provider->Release( provider );
    
    primary->StretchBlit(primary,image,NULL,NULL);
	//primary->Flip(primary,NULL,DSFLIP_BLIT);
	
    printf("press enter to continue.\n");
    getchar();
    image->Release(image);
    primary->Release(primary);
}

void* keyboard_threadproc(void* context)
{
	for(;;)
	{
		int c=getchar();
		if(c=='a')
		{
			direction=-1;
			stop_at-=2*PI/N_ITEMS;
		}
		else if(c=='s')
		{
			direction=1;
			stop_at+=2*PI/N_ITEMS;
		}
	}
	return NULL;
}

int main(int argc, char* argv[])
{
    int ret = EXIT_SUCCESS;
	
    DFBResult     err;
	DFBSurfaceDescription dsc;
    
     bdvd_display_h bdisplay = NULL;

     if (bdvd_init(BDVD_VERSION) != bdvd_ok) {
         D_ERROR("Could not init bdvd\n");
         ret = EXIT_FAILURE;
         goto out;
     }
     if ((bdisplay = bdvd_display_open(B_ID(0))) == NULL) {
         D_ERROR("Could not open bdvd_display B_ID(0)\n");
         ret = EXIT_FAILURE;
         goto out;
     }

    if(argc<2)
    {
        printf(
            "usage: dfbbcm_myfx fx_number"
            "\n"
            "\n"
            "EFFECTS:\n"
            "1. blends\n"
            "2. EnCentrus sliders\n"
            "3. planets\n"
            "4. pan and scan\n"
            "5. load image (filename specified in extra parameter)\n");
        return 0;
    }
     
     DFBCHECK(DirectFBInit( &argc, &argv ));

     /* create the super interface */
     DFBCHECK(DirectFBCreate( &dfb ));
     
     dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN );
     if (err)
       DirectFBError( "Failed to get exclusive access", err );

     pthread_t kb;
     pthread_create(&kb,NULL,keyboard_threadproc,NULL);

	switch(argv[1][0])
	{
	case '1':
		blend();
		break;
	case '2':
		encentrus_sliders();
		break;
	case '3':
		planets();
		break;
	case '4':
		pan_and_scan();
		break;
	case '5':
		load_image(argv[2]);
		break;
	default:
		printf("ERROR: unknown effect\n");
		return 0;
	}
	
	if(primary)
		primary->Release(primary);
	
	if(dfb)
		dfb->Release(dfb);
		
out:
	return 0;
}
