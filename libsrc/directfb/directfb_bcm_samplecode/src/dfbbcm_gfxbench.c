/***************************************************************************
 *     Copyright (c) 2005-2009, Broadcom Corporation
 *     All Rights Reserved;
 *     Confidential Property of Broadcom Corporation
 *
 *  THIS SOFTWARE MAY ONLY BE USED SUBJECT TO AN EXECUTED SOFTWARE LICENSE
 *  AGREEMENT  BETWEEN THE USER AND BROADCOM.  YOU HAVE NO RIGHT TO USE OR
 *  EXPLOIT THIS MATERIAL EXCEPT SUBJECT TO THE TERMS OF SUCH AN AGREEMENT.
 *
 ***************************************************************************/

#include "bdvd.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include "bstd.h"
#include "bkni.h"
#include "bkni_multi.h"

#define BUFFER_SIZE      (18*1024)
#define NUM_BUFFERS      (16)
#define SIZEOF_ARRAY(a)  (sizeof(a)/sizeof(a[0]))

int video_pid  = 0x1011;
int video_codec = bdvd_video_codec_mpeg2;

int audio_pid  = 0x1100;
int audio_format = bdvd_audio_format_ac3;

int packet_pid = 0x1200;

typedef struct {
	bdvd_display_h bdisplay;
	bdvd_video_format_e video_format;
} dfb_thread_context;

static void* dfb_threadproc(void* context);

static void
usage()
{
  printf("\nDirectFB benchmark application (including video streaming based on app_bda)\n");
  printf("\nUsage:");
  printf("\nlaunch dfbbcm_gfxbench <args>\n");
  printf("\nArgument:              Description:");
  printf("\n-file <val>            Source content file");
  printf("\n-video_pid <val>       Video Stream PID [%d]", video_pid);
  printf("\n-audio_pid <val>       Audio Stream PID [%d]", audio_pid);
  printf("\n-packet_pid <val>      Packet Stream PID [%d]", packet_pid);
  printf("\n-video_codec <val>     MPEG2=2, AVC=27, etc. [%d]", video_codec);
  printf("\n-audio_format <val>    AC3=129, AAC=15, etc. [%d]", audio_format);
  printf("\n-1080i                 1080i Output");
  printf("\n-720p                  720p Output");
  printf("\n-480p                  480p Output");
  printf("\n-480i                  480i Output");
  printf("\n-progress              Progress indicator");
  printf("\n-loop <val>            Loop playback specified times");
  printf("\n-seamless              Seamless looping of stream\n");
  printf("\nExample:\n");
  printf("\nlaunch dfbbcm_gfxbench -file stream.m2ts -loop 5\n\n");
}

void
standard_callback(void *event)
{
     BKNI_SetEvent((BKNI_EventHandle)event);
}

uint8_t *buffers[NUM_BUFFERS];
unsigned next_buffer;
unsigned busy_buffers;

volatile int playback_done;

