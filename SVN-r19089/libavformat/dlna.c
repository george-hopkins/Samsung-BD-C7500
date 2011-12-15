#include "libavformat/avformat.h"
#include <dlfcn.h>

//#define DLNA_USE_STATIC

#include "DLNAReader.h"

#ifndef DLNA_USE_STATIC

typedef int (*pDMP_Open)(char* path);
typedef int (*pDMP_Close)(int id);
typedef int (*pDMP_Read)(int id, void* buffer, long reqBytes, long* retBytes);
typedef long long (*pDMP_SetPosition)(int id, long long position, double timePosition, int whence);
typedef int (*pDMP_CheckServerSeekCapability)(int id);
typedef int (*pDMP_GetFileSize)(int id, long long* length);
typedef int (*pDMP_GetAudioFormatInfo)(int id, DLNAAudioInfo* MusicInfo);
typedef int (*pDMP_GetFileDuration)(int id, long long* duration);
typedef int (*pDMP_GetCurPosition)(int id, long long* position);

pDMP_Open dlna_DMP_Open;
pDMP_Close dlna_DMP_Close;
pDMP_Read dlna_DMP_Read;
pDMP_SetPosition dlna_DMP_SetPosition;
pDMP_CheckServerSeekCapability dlna_DMP_CheckServerSeekCapability;
pDMP_GetFileSize dlna_DMP_GetFileSize;
pDMP_GetAudioFormatInfo dlna_DMP_GetAudioFormatInfo;
pDMP_GetFileDuration dlna_DMP_GetFileDuration;
pDMP_GetCurPosition dlna_DMP_GetCurPosition;

void* hLib_dlna = NULL;
void* dlError = NULL;
#endif

static int dlna_lib_initialized = 0;
static int DLNA_ID = 0;
char g_path[1024];
static long long glfileSize = 0;
DLNAAudioInfo DLNA_AudioInfo;
extern int ff_DLNA_GetAudioFormatInfo(int* type, int* SampleRate, int* Channel);
extern int64_t ff_DLNA_GetFileSize(void);

static int dlna_open(URLContext *h, const char *url, int flags) {

        printf("[dlna_open]DLNA ID = %d \n", DLNA_ID);
        if(DLNA_ID != 0) {
            printf("Cannot connect to DLNA server, DLNA is already connected!!!\n");
            return -1;
        }

	#ifndef DLNA_USE_STATIC
       if( dlna_lib_initialized == 0) {
	if(dlna_Initialize() == -1) {
		printf("ERROR : DLNA dlopen fail\n");
                    dlna_lib_initialized = 0;
		return -1;
	}
       }
	#endif

	char *p;
	memset(g_path,0,1024);

	p = strstr(url,"http://");
	strcpy(g_path, p);

	printf("[dlna_open] Parsed DLNA URL = %s",g_path);
#ifdef DLNA_USE_STATIC
	DLNA_ID = DMP_Open(g_path);
#else
	DLNA_ID = dlna_DMP_Open(g_path);
#endif
	if(DLNA_ID == -1) {
		printf("[dlna_open] FAIL \n");
             DLNA_ID = 0;        
		return -1;
	}
	printf("[dlna_open]DMP_Open SUCCESS\n");
        glfileSize = 0;
#ifdef DLNA_USE_STATIC
	if(DMP_CheckServerSeekCapability(DLNA_ID) == DMP_FALSE) {
		//printf("[dlna_open] DLNA SERVER CANNOT SEEK!!!\n");
		h->is_streamed = 1;
	} else
	{
             //printf("[dlna_seek] DLNA SERVER CAN SEEK~~~\n");      
		h->is_streamed = 0;
	}
	DMP_GetFileSize(DLNA_ID,&glfileSize );    
       printf("\n [dlna_open]  FILE SIZE! <%lld>\n", glfileSize);        
#else
	if(dlna_DMP_CheckServerSeekCapability(DLNA_ID) == DMP_FALSE) {
		printf("[dlna_open] DLNA SERVER CANNOT SEEK!!!\n");
		h->is_streamed = 1;
	} else
	{
             printf("[dlna_seek] DLNA SERVER CAN SEEK~~~\n");            
		h->is_streamed = 0;
	}

	if(dlna_DMP_GetFileSize(DLNA_ID,&glfileSize ) == DMP_FALSE) {    
           printf("\n [dlna_open]  Get File Size Fail! \n");
           glfileSize = -1;
	}
       printf("\n [dlna_open]  FILE SIZE! <%lld>\n", glfileSize);    
#endif
	return 0;
}

