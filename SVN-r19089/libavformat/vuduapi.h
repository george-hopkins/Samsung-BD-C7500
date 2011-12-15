#ifndef __VUDUAPI_h__
#define __VUDUAPI_h__

#define UINT64 unsigned long long

int test_interface();

int IVudu_Initialize(char* pInURL);

int DeInitialize();

int IVudu_Read(char* pBuf, const unsigned int bufSize);

int IGetTotalStreamSize(int64_t* pStreamSize);

int IVudu_SetCurrentPosition(int64_t* pCurrent);

UINT64 IVudu_GetTotalTime();
	
#endif //vuduapi.h

