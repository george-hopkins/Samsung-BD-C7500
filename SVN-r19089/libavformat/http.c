/*
 * HTTP protocol for ffmpeg client
 * Copyright (c) 2000, 2001 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "libavutil/base64.h"
#include "libavutil/avstring.h"
#include "avformat.h"
#include <unistd.h>
#include "network.h"
#include "os_support.h"
#include "wmdrm_wrapper.h"//###################################for WMDRM_NETFLIX
#include <netdb.h>//###################################for Netflix
#include <sys/socket.h>//###################################for Netflix
#include <arpa/inet.h>//###################################for Netflix
#include <netinet/in.h>//###################################for Netflix
#include <sys/time.h>//###################################for Netflix

/* XXX: POST protocol is not completely implemented because ffmpeg uses
   only a subset of it. */

//#define DEBUG

/* used for protocol handling */
#define BUFFER_SIZE 1024
#define URL_SIZE    4096
#define MAX_REDIRECTS 8

// For WMDRM
#ifdef WMDRM//###################################for WMDRM
typedef void (*DrmData)(unsigned char* pData);
#define WMDRM_OUT_MAX		70000
#define WMDRM_IN_MAX		1024*35
#define WAITTING 			1
#define TAKED				2
#define INSTALL_ERROR		3
#define NORMAL 				0
#define NO_LICENCE			1

typedef struct {
	char url[200];
	char Down_ID[20];
	char Resolution[10];
	char Bitrate[10];
	unsigned long long bps;
}NETFLIX_URL_INFO;
NETFLIX_URL_INFO URL_info[8] = {'\0', };
MSD_DRM_STATUS pStatus;

/***************************************************************************************/
/*************************  		for WMDRM                      *****************/
/***************************************************************************************/

typedef struct {
	char IPlayer_url[200];
	char Down_ID[20];

}IPlayerInfo;
IPlayerInfo IPlayer_URL_info ;

/***************************************************************************************/
/*************************  		for WMDRM                      *****************/
/***************************************************************************************/

enum Info_Index
{
	DLID,
	DLIDOLD,
	URL1,
	URL2,
	IP,
	Second_IP,
	OLD_IP,
	CUR_URL,
	CUR_ID,
	INITIALBW,
	STREAM_SW,
	STOP_WMDRM,
	CLEAN_STOP,
	SET_NFX,
	CLR_NFX,
	CHK_LICENCE,
	CHK_EOF,
	CHK_SPEED,
	CHK_BWRESULT,
};
unsigned char *pChallenge = NULL;
unsigned char *wmdrmIn = NULL;
unsigned char *wmdrmOut = NULL;
unsigned char *pwrite_Inbuf;
unsigned char *pread_Inbuf;
unsigned char *pread_outbuf ;
unsigned char *pwrite_outbuf ;
unsigned char removeheader = 0;
unsigned char NFX_err_cnt = 0;
unsigned char EndOfFile = 0;
unsigned char done_speedchk = 0;
unsigned char Request_speedchk = 0;
unsigned char Macro4NFX = 0;
unsigned char CGMS4NFX = 0;
unsigned char ICT4NFX = 0;
unsigned int available_wmdrm = 0;
unsigned int NFX_timebuf = 0;
int NetFlix_connect = 0;
int64_t ASF_hdsize = 0;
int64_t ASF_pksize = 0;
int64_t ASF_IndexPosition = 0;
int64_t ASF_Correction = 0;
int64_t BackupOffset = 0;
unsigned char* challenge_data = NULL;
unsigned int challenge_len = 0;
unsigned char did_you_get_Lice = 0;
unsigned char EscapeByStop = 0;
unsigned char Licence_null = 0;
unsigned char SellectedID = 0;
unsigned char is_it_stream_switching = 0;
DrmData g_CbFunction;
DrmData g_CbIPlayerFunction = NULL;
int is_it_Netflix = 0;
int is_it_IPlayerWMDRM = 0;
int IPlayer_connect = 0;
int URL_number = 0;
int NFX_Resume = 0;
char Network_error = 0;
char bypassf = 0;//shintest
char Pre_Down_ID[20] = {'\0', }; //previous downloadable ID
char IP1[30] = {'\0', };
char IP2[30] = {'\0', };
char IP_OLD[30] = {'\0', };
char Second_URL[200] = {'\0', };
unsigned long long bandwidth = 0;
int WMDRM_read(URLContext *h, uint8_t *buf, int size);
int Get_availableSize(void);
int Iitialize_WMDRM(char *cpName);
void Initialize_URL_buffer(void);
void Get_IP(char *IP, char *URL);
#endif//#########################################for WMDRM

int is_open=0;
int ContentRange = 0;

typedef struct {
    URLContext *hd;
    unsigned char buffer[BUFFER_SIZE], *buf_ptr, *buf_end;
    int line_count;
    int http_code;
    int64_t off, filesize;
    char location[URL_SIZE];
} HTTPContext;

static int http_connect(URLContext *h, const char *path, const char *hoststr,
                        const char *auth, int *new_location);
static int http_write(URLContext *h, uint8_t *buf, int size);


/* return non zero if error */
static int http_open_cnx(URLContext *h)
{
    const char *path, *proxy_path;
    char hostname[1024], hoststr[1024];
    char auth[1024];
    char path1[1024];
    char buf[1024];
    int port, use_proxy, err, location_changed = 0, redirects = 0;
    HTTPContext *s = h->priv_data;
    URLContext *hd = NULL;

    proxy_path = getenv("http_proxy");
    use_proxy = (proxy_path != NULL) && !getenv("no_proxy") &&
        av_strstart(proxy_path, "http://", NULL);

    /* fill the dest addr */
 redo:
    /* needed in any case to build the host string */
    url_split(NULL, 0, auth, sizeof(auth), hostname, sizeof(hostname), &port,
              path1, sizeof(path1), s->location);
    if (port > 0) {
        snprintf(hoststr, sizeof(hoststr), "%s:%d", hostname, port);
    } else {
        av_strlcpy(hoststr, hostname, sizeof(hoststr));
    }

    if (use_proxy) {
        url_split(NULL, 0, auth, sizeof(auth), hostname, sizeof(hostname), &port,
                  NULL, 0, proxy_path);
        path = s->location;
    } else {
        if (path1[0] == '\0')
            path = "/";
        else
            path = path1;
    }
    if (port < 0)
        port = 80;

    snprintf(buf, sizeof(buf), "tcp://%s:%d", hostname, port);
    err = url_open(&hd, buf, URL_RDWR);
    if (err < 0)
        goto fail;

    s->hd = hd;
    if (http_connect(h, path, hoststr, auth, &location_changed) < 0)
        goto fail;
//Org    if ((s->http_code == 302 || s->http_code == 303) && location_changed == 1) {
    if ((s->http_code == 301||s->http_code == 302 || s->http_code == 303 || s->http_code == 307) && location_changed == 1) {  //added by shin hyun koog for Netflix add 301 KOO
        /* url moved, get next */
        url_close(hd);
        if (redirects++ >= MAX_REDIRECTS)
        {
#ifdef WMDRM//################################ for WMDRM
	  	strcpy(Second_URL, s->location);	        
 	  	Get_IP(IP2,Second_URL);
            	printf("___^ ^___[NFX log]:Redirected URL2 = %s \n",s->location);
#endif// WMDRM//################################ for WMDRM
            return AVERROR(EIO);
        }
        location_changed = 0;
        goto redo;
    }
    return 0;
 fail:
    if (hd)
        url_close(hd);
    if(err == -99)
	return -99;
    return AVERROR(EIO);
}