static int dlna_read(URLContext *h, uint8_t *buf, int size) {
	long retBytes;
       int iRet=RET_READ_OK;

#ifdef DLNA_USE_STATIC
	iRet = DMP_Read(DLNA_ID, buf , size , &retBytes);
#else
	iRet = dlna_DMP_Read(DLNA_ID, buf , size , &retBytes);
	//printf("[dlna_read] Size[%d] retBytes[%d] ret = %d\n", size ,  (int)retBytes, iRet);
#endif

        if(((int)retBytes > size)) {
            printf("[dlna read]retBytes > reqBytes : Something is wrong...\n");
            printf("[dlna read]retBytes > reqBytes : Something is wrong...\n");
            printf("[dlna read]retBytes > reqBytes : Something is wrong...\n");
            return -1;
        }

        if(retBytes > 0) {
            //dlna server transferred the data to us.
            return (int)retBytes;
        }
        
        switch(iRet) {
            case RET_READ_OK:
                h->server_ret_value = 0;
                break;
            case RET_READ_ERR_DISCONNECTED:
                printf("[FFMPEG][%s] DLNA DISCONNECTED!\n",__FUNCTION__);
                h->server_ret_value = -777;
                break;
//            case RET_EOS:
//                printf("[FFMPEG][%s] END OF STREAM\n",__FUNCTION__);
//                break;
            case RET_READ_ERR_TIMEOUT:
                printf("[FFMPEG][%s] DLNA NETWORK TIMEOUT!\n",__FUNCTION__);
                h->server_ret_value = -777;
                break;
            case DMP_FALSE:
                printf("[FFMPEG][%s] DLNA READ FAILED!\n",__FUNCTION__);
                h->server_ret_value  = -1;
                break;
            default:
                break;
        }

	return (int)retBytes;
}

static int dlna_write(URLContext *h, uint8_t *buf, int size) {
	printf("[FFMPEG] dlna_write\n");
}