int
main(int argc, const char *argv[])
{
  FILE *file_ptr = NULL;
  char content_file[100]={0};
  int progress=0, showProgress=0;
  int loopMax=0, loopNum=0, seamless=0, i;
  bdvd_video_format_e video_format=bdvd_video_format_ntsc;
  BKNI_EventHandle playback_event;

  pthread_t dfbthread;
  dfb_thread_context dfbcontext;

  bdvd_decode_h decode = NULL;
  bdvd_decode_window_h window = NULL;
  bdvd_display_h display = NULL;
  bdvd_playback_h playback = NULL;
  bdvd_display_settings_t display_settings;
  bdvd_playback_open_params_t playback_params;
  bdvd_playback_start_params_t start_params;
  bdvd_decode_audio_params_t audio_params;
  bdvd_decode_video_params_t video_params;
  bdvd_decode_window_settings_t decodeWindowSettings;
  pthread_t packet_thread;
  extern void *packet_main();

  /*if ( 2 >= argc ) {
    usage();
    return bdvd_err_unspecified; 
  } else */ {
    for (i = 1; i < argc; i++) {
      if ( !strcmp(argv[i], "-file") ) {
	strcpy(content_file, argv[++i]);
      } else if ( !strcmp(argv[i], "-video_pid") ) {
	video_pid = atoi(argv[++i]);
	if ( !video_pid ) {
	  usage();
	  return bdvd_err_unspecified;
	}
      } else if ( !strcmp(argv[i], "-audio_pid") ) {
	audio_pid = atoi(argv[++i]);
	if ( !audio_pid ) {
	  usage();
	  return bdvd_err_unspecified;
	}
      } else if ( !strcmp(argv[i], "-packet_pid") ) {
	packet_pid = atoi(argv[++i]);
	if ( !packet_pid ) {
	  usage();
	  return bdvd_err_unspecified;
	}
      } else if ( !strcmp(argv[i], "-video_codec") ) {
	video_codec = atoi(argv[++i]);
	if ( !video_codec ) {
	  usage();
	  return bdvd_err_unspecified;
	}
      } else if ( !strcmp(argv[i], "-audio_format") ) {
	audio_format = atoi(argv[++i]);
	if ( !audio_format ) {
	  usage();
	  return bdvd_err_unspecified;
	}
      } else if ( !strcmp(argv[i], "-1080i") ) {
	video_format = bdvd_video_format_1080i;
      } else if ( !strcmp(argv[i], "-720p") ) {
	video_format = bdvd_video_format_720p;
      } else if ( !strcmp(argv[i], "-480p") ) {
	video_format = bdvd_video_format_480p;
      } else if ( !strcmp(argv[i], "-480i") ) {
	video_format = bdvd_video_format_ntsc;
      } else if ( !strcmp(argv[i], "-progress") ) {
	showProgress = 1;
      } else if ( !strcmp(argv[i], "-loop") ) {
	loopMax = atoi(argv[++i]);
      } else if ( !strcmp(argv[i], "-seamless") ) {
	seamless = 1;
      } else {
	usage();
	return bdvd_err_unspecified;
      }
    }		
  }

  if(*content_file)
  {
  	file_ptr = fopen(content_file, "r");
  	if (file_ptr == NULL) {
  	  printf("fopen failed\n");
  	  return bdvd_err_unspecified;
  	}
  }
  
  if (bdvd_init(BDVD_VERSION) != bdvd_ok) {
    printf("bdvd_init failed\n");
    return bdvd_err_unspecified;
  }

  if (file_ptr && pthread_create(&packet_thread, NULL, packet_main, NULL) < 0) {
    printf("pthread_create failed\n");
    return bdvd_err_unspecified;
  }

  playback_params.stream_type = bdvd_playback_stream_type_bd;
  playback_params.buffer_size = 0;
  playback_params.alignment = 0;
  playback_params.num_descriptors = NUM_BUFFERS;

  if(file_ptr)
  for (i=0; i<NUM_BUFFERS; i++) {
    buffers[i] = bdvd_playback_alloc_buffer(BUFFER_SIZE, 0);
    if (buffers[i] == NULL) {
      printf("bdvd_playback_alloc_buffer failed\n");
      return bdvd_err_unspecified;
    }
  }

	if(file_ptr)
	{  playback = bdvd_playback_open(B_ID(0), &playback_params);
	  if (playback == NULL) {
	    printf("bdvd_playback_open failed\n");
	    return bdvd_err_unspecified;
	  }
	}
	
  if(file_ptr)
  if (BKNI_CreateEvent(&playback_event) != BERR_SUCCESS) {
    printf("BKNI_CreateEvent failed\n");
    return bdvd_err_unspecified;
  }

  start_params.buffer_available_cb = standard_callback;
  start_params.callback_context = playback_event;
  
  if(file_ptr)
  if (bdvd_playback_start(playback, &start_params) != bdvd_ok) {
    printf("bdvd_playback_start failed\n");
    return bdvd_err_unspecified;
  }

  display = bdvd_display_open(B_ID(0));
  if (display == NULL) {
    printf("bdvd_display_open failed\n");
    return bdvd_err_unspecified;
  }

  bdvd_display_get(display, &display_settings);

  display_settings.format = video_format;

  if (video_format!=bdvd_video_format_ntsc) {
    display_settings.svideo = NULL;
    display_settings.composite = NULL;
    display_settings.rf = NULL;
  }

  if (bdvd_display_set(display, &display_settings) != bdvd_ok) {
    printf("bdvd_display_set failed\n");
    return bdvd_err_unspecified;
  }

  //------------------
  
  dfbcontext.video_format=video_format;
  dfbcontext.bdisplay=display;
  
  if (pthread_create(&dfbthread,NULL,dfb_threadproc,&dfbcontext) < 0) {
  	printf("pthread_create failed\n");
    return bdvd_err_unspecified;
  };

  if(file_ptr)
  {
	  decode = bdvd_decode_open(B_ID(0), NULL, NULL);
	  if (decode == NULL) {
	    printf("bdvd_decode_open failed\n");
	    return bdvd_err_unspecified;
	  }
	
	  video_params.pid          = video_pid;
	  video_params.stream_id    = 0x00;
	  video_params.substream_id = 0x00;
	  video_params.format       = video_codec;
	  video_params.playback_id  = B_ID(0);
	
	  if (bdvd_decode_select_video_stream(decode, &video_params) != bdvd_ok) {
	    printf("bdvd_decode_select_video_stream failed\n");
	    return bdvd_err_unspecified;
	  }
	
	  audio_params.pid          = audio_pid;
	  audio_params.stream_id    = 0x00;
	  audio_params.substream_id = 0x00;
	  audio_params.format       = audio_format;
	  audio_params.playback_id  = B_ID(0);
	
	  if (bdvd_decode_select_audio_stream(decode, &audio_params) != bdvd_ok) {
	    printf("bdvd_decode_select_audio_stream failed\n");
	    return bdvd_err_unspecified;
	  }

      window = bdvd_decode_window_open(B_ID(0), display);
      if (window == NULL) {
        printf("bdvd_decode_window_open failed\n");
        return bdvd_err_unspecified;
      }

	  if (bdvd_decode_start(decode, window) != bdvd_ok) {
	    printf("bdvd_decode_start\n");
	    return bdvd_err_unspecified;
	  }
	  
	  while (1) {
	    unsigned done_count;
	    uint8_t *buffer;
	    int n;
	
	    if (busy_buffers == NUM_BUFFERS) {
	      BKNI_WaitForEvent(playback_event, 250);
	
	      if (bdvd_playback_check_buffers(playback, &done_count) != bdvd_ok) {
		printf("bdvd_playback_check_buffers failed\n");
		return bdvd_err_unspecified;
	      }
	
	      busy_buffers -= done_count;
	      continue;
	    } else {
	      buffer = buffers[next_buffer];
	    }
	
	    n = fread(buffer, 1, BUFFER_SIZE, file_ptr);
	
	    if (n > 0) {
	      int buffer_added = 0;
	
	      while (!buffer_added) {
		switch (bdvd_playback_add_buffer(playback, buffer, n)) {
		case bdvd_ok:
		  next_buffer++;
		  if (next_buffer == NUM_BUFFERS) {
		    next_buffer = 0;
		  }
		  busy_buffers++;
		  buffer_added=1;
		  break;
		case bdvd_err_busy:
		case bdvd_err_no_descriptor:
		  BKNI_WaitForEvent(playback_event, 250);
		  continue;
		default:
		  printf("bdvd_playback_add_buffer failed\n");
		  return bdvd_err_unspecified;
		}
	      }
		
	      if (showProgress) {
		progress += n;
		if (progress > (1024*1024)) {
		  progress = 0; putchar('+'); fflush(stdout);
		}
	      }
	    } else {
	      /*
	       * Wait for the playback buffer to drain; should just take
	       * a few milliseconds.
	       */
	      while (busy_buffers) {
		BKNI_WaitForEvent(playback_event, 250);
	
		if (bdvd_playback_check_buffers(playback, &done_count) != bdvd_ok) {
		  printf("bdvd_playback_check_buffers failed\n");
		  return bdvd_err_unspecified;
		}
		busy_buffers -= done_count;
	      }
	
	      if (++loopNum < loopMax) {
		printf("\nIteration %d\n\n", loopNum+1);
		fseek(file_ptr, 0, SEEK_SET);
	
		if (!seamless) {
		  if (bdvd_decode_stop(decode, true) != bdvd_ok) {
		    printf("bdvd_decode_stop(true) failed\n");
		    return bdvd_err_unspecified;
		  }
	
		  if (bdvd_decode_start(decode, window) != bdvd_ok) {
		    printf("bdvd_decode_start failed\n");
		    return bdvd_err_unspecified;
		  }
		}
	      } else {
		fclose(file_ptr);
		break;
	      }
	    }
	  }
	
	  playback_done = 1;
	  pthread_join(packet_thread, NULL);
	 
	  bdvd_playback_stop(playback);
	  bdvd_decode_stop(decode, false);
	    
	  bdvd_decode_window_close(window);
  }
  
  //make sure the dfb thread has completed before cleaning up
  pthread_join(dfbthread,NULL);

  if(decode)
      bdvd_decode_close(decode);
  if(playback)
	  bdvd_playback_close(playback);

  if(file_ptr)
  for (i=0; i<NUM_BUFFERS; i++) {
    if (bdvd_playback_free_buffer(buffers[i]) != bdvd_ok) {
      printf("*** bdvd_playback_free_buffer error\n");
      return bdvd_err_unspecified;
    }
  }

  if (display) bdvd_display_close(display);
  bdvd_uninit();

  return bdvd_ok;
}