//FILE *fid = NULL;//shintest
static int http_open(URLContext *h, const char *uri, int flags)
{
    HTTPContext *s;
    int ret;
    is_open =1;
    ContentRange = 0;
#ifdef WMDRM//################################ for WMDRM
	if(is_it_Netflix==1)
	{
		Network_error = 0;
		done_speedchk = 0;
		if(is_it_stream_switching ==1)
		{
			printf("___^ ^___[NFX log]:it's stream switching\n");
			removeheader = 0;
			is_it_stream_switching = 0;
		}
		else
		{
		Pharse_WMDRM_URL(uri);	
		}
		
		if(NetFlix_connect== 1)
		{
			//if(fid == NULL)//**************************************shintest
			//fid = fopen("/dtv/usb/sda1/testfile1.wmv","wa");//shintest
//			ret = Iitialize_WMDRM();
			ret = Iitialize_WMDRM("WMDRM4NFX");
			if(ret == 1)//success to initialize WMDRM
			{
				strcpy(IP_OLD, IP1);
				Get_IP(IP1,URL_info[SellectedID].url);
			}
			else//fail to initialize WMDRM
				return -1;
		}
	}
	else if(is_it_IPlayerWMDRM == 1)
	{
		Parse_IPlayerWMDRM_URL(uri);
		printf("==================================== IPlayer_connect is %d \n", IPlayer_connect); 	//sunye add
			ret = Iitialize_WMDRM("WMDRM");
			if(ret == 1)
			{
				IPlayer_connect = 1;
			}
			else
			{
				return -1;
			}
	}

#endif //WMDRM//################################ for WMDRM

    h->is_streamed = 1;

    s = av_malloc(sizeof(HTTPContext));
    if (!s) {
        return AVERROR(ENOMEM);
    }
    h->priv_data = s;
    s->filesize = -1;
    s->off = 0;
#ifdef WMDRM//################################ for WMDRM
	if(is_it_Netflix== 1)
	{
		av_strlcpy(s->location, URL_info[SellectedID].url, URL_SIZE);
		printf("___^ ^___[NFX log]:URL= %s, Bitrate = %s\n",URL_info[SellectedID].url,URL_info[SellectedID].Bitrate);
	}
	else if (is_it_IPlayerWMDRM == 1) 											//sunye add for IPlayer
	{
		printf("___^ ^___[NFX log]:URL= %s\n",IPlayer_URL_info.IPlayer_url);
    	av_strlcpy(s->location, IPlayer_URL_info.IPlayer_url, URL_SIZE);				//sunye add for IPlayer
	}
	else
    av_strlcpy(s->location, uri, URL_SIZE);
#else
		av_strlcpy(s->location, uri, URL_SIZE);
#endif//WMDRM//################################ for WMDRM
    ret = http_open_cnx(h);

//<- KOO 416 Requested Range Not Satisfiable 이거나  400 Bad Request 인 경우 Range 와 Authorization를  삭제 후 다시 연결을 요청한다.
#ifdef DEBUG
		printf("\n\nhttp_open ::http_open_cnx = %d\n\n\n", ret);
		printf("http_open ::s->http_code=%d\n\n", s->http_code);
#endif

    if((s->http_code ==416)||(s->http_code ==400))
    {
    	#ifdef DEBUG
            printf("\nhttp_open ::second http_open_cnx start\n\n", ret);
	#endif
	ret = http_open_cnx(h);
    }	
    is_open=0;	
//KOO ->

    if (ret != 0)
        av_free (s);
    return ret;
}
static int http_getc(HTTPContext *s)
{
    int len;
    if (s->buf_ptr >= s->buf_end) {
        len = url_read(s->hd, s->buffer, BUFFER_SIZE);
        if (len < 0) {
            return AVERROR(EIO);
        } else if (len == 0) {
            return -1;
        } else {
            s->buf_ptr = s->buffer;
            s->buf_end = s->buffer + len;
        }
    }
    return *s->buf_ptr++;
}

static int process_line(URLContext *h, char *line, int line_count,
                        int *new_location)
{
    HTTPContext *s = h->priv_data;
    char *tag, *p;

    /* end of header */
    if (line[0] == '\0')
        return 0;

    p = line;
    if (line_count == 0) {
        while (!isspace(*p) && *p != '\0')
            p++;
        while (isspace(*p))
            p++;
        s->http_code = strtol(p, NULL, 10);
#ifdef DEBUG
        printf("http_code=%d\n", s->http_code);
#endif
        /* error codes are 4xx and 5xx */
        if (s->http_code >= 400 && s->http_code < 600)
            return -1;
    } else {
        while (*p != '\0' && *p != ':')
            p++;
        if (*p != ':')
            return 1;

        *p = '\0';
        tag = line;
        p++;
        while (isspace(*p))
            p++;
        if (!strcmp(tag, "Location")||!strcmp(tag, "location")) {
            strcpy(s->location, p);
            *new_location = 1;
        } else if (!strcmp (tag, "Content-Length") && s->filesize == -1) {
            s->filesize = atoll(p);
        } else if (!strcmp (tag, "Content-Range")) {
            /* "bytes $from-$to/$document_size" */
            const char *slash;
            if (!strncmp (p, "bytes ", 6)) {
                p += 6;
                s->off = atoll(p);
                if ((slash = strchr(p, '/')) && strlen(slash) > 0)
                    s->filesize = atoll(slash+1);
            }
            //h->is_streamed = 0; /* we _can_ in fact seek */  //for lun lun panda
		ContentRange =1;   //for lun lun panda
        }
    }
    return 1;
}


