#include <directfb.h>
#include <direct/debug.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <bsettop.h>
#include <bdvd.h>

class CSprite
{
public:
	void Init(IDirectFB* dfb, IDirectFBSurface * surface, int width, int height)
	{
        m_surface = surface;
		m_screen_width=width;
		m_screen_height=height;
		
		Reset();
	}
	
	void Reset()
	{
		m_x=m_screen_width/2;
		m_y=m_screen_height*2/3;
		m_dx=rand()%400 - 200;
		m_dy=rand()%300 - 150 - 400;
		m_size=1;
		m_alpha=0xFF;
	}
	
	void Step(float ms)
	{
		m_size+=ms/30;
		m_dy+=ms/2;
		m_x+=m_dx*ms/1000;
		m_y+=m_dy*ms/1000;
		
		if(m_alpha<ms/10)
			Reset();
		
		m_alpha-=(unsigned char)ms/10;
		
		if(m_y-m_size/2>=m_screen_height || m_x<0 || m_x-m_size/2>m_screen_width)
			Reset();
	}
	
	void Put(IDirectFBSurface* dest)
	{
		dest->SetBlittingFlags(dest,(DFBSurfaceBlittingFlags)(DSBLIT_BLEND_ALPHACHANNEL|DSBLIT_BLEND_COLORALPHA));
		dest->SetColor(dest,0,0,0,m_alpha);
		DFBRectangle r={(int)(m_x-m_size/2),(int)(m_y-m_size/2),(int)m_size,(int)m_size};
		dest->StretchBlit(dest,m_surface,NULL,&r);
	}
	
	virtual ~CSprite()
	{
	}
protected:

	IDirectFBSurface* m_surface;
	float m_x,m_y;
	float m_dx,m_dy;
	float m_size;
	unsigned char m_alpha;
	
	int m_screen_height;
	int m_screen_width;
	
};

void print_usage(const char * program_name) {

    fprintf(stderr, "usage: %s\n", program_name);
    fprintf(stderr, "n <nb sparks>\n");
    fprintf(stderr, "f: front only\n");
}

int main(int argc, char * argv[])
{
    CSprite * s = NULL;
    int i;
    int w,h;
    IDirectFB* dfb=NULL;
    IDirectFBSurface* fb=NULL;
    IDirectFBSurface* spark_surface=NULL;
    IDirectFBImageProvider* provider=NULL;
    DFBSurfaceDescription sdesc;

    char c;

    int nb_sparks = 50;
    bool frontonly = false;

     /* bsettop init begin */
     bdisplay_t bcm_display = NULL;
     if (bsettop_init(BSETTOP_VERSION) != b_ok) {
         fprintf(stderr, "Could not init bsettop\n");
         
         goto out;
     }
     if ((bcm_display = bdisplay_open(B_ID(0))) == NULL) {
         fprintf(stderr, "Could not open bdisplay B_ID(0)\n");
         
         goto out;
     }
     /* bsettop init end */

     while ((c = getopt (argc, argv, "fn:")) != -1) {
        switch (c) {
        case 'n':
            nb_sparks = strtoul(optarg, NULL, 10);
        break;
        case 'f':
            frontonly = true;
        break;
        case '?':
        default:
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        break;
        }
    }

    s = new CSprite[nb_sparks];

	DirectFBInit(&argc,&argv);
	
	DirectFBCreate(&dfb);
    dfb->SetCooperativeLevel (dfb, DFSCL_FULLSCREEN);
	
	sdesc.flags=DSDESC_CAPS;
	sdesc.caps=(DFBSurfaceCapabilities)(DSCAPS_PRIMARY|(frontonly ? 0 : DSCAPS_TRIPLE));

	dfb->CreateSurface(dfb,&sdesc,&fb);

	printf("creating provider...\n");
	dfb->CreateImageProvider(dfb,DATADIR"/spark.png",&provider);
	
	if(provider==NULL)
	{
		printf("cant read file!!\n");
		return 1;
	}
    provider->GetSurfaceDescription(provider,&sdesc);
    dfb->CreateSurface(dfb,&sdesc,&spark_surface);
    provider->RenderTo(provider,spark_surface,NULL);

	fb->GetSize(fb,&w,&h);
	for(i=0;i<nb_sparks;i++)
		s[i].Init(dfb,spark_surface,w,h);
	
	int j;
	for(j=0;j<200;j++)
		for(i=0;i<nb_sparks;i++)
			s[i].Step(200);	
	
	for(;;)
	{
		//printf("stepping...\n");
		fb->Clear(fb,0,0,0,0);
		
		for(i=0;i<nb_sparks;i++)
		{
			s[i].Step(33);
			s[i].Put(fb);
		}
		
		fb->Flip(fb,NULL,DSFLIP_NONE);
	}

	getchar();

out:

    if (s) delete[] s;

    if (spark_surface) spark_surface->Release(spark_surface);

    if (provider) provider->Release(provider);

    if (fb) fb->Release(fb);
    
    if (dfb) dfb->Release(dfb);

    /* bsettop uninit begin */
    if (bcm_display) bdisplay_close(bcm_display);
    bsettop_uninit();
    /* bsettop uninit end */
}