void overflow_callback(void *event)
{
  BKNI_Printf("---\n----> Packet Channel Buffer Overflow\n---\n");
  BKNI_SetEvent((BKNI_EventHandle)event);
}

/*
 * Thread for receiving PES packets via the DVD Packet API.
 */
void *packet_main()
{
  bdvd_packet_h packet;
  bdvd_packet_params_t packet_params;
  bdvd_packet_cntrl_t packet_cntrl;
  BKNI_EventHandle packet_event;
  size_t to_consume=0, consumed;

  BKNI_CreateEvent(&packet_event);

  packet_params.buffer_size = 512*1024;
  packet_params.ready_callback = standard_callback;
  packet_params.overflow_callback = overflow_callback;
  packet_params.callback_context = packet_event;

  packet = bdvd_packet_open(B_ID(0), &packet_params);

  if (packet == NULL) {
    printf("bdvd_packet_open failed\n");
    return NULL;
  }

  packet_cntrl.pid = packet_pid;
  packet_cntrl.stream_id = 0;
  packet_cntrl.substream_id = 0;

  if (bdvd_packet_add_pid_channel(packet, &packet_cntrl, bdvd_packet_type_pes)) {
    printf("bdvd_packet_add_pid_channel failed\n");
    return NULL;
  }
  if (bdvd_packet_start(packet)) {
    printf("bdvd_packet_start failed\n");
    return NULL;
  }

  while (!playback_done) {
    uint8_t *buffer;
    uint8_t *pes_pkt;
    uint8_t stream_id;
    uint16_t pes_len;
    size_t size;

    if (bdvd_packet_recv(packet, &buffer, &size) != bdvd_ok) {
      printf("bdvd_packet_recv failed\n");
      return NULL;
    }

    if (size == 0) {
      BKNI_WaitForEvent(packet_event, 1000);
      continue;
    }

    if (to_consume == 0) {
      if (size < 6) {
	printf("*** Sorry, can't deal with runt data for now\n");
	return NULL;
      }

      pes_pkt = (uint8_t*)buffer;
      if (pes_pkt[0] != 0x00 || pes_pkt[1] != 0x00 || pes_pkt[2] != 0x01) {
	printf("*** Sorry, PES start code not found\n");
	return NULL;
      }

      stream_id = pes_pkt[3];
      pes_len = pes_pkt[4];
      pes_len <<= 8;
      pes_len |= pes_pkt[5];

      printf("PID 0x%04x: PES packet: Stream ID 0x%02x, PES Length %d\n",
	     packet_pid, stream_id, pes_len);

      to_consume = (((pes_len+6)+3)/4)*4;
    }

    if (size >= to_consume) {
      consumed = to_consume;
    } else {
      consumed = size;
    }

    to_consume -= consumed;

    if (bdvd_packet_done(packet, consumed) != bdvd_ok) {
      printf("bdvd_packet_done failed\n");
      return NULL;
    }
  }

  if (bdvd_packet_stop(packet) != bdvd_ok) {
    printf("bdvd_packet_stop failed\n");
    return NULL;
  }

  if (bdvd_packet_close(packet) != bdvd_ok) {
    printf("bdvd_packet_close failed\n");
    return NULL;
  }

  BKNI_DestroyEvent(packet_event);
  BKNI_Sleep(1000);
  return NULL;
}