static int64_t dlna_seek(URLContext *h, int64_t off, int whence) {
	long long ret;
	//printf("\n[dlna_seek]  SEEK!!!off[%lld]  whence[%d]\n",off,whence);
#if 0 // reconnecting
      int64_t current_position;
      // check current pos
      if(dlna_DMP_GetCurPosition(DLNA_ID, &current_position) == DMP_TRUE) {
  		printf("[DLNA SEEK] get current position = %lld , filesize = %lld \n",current_position, glfileSize);        
            //compare current position with filesize 
            if(current_position == glfileSize) {
                //try to reconnect
                int iRet;
                iRet = dlna_DMP_Close(DLNA_ID);
                if(iRet == 0) {
                    // close success
                    DLNA_ID = 0;
                    printf("[DLNA SEEK] Try to Reconnect .. url : %s \n",g_path);
                    DLNA_ID = dlna_DMP_Open(g_path);
                	if(DLNA_ID == -1) {
                		printf("[DLNA SEEK] current position == file position and reconnecting open fail [%s][%d]\n",__FUNCTION__,__LINE__);
                           DLNA_ID = 0;        
                		return -1;
                	}
                    //set target position
                    switch(whence) {
                        case SEEK_SET:
                            // do off set control below.
                            break;
                        default:
                            printf("[%s][%d]This is other case..hmm let me think about it \n",__FUNCTION__,__LINE__);
                            break;
                    }
                } else {
                    // seek fail
                    printf("[DLNA SEEK] current position == file position and reconnecting close fail [%s][%d]\n",__FUNCTION__,__LINE__);
                    return -1;
                }
            }
      }
#endif
    
#ifdef DLNA_USE_STATIC
	if (whence == AVSEEK_SIZE && glfileSize > 0) {
		return (int64_t)glfileSize;
	}
	else if(DMP_CheckServerSeekCapability(DLNA_ID) == DMP_FALSE) {
		printf("[dlna_seek] DLNA SERVER CANNOT SEEK!!!\n");
		return -1;
	}

       //printf("off = %lld , filesize = %lld \n",off,glfileSize);
        if((glfileSize > 0) && (off > glfileSize)) {

           printf("[DLNA_SEEK]!! ReqeustOffset > FileSize reqested!!!!! RequestedOffset=%lld, FileSize=%lld\n",off,glfileSize);            
           return -1;            
           
        } 
        // prevent disconnecting from dlna server.
        if(glfileSize > 0 && off == glfileSize) {
            printf("[DLNA_SEEK]!!!!!! ReqeustOffset == FileSize !!!!!adjust offset(-2)  Offset=%lld, FileSize=%lld\n",(off - 2),glfileSize);            
            off = glfileSize -2;
        }

	switch(whence) {
		case SEEK_SET:
			ret =  DMP_SetPosition(DLNA_ID , (long long)off , 0 , whence);
			break;
		case SEEK_CUR:
			printf("[SEEK CURRENT!!]\n");
			ret = 0;
			break;
		case SEEK_END:
                    ret =glfileSize;
			break;
		default:
			ret = -1;
	}
#else
	if ((whence == AVSEEK_SIZE) && (glfileSize > 0)) {
		return (int64_t)glfileSize;
	}
	else if(dlna_DMP_CheckServerSeekCapability(DLNA_ID) == DMP_FALSE) {
		//printf("[dlna_seek] DLNA SERVER CANNOT SEEK!!!\n");
		return -1;
	}

       //printf("off = %lld , filesize = %lld \n",off,glfileSize);
        if((glfileSize > 0) && (off > glfileSize)) {

           printf("[DLNA_SEEK]!! ReqeustOffset > FileSize reqested!!!!! RequestedOffset=%lld, FileSize=%lld\n",off,glfileSize);            
           return -1;            
           
        } 
        // prevent disconnecting from dlna server.
        if((glfileSize > 0) && (off == glfileSize)) {
            printf("[DLNA_SEEK]!!!!!! ReqeustOffset == FileSize !!!!!adjust offset(-2)  Offset=%lld, FileSize=%lld\n",(off - 2),glfileSize);            
            off = glfileSize -2;
        }

	switch(whence) {
		case SEEK_SET:
                    //printf("SEEK_SET..\n");
			ret =  dlna_DMP_SetPosition(DLNA_ID , (long long)off , 0 , whence);
			break;
		case SEEK_CUR:
			printf("[SEEK CURRENT!!]\n");
			ret = 0;
			break;
		case SEEK_END:
                    //printf("SEEK_END..\n");          
        		//ret = glfileSize;
			break;
		default:
			ret = -1;
	}
#endif
      //printf("[dlna seek] dmp_setPosition Ret = %lld\n",(int64_t)ret);
	return (int64_t)ret;
}

static int dlna_close(URLContext *h) {
    int iRet=0;
	#ifdef DLNA_USE_STATIC
    iRet = DMP_Close(DLNA_ID);
	#else
    iRet = dlna_DMP_Close(DLNA_ID);
	#endif
    printf("\n [dlna_close] iRet = %d \n", iRet);

    if(iRet == 0)
    {
        DLNA_ID = 0; // must be set to 0. fscherry
        glfileSize = 0;
        //dlna_DeInitialize(); // DO NOT CALL for speed. 
        return 0;
    }
    else if(iRet == -1)
    {
        return -1;
    }
}