#ifdef IPLAYER
/*! get cookies from shared file, clin.li@SCRC for iplayer application, 2009.10.6 */
static int get_cookies(char **cookies, const char *hoststr)
{
    FILE *fp;
    int len;
    char *pBuf;
    char strFile[256];

    snprintf(strFile, sizeof(strFile), "/dtv/%s", hoststr);

    fp = fopen(strFile, "r"); //< now we are using host string as file name, consider some modifications for a better appearance
    if (fp == NULL) 
    {
        av_log(NULL, AV_LOG_INFO, "No shared cookie found!(%s)\n", strFile);
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    pBuf = (char *) av_malloc(len+1);
    if (!pBuf)
    {
        fclose(fp);
        return 0;
    }

    fseek(fp, 0, SEEK_SET);
    if (fread(pBuf, 1, len, fp) != len) 
    {
        av_free(pBuf);
        fclose(fp);
        return 0;
    }

   pBuf[len] = 0; //< add '\0' as the end of a string
   *cookies = pBuf;

    fclose(fp);
    return len;
}
/*! end clin.li */
#endif


static int http_connect(URLContext *h, const char *path, const char *hoststr,
                        const char *auth, int *new_location)
{
    HTTPContext *s = h->priv_data;
    int post, err, ch;
    char line[1024], *q;
    char *auth_b64;
    int auth_b64_len = (strlen(auth) + 2) / 3 * 4 + 1;
    int64_t off = s->off;
#ifdef IPLAYER    
    char *cookies = NULL;  //< added by clin.li@SCRC for iPlayer application, 2009.10.6
#endif

    /* send http header */
    post = h->flags & URL_WRONLY;
    auth_b64 = av_malloc(auth_b64_len);
    av_base64_encode(auth_b64, auth_b64_len, auth, strlen(auth));
#ifdef IPLAYER    
    /*! modified by clin.li@SCRC for iPlayer application, 2009.10.6, for cookie setting */
    if (get_cookies(&cookies, hoststr) !=0)
    {
        snprintf(s->buffer, sizeof(s->buffer),
                 "%s %s HTTP/1.1\r\n"
                 "User-Agent: %s\r\n"
                 "Accept: */*\r\n"
                 "Range: bytes=%"PRId64"-\r\n"
                 "Host: %s\r\n"
                   "Cookie: %s\r\n"
                 "Authorization: Basic %s\r\n"
                 "Connection: close\r\n"
                 "\r\n",
                 post ? "POST" : "GET",
                 path,
                 LIBAVFORMAT_IDENT,
                 s->off,
                 hoststr,
                   cookies,
                 auth_b64);
        av_free(cookies);        
    /*! end clin.li */        
    }
    else
#endif
    {
        if(((s->http_code==416) ||(s->http_code==400))  &&  is_open==1)
    	{
    		snprintf(s->buffer, sizeof(s->buffer),
             "%s %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "User-Agent: %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             post ? "POST" : "GET",
             path,
             hoststr,
             LIBAVFORMAT_IDENT);
    	}
	else
	{
    snprintf(s->buffer, sizeof(s->buffer),
             "%s %s HTTP/1.1\r\n"
             "User-Agent: %s\r\n"
             "Accept: */*\r\n"
             "Range: bytes=%"PRId64"-\r\n"
             "Host: %s\r\n"
             "Authorization: Basic %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             post ? "POST" : "GET",
             path,
             LIBAVFORMAT_IDENT,
             s->off,
             hoststr,
             auth_b64);
    }
    }
    av_freep(&auth_b64);
#ifdef DEBUG
            printf("[%s]\n",s->buffer);
#endif
    if (http_write(h, s->buffer, strlen(s->buffer)) < 0)
        return AVERROR(EIO);

    /* init input buffer */
    s->buf_ptr = s->buffer;
    s->buf_end = s->buffer;
    s->line_count = 0;
    s->off = 0;
    s->filesize = -1;
    if (post) {
        return 0;
    }

    /* wait for header */
    q = line;
    for(;;) {
        ch = http_getc(s);
        if (ch < 0)
            return AVERROR(EIO);
        if (ch == '\n') {
            /* process line */
            if (q > line && q[-1] == '\r')
                q--;
            *q = '\0';
#ifdef DEBUG
            printf("header='%s'\n", line);
#endif
            err = process_line(h, line, s->line_count, new_location);
            if (err < 0)
                return err;
            if (err == 0)
                break;
            s->line_count++;
            q = line;
        } else {
            if ((q - line) < sizeof(line) - 1)
                *q++ = ch;
        }
    }

    return (off == s->off) ? 0 : -1;
}


static int http_read(URLContext *h, uint8_t *buf, int size)
{
    HTTPContext *s = h->priv_data;
    int len;

    int ret;
    int i;//##########for Netflix
    int j;//##########for Netflix

    /* read bytes from input buffer first */
    len = s->buf_end - s->buf_ptr;
//######################################## for WMDRM
#ifdef WMDRM//################################ for WMDRM
	if(NetFlix_connect==1)
	{
		if((did_you_get_Lice == INSTALL_ERROR) || (EscapeByStop == 1))
		{
			printf("___^ ^___[NFX log]: http_read:returned by INSTALL_ERROR\n");
		return -1;
		 }
		else if(bypassf == 1)
    {
	 printf("___^ ^___[NFX log]: http_read bypass\n");//shintest
    }
		else if(EndOfFile == 1)
		{
			return -32;
		}
		else if(Network_error == 1)
		{
			return -99;
		}
		else
    {
   	len = WMDRM_read(h, buf, size);
			if(did_you_get_Lice == INSTALL_ERROR)
	{
				return -1;
	}
			else if(len > 0)
			{
				if(ASF_Correction>0)
				{
				 	j = (int)ASF_Correction;
					s->off += len;
					len = len - j;
					for(i=0;i<len;i++)
	{
					 	buf[i] = buf[i+j];
					}
					ASF_Correction = 0; 
				}
				else
    	s->off += len;
		 //fwrite((char *)buf,1,len,fid);//shintest	
	return len;
    }
			else if(len==-32)
			{
				printf("___^ ^___[NFX log]: End of file\n");//shintest
				return len;
			}
			else if(len==-99)
			{
				printf("___^ ^___[NFX log]: There was Network error on HTTP\n");
			 	Network_error = 1;
				return len;
			}
		}
    }
	else if((is_it_Netflix== 1) && (done_speedchk != 1))
	{
		struct timeval t1;
    		struct timeval t2;
		char chk_speed[400000] = {'\0', };
		unsigned long long total_len = 0;
		signed long long delta_t = 0;
		int found = -1;
    		int cont;
		int biggest = 0;
		int zerocnt = 0;
		
		if (len > 0)
		{
	      		if (len > size)
	            		len = size;
	        	memcpy(buf, s->buf_ptr, len);
	        	s->buf_ptr += len;
		} 
		else
		{
			gettimeofday(&t1 , NULL);
			while(total_len < 300000)//700000
			{
				len = url_read(s->hd, chk_speed+total_len, size);
				if(len > 0)
				{
					zerocnt = 0;
					total_len = total_len + (unsigned long long)len;
				}
				else if(len == 0)
				{
					zerocnt++;
					if(zerocnt > 99)
						break;
				}
				else if((len == -99) || (Network_error == 1))
					return -99;
			}
			gettimeofday(&t2 , NULL);
			delta_t = (int64_t)t2.tv_sec * 1000000 + t2.tv_usec - ((int64_t)t1.tv_sec * 1000000 + t1.tv_usec);
			bandwidth =(unsigned long long) ((total_len * 8*1000000) / delta_t) / 1024; //Kbps
			if(bandwidth > 2000)
				bandwidth = bandwidth*173/100;
			else if(bandwidth > 1500)
				bandwidth = bandwidth*16/10;
			else if(bandwidth > 1000)
				bandwidth = bandwidth*14/10;
			else if(bandwidth > 700)
				bandwidth = bandwidth*12/10;
			done_speedchk = 1;
			memcpy(buf, chk_speed, size);
			memset(Pre_Down_ID, 0, 20);			
			strcpy(Pre_Down_ID,  URL_info[SellectedID].Down_ID);
			
			for(cont=0; cont<URL_number; cont++)
			{
				if((URL_info[cont].bps <= bandwidth) && (URL_info[cont].bps > URL_info[biggest].bps))
				{
					biggest = cont;
					found = cont;
				}
				else if((URL_info[cont].bps <= bandwidth) && (found == -1))
				{
					biggest = cont;
					found = cont;
				}
			}
			if(found < 0)
			{
				unsigned long long Low_bitrate = URL_info[0].bps;
				found = 0;
				for(cont=0; cont<URL_number; cont++)
				{
					if(Low_bitrate > URL_info[cont].bps)
					{
						Low_bitrate = URL_info[cont].bps;
						found = cont;
					}
				}
			}
			SellectedID = (unsigned char)found;
			len = size;
			printf("___^ ^___[NFX log]:bandwidth = %lld,	SellectedID = %d \n", bandwidth, SellectedID);
	    	}
		if (len > 0)
		    s->off += len;
		return len;
	}
	else if( is_it_IPlayerWMDRM == 1)
	{
		if(bypassf == 1)
		{
			printf("___^ ^___[NFX log]: http_read bypass\n");//shintest
		}
		else
		{
	  	len = WMDRM_read(h, buf, size);
	  	 if(len > 0)
		{
			if(ASF_Correction>0)
			{
			 	j = (int)ASF_Correction;
				s->off += len;
				len = len - j;
				for(i=0;i<len;i++)
				{
					 buf[i] = buf[i+j];
				}
					ASF_Correction = 0; 
			}
			else
				{
    				s->off += len;
			 //fwrite((char *)buf,1,len,fid);//shintest	
				}
			return len;
		}
	}
	}
#endif
//######################################## for WMDRM
    if (len > 0) {
        if (len > size)
            len = size;
        memcpy(buf, s->buf_ptr, len);
        s->buf_ptr += len;
    } else {
        len = url_read(s->hd, buf, size);
    }
    if (len > 0)
        s->off += len;
    return len;
}

/* used only when posting data */
static int http_write(URLContext *h, uint8_t *buf, int size)
{
    HTTPContext *s = h->priv_data;
    return url_write(s->hd, buf, size);
}

static int http_close(URLContext *h)
{
    HTTPContext *s = h->priv_data;
    url_close(s->hd);
    av_free(s);
#ifdef WMDRM//###############################WMDRM
	    if(NetFlix_connect==1)
	    {
	    	if(wmdrmIn != NULL)
		av_free(wmdrmIn);
	    	if(wmdrmOut != NULL)
	       av_free(wmdrmOut);
	    	if(pChallenge != NULL)
		      av_free(pChallenge);
		did_you_get_Lice = 0; 
	    	NetFlix_connect = 0;
		removeheader = 0;
		bypassf = 0;
		ASF_Correction = 0;
		NFX_err_cnt = 0;
		EndOfFile = 0;
		done_speedchk = 0;
		Network_error = 0;
		WR_wmdrm_Close();	
		printf("___^ ^___[NFX log]:Netflix was closed \n");
	    }
		if (is_it_IPlayerWMDRM == 1)
   		{
			if(IPlayer_connect == 1)
			{
				av_free(wmdrmIn);
	 	        av_free(wmdrmOut);
		       if(did_you_get_Lice==WAITTING)
			      av_free(pChallenge);
	  	       is_it_IPlayerWMDRM= 0;
     	       IPlayer_connect= 0;
				removeheader = 0;
				WR_wmdrm_Close();	
			}
   		}
	printf("___^ ^___[NFX log]:http_close \n");
	
#endif//#####################################WMDRM
    return 0;
}

static int64_t http_seek(URLContext *h, int64_t off, int whence)
{
    HTTPContext *s = h->priv_data;
    URLContext *old_hd = s->hd;
    int64_t old_off = s->off;
    int iRet;
#ifdef WMDRM//######################################## for WMDRM
     int64_t temp_off = 0;
    if(off>100000000000)//it's emergency. so escape this state
		return -1;
	
    if(NetFlix_connect==1)
    {
		if(Network_error ==1)
	 	{
		 	return -99;
	 	}
		
    		if((did_you_get_Lice == INSTALL_ERROR) || (EscapeByStop == 1))
				return -1;
			
    		if(removeheader == 2)
    		{
   			if((off >= ASF_IndexPosition) && (ASF_IndexPosition > 0))
   			{
   				//off = ASF_IndexPosition;
	    			bypassf = 1;
   	    			printf("___^ ^___[NFX log]:search ASF index position = %lld\n", ASF_IndexPosition);//shintest
   			}  
			else if(whence==SEEK_SET)
			{
				printf("**********> ============<***********\n");
				printf("**********> ============<***********\n");
				printf("**********> OFF set size = %lld<***********\n",off);
				printf("**********> ============<***********\n");
				printf("**********> ============<***********\n");
				BackupOffset = off;
				temp_off = off - ASF_hdsize;
				off = off - (temp_off % ASF_pksize);
				ASF_Correction = temp_off % ASF_pksize;//shin1220
				WR_wmdrm_Reset();
				available_wmdrm = 0;
				pwrite_Inbuf = wmdrmIn;//set writting point to address of "wmdrmIn"
				pread_Inbuf = wmdrmIn;//set reading point to address of "wmdrmIn"
				pread_outbuf = wmdrmOut;//set reading point to address of "wmdrmOut"
				pwrite_outbuf = wmdrmOut;//set writting point to address of "wmdrmOut"
				bypassf = 0;
			}
			else if(whence>0 && off==0)
			{
				printf("**********> ============<***********\n");
				printf("**********> ============<***********\n");
				printf("**********> WHENCE size = %d<***********\n",whence);
				printf("**********> ============<***********\n");
				printf("**********> ============<***********\n");
				//temp_off = whence - ASF_hdsize;
				//whence = whence - (temp_off % ASF_pksize);
				//WR_wmdrm_Reset();
				//available_wmdrm = 0;
				//pwrite_Inbuf = wmdrmIn;//set writting point to address of "wmdrmIn"
				//pread_Inbuf = wmdrmIn;//set reading point to address of "wmdrmIn"
				//pread_outbuf = wmdrmOut;//set reading point to address of "wmdrmOut"
				//pwrite_outbuf = wmdrmOut;//set writting point to address of "wmdrmOut"
				bypassf = 0;
			}
			
    		}
 printf("___^ ^___[NFX log]:http_seek: %d ~ ,  off: %lld\n",whence,off);//shintest
    }
    else if(is_it_IPlayerWMDRM==1)
    {
    		if(removeheader == 2)
    		{
			WR_wmdrm_Status(&pStatus);
			ASF_hdsize = (int64_t)pStatus.m_nAsfHeader + 50;
			ASF_pksize = (int64_t)pStatus.m_nMaxPacket;
			ASF_IndexPosition = ASF_hdsize + ASF_pksize*pStatus.m_nNumOfPackets;
//			printf("___^ ^___[NFX log]:header size = %d\n",pStatus.m_nAsfHeader);
//			printf("___^ ^___[NFX log]:packet size = %d\n",pStatus.m_nMaxPacket);
//			printf("___^ ^___[NFX log]:total packets = %d\n",pStatus.m_nNumOfPackets);
//			printf("___^ ^___[NFX log]:Index Position = %lld\n",ASF_IndexPosition);

   			if((off >= ASF_IndexPosition) && (ASF_IndexPosition > 0))
   			{
   				//off = ASF_IndexPosition;
	    			bypassf = 1;
   	    			printf("___^ ^___[NFX log]:search ASF index position = %lld\n", ASF_IndexPosition);//shintest
   			}  
			else if(whence==SEEK_SET)
			{
//				printf("**********> ============<***********\n");
//				printf("**********> ============<***********\n");
//				printf("**********> OFF set size = %lld<***********\n",off);
//				printf("**********> ============<***********\n");
//				printf("**********> ============<***********\n");
				BackupOffset = off;
				temp_off = off - ASF_hdsize;
				off = off - (temp_off % ASF_pksize);
				ASF_Correction = temp_off % ASF_pksize;//shin1220
				WR_wmdrm_Reset();
				available_wmdrm = 0;
				pwrite_Inbuf = wmdrmIn;//set writting point to address of "wmdrmIn"
				pread_Inbuf = wmdrmIn;//set reading point to address of "wmdrmIn"
				pread_outbuf = wmdrmOut;//set reading point to address of "wmdrmOut"
				pwrite_outbuf = wmdrmOut;//set writting point to address of "wmdrmOut"
				bypassf = 0;
			}
    		}
    }	
#endif//######################################## for WMDRM

#ifdef DEBUG
 printf("http_seek: %d ~ ,  off: %lld\n",whence,off);
 #endif
 

//////////////////////////org//////////////////////////////////////////
//    if (whence == AVSEEK_SIZE)
//        return s->filesize;
//    else if ((s->filesize == -1 && whence == SEEK_END) || h->is_streamed)
//        return -1;
///////////////////////////////////////////////////////////////////
    if (whence == AVSEEK_SIZE)
        return s->filesize;
    else if ((s->filesize == -1 && whence == SEEK_END) || ContentRange == 0||(off < 0)||(off >s->filesize))
        return -1;

    /* we save the old context in case the seek fails */
    s->hd = NULL;
    if (whence == SEEK_CUR)
        off += s->off;
    else if (whence == SEEK_END)
        off += s->filesize;
    s->off = off;

    /* if it fails, continue on old connection */
    iRet = http_open_cnx(h) ;
    if (iRet < 0) {
        s->hd = old_hd;
        s->off = old_off;
	 if (iRet == -99)	
	 	return -99;
        return -1;
    }
    url_close(old_hd);
    return off;
}

static int
http_get_file_handle(URLContext *h)
{
    HTTPContext *s = h->priv_data;
    return url_get_file_handle(s->hd);
}

URLProtocol http_protocol = {
    "http",
    http_open,
    http_read,
    http_write,
    http_seek,
    http_close,
    .url_get_file_handle = http_get_file_handle,
};

#ifdef WMDRM//###############################for Netflix
int WMDRM_read(URLContext *h, uint8_t *buf, int size)
{
	int i;
	int wmsize = 0;
       int test_len = 0;
	int temp_cal = 0;
	int temp_cnt = 0;
	int read_err_cnt = 0;
    	HTTPContext *s = h->priv_data;
	int len;
	int result = 0;
	unsigned char temp_out[9000];
	unsigned char state = NORMAL;
	unsigned int time_count = 0;
	int write_size = 0;//shintest
	int64_t Org_offset = 0;
	int macrovision = 0;
//	NFX_timebuf = 300;
	if(s->off >= ASF_IndexPosition)
		Org_offset = BackupOffset;
	else	
	Org_offset = s->off;
	len = s->buf_end - s->buf_ptr;
	if(len>0)
	{
		memcpy(buf, s->buf_ptr, len);
	 	for(i = 0;i<len;i++)
	 	{
			*pwrite_Inbuf = buf[i];
			pwrite_Inbuf++;
			if(pwrite_Inbuf >= wmdrmIn + WMDRM_IN_MAX)
				pwrite_Inbuf = wmdrmIn;
	 	}
		available_wmdrm += len;
		s->buf_ptr += len;
	}

	wmsize = Get_availableSize();
	//printf("___^ ^___[NFX log]:available output buffer size = %d\n",wmsize);

	while(wmsize < size)
	{
		if(Request_speedchk == 1)
		{
			unsigned char OldSellectedID = SellectedID;
			len = Speed_Gun(h, buf, size);
			if(len == -99)
				return -99;
			if(OldSellectedID != SellectedID)
			{
				printf("___^ ^___[NFX log]: Bandwidth was changed. so stream will be changed\n");
				is_it_stream_switching = 1;
			}
			else
				is_it_stream_switching = 2;
			
			Request_speedchk = 0;
			http_seek(h, Org_offset, SEEK_SET);
			len = s->buf_end - s->buf_ptr;
			if(len>0)
			{
				memcpy(buf, s->buf_ptr, len);
			 	for(i = 0;i<len;i++)
			 	{
					*pwrite_Inbuf = buf[i];
					pwrite_Inbuf++;
					if(pwrite_Inbuf >= wmdrmIn + WMDRM_IN_MAX)
						pwrite_Inbuf = wmdrmIn;
			 	}
				available_wmdrm += len;
				s->buf_ptr += len;
			}
		}

    		len = url_read(s->hd, buf, size);
		
		if(len > 0)
		{
			for(i = 0;i<len;i++)
			{
				*pwrite_Inbuf = buf[i];
				pwrite_Inbuf++;
				if(pwrite_Inbuf >= wmdrmIn + WMDRM_IN_MAX)
					pwrite_Inbuf = wmdrmIn;
			}
			available_wmdrm += len;
			//printf("___^ ^___[NFX log]:readed url data = %d\n",len);
			//printf("___^ ^___[NFX log]:available input buffer size = %d\n",available_wmdrm);
		}
		else if((len==-99) || (Network_error == 1))//network disconnected = -99
		{
			return -99;
		}
		else if(len <= 0)
		{
			if(EscapeByStop == 1)
				return 0;
			read_err_cnt++;
			printf("___^ ^___[NFX log]: it's url_read error[%d][%d] \n", len, read_err_cnt);
			temp_cnt = 0;
			while(http_open_cnx(h)!=0)
			{
				if(EscapeByStop == 1)
					return -1;
				
				printf("___^ ^___[NFX log]: it's trying to reconnect \n");
				usleep(500000);
				temp_cnt++;
				if((temp_cnt > 20) || (Network_error == 1))
				{
					Network_error = 1;
					return -99;
			}
		}
			
			if(read_err_cnt > 5)
		{
				EndOfFile = 1;
				return -32;
		}
			
			if(NFX_err_cnt<3)
		{
				http_seek(h, Org_offset, SEEK_SET);
				len = s->buf_end - s->buf_ptr;
				if(len>0)
				{
					memcpy(buf, s->buf_ptr, len);
				 	for(i = 0;i<len;i++)
		{
						*pwrite_Inbuf = buf[i];
						pwrite_Inbuf++;
						if(pwrite_Inbuf >= wmdrmIn + WMDRM_IN_MAX)
							pwrite_Inbuf = wmdrmIn;
		}
					available_wmdrm += len;
					s->buf_ptr += len;
				}
			}
		}
		
		while(available_wmdrm > 1024)
		{
			test_len = WR_wmdrm_Read(pread_Inbuf, 1024, temp_out, &state);
			if((state == 0xff)&&(NFX_err_cnt<3))
			{
				NFX_err_cnt++;
				http_seek(h, Org_offset, SEEK_SET);
				printf("___^ ^___[NFX log]: It's http_seek by WR_wmdrm_Read error [%d]\n", NFX_err_cnt);
				if(NFX_err_cnt == 3)
				{
					EndOfFile = 1;
					return -32;
				}
				break;
			}
			else if(state == 0xee)//this case means when wmdrm engine had decription with wrong licence.
			{
				did_you_get_Lice = INSTALL_ERROR;
				return -1;
			}
			//fwrite((char *)pread_Inbuf,1,1024,fid);//shintest	
			//printf("___^ ^___[NFX log]:received data = %d\n",test_len);
			if(test_len > 0)
				NFX_err_cnt = 0;

			pread_Inbuf +=1024;
			if(pread_Inbuf >= wmdrmIn + WMDRM_IN_MAX)
				pread_Inbuf = wmdrmIn;
		
			available_wmdrm -= 1024;

//##################################################To get Licence data
			if(state == NO_LICENCE)
			{
				state = NORMAL;
				if(did_you_get_Lice != INSTALL_ERROR)
				did_you_get_Lice = WAITTING;
				result = WR_wmdrm_GetLicense(&challenge_data,&challenge_len);
				if(result==0)//this case means ffmpeg fail to get challenge data from wmdrm engine
				{
					did_you_get_Lice = INSTALL_ERROR;
					return -1;
				}
				
				for(i=0;i<challenge_len;i++)
				{
					pChallenge[i] = challenge_data[i];
				}
				pChallenge[challenge_len] = '|';
				for(i=0;i<10;i++)
				{
					pChallenge[challenge_len+i+1] = URL_info[SellectedID].Down_ID[i] ;
				}
				pChallenge[challenge_len+11] = '\0';
				g_CbFunction(pChallenge);
				//printf("___^ ^___[NFX log]:Challenge Data: %s\n",pChallenge);
				while(did_you_get_Lice==WAITTING || did_you_get_Lice == INSTALL_ERROR)
				{
					printf("___^ ^___[NFX log]:Waitting Licence data from NCCP : %d\n", challenge_len);
					if((did_you_get_Lice == INSTALL_ERROR) ||(EscapeByStop == 1))
					{
						printf("___^ ^___[NFX log]: Escape from Waitting Licence by error state\n");
						return -1;
					}
					usleep(500000);
//					if(time_count++ > NFX_timebuf)
//					{//this is emergency state. because we didn't receive a licence from NCCP for 150sec
//						printf("___^ ^___[NFX log]:There was no receiving licence from NCCP for 150sec\n");
//						did_you_get_Lice = INSTALL_ERROR;
//						return -1;
//					}
				}
				WR_wmdrm_Status(&pStatus);
				
				macrovision = WR_wmdrm_GetRightsInfo(&Macro4NFX,&CGMS4NFX,&ICT4NFX);
				if(macrovision == 1)
				{
					printf("___^ ^___[NFX log]:Analog output checking is ok\n");
				}
				
				result = 0;
				result = WR_wmdrm_HasDeviceRightsToPlay(NULL,0);
				printf("___^ ^___[NFX log]: WR_wmdrm_HasDeviceRightsToPlay = %d\n",result);	
				if(result == 0)
				{
					did_you_get_Lice = INSTALL_ERROR;
					return -1;
				}
				result = 0;
				ASF_hdsize = (int64_t)pStatus.m_nAsfHeader + 50;
				ASF_pksize = (int64_t)pStatus.m_nMaxPacket;
				ASF_IndexPosition = ASF_hdsize + ASF_pksize*pStatus.m_nNumOfPackets;
				printf("___^ ^___[NFX log]:header size = %d\n",pStatus.m_nAsfHeader);
				printf("___^ ^___[NFX log]:packet size = %d\n",pStatus.m_nMaxPacket);
				printf("___^ ^___[NFX log]:total packets = %d\n",pStatus.m_nNumOfPackets);
				printf("___^ ^___[NFX log]:Index Position = %lld\n",ASF_IndexPosition);
			}
//##################################################To get Licence data
			if(test_len > 0)
			{
				for(i=0;i<test_len;i++)
				{
					*pwrite_outbuf = temp_out[i];
					pwrite_outbuf++;
					if(pwrite_outbuf >= wmdrmOut+WMDRM_OUT_MAX)
					{
						pwrite_outbuf = wmdrmOut;
					}
				}
				//printf("___^ ^___[NFX log]:test length = %d\n",test_len);
			}
		}
		wmsize = Get_availableSize();		
		usleep(5000);	
	}
	
	//printf("___^ ^___[NFX log]:pwrite_outbuf = %d,    pread_outbuf = %d,   wmdrmout= %d\n",pwrite_outbuf,pread_outbuf,wmdrmOut);
	//printf("___^ ^___[NFX log]:available output buffer size = %d\n",wmsize);

	removeheader = 2;
	
	for(i=0;i<size;i++)
	{
		buf[i] = *pread_outbuf;
		pread_outbuf++;
		if(pread_outbuf >= wmdrmOut+WMDRM_OUT_MAX)
			pread_outbuf = wmdrmOut;
	}
//	s->off += size;
	//printf("___^ ^___[NFX log]:returned size = %d      offset = %d\n\n\n\n",size,s->off);
    	return size;
}

int Get_availableSize(void)
{
	int ret;
	if(pwrite_outbuf >= pread_outbuf)
		ret = pwrite_outbuf - pread_outbuf;
	else
		ret = pwrite_outbuf - pread_outbuf + WMDRM_OUT_MAX;
	
	return ret;
}

int Iitialize_WMDRM(char *cpName)
{
	int ret = 0;
	ret = WR_get_MSD_Interface(cpName);
	if(ret == 1)
	{
		available_wmdrm = 0;
		bypassf = 0;
		ASF_Correction = 0;
		NFX_err_cnt = 0;
		EndOfFile = 0;
		Licence_null = 0;
		did_you_get_Lice = 0;
		done_speedchk = 0;
		printf("___^ ^___[NFX log]: Success WR_get_MSD_Interface\n");
		ret = WR_wmdrm_Open(NULL, MSD_OPEN_BUFFER, MSD_SILENT_DLA);
		if(ret == 1)
		{
			printf("___^ ^___[NFX log]:Success WR_wmdrm_Open\n");
			wmdrmIn = av_malloc(WMDRM_IN_MAX);//memory allocation for WMDRM input buffer
			pwrite_Inbuf = wmdrmIn;//set writting point to address of "wmdrmIn"
			pread_Inbuf = wmdrmIn;//set reading point to address of "wmdrmIn"
			
			wmdrmOut = av_malloc(WMDRM_OUT_MAX);//memory allocation for WMDRM output buffer
			pread_outbuf = wmdrmOut;//set reading point to address of "wmdrmOut"
			pwrite_outbuf = wmdrmOut;//set writting point to address of "wmdrmOut"

			pChallenge = av_malloc(22352+20);//to store challenge data and downloadable ID
			WR_wmdrm_CleanInvalidLicense();
		}
		else
		{
			printf("___^ ^___[NFX log]: Fail WR_wmdrm_Open\n");
		}
	}
	else
		ret = -1;

	return ret;
}

void Pharse_WMDRM_URL(char *uri)
{
	int i = 0;
	int j = 0;
	int k = 0;
	int url_num = 0;
	int is_it_endofURL = 0;
	char Resume[20] = {'\0', };
	unsigned long long Max_bitrate = 0;
	Initialize_URL_buffer();
	URL_number = 0;
	NFX_Resume = 0;
	if(NetFlix_connect !=1)
		SellectedID = 0;
		
	while(is_it_endofURL != 1)
	{
		while(uri[i] != '|')
		{
			URL_info[url_num].url[j++] = uri[i++];
		}
		
		j = 0;
		i += 16;//skip "downloadableID="
		while(uri[i] != '|')
		{
			URL_info[url_num].Down_ID[j++] = uri[i++];
		}

		j = 0;
		i += 12;//skip "resolution="
		while(uri[i] != '|')
		{
			URL_info[url_num].Resolution[j++] = uri[i++];
		}

		j = 0;
		i += 9;//skip "bitRate="
		while(uri[i] != '|')
		{
			URL_info[url_num].Bitrate[j++] = uri[i++];
		}
		URL_info[url_num].bps = (unsigned long long)atoi(URL_info[url_num].Bitrate);

		url_num++;
		j = 0;
		if(url_num>7)
		{//this is very special case. because there are 6 URLs normally.
			is_it_endofURL = 1;
		}
		
		if(uri[i+1] == 'R')//to get resume data
		{
			if(uri[i+2] == 'E')
			{
				if(uri[i+3] == 'S')
				{
					i = i+8;
					while(uri[i] != '|')
					{
						Resume[k++]= uri[i++];
					}
					NFX_Resume = (int)atoi(Resume);
					is_it_endofURL = 1;
					URL_number = url_num;
				}
			}	
		}
		
		if(uri[i+1] == 'C')
		{
			if(uri[i+2] == 'O')
			{
				if(uri[i+3] == 'M')
				{
					is_it_endofURL = 1;
					URL_number = url_num;
				}
			}	
		}
		i++;
	}

	Max_bitrate = URL_info[0].bps; 
	for(i=0; i<url_num; i++)
	{
		if(Max_bitrate < URL_info[i].bps)
		{
			Max_bitrate = URL_info[i].bps; 
			//SellectedID = (unsigned char)i;
		}
	}
	printf("___^ ^___[NFX log]: Total %d URLS,   Sellected ID = %d,   Resume= %d\n", url_num, SellectedID, NFX_Resume);
}

/***************************************************************************************/
/*************************  		for WMDRM                      *****************/
/***************************************************************************************/

void Parse_IPlayerWMDRM_URL(char *uri)
{
	int i = 0;
	int j = 0;
	int url_num = 0;
	int is_it_endofURL = 0;
	while(is_it_endofURL != 1)		//sunye add for temperate this will be modified by the formal URL address
	{
		while(uri[i] != '|' && uri[i] != '\0')
		{
			IPlayer_URL_info.IPlayer_url[j++] = uri[i++];
		}
		is_it_endofURL = 1;
	}
}
/***************************************************************************************/
/*************************  		for WMDRM                      *****************/
/***************************************************************************************/

void Initialize_URL_buffer(void)
{
	int i, j;
	for(j=0; j<8; j++)
	{
		for(i=0;i<200;i++)
		{
			URL_info[j].url[i] = '\0';
		}
		
		for(i=0;i<20;i++)
		{
			URL_info[j].Down_ID[i] = '\0';
		}

		for(i=0;i<10;i++)
		{
			URL_info[j].Resolution[i] = '\0';
		}

		for(i=0;i<10;i++)
		{
			URL_info[j].Bitrate[i] = '\0';
		}

		URL_info[j].bps = 0;
	}
}


void Get_IP(char *IP, char *URL)
{
    struct hostent     *myent;
    struct in_addr  myen;
    long int *add;
    char temp_url[50] = {'\0', }; 
    char *ip = NULL;
    int i = 0;
    while((URL[i+7] != '/') && (URL[i+7] != '\0'))
    {
    	 	temp_url[i] = URL[i+7];
		i++;
    }
	
    myent = gethostbyname(temp_url);
	
    if (myent == NULL)
    {
        printf("ERROR\n");
	 return;
    }

    while(*myent->h_addr_list != NULL)
    {
        add = (long int *)*myent->h_addr_list;
        myen.s_addr = *add;
    	 ip =  inet_ntoa(myen);
//org        printf("%s\n", inet_ntoa(myen));
        strcpy(IP, ip);	  
        printf("___^ ^___[NFX log]:IP1 = %s\n", ip);
        myent->h_addr_list++;
    }
}

int Speed_Gun(URLContext *h, uint8_t *buf, int size)
{
	struct timeval t1;
	struct timeval t2;
	HTTPContext *s = h->priv_data;
	char chk_speed[600000] = {'\0', };
	signed long long total_len = 0;
	signed long long delta_t = 0;
	int found = -1;
	int cont;
	int biggest = 0;
	int len = 0;
	int zerocnt = 0;
	gettimeofday(&t1 , NULL);
	while(total_len < 500000)//700000
	{
		len = url_read(s->hd, chk_speed+total_len, size);
		if(len > 0)
		{
			total_len = total_len + (unsigned long long)len;
			zerocnt = 0;
		}
		else if(len == 0)
		{
			zerocnt++;
			if(zerocnt > 99)
				break;			
		}
		else if((len == -99) || (Network_error == 1))
			return -99;
	}
	gettimeofday(&t2 , NULL);
	delta_t = (int64_t)t2.tv_sec * 1000000 + t2.tv_usec - ((int64_t)t1.tv_sec * 1000000 + t1.tv_usec);
	bandwidth =(unsigned long long) ((total_len * 8*1000000) / delta_t) / 1024; //Kbps
	if(bandwidth > 2100)
		bandwidth = bandwidth*13/10;
	else if(bandwidth > 1700)
		bandwidth = bandwidth*12/10;
	done_speedchk = 1;
	memset(Pre_Down_ID, 0, 20);			
	strcpy(Pre_Down_ID,  URL_info[SellectedID].Down_ID);

	for(cont=0; cont<URL_number; cont++)
	{
		if((URL_info[cont].bps <= bandwidth) && (URL_info[cont].bps > URL_info[biggest].bps))
		{
			biggest = cont;
			found = cont;
		}
		else if((URL_info[cont].bps <= bandwidth) && (found == -1))
		{
			biggest = cont;
			found = cont;
		}
	}
	if(found < 0)
	{
		unsigned long long Low_bitrate = URL_info[0].bps;
		found = 0;
		for(cont=0; cont<URL_number; cont++)
		{
			if(Low_bitrate > URL_info[cont].bps)
			{
				Low_bitrate = URL_info[cont].bps;
				found = cont;
			}
		}
	}
	SellectedID = (unsigned char)found;
	printf("___^ ^___[NFX log]:bandwidth = %lld,	SellectedID = %d by speed gun\n", bandwidth, SellectedID);
	return 0;
}

int av_NFX_SetCallback4WMDRM(DrmData cbFunction)
{
	printf("___^ ^___[NFX log]:av_NFX_SetCallback4WMDRM was called\n");
	g_CbFunction = cbFunction;
	return 1;
}

/***************************************************************************************/
/*************************  		for WMDRM                      *****************/
/***************************************************************************************/

int av_IPlayer_SetCallback4WMDRM(DrmData cbFunction)
{
	printf("++++>[WMDRM] : av_NFX_SetCallback4WMDRM was called\n");
	g_CbIPlayerFunction = cbFunction;
	return 1;

}
/***************************************************************************************/
/*************************  		for WMDRM                      *****************/
/***************************************************************************************/

int av_NFX_Get_Log_Info(unsigned char ID, char * pTarget)
{
	switch(ID)
	{
		case DLID:
    		       strcpy(pTarget, URL_info[SellectedID].Down_ID);
			break;
			
		case DLIDOLD:
    		       strcpy(pTarget, Pre_Down_ID);
			break;
			
		case URL1:
   		       strcpy(pTarget, URL_info[SellectedID].url);
			break;
			
		case URL2:
   		       strcpy(pTarget, Second_URL);
			break;
			
		case IP:
			strcpy(pTarget, IP1);
 			break;

		case Second_IP:
			strcpy(pTarget, IP2);
 			break;
			
		case OLD_IP:
			strcpy(pTarget, IP_OLD);
 			break;
			
		case CUR_URL:
   		       strcpy(pTarget, URL_info[SellectedID].url);
			break;
			
		case CUR_ID:
			*pTarget = SellectedID;
			break;
			
		case INITIALBW:
			snprintf(pTarget, 20,"%lld", bandwidth);
			break;
			
		case STREAM_SW:
			break;
			
		case STOP_WMDRM:
			printf("___^ ^___[NFX log]:STOP_WMDRM was requested\n");
			EscapeByStop = 1;
 			break;

		case CLEAN_STOP:
			printf("___^ ^___[NFX log]:EscapeByStop flag was cleared\n");
			EscapeByStop = 0;
 			break;

		case SET_NFX:
			printf("___^ ^___[NFX log]:NetFlix_connect was set to 1\n");
			NetFlix_connect = 1;
			break;
			
		case CLR_NFX:
			printf("___^ ^___[NFX log]:NetFlix_connect was set to 0\n");
			if(NetFlix_connect == 1)
			{
				if(wmdrmIn != NULL)
					av_free(wmdrmIn);
		    		if(wmdrmOut != NULL)
		       		av_free(wmdrmOut);
		    		if(pChallenge != NULL)
			      		av_free(pChallenge);
				did_you_get_Lice = 0; 
				removeheader = 0;
				bypassf = 0;
				ASF_Correction = 0;
				NFX_err_cnt = 0;
				EndOfFile = 0;
				done_speedchk = 0;
				Network_error = 0;
				Request_speedchk = 0;
				is_it_stream_switching = 0;
				WR_wmdrm_Close();	
				NetFlix_connect = 0;
			}
			break;
			
		case CHK_LICENCE:
			printf("___^ ^___[NFX log]:WMDRM licence checking was done\n");
			if(did_you_get_Lice == TAKED)
			{
				*pTarget = 1;
			}
			else if(Licence_null == 1)
			{
				*pTarget = 44;
				Licence_null = 0;
			}
			break;
			
		case CHK_EOF:
			if((EndOfFile == 1)||(NFX_err_cnt>0))
			{
				*pTarget = 1;
			}
			printf("___^ ^___[NFX log]:EndOfFile = %d, 	NFX_err_cnt = %d\n", EndOfFile, NFX_err_cnt);
			EndOfFile = 0;//shintest
			NFX_err_cnt = 0;//shintest
			break;

		case CHK_SPEED:
			Request_speedchk = 1;
			EndOfFile = 0;
			NFX_err_cnt = 0;
			is_it_stream_switching = 0;
			printf("___^ ^___[NFX log]:Bandwidth checking was requested\n");
			break;

		case CHK_BWRESULT:
			if(is_it_stream_switching == 1)
				*pTarget = 1;
			else if(is_it_stream_switching == 2)
				*pTarget = 2;				
			break;
	}
	return 1;
}

int av_NFX_Get_Url_Info(void **url_info)
{
	*url_info = URL_info;
	return URL_number;
}

int av_NFX_Get_CopyProtectInfo(unsigned char* Macrovision, unsigned char* CGMS, unsigned char* ICT)
{
	*Macrovision = Macro4NFX;
	*CGMS = CGMS4NFX;
	*ICT = ICT4NFX;
	printf("___^ ^___[NFX log]:Macrovision= %d, CGMS= %d, ICT= %d\n", Macro4NFX, CGMS4NFX, ICT4NFX);
	return 1;
}

int av_WMDRM_setLicence(unsigned char* pData)
{
	unsigned int LicLen = 0;
	int result;

	if(did_you_get_Lice == TAKED)
		return 0;
	
	if(pData[0] == 'N')
	{
		if(pData[1] == 'U')
		{
			if(pData[2] == 'L')
			{
				if(pData[3] == 'L')
				{
					printf("___^ ^___[NFX log]: Invaled Licence = NULL !!!!!!!\n");
					did_you_get_Lice = INSTALL_ERROR;
					Licence_null = 1;
					return 0;		
				}
			}
		}
	}

	if(pData[0] == 'W')
	{
		if(pData[1] == 'A')
		{
			if(pData[2] == 'I')
			{
				if(pData[3] == 'T')
				{
					printf("___^ ^___[NFX log]: Wait Licence forever !!!!!!!\n");
					NFX_timebuf = 10000000;
					return 0;		
				}
			}
		}
	}

	LicLen = strlen(pData);
	result = WR_wmdrm_InstallLicense((void*)pData, LicLen);
	if(result)
	{
		did_you_get_Lice = TAKED;
		printf("___^ ^___[NFX log]:SetLicence OK\n");
		return 1;		
	}
	else
	{
		did_you_get_Lice = INSTALL_ERROR;
		return 0;		
	}
}
#endif//#####################################for Netflix