//============================================================
//============== DirectFB bench functions ====================
//============================================================

#include <sys/time.h>
#define TRACE(x...) { fprintf(stderr,x); }
#define PRINT(x...) { fprintf(stderr,x); }

#define ALIGN_NONE     0x00 // x and y values are used as is
#define ALIGN_HLEFT    0x01
#define ALIGN_HCENTER  0x02
#define ALIGN_HRIGHT   0x03
#define ALIGN_VTOP     0x10
#define ALIGN_VCENTER  0x20
#define ALIGN_VBOTTOM  0x30

typedef struct 
{
	IDirectFBSurface* source;
	DFBSurfaceBlittingFlags blitting_flags;
	int alignment; //use ALIGN_XXX constants
	int x,y; //these are generated by the system, based on alignment (if not equal to ALIGN_NONE)
	int width,height; // sets the destination rect size.  if set to 0, use source surface size
} dfb_operation;

typedef struct
{
	int w,h;
} surface_resolution;

typedef struct 
{
	surface_resolution* resolutions;
	int n_resolutions;
	
	struct {
		char name[64];
		bdvd_dfb_ext_layer_type_e type;
		DFBSurfacePixelFormat pixel_format;
		
		dfb_operation* operations;
		int n_operations;
		
		int n_loops;
		
	} layer[2];
	
} test_configuration;

//thread context
typedef struct {
	
	char name[64];
	
	//test parameters
	
	IDirectFBSurface* destination;
	dfb_operation* operations;
	int n_operations;
	int n_loops;
	
	//test results
	
	int ms_elapsed;
	
} dfb_operation_context;

//------------------------------------------------------------


