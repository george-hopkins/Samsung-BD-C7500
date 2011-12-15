#include "libavformat/avformat.h"
#include "WR_wvutil.h"
#include "config.h"

int is_it_widevine = 0;
UINT64 WVduration = 0;
//Widevine_Moon
float WVSpeed = 1;
//Widevine_Moon
void GetCharFromURL(char *pTarget, const char *url, const char *ID)
{
	int i = 0;
	int j, k = 0;
	int IDlen = strlen(ID);
	char *pTemp = pTarget;
	if(strcmp("URL", ID)==0)
	{
		while(url[i] != '|')
		{
			pTemp[k++] = url[i++];
		}
	}
	else
	{
		while(url[i] != '\0')
		{
			for(j=0;j<IDlen;j++)
			{
				if(url[i++] == ID[j])
				{
					if(j == (IDlen-1))
					{
						i++;//to skip '='
						while(url[i] != '|')
						{
							pTemp[k++] = url[i++];
						}
						return;
					}
				}
				else
					j = IDlen;
			}
		}
	}
}

static int WV_open(URLContext *h, const char *url, int flags)
{
	int result;
	int retry;
	int length = 0;
	char m_pDeviceID[128] = {'\0', };
	char m_pType_ID[64] = {'\0', };
	char m_pStreamID[128] = {'\0', };
	char m_pIPAddr[128] = {'\0', };
	char m_pDRMURL[128] = {'\0', };
	char m_pUserData[128] = {'\0', };
	char m_pAckURL[128] = {'\0', };
	char m_pHeartbeatURL[128] = {'\0', };
	char m_pURL[256]  = {'\0', };
	char m_pHeartbeatPeriod[16]  = {'\0', };
	unsigned int m_heartbeatPeriod = 0;

	GetCharFromURL(m_pDeviceID, url, "DEVICE_ID");
	GetCharFromURL(m_pType_ID, url, "DEVICET_TYPE_ID");
	if(m_pType_ID[0] != '\0')
	{
		length = strlen(m_pDeviceID);
		m_pDeviceID[length] = '|';
		strcpy(&m_pDeviceID[length+1], m_pType_ID);
	}

	GetCharFromURL(m_pStreamID, url, "STREAM_ID");
	GetCharFromURL(m_pIPAddr, url, "IP_ADDR");
	GetCharFromURL(m_pDRMURL, url, "DRM_URL");
	GetCharFromURL(m_pUserData, url, "USER_DATA");
	GetCharFromURL(m_pAckURL, url, "ACK_URL");
	GetCharFromURL(m_pHeartbeatURL, url, "HEARTBEAT_URL");
	GetCharFromURL(m_pHeartbeatPeriod, url, "HEARTBEAT_PERIOD");
	GetCharFromURL(m_pURL, url, "URL");
	m_heartbeatPeriod = atoi(m_pHeartbeatPeriod);
	//av_log(NULL,AV_LOG_INFO,"======>[DEVICE_ID = %s]\n",m_pDeviceID);
 	//av_log(NULL,AV_LOG_INFO,"======>[STREAM_ID = %s]\n",m_pStreamID);
  	//av_log(NULL,AV_LOG_INFO,"======>[IP_ADDR = %s]\n",m_pIPAddr);
   	//av_log(NULL,AV_LOG_INFO,"======>[URL = %s]\n",m_pURL);

    	h->is_streamed = 1;
	for(retry=0;retry<5;retry++)
	{
		result = WR_Initialize();
		if(result != 1)
		{
			if(retry==4)
			{
//				av_log(NULL,AV_LOG_INFO,"======>[Wrapper fail to initialize]<======\n");
				return -1;
			}
		}
		else
		{
//			av_log(NULL,AV_LOG_INFO,"======>[Wrapper success to initialize]<======\n");
			retry = 5;//success to initialize
		}
	}

	for(retry=0;retry<5;retry++)
	{
		result = WR_Connect(m_pDeviceID, m_pStreamID, m_pIPAddr, m_pDRMURL, m_pUserData, m_pAckURL,
		                                                                     m_pHeartbeatURL, m_heartbeatPeriod, m_pURL);
		if(result == 1)
		{
//			av_log(NULL,AV_LOG_INFO,"======>[Wrapper success to connect]<======\n");
			WVduration = WR_GetTotalTime();
//			av_log(NULL,AV_LOG_INFO,"======>[Duration] : %lld<======\n",WVduration);
			return 0;
		}
	}
	av_log(NULL,AV_LOG_INFO,"======>[Wrapper fail to connect]<======\n");
	return -1;
}