int dlna_Initialize(void) {
	#ifndef DLNA_USE_STATIC
       printf("DLNA lib  dlloading...\n");
	hLib_dlna = dlopen("libdmp.so",RTLD_LAZY);
	dlError = dlerror();
	if(hLib_dlna == NULL)
	{
		printf("\n [libdmp.so dlopen error]  %s\n", (char*)dlError);
		return -1;
	}

	//so library dlsym
	dlna_DMP_Open = (pDMP_Open)dlsym(hLib_dlna, "DMP_Open");
	if(dlna_DMP_Open == NULL)
	{
		printf( "\n [dlna_DMP_Open dlsym error] \n");
		dlclose(hLib_dlna);
		return -1;
	}

	dlna_DMP_Close = (pDMP_Close)dlsym(hLib_dlna, "DMP_Close");
	if(dlna_DMP_Close == NULL)
	{
		printf( "\n [dlna_DMP_Close dlsym error] \n");
		dlclose(hLib_dlna);
		return -1;
	}

	dlna_DMP_Read = (pDMP_Read)dlsym(hLib_dlna, "DMP_Read");
	if(dlna_DMP_Read == NULL)
	{
		printf( "\n [dlna_DMP_Read dlsym error] \n");
		dlclose(hLib_dlna);
		return -1;
	}

	dlna_DMP_SetPosition = (pDMP_SetPosition)dlsym(hLib_dlna, "DMP_SetPosition");
	if(dlna_DMP_SetPosition == NULL)
	{
		printf( "\n [dlna_DMP_SetPosition dlsym error] \n");
		dlclose(hLib_dlna);
		return -1;
	}

	dlna_DMP_CheckServerSeekCapability = (pDMP_CheckServerSeekCapability)dlsym(hLib_dlna, "DMP_CheckServerSeekCapability");
	if(dlna_DMP_CheckServerSeekCapability == NULL)
	{
		printf( "\n [dlna_DMP_CheckServerSeekCapability dlsym error] \n");
		dlclose(hLib_dlna);
		return -1;
	}
	dlna_DMP_GetFileSize = (pDMP_GetFileSize)dlsym(hLib_dlna, "DMP_GetFileSize");
	if(dlna_DMP_GetFileSize == NULL)
	{
		printf( "\n [dlna_DMP_GetFileSize dlsym error] \n");
		dlclose(hLib_dlna);
		return -1;
	}

	dlna_DMP_GetAudioFormatInfo = (pDMP_GetAudioFormatInfo)dlsym(hLib_dlna, "DMP_GetAudioFormatInfo");
	if(dlna_DMP_GetAudioFormatInfo == NULL)
	{
		printf( "\n [dlna_DMP_GetAudioFormatInfo dlsym error] \n");
		dlclose(hLib_dlna);
		return -1;
	}

	dlna_DMP_GetFileDuration = (pDMP_GetFileDuration)dlsym(hLib_dlna, "DMP_GetFileDuration");
	if(dlna_DMP_GetFileDuration == NULL)
	{
		printf( "\n [dlna_DMP_GetFileDuration dlsym error] \n");
		dlclose(hLib_dlna);
		return -1;
	}

       dlna_DMP_GetCurPosition = (pDMP_GetCurPosition)dlsym(hLib_dlna, "DMP_GetCurPosition");
	if(dlna_DMP_GetCurPosition == NULL)
	{
		printf( "\n [dlna_DMP_GetCurPosition dlsym error] \n");
		dlclose(hLib_dlna);
		return -1;
	}       
       printf("DLNA lib  dlloading DONE\n");
       dlna_lib_initialized = 1;
	#endif
	return 0;
}

int dlna_DeInitialize(void) {
	#ifndef DLNA_USE_STATIC
       printf( "\n dlna lib dlclose \n");    
	if(dlclose(hLib_dlna)) {
		printf( "\n [dlclose error] \n");
		return -1;
	}
	#endif
	//success
	return 0;
}

int ff_DLNA_GetAudioFormatInfo(int* type, int* SampleRate, int* Channel) {

      if(dlna_DMP_GetAudioFormatInfo(DLNA_ID , &DLNA_AudioInfo) == DMP_FALSE) {
        return -1;
      }
   // av_log(NULL,AV_LOG_INFO,"\nDLNA_AudioInfo.Type = %d,*SampleRate=%d ,*Channel =%d\n",DLNA_AudioInfo.Type,DLNA_AudioInfo.SamplingRate,DLNA_AudioInfo.Channel);

      switch(DLNA_AudioInfo.Type) {
        case DLNA_NO_MEDIA_TYPE:
            *type = -1;
            break;
        case DLNA_LPCM:
            *type = 1;
            break;
        case DLNA_MP3:
            *type = 2;
            break;
        default:
            break;
      }

      *SampleRate = DLNA_AudioInfo.SamplingRate;
      *Channel = DLNA_AudioInfo.Channel;

      return 0;

}

int64_t ff_DLNA_GetFileSize(void) {

    long long fileSize;
#ifdef DLNA_USE_STATIC
    DMP_GetFileSize(DLNA_ID,&fileSize );
#else
    dlna_DMP_GetFileSize(DLNA_ID,&fileSize );
#endif
    printf("[DLNA ff_DLNA_GetAudioFormatInfo] GET FILESIZE = %lld \n" , (int64_t)fileSize);

    return (int64_t)fileSize;
    
}

URLProtocol dlna_protocol = {
	"dlna",
	dlna_open,
	dlna_read,
	dlna_write,
	dlna_seek,
	dlna_close,
};