void ResetLayer(IDirectFBDisplayLayer* layer, int width, int height, DFBSurfacePixelFormat pixel_format)
{
	DFBDisplayLayerConfig config;
	
	layer->GetConfiguration(layer,&config);
	
	unsigned int flags=DLCONF_WIDTH|DLCONF_HEIGHT|DLCONF_PIXELFORMAT;//|DLCONF_SURFACE_CAPS;
	
	/*if(pixel_format==DSPF_LUT8)
	{
		//paletized surfaces cannot be premultiplied
		config.surface_caps = DSCAPS_NONE;
	}
	else
	{
		config.surface_caps = DSCAPS_PREMULTIPLIED;
	}*/
	
	config.flags=flags;
	config.width=width;
	config.height=height;
	config.pixelformat=pixel_format;
	
	layer->SetConfiguration(layer,&config);
}

void* dfb_operation_threadproc(void* void_context)
{
	dfb_operation_context* context=(dfb_operation_context*)void_context;
	
	//TRACE("%s thread begins\n",context->name);
	struct timeval before;
	struct timeval after;
	
	typedef struct 
	{
		int scroll_x,scroll_y;
		bool h_direction,v_direction;
	}operation_state;
	
	operation_state* opstate=(operation_state*)malloc(context->n_operations*sizeof(operation_state));
	memset(opstate,0,context->n_operations*sizeof(operation_state));
	
	if(context->n_operations)
	{
		// if the pixel format of the surface in the first operation is LUT8,
		// set the destination surface to use that same palette.
		
		DFBSurfacePixelFormat pf;
	
		IDirectFBSurface* s=context->operations[0].source;
		s->GetPixelFormat(s,&pf);
		
		if(pf==DSPF_LUT8)
		{
			IDirectFBPalette* palette;
			s->GetPalette(s,&palette);
			context->destination->SetPalette(context->destination,palette);
			palette->Release(palette);
		}
	}
	
	//begin with a flip so we can start counting time right after it completes.
	context->destination->Flip(context->destination,NULL,DSFLIP_NONE);
	//START!!
	gettimeofday(&before,NULL);
	
	int f;
	for(f=0;f<context->n_loops;f++)
	{
		int i;
		for(i=0;i<context->n_operations;i++)
		{
			dfb_operation* op=&context->operations[i];
			
			int total_w,total_h;
			op->source->GetSize(op->source,&total_w,&total_h);
			
			//moving rect
			if(opstate[i].h_direction)
			{
				if(opstate[i].scroll_x + op->width >= total_w)
					opstate[i].h_direction=!opstate[i].h_direction;
				else
					opstate[i].scroll_x++;
			}
			else
			{
				if(opstate[i].scroll_x <= 0)
					opstate[i].h_direction=!opstate[i].h_direction;
				else
					opstate[i].scroll_x--;
			}
			
			if(opstate[i].v_direction)
			{
				if(opstate[i].scroll_y + op->height >= total_h)
					opstate[i].v_direction=!opstate[i].v_direction;
				else
					opstate[i].scroll_y++;
			}
			else
			{
				if(opstate[i].scroll_y <= 0)
					opstate[i].v_direction=!opstate[i].v_direction;
				else
					opstate[i].scroll_y--;
			}
			
			//blitting
			context->destination->SetBlittingFlags(context->destination, op->blitting_flags);
			DFBRectangle rect;
			rect.x=opstate[i].scroll_x;
			rect.y=opstate[i].scroll_y;
			rect.w=op->width;
			rect.h=op->height;
			context->destination->Blit(context->destination,op->source,&rect,op->x,op->y);
		}
		
		context->destination->Flip(context->destination,NULL,DSFLIP_NONE);
	}
	
	//STOP!!
	gettimeofday(&after,NULL);
	
	//report test duration
	context->ms_elapsed=(after.tv_sec*1000 + after.tv_usec/1000) - (before.tv_sec*1000 + before.tv_usec/1000);
	
	free(opstate);
	
	return NULL;
}