static int WV_read(URLContext *h, uint8_t *buf, int size)
{
	int result;
	unsigned char retry;
       do
	{
//       	av_log(NULL,AV_LOG_INFO,"======>[Requested data size : %d]<======\n",size);
		result = WR_Read((char*)buf, size);
		if(result == 0)
		{
	  		usleep(100000);
		}
       } while (result == 0);

//Widevine_Moon
	if (result == -1000)
	{
		//av_log(NULL,AV_LOG_INFO,"End of Media detected. server_ret_value == -1000\n");
		h->server_ret_value = -1000;
		return -1;
	}
	else
	{
		h->server_ret_value = 0;
	}
//Widevine_Moon

	if(result < 0)
	{
		for(retry=0;retry<5;retry++)
		{
			result = WR_Read((char*)buf, size);
			if(result > 0)
			{
				retry = 5;
//				av_log(NULL,AV_LOG_INFO,"====>[received size is %d]<====\n", result);
				return result;
			}
			else
			{
				usleep(100000);
			}
		}
		av_log(NULL,AV_LOG_INFO,"======>[WV_read fail]<======\n");
		return -1;
	}
	else
	{
//		av_log(NULL,AV_LOG_INFO,"====>[received size is %d]<====\n", result);
		return result;
	}
}

static int WV_write(URLContext *h, uint8_t *buf, int size)
{

	return 0;
}

static int64_t WV_seek(URLContext *h, int64_t off, int whence)
{
 	int result;
	float Speed = (float)av_WV_getSpeed();
	if(whence != 0 || off != 0)
	{
	return -1;
}
	result = WR_Seek(Speed, whence+off);
	av_log(NULL,AV_LOG_INFO,"====>[offset: %lld,   whence:%d]<====\n", off, whence);
	return 0;
}

static int WV_close(URLContext *h)
{
	int result;
	result = WR_Deinitialize();
	if(result == 1)
	{
		av_log(NULL,AV_LOG_INFO,"====>[Widevine util Succeessfully deinitialized.]<====\n");
		return 0;
	}
	else
	{
		av_log(NULL,AV_LOG_INFO,"====>[Widevine Termination failed.]<====\n");
		return -1;
	}
}


URLProtocol wvine_protocol = {
	"wvine",
	WV_open,
	WV_read,
	WV_write,
	WV_seek,
	WV_close,
};

int av_WV_setSpeed(float Speed, unsigned long long wvpts)
{
	int result = 0;

	result = WR_Seek(Speed, wvpts);
	if(result != 1)
	{
		av_log(NULL,AV_LOG_INFO,"======>[Wrapper fail to change speed]<======\n");
		return 0;
	}
//Widevine_Moon
		WVSpeed = Speed;
//Widevine_Moon
		return 1;
}

//Widevine_Moon
int av_WV_getSpeed(void)
{
	return (int)WVSpeed;
}
//Widevine_Moon

int av_WV_GetAvailableBitrates(unsigned int* pNumOfBitrates, unsigned long** ppBitrates)
{
	int result = 0;
	if(pNumOfBitrates==NULL)
		return 0;

	unsigned int currentIndex=0;
	result = WR_GetAdaptiveBitrates(ppBitrates, pNumOfBitrates, &currentIndex);
	if(result == 1)
	{
		av_log(NULL,AV_LOG_INFO,"======>[Wrapper success to get available bitrates]<======\n");
		return 1;
	}
	else
		return 0;//fail to get available bitrates
}

int av_WV_GetCurrentBitrates(unsigned long* pCurrentBitrates)
{
	int result = 0;
	unsigned long* pBitrates=NULL;
	unsigned int ratesReturned=0;
	unsigned int currentIndex=0;
	if(pCurrentBitrates==NULL)
		return 0;

	result = WR_GetAdaptiveBitrates(&pBitrates, &ratesReturned, &currentIndex);
	if( result==0 || currentIndex>=ratesReturned )
	{
		av_log(NULL,AV_LOG_INFO,"======>[Wrapper fail to get current bitrates]<======\n");
		return 0;
	}
	else
	{
		*pCurrentBitrates=pBitrates[currentIndex];
		return 1;//success to get current bitrates
	}
}

int av_WV_GetErrorCode(void)
{
	int Err_Code = 0;

	Err_Code = WR_GetErrorCode();
	if( Err_Code == 200 ) // Error Code '200' means WV_STATUS_OK
	{
		av_log(NULL,AV_LOG_INFO,"======>[Wrapper fail to get error code]<======\n");
		return 0;
	}
	else
	{
		return Err_Code;
	}
}

int av_WV_GetCopyProtection(unsigned char* macrovision, unsigned char* enableHDCP, unsigned char* CIT)
{
	int Err_Code = 0;

	Err_Code = WR_GetCopyProtection(macrovision, enableHDCP, CIT);
	if (Err_Code != 0)
	{
		av_log(NULL,AV_LOG_INFO,"======>[Wrapper fail to get Copy Protection Information]<======\n");
		return Err_Code;		
	}

	return Err_Code;
}

int av_WV_GetTotalTime(unsigned long long* ulTotalTime)
{
	*ulTotalTime = WR_GetTotalTime( );	

	return 0;
}


int av_WV_GetCurrentTime(unsigned long long* ulCurrentTime)
{

	*ulCurrentTime = WR_GetCurrentTime( );	

	return 0;
}



int av_WV_GetMediaTime(unsigned long long* ulMediaTime, unsigned long long curPTS)
{

	*ulMediaTime = WR_GetMediaTime(curPTS);	

	return 0;
}

