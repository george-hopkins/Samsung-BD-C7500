#include "avformat.h"
#include <unistd.h>
#include "mms.h"

mms_t *pmms=NULL;
const char *MMS_URL=NULL;
URLContext *MMS_URLContext=NULL;

static int mms_proto_close(URLContext *h);
static int mms_proto_open(URLContext *h, const char *url, int flags);
int mms_proto_open(URLContext *h, const char *url, int flags) {
	
	printf("\n[FFMPEG]mms_proto_open\n");
	MMS_URL = url;
	MMS_URLContext = h;

//	pmms = mms_connect(NULL,NULL,url,1);
	pmms = mms_connect(NULL,NULL,url,1000*1024);	
	if(pmms){
		printf("[FFMPEG]Connect OK\n");

		if(!mms_get_seekable(pmms))
		{
			printf("This is not Seekable]\n");
			h->is_streamed = 1;
		}
		else
		{
			h->is_streamed = 0;
		}
		h->is_streamed = 1;
		return 0;
  	}else{
    		printf("[FFMPEG]Connect Failed\n");
		return -1;
  	}
	
}

static int mms_proto_read(URLContext *h, uint8_t *buf, int size) {
	 int res = mms_read(NULL, pmms, buf, size);
//	printf("return data1 = %d \n",res);
	 return res;
	}
	 
static int mms_proto_write(URLContext *h, uint8_t *buf, int size) {
	//printf("\n[BT][bt_write] CALLED ...\n");
	//
	return 0;
}
#define MMS_ADJUST_SEEK_POSITION 128000
static int64_t mms_proto_seek(URLContext *h, int64_t off, int whence) {
	
	int res;

	printf("\n [FFMpeg][mms_proto_seek(^o^)]  Off[%lld] , whence[%d]\n",off , whence);
	if((mms_get_length(pmms) - MMS_ADJUST_SEEK_POSITION) < (int)off)
	{
		printf("[FFMpeg][mms_seek] adjust Off set\n");
		off = off - MMS_ADJUST_SEEK_POSITION;
		//off =(int64_t) mms_get_current_pos(pmms);
	}

	if((int)mms_get_current_pos(pmms) == mms_get_length(pmms))
	{
		printf("[FFMpeg][mms_seek] Current Position is at the end of the stream!!!\n");
		mms_proto_close(pmms);
		printf("TRY RECONNECT...\n\n");
		while(mms_proto_open(MMS_URLContext, MMS_URL, 0) < 0)
		{
			printf("[FFMpag][MMS_SEEK] RE Connect Fail!!!\n");
			//mms_proto_close(pmms);
			usleep(2000);
		}
		//return -1;
		//off = 0;
		//peek_and_set_pos(NULL,pmms);
		off = off - MMS_ADJUST_SEEK_POSITION;
		//off =(int64_t) mms_get_current_pos(pmms);
	}
	
	if (whence == AVSEEK_SIZE) 
	{
        	return mms_get_length(pmms);
	}

//	if((whence == SEEK_SET)  && ((int)off >= (mms_get_length(pmms) - MMS_ADJUST_SEEK_POSITION)))
//	{
//		printf("[FFMpeg][mms_seek](><;;;;;;;)) Off is Too big  OFF[%d] , MMS_LENGTH[%d]\n" , (int)off , mms_get_length(pmms));
//		return mms_get_length(pmms);
//		//return  -1;
//		//return 0;
//	}

	switch (whence) {
		case SEEK_SET:
			res=mms_seek(NULL,pmms,(int)off, SEEK_SET);
			printf("[FFMpeg] SEEK_SET : RET = %d \n" , res);			
			break;
		case SEEK_CUR:
			res = mms_seek(NULL,pmms,(int)off, SEEK_CUR);
			printf("[FFMpeg] SEEK_CUR : RET = %d \n" , res);			
			break;
		case SEEK_END:
			//res=mms_seek(NULL,pmms,(int)off, 2);
			//res = mms_get_length(pmms);
			res = -1;
			printf("[FFMpeg] SEEK_END : RET = %d \n" , res);
			break;
		default:
			printf ("[FFMpeg][mms_seek]input_mms: unknown origin in seek!\n");
			res=-1;
	}
	printf("[FFMpeg][mms_seek](int64_t)res = %d\n", res);
	
	if(res == mms_get_length(pmms))
	{
		printf("[FFMpeg] [MMS_SEEK] AT THE END OF THE FILE ... Try Reconnect ! \n");
		mms_proto_close(pmms);
		printf("TRY RECONNECT...\n\n");
		while(mms_proto_open(MMS_URLContext, MMS_URL, 0) < 0)
		{
			printf("[FFMpag][MMS_SEEK] RE Connect Fail!!!\n");
			usleep(2000);
		}
		
	}
	return (int64_t)res;
	
}


int mms_proto_close(URLContext *h) {
	
	mms_close(pmms);
	pmms = NULL;
	return 0;
	
}


int MMSTimeSeek(double time_sec)
{
	int res=mms_time_seek (NULL,pmms,time_sec);
	return res;
}

URLProtocol mms_protocol = {
	"mms",
	mms_proto_open,
	mms_proto_read,
	mms_proto_write,
	mms_proto_seek,
	mms_proto_close,
};