void RunTests(IDirectFB* dfb, test_configuration* config)
{
	bdvd_dfb_ext_h layer_h[2];
	IDirectFBDisplayLayer* layer[2];
	int i;
	
	TRACE("obtaining layers and layer surfaces...\n");
	for(i=0;i<2;i++)
	{	
		bdvd_dfb_ext_layer_settings_t layer_settings;
	
		layer_h[i]=bdvd_dfb_ext_layer_open(config->layer[i].type);
		bdvd_dfb_ext_layer_get(layer_h[i],&layer_settings);
		dfb->GetDisplayLayer(dfb,layer_settings.id,&layer[i]);
		layer[i]->SetCooperativeLevel(layer[i],DLSCL_ADMINISTRATIVE);
		layer[i]->SetLevel(layer[i],i+1);
	}
	
	int res;
	for(res=0;res<config->n_resolutions;res++)
	{
		pthread_t thread[2];
		IDirectFBSurface* layer_surface[2];
		dfb_operation_context context[2];
		
		TRACE("Starting test for destination resolution %dx%d...\n",config->resolutions[res].w,config->resolutions[res].h);
		
		for(i=0;i<2;i++)
		{
			int dw,dh; //dest dimensions
			
			//set the layer's configuration
			ResetLayer(layer[i], config->resolutions[res].w, config->resolutions[res].h, config->layer[i].pixel_format);
			layer[i]->GetSurface(layer[i],&layer_surface[i]);
			
			layer_surface[i]->GetSize(layer_surface[i],&dw,&dh);
			
			//generate x and y according to alignment
			int j;
			for(j=0;j<config->layer[i].n_operations;j++)
			{
				dfb_operation* op=&config->layer[i].operations[j];
				
				switch(op->alignment&0x3) //horizontal
				{
				case ALIGN_HLEFT:
					op->x=0; break;
				case ALIGN_HCENTER:
					op->x=dw/2 - op->width/2; break;
				case ALIGN_HRIGHT:
					op->x=dw - op->width; break;
				}
				
				switch(op->alignment&0x30) //vertical
				{
				case ALIGN_VTOP:
					op->y=0; break;
				case ALIGN_VCENTER:
					op->y=dh/2 - op->height/2; break;
				case ALIGN_VBOTTOM:
					op->y=dh - op->height; break;
				}
			}
			
			//prepare the context and start the thread
			strcpy(context[i].name,config->layer[i].name);
			context[i].destination=layer_surface[i];
			context[i].operations=config->layer[i].operations;
			context[i].n_operations=config->layer[i].n_operations;
			context[i].n_loops=config->layer[i].n_loops;
			pthread_create(&thread[i],NULL,dfb_operation_threadproc,&context[i]);
		}
		
		//***the threads will run here in parallel***
		
		//wait for all threads to end
		for(i=0;i<2;i++)
			pthread_join(thread[i],NULL);
		
		for(i=0;i<2;i++)
		{
			int flip_duration_us;
			int width,height;
			context[i].destination->GetSize(context[i].destination,&width,&height);
			
			unsigned long n_pixels=0;
			int j;
			for(j=0;j<context[i].n_operations;j++)
			{
				n_pixels+=context[i].operations[j].width*context[i].operations[j].height;
			}
			
			//measure duration of a Flip
			{
				struct timeval before,after;
				
				//measure more a few calls to Flip to obtain an accurate average value
				context[i].destination->Flip(context[i].destination,NULL,DSFLIP_NONE);
				gettimeofday(&before,NULL);
				int j;
				for(j=0;j<60;j++)
					context[i].destination->Flip(context[i].destination,NULL,DSFLIP_NONE);
				
				gettimeofday(&after,NULL);
			
				flip_duration_us=( (after.tv_sec-before.tv_sec)*1000000 + after.tv_usec - before.tv_usec ) / 60;
			}
			
			int expected_ms=(context[i].n_loops*flip_duration_us)/1000;
			int skipped=(context[i].ms_elapsed - expected_ms + flip_duration_us/2/1000) / (flip_duration_us/1000);
			
			PRINT(
				"-----------------------------------------\n"
				"'%s' test\n"
				"Target resolution: %dx%d\n"
				"Flip duration    : %d.%03d ms\n"
				"Duration         : %d frames\n"
				"Number of Pixels : %d.%03d MPixels\n"
				"Time Expected    : %d ms\n"
				"Time Elapsed     : %d ms\n"
				"Skipped Frames   : %d (estimated)\n",
				context[i].name,
				width, height,
				flip_duration_us/1000, flip_duration_us%1000,
				context[i].n_loops,
				n_pixels/1000000, (n_pixels%1000000)/1000,
				expected_ms,
				context[i].ms_elapsed,
				skipped );
				
			//cleanup
			layer_surface[i]->Release(layer_surface[i]);
		}
	}
	
	dfb->WaitIdle(dfb);
	
	TRACE("all tests done, cleaning up layers...\n");
	for(i=0;i<2;i++)
	{	
		layer[i]->Release(layer[i]);
		TRACE("bdvd_dfb_ext_layer_close [layer_h[%d] -> %08X]\n",i,layer_h[i]);
		bdvd_dfb_ext_layer_close(layer_h[i]);
		TRACE("bdvd_dfb_ext_layer_close OK!\n");
	}
}

