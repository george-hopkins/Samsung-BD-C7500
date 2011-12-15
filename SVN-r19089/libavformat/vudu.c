#include "libavformat/avformat.h"
#include "config.h"
#include "vuduapi.h"
#include <sys/time.h>
//#ifdef VUDU

int is_it_VUDU = 0;
UINT64 VUDUduration = 0;

extern void Check_vudu_decoder_Status(void);

enum StreamingStatus
{
	Status_OK=0,
	Status_Fail=-1,	
	Status_Stream_Not_Found=-404,
	Status_Connection_Failed=-1000,
	Status_Network_Disconnected=-1001,
	Status_Network_Slow=-1002,
	Status_Authentication_Failed=-1100,
	Status_End_Of_Stream=-2000
};

static int vudu_open(URLContext *h, const char *uri, int flags,int lFilePos) 
{

	int retval ;
	printf("@@@KOO vudu_open start ::%s\n",uri);
	retval=IVudu_Initialize(uri);

	if(retval==0)
	{
		VUDUduration = IVudu_GetTotalTime();	
		printf(" @@@KOO vudu_open VUDUduration : %lld\n",VUDUduration);
	}	
	printf("@@@KOO vudu_open close\n");
	return retval;
}

static int vudu_read(URLContext *h, uint8_t *buf, int length) 
{
	int result;
	unsigned char retry;
	

	struct timeval starttime,endtime;
	gettimeofday(&starttime , NULL);

	do
	{
		if (url_interrupt_cb())
		{
			printf("@@@ call back is called\n");
			return AVERROR(EINTR);
		}
		gettimeofday(&endtime , NULL);
		if( (endtime.tv_sec  - starttime.tv_sec) > 30)
		{
			printf("@@@ vudu_read time out 30 sec\n");
			//return -1;
			//return -99;
		}
		
		result = IVudu_Read((char*)buf, length);
		if(result ==0 )
		{
			//printf("@@@ vudu_read result=%d, request length=%d\n",result, length);
			usleep(90000);
			Check_vudu_decoder_Status();
		}
		else if(result < 0)
		{
			printf("@@@KOO vudu_read %d, %d\n",result, length);

			if(result==Status_Network_Disconnected)
	{
				printf("@@@Network Connection disconnected code= %d\n",Status_Network_Disconnected);
				return -99;
			}
			if(result==Status_End_Of_Stream)
			{
				printf("@@@Status_End_Of_Stream code= %d\n",Status_End_Of_Stream);
				return -1;
			}	
				
			
		}else
	{
			//printf("@@@ vudu_read result=%d, request length=%d\n",result, length);
		return result;
	}

       } while (result ==0 );


}

static int vudu_write(URLContext *h, uint8_t *buf, int size) 
{
	return 0;
}

static int64_t vudu_seek(URLContext *h, int64_t off, int whence) 
{
	printf("@@@KOO vudu_seek start whence : %d , off: %lld\n",whence, off);
	printf("@@@KOO vudu_seek end\n");
	return 0;
}

static int vudu_close(void) 
{
	int retval;
	printf("@@@KOO vudu_close start \n");
	retval=IVudu_DeInitialize();
	is_it_VUDU =0;
	VUDUduration =0;
	if(retval==0)
	{
		printf("@@@KOO vudu_close end OK\n");
		
		return 0;
	}
	else
	{
		printf("@@@KOO vudu_close end False\n");
		return -1;
	}
}

URLProtocol vudu_protocol = {
	"vudu",
	vudu_open,
	vudu_read,
	vudu_write,
	vudu_seek,
	vudu_close,
};


av_VUDU_SetCurrentPosition(unsigned long long* pCurrent)
{
	printf("@@@KOO av_VUDU_SetCurrentPosition start \n");
	IVudu_SetCurrentPosition(pCurrent);
	printf("@@@KOO av_VUDU_SetCurrentPosition end \n");
}

//#endif //VUDU

