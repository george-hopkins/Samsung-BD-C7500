#ifndef __WVUtil_h__
#define __WVUtil_h__


#define UINT64 unsigned long long

int WR_Initialize(void);
int WR_Deinitialize(void);
int WR_Connect(const char* pDeviceID, const char* pStreamID, const char* pIPAddr, const char* pDRMURL, const char* pUserData, const char* pAckURL, const char* pHeartbeatURL, unsigned int heartbeatPeriod, const char* pURL);
void WR_Close(void);
int WR_SetPlayProperty(float* pPlaySpeed, long lOffsetInSec);
int WR_Pause(void);
void WR_GetContainerInfo(void);
void WR_GetVideoInfo(void);
void WR_GetAudioInfo(void);

long WR_GetNetworkBandwidth(void);
long WR_GetStreamBitRate(void);

UINT64 WR_NPT2MilliSec(const char* nptTime);

UINT64 WR_GetTotalTime();
UINT64 WR_GetCurrentTime();
unsigned long long WR_GetMediaTime(unsigned long long pts);

int WR_Seek(float speed, unsigned long long wvpts);

int WR_InitialBandwidthCheck(const char* pURL);

int WR_Read(char* pBuf, const unsigned int bufSize);

int WR_GetAdaptiveBitrates(unsigned long** ppAvailableBitrates, unsigned int* pNumOfBitrates, unsigned int* pCurrentIndex);

int WR_GetErrorCode(void);

int WR_GetCopyProtection(unsigned char* macrovision, unsigned char* enableHDCP, unsigned char* CIT);

#endif