void* dfb_threadproc(void* void_context)
{
	dfb_thread_context* context=(dfb_thread_context*)void_context;
	IDirectFB* dfb=NULL;
	DFBSurfaceDescription sdesc;
	IDirectFBSurface* argb_src_surface=NULL;
	IDirectFBSurface* lut8_src_surface=NULL;
	
	bool progressive_display = false;
	
	int op_width, op_height;
	
	int i;
	
	//open DirectFB
	dfb=bdvd_dfb_ext_open(6);
		
	//let DirectFB know about the current display format
	int w,h;
	switch(context->video_format)
	{
	case bdvd_video_format_ntsc:
		w=720;
		h=480;
		break;
	case bdvd_video_format_480p:
		w=720;
		h=480;
		progressive_display = true;
		break;
	case bdvd_video_format_720p:
		w=1280;
		h=720;
		progressive_display = true;
		break;
	case bdvd_video_format_1080i:
		w=1920;
		h=1080;
		break;
    default:;
	}
	
	PRINT("Setting Video Mode : %d x %d @ 32 bpp\n",w,h);
	dfb->SetVideoMode(dfb,w,h,32);
	
	//---------------------------------------------
	// create a first test surface in ARGB format
	
	sdesc.flags=DSDESC_WIDTH|DSDESC_HEIGHT|DSDESC_PIXELFORMAT;
	sdesc.width=800;
	sdesc.height=400;
	sdesc.pixelformat=DSPF_ARGB;
	dfb->CreateSurface(dfb,&sdesc,&argb_src_surface);
	
	//--fill it with something--
	
	//draw vertical bars in different colors
	int nbars=1<<6;
	for(i=0;i<nbars;i++)
	{
		argb_src_surface->SetColor(argb_src_surface,(i&0x3)*85,((i&0xC)>>2)*85,((i&0x30)>>4)*85,0x40);	
		argb_src_surface->FillRectangle(argb_src_surface,i*(sdesc.width/nbars),0,sdesc.width/nbars,sdesc.height);
	}
	
	//draw an X on top
	argb_src_surface->SetColor(argb_src_surface,0x80,0,0,0xFF);
	argb_src_surface->DrawLine(argb_src_surface,0,0,sdesc.width,sdesc.height);
	argb_src_surface->DrawLine(argb_src_surface,0,sdesc.height,sdesc.width,0);
	dfb->WaitIdle(dfb);
	
	//---------------------------------------------
	// create a second surface in LUT8 format
	
	//create a 256-colors palette
	DFBColor entries[256];
	for(i=0;i<256;i++)
	{
		entries[i].a= (i==0?0:0x40);
		entries[i].r=0;
		entries[i].g=i;
		entries[i].b= (i==0?0:255-i);
	}
	
	sdesc.flags=DSDESC_WIDTH|DSDESC_HEIGHT|DSDESC_PIXELFORMAT|DSDESC_PALETTE;
	sdesc.width=400;
	sdesc.height=350;
	sdesc.pixelformat=DSPF_LUT8;
	sdesc.palette.entries=entries;
	sdesc.palette.size=256;
	dfb->CreateSurface(dfb,&sdesc,&lut8_src_surface);
	
	//--fill it with something--
	{
		unsigned char* data;
		int pitch;
	
		//fill with a gradient of the palette values
		lut8_src_surface->Lock(lut8_src_surface,DSLF_WRITE,(void**)&data,&pitch);
		
		for(i=0;i<sdesc.height;i++)
			memset(data+i*pitch,i%256,sdesc.width);
		
		lut8_src_surface->Unlock(lut8_src_surface);
		
		//draw an X on top
		lut8_src_surface->SetColorIndex(lut8_src_surface,128);
		lut8_src_surface->DrawLine(lut8_src_surface,0,0,sdesc.width,sdesc.height);
		lut8_src_surface->DrawLine(lut8_src_surface,sdesc.width,0,0,sdesc.height);
	}
	
	/*------------------------------------------------------------------------
	 * WHAT THE PROGRAM DOES:
	 *------------------------------------------------------------------------
	 * 
	 * For each batch of tests, a set of screen regions is defined into the 
	 * display layer.  A source surface is associated to each of those regions.
	 * The size of the region and the size of the surface do not have to match.
	 * When they are different, a sliding window into the source surface is 
	 * used and only that section of the surface will be blit into the region.
	 * For each frame, the sliding window progresses and bounces when it 
	 * reaches one of the boundaries of the surface.
	 */
	
	
	//--------------------------------------
	// BD-J
	{
		TRACE("==== Beginning BD-J tests ====\n");
	
		surface_resolution resolutions[] = {
			{960,1080}, //1080i half-res
			{1280,720}, //720p
			{720,480}   //480i
		};
		
		// first batch of tests

		op_width = 400/(progressive_display ? 2 : 1);
		op_height = 250;

		dfb_operation pg_layer_nofx_ops[]=
		{
			/*      Regions are as follows
			 *     +----------------------+
			 *     |####.            .####|
			 *     |.....   ......   .....|
			 *     |        .####.        |
			 *     |        ......        |
			 *     |.....            .....|
			 *     |####.            .####|
			 *     +----------------------+
			 */
			{lut8_src_surface,  DSBLIT_NOFX, ALIGN_VTOP|ALIGN_HLEFT,      0, 0, op_width, op_height },
			{lut8_src_surface,  DSBLIT_NOFX, ALIGN_VBOTTOM|ALIGN_HLEFT,   0, 0, op_width, op_height },
			{lut8_src_surface,  DSBLIT_NOFX, ALIGN_VTOP|ALIGN_HRIGHT,     0, 0, op_width, op_height },
			{lut8_src_surface,  DSBLIT_NOFX, ALIGN_VBOTTOM|ALIGN_HRIGHT,  0, 0, op_width, op_height },
			{lut8_src_surface,  DSBLIT_NOFX, ALIGN_VCENTER|ALIGN_HCENTER, 0, 0, op_width, op_height },
		};

		op_width = 500/(progressive_display ? 2 : 1);

		dfb_operation bdj_layer_nofx_ops[]=
		{
			/*      Regions are as follows
			 *     +----------------------+
			 *     |        .####.        |
			 *     |....    ......    ....|
			 *     |###.              .###|
			 *     |###.              .###|
			 *     |....    ......    ....|
			 *     |        .####.        |
			 *     +----------------------+
			 */
			{argb_src_surface, DSBLIT_NOFX, ALIGN_VCENTER|ALIGN_HLEFT,    0, 0, op_width, op_height },
			{argb_src_surface, DSBLIT_NOFX, ALIGN_VCENTER|ALIGN_HRIGHT,   0, 0, op_width, op_height },
			{argb_src_surface, DSBLIT_NOFX, ALIGN_VTOP|ALIGN_HCENTER,     0, 0, op_width, op_height },
			{argb_src_surface, DSBLIT_NOFX, ALIGN_VBOTTOM|ALIGN_HCENTER,  0, 0, op_width, op_height },
		};
		
		test_configuration config=
		{
			resolutions,
			SIZEOF_ARRAY(resolutions),
			{
				{
					"Presentation Graphics NOFX",
					bdvd_dfb_ext_bd_hdmv_presentation_layer, 
					DSPF_LUT8, 
					pg_layer_nofx_ops, 
					SIZEOF_ARRAY(pg_layer_nofx_ops), 
					300 
				},
				{ 
					"BD-J Graphics NOFX",
					bdvd_dfb_ext_bd_j_graphics_layer, 
					DSPF_ARGB, 
					bdj_layer_nofx_ops, 
					SIZEOF_ARRAY(bdj_layer_nofx_ops), 
					300
				}
			}
		};
		
		RunTests(dfb,&config);
		
		//---------------------------------------------------
		//second batch of tests
		
		dfb_operation bdj_layer_alphach_ops[]=
		{
			/*      Regions are as follows
			 *     +----------------------+
			 *     |        .####.        |
			 *     |....    ......    ....|
			 *     |###.              .###|
			 *     |###.              .###|
			 *     |....    ......    ....|
			 *     |        .####.        |
			 *     +----------------------+
			 */
			{argb_src_surface, DSBLIT_BLEND_ALPHACHANNEL, ALIGN_VCENTER|ALIGN_HLEFT,    0, 0, op_width, op_height },
			{argb_src_surface, DSBLIT_BLEND_ALPHACHANNEL, ALIGN_VCENTER|ALIGN_HRIGHT,   0, 0, op_width, op_height },
			{argb_src_surface, DSBLIT_BLEND_ALPHACHANNEL, ALIGN_VTOP|ALIGN_HCENTER,     0, 0, op_width, op_height },
			{argb_src_surface, DSBLIT_BLEND_ALPHACHANNEL, ALIGN_VBOTTOM|ALIGN_HCENTER,  0, 0, op_width, op_height },
		};
		
		test_configuration config2=
		{
			resolutions,
			SIZEOF_ARRAY(resolutions),
			{
				{ 
					"Presentation Graphics NOFX",
					bdvd_dfb_ext_bd_hdmv_presentation_layer, 
					DSPF_LUT8, 
					pg_layer_nofx_ops, 
					SIZEOF_ARRAY(pg_layer_nofx_ops), 
					300 
				},
				{ 
					"BD-J Graphics BLEND_ALPHACHANNEL",
					bdvd_dfb_ext_bd_j_graphics_layer, 
					DSPF_ARGB, 
					bdj_layer_alphach_ops, 
					SIZEOF_ARRAY(bdj_layer_alphach_ops), 
					300
				}
			}
		};
		
		RunTests(dfb,&config2);
		
		TRACE("==== All BD-J tests done ====\n");
	}
	//--------------------------------------

	//cleanup
	lut8_src_surface->Release(lut8_src_surface);
	argb_src_surface->Release(argb_src_surface);
	bdvd_dfb_ext_close();

	return NULL;
}
