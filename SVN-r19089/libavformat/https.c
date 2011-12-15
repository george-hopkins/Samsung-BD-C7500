#include "libavutil/base64.h"
#include "libavutil/avstring.h"
#include "avformat.h"
#include <stdio.h>
#include <unistd.h>
#include "network.h"
#include "os_support.h"

#include <sys/time.h>
#include <stdarg.h>


#define BUFFER_SIZE 1024
#define URL_SIZE    4096


#define HTTPS_BUFFER_SIZE	1024
#define HTTPS_INT_MAX		((int)(~0U>>1))

static int https_close(void);

char HEADER_BUF[HTTPS_BUFFER_SIZE];

char m_header[HTTPS_BUFFER_SIZE+1];
char* pURL = NULL;
char *p;
int isHeader = 1;
int64_t totallength =0;
int g_dataSize =0;
URLContext * https_urlcontext = NULL;
unsigned long m_headerSize=0;
unsigned long first_data_size =0;


extern int bSocketSslOpenState;
extern int proto_SslOpen(char* pURL, int port);
extern int proto_SslClose(void);
extern int proto_SslSend(char* buffer , unsigned int size);
extern int proto_SslReceive(char* buffer , unsigned int size, int isheader);

///////////////////////////////////

//¸ðµç function
//		-1; fail
//   		0; success
char* HttpsUtil_StringInString(const char* str1, const char* str2) {
	if((str1 == NULL) ||( str2 == NULL)) {
		return NULL;
	}
		
	return strstr(str1,str2);
	
}
int HttpsUtil_GetToken(char* str, char* data, const char* beforeStr, const char* afterStr, unsigned long* pSize)
{

	char* pStart;
	char* pEnd;

	if( data == NULL) {
		return -1;
	}

	if( (str==NULL) ||(data==NULL) ) {
		return -1;
	}

	if(beforeStr!=NULL)
	{
		pStart = HttpsUtil_StringInString(data,beforeStr);
		if(pStart==NULL)
		{
			return -1;
		}
		pStart += strlen(beforeStr);
	}	
	else {
		pStart = data;
	}

	pEnd = NULL;

	if(afterStr!=NULL)
		pEnd = HttpsUtil_StringInString(pStart, afterStr);

	if(pEnd==NULL)
	{
		int length = strlen(data);
		pEnd = (char*)(data + length);
	}

	memset(str,0,*pSize);

	if(*pSize >= (unsigned long)(pEnd-pStart))
	{ 
		strncpy(str, pStart, pEnd-pStart);
		*pSize = pEnd-pStart;
	}
	else
	{
		strncpy(str, pStart, *pSize);
	}

	return 0;
		 
}
///////////////////////////////////

static int https_open(URLContext *h, const char *uri, int flags,int64_t lFilePos) {
	struct timeval starttime,endtime;
	starttime.tv_sec = 0;
	endtime.tv_sec = 0;

	int pURL_length = 0;

	char *pLine;
	char *pInfo;

	char hostName[1024];
	char tmp[1024];
	char path[1024];

	unsigned long info_size=1023;
	int port;
	char strPort[32];

	https_urlcontext = h;

	//Set URL
	pURL_length = strlen(uri) + 1;
	pURL = av_malloc(pURL_length);
	memset(pURL,0,pURL_length);
	strcpy(pURL,uri);	
	printf("[https_open](VER 1) URL = %s  , %lld\n",pURL,lFilePos);

	pLine = av_malloc(pURL_length + 128);
	pInfo = av_malloc(pURL_length);

	memset(pLine , 0 , pURL_length+128);
	memset(pInfo,0,pURL_length);
	
	memset(m_header,0,HTTPS_BUFFER_SIZE+1);
	memset(hostName,0,1024);
	memset(tmp,0,1024);
	memset(path,0,1024);

	memset(strPort,0,32);
	HttpsUtil_GetToken(tmp,pURL,"https://","/",&info_size);
	
	info_size = 1023;
	HttpsUtil_GetToken(hostName,tmp,NULL,":",&info_size);

	printf("[https_open]  hostName = %s\n",hostName);

	unsigned long portSize = 31;
	int  isPort = HttpsUtil_GetToken(strPort,tmp,":",NULL,&portSize);

	if(isPort == 0) { // if success
		port = atoi(strPort);
		printf("Port set by Integer %d\n",port);
	} else {
		port = 443;
		printf("Port 443\n");
	}

	p = HttpsUtil_StringInString(pURL,"https://");
	if( p == NULL ) {
		//URL
		av_free(pLine);
		av_free(pInfo);
		return -1;
	}
	p += strlen("https://");
	p = HttpsUtil_StringInString(p,"/");

	char *p1 = HttpsUtil_StringInString(p,"\r\n");
	if(p1)
	{
		unsigned long info_size2=1019;
		memset(tmp,0,1024);
		HttpsUtil_GetToken(tmp,p1,"\r\n","\r\n\r\n",&info_size2);
		snprintf(pInfo,HTTPS_INT_MAX,"\r\n%s",tmp);
	}
	if(p)
	{
		if(p1) {
			strncpy(path,p,p1-p);
		}
		else {		
			strcpy(path,p);
		}

	}
	else
	{
		strcpy(path,"/");
	}
	p1 = HttpsUtil_StringInString(p1,"\r\n\r\n");

#if 0
	if(lFilePos > 0) {
		if(p1)
		{
			//PCString::Print(pLine,"%s %s HTTP/1.1%s\r\nHost: %s\r\nConnection: close%s",t_LoadType(type),path,pInfo,hostName,p1);
			snprintf(pLine,HTTPS_INT_MAX ,"%s %s HTTP/1.1%s\r\nHost: %s\r\nRange: bytes=%lld-%lld\r\nConnection: close%s","GET",path,pInfo,hostName,lFilePos,lFileLength,p1);
		}
		else
		{
			//PCString::Print(pLine,"%s %s HTTP/1.1%s\r\nHost: %s\r\nConnection: close\r\n\r\n",t_LoadType(type),path,pInfo,hostName);
			snprintf(pLine,HTTPS_INT_MAX ,"%s %s HTTP/1.1%s\r\nHost: %s\r\nRange: bytes=%lld-%lld\r\nConnection: close\r\n\r\n","GET",path,pInfo,hostName,lFilePos,lFileLength);
		}
	}
	else {
#else
	if(1){
#endif
		if(p1) {
			snprintf(pLine,HTTPS_INT_MAX ,"%s %s HTTP/1.1%s\r\nHost: %s\r\nConnection: close%s","GET",path,pInfo,hostName,p1);
		}
		else {
			snprintf(pLine,HTTPS_INT_MAX,"%s %s HTTP/1.1%s\r\nHost: %s\r\nConnection: close\r\n\r\n","GET",path,pInfo,hostName);
		}
	}
	

//	printf("pLine = %s \n",pLine);
		
	char hostssl[1030];
	snprintf(hostssl,HTTPS_INT_MAX,"%s:https",hostName);

	printf(" hostssl = %s \n", hostssl);
	
	if(bSocketSslOpenState==0)
	{
		printf("\n Try.. SSL open\n");
		gettimeofday(&starttime , NULL);
		if(proto_SslOpen(hostssl, port) == -1)
		{
			gettimeofday(&endtime, NULL);
			printf("Socket Create Time = %d", (endtime.tv_sec  - starttime.tv_sec));
			av_free(pLine);
			av_free(pInfo);
			return -99;  //for network time out 
		}
	}else{
	
		//return -1;
	}
//	printf("pLine = %s , length = %d \n",pLine, strlen(pLine));
	int err = 0;
	err=	proto_SslSend(pLine, strlen(pLine));
	
	printf("\n[FFMPEG] HTTPS OPEN proto_SslSend[%d]\n" , err);
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
	int iRET=0;
	
	iRET = proto_SslReceive(HEADER_BUF, HTTPS_BUFFER_SIZE,1);

//	for(int i=0;i< HTTPS_BUFFER_SIZE  ;i++)
//	 printf("%c",HEADER_BUF[i]);
	
	first_data_size = iRET;
//	printf("\n[HTTPS_OPEN]  proto_SslReceive RET =[%d] \n" , iRET);
	printf("[%s]\n",HEADER_BUF);
	if(isHeader) {
		if(iRET<=0) {
			printf("[https_read] Receive Header Failed\n");
			return -1;
		}
	      	//printf("[%s]\n",HEADER_BUF);
		if ((p = HttpsUtil_StringInString(HEADER_BUF,"\r\n\r\n")) !=NULL) {
			*p = '\0';
			p = p+4;
			m_headerSize = strlen(HEADER_BUF);
			strncpy(m_header, HEADER_BUF, m_headerSize);

			//to check code
			int code;
			sscanf(m_header+strlen("HTTP/1.1 "),"%d",&code);	
			if(code!=200 && code !=206 ) {
				printf("Result code!=200 && 206  code=%d \n",code);	
				//1 TODO CHECK RETURN VALUE
				//return 0;
				return -1;
			}

			/*				
			//to check header
			if( t_IsChunkedData(m_header) == true)
			{
				if(t_MergeChunkedData()==false)
				{
				printf("Chunked Data error\n");	
				return -1;
				}
			}
			*/
					//to get contentlength
			int64_t contentlength=0;
			char*tempstr=NULL;
			if ( (tempstr=HttpsUtil_StringInString(HEADER_BUF, "Content-Length: "))!=NULL ) {
				tempstr+=16;
				contentlength=atoll(tempstr);
			}
			printf("Content-Length=%d\n",contentlength);				

			if(contentlength<=0)
			{
				printf("Content have problem [contentlength<0]\n");	
				//1 TODO CHECK RETURN VALUE			
				return 0;
			}
			totallength=contentlength;
			//calculate datasize right after header
			g_dataSize = iRET - ( p - HEADER_BUF);
			memcpy(HEADER_BUF, p, g_dataSize);
			isHeader	 = 0;
			//return g_dataSize;
		}
		else {
			printf("[https_read] Bad Header\n");
			return -1;	
		}
		printf("End https_open\n");	
	}

	h->is_streamed = 1;

	av_free(pLine);
	av_free(pInfo);
//////////////////////////////////////////////////////////
	return 0;
}


static int httpscon(URLContext *h, const char *uri, int64_t lFilePos,int flags)  {
	struct timeval starttime,endtime;
	starttime.tv_sec = 0;
	endtime.tv_sec = 0;

	int pURL_length = 0;

	char *pLine;
	char *pInfo;

	char hostName[1024];
	char tmp[1024];
	char path[1024];

	unsigned long info_size=1023;
	int port;
	char strPort[32];

	https_urlcontext = h;

	//Set URL
	pURL_length = strlen(uri) + 1;
	//pURL = av_malloc(pURL_length);
	//memset(pURL,0,pURL_length);
	//strcpy(pURL,uri);	
	printf("[httpscon](VER 1) URL = %s\n",pURL);
	printf("[httpscon](VER 2) lFilePos = %lld\n",lFilePos);

	pLine = av_malloc(pURL_length + 128);
	pInfo = av_malloc(pURL_length);

	memset(pLine , 0 , pURL_length+128);
	memset(pInfo,0,pURL_length);
	
	memset(m_header,0,HTTPS_BUFFER_SIZE+1);
	memset(hostName,0,1024);
	memset(tmp,0,1024);
	memset(path,0,1024);

	memset(strPort,0,32);
	HttpsUtil_GetToken(tmp,pURL,"https://","/",&info_size);
	
	info_size = 1023;
	HttpsUtil_GetToken(hostName,tmp,NULL,":",&info_size);

	printf("[https_open]  hostName = %s\n",hostName);

	unsigned long portSize = 31;
	int  isPort = HttpsUtil_GetToken(strPort,tmp,":",NULL,&portSize);

	if(isPort == 0) { // if success
		port = atoi(strPort);
		printf("Port set by Integer %d\n",port);
	} else {
		port = 443;
		printf("Port 443\n");
	}

	p = HttpsUtil_StringInString(pURL,"https://");
	if( p == NULL ) {
		//URL
		av_free(pLine);
		av_free(pInfo);
		return -1;
	}
	p += strlen("https://");
	p = HttpsUtil_StringInString(p,"/");

	char *p1 = HttpsUtil_StringInString(p,"\r\n");
	if(p1)
	{
		unsigned long info_size2=1019;
		memset(tmp,0,1024);
		HttpsUtil_GetToken(tmp,p1,"\r\n","\r\n\r\n",&info_size2);
		snprintf(pInfo,HTTPS_INT_MAX,"\r\n%s",tmp);
	}
	if(p)
	{
		if(p1) {
			strncpy(path,p,p1-p);
		}
		else {		
			strcpy(path,p);
		}

	}
	else
	{
		strcpy(path,"/");
	}
	p1 = HttpsUtil_StringInString(p1,"\r\n\r\n");

#if 0
	if(lFilePos > 0) {
		if(p1)
		{
			//PCString::Print(pLine,"%s %s HTTP/1.1%s\r\nHost: %s\r\nConnection: close%s",t_LoadType(type),path,pInfo,hostName,p1);
			snprintf(pLine,HTTPS_INT_MAX ,"%s %s HTTP/1.1%s\r\nHost: %s\r\nRange: bytes=%lld-%lld\r\nConnection: close%s","GET",path,pInfo,hostName,lFilePos,lFileLength,p1);
		}
		else
		{
			//PCString::Print(pLine,"%s %s HTTP/1.1%s\r\nHost: %s\r\nConnection: close\r\n\r\n",t_LoadType(type),path,pInfo,hostName);
			snprintf(pLine,HTTPS_INT_MAX ,"%s %s HTTP/1.1%s\r\nHost: %s\r\nRange: bytes=%lld-%lld\r\nConnection: close\r\n\r\n","GET",path,pInfo,hostName,lFilePos,lFileLength);
		}
	}
	else {
#else
	if(1){
#endif
		if(p1) {
			snprintf(pLine,HTTPS_INT_MAX ,"%s %s HTTP/1.1%s\r\nHost: %s\r\nConnection: close%s","GET",path,pInfo,hostName,p1);
		}
		else {
			snprintf(pLine,HTTPS_INT_MAX,"%s %s HTTP/1.1%s\r\nHost: %s\r\nRange: bytes=%lld-\r\nConnection: close\r\n\r\n","GET",path,pInfo,hostName,lFilePos);
		}
	}
	

	printf("pLine = %s \n",pLine);
		
	char hostssl[1030];
	snprintf(hostssl,HTTPS_INT_MAX,"%s:https",hostName);

	printf(" hostssl = %s \n", hostssl);
	
	if(bSocketSslOpenState==0)
	{
		printf("\n Try.. SSL open\n");
		gettimeofday(&starttime , NULL);
		if(proto_SslOpen(hostssl, port) == -1)
		{
			gettimeofday(&endtime, NULL);
			printf("Socket Create Time = %d", (endtime.tv_sec  - starttime.tv_sec));
			av_free(pLine);
			av_free(pInfo);
			return -99;//for network time out 
		}
	}else{
		proto_SslClose();
		printf("\n Try.. SSL open\n");
		gettimeofday(&starttime , NULL);
		if(proto_SslOpen(hostssl, port) == -1)
		{
			gettimeofday(&endtime, NULL);
			printf("Socket Create Time = %d", (endtime.tv_sec  - starttime.tv_sec));
			av_free(pLine);
			av_free(pInfo);
			return -99;//for network time out 
		}
		//return -1;
	}
//	printf("pLine = %s , length = %d \n",pLine, strlen(pLine));
	int err = 0;
	err=	proto_SslSend(pLine, strlen(pLine));
	
	printf("\n[FFMPEG] HTTPS OPEN proto_SslSend[%d]\n" , err);
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
	int iRET=0;
	iRET = proto_SslReceive(HEADER_BUF, HTTPS_BUFFER_SIZE, 1);
	first_data_size = iRET;
	printf("\n[HTTPS_OPEN]  proto_SslReceive RET =[%d] \n" , iRET);
	printf("[%s]\n",HEADER_BUF);

	if(iRET<=0) 
	{
		printf("[https_read] Receive Header Failed\n");
		return -1;
	}
      	//printf("[%s]\n",HEADER_BUF);
	if ((p = HttpsUtil_StringInString(HEADER_BUF,"\r\n\r\n")) !=NULL) {
		*p = '\0';
		p = p+4;
		m_headerSize = strlen(HEADER_BUF);
		strncpy(m_header, HEADER_BUF, m_headerSize);

		//to check code
		int code;
		sscanf(m_header+strlen("HTTP/1.1 "),"%d",&code);	
		if(code!=200 && code !=206 ) {
			printf("Result code!=200 && 206  code=%d \n",code);	
			//1 TODO CHECK RETURN VALUE
			//return 0;
			return -1;
		}

		/*				
		//to check header
		if( t_IsChunkedData(m_header) == true)
		{
			if(t_MergeChunkedData()==false)
			{
			printf("Chunked Data error\n");	
			return -1;
			}
		}
		*/
				//to get contentlength
		int64_t contentlength=0;
		char*tempstr=NULL;
		if ( (tempstr=HttpsUtil_StringInString(HEADER_BUF, "Content-Length: "))!=NULL ) {
			tempstr+=16;
			contentlength=atoll(tempstr);
		}
		printf("Content-Length=%lld\n",contentlength);				

		if(contentlength<=0)
		{
			printf("Content have problem [contentlength<0]\n");	
			//1 TODO CHECK RETURN VALUE			
			return 0;
		}
	//	totallength=contentlength;
		//calculate datasize right after header
		g_dataSize = iRET - ( p - HEADER_BUF);
		memcpy(HEADER_BUF, p, g_dataSize);
		isHeader	 = 0;
		//return g_dataSize;
	}
	else 
	{
		printf("[https_read] Bad Header\n");
		return -1;	
	}
	printf("End httpscon\n");	

	av_free(pLine);
	av_free(pInfo);
//////////////////////////////////////////////////////////
	return 0;
}


static int https_read(URLContext *h, uint8_t *buf, int length) {

    	int len;


	if(buf == NULL) {
		printf("[https_read] Buffer pointer is NULL\n");				
		return -1;
	}

	if(first_data_size  > m_headerSize +4 )
	{
		int size_test;
		size_test =first_data_size -m_headerSize -4 ;
		//for(int i=0;i< HTTPS_BUFFER_SIZE -size_test  ;i++)
		//	 printf("%d",HEADER_BUF[i]);
		first_data_size =0;
		m_headerSize =0;
		memcpy(buf, HEADER_BUF,  size_test );
		return size_test;
}

	len = proto_SslReceive(buf, length,0);
//	printf("\n[https_read]  proto_SslReceive RET =[%d] \n" , len);
//	printf("End https_read %d\n",len);	
	return len;
}




static int https_write(URLContext *h, uint8_t *buf, int size) 
{
	return 0;
}

static int64_t https_seek(URLContext *h, int64_t off, int whence) 
{
	int ret;
	printf("htt_seek start whence: %d, off:%lld\n",whence,off);

	if (whence == AVSEEK_SIZE) 
	{
		printf("[!!!!!!!https_seek] totallength = %lld \n" , totallength);
		return totallength;
	}
	else if((totallength == 0 && whence == SEEK_END) )
       {
       	return -1;
	} 

	proto_SslClose();

	ret=httpscon(h, pURL, off,0) ;
	if(ret <0)
		return -1;
	
	return off;
}

static int https_close(void) {
	printf("@@@KOO https_close start \n");
	av_free(pURL);
	proto_SslClose();
	isHeader = 1;
	printf("@@@KOO https_close end \n");
	return 0;
}

int HttpsGetLength(void) {
	return totallength;
}

URLProtocol https_protocol = {
	"https",
	https_open,
	https_read,
	https_write,
	https_seek,
	https_close,
};

