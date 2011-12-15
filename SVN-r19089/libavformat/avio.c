/*
 * Unbuffered io for ffmpeg system
 * Copyright (c) 2001 Fabrice Bellard
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

#include "libavutil/avstring.h"
#include "libavcodec/opt.h"
#include "os_support.h"
#include "avformat.h"

//##########################################for widevine and Netflix
#define Widevine_PROTOCOL	1
#define Netflix_PROTOCOL		2
#define VUDU_PROTOCOL		3
#define IPLAY_WMDRM			4
#define TF1_WMDRM			5
#define IPLAY_SSL_WMDRM	6
#define WMDRM				
int get_special_protocol_type(char *filename, char *name);
//##########################################for widevine and Netflix

#if LIBAVFORMAT_VERSION_MAJOR >= 53
/** @name Logging context. */
/*@{*/
static const char *urlcontext_to_name(void *ptr)
{
    URLContext *h = (URLContext *)ptr;
    if(h->prot) return h->prot->name;
    else        return "NULL";
}
static const AVOption options[] = {{NULL}};
static const AVClass urlcontext_class =
        { "URLContext", urlcontext_to_name, options };
/*@}*/
#endif

static int default_interrupt_cb(void);

URLProtocol *first_protocol = NULL;
URLInterruptCB *url_interrupt_cb = default_interrupt_cb;

URLProtocol *av_protocol_next(URLProtocol *p)
{
    if(p) return p->next;
    else  return first_protocol;
}

int av_register_protocol(URLProtocol *protocol)
{
    URLProtocol **p;
    p = &first_protocol;
    while (*p != NULL) p = &(*p)->next;
    *p = protocol;
    protocol->next = NULL;
    return 0;
}

#if LIBAVFORMAT_VERSION_MAJOR < 53
int register_protocol(URLProtocol *protocol)
{
    return av_register_protocol(protocol);
}
#endif

int url_open_protocol (URLContext **puc, struct URLProtocol *up,
                       const char *filename, int flags)
{
    URLContext *uc;
    int err;

    uc = av_malloc(sizeof(URLContext) + strlen(filename) + 1);
    if (!uc) {
        err = AVERROR(ENOMEM);
        goto fail;
    }
#if LIBAVFORMAT_VERSION_MAJOR >= 53
    uc->av_class = &urlcontext_class;
#endif
    uc->filename = (char *) &uc[1];
    strcpy(uc->filename, filename);
    uc->prot = up;
    uc->flags = flags;
    uc->is_streamed = 0; /* default = not streamed */
    uc->is_optical = 0;
    uc->max_packet_size = 0; /* default: stream file */
    err = up->url_open(uc, filename, flags);
    if (err < 0) {
        av_free(uc);
        *puc = NULL;
        return err;
    }

    //We must be carefull here as url_seek() could be slow, for example for http
    if(   (flags & (URL_WRONLY | URL_RDWR))
       || !strcmp(up->name, "file"))
        if(!uc->is_streamed && url_seek(uc, 0, SEEK_SET) < 0)
            uc->is_streamed= 1;
    *puc = uc;
    return 0;
 fail:
    *puc = NULL;
    return err;
}

#ifdef WMDRM
extern int is_it_IPlayerWMDRM;
extern int is_it_Netflix;//************************for NetFlix
#endif


int url_open(URLContext **puc, const char *filename, int flags)
{
    URLProtocol *up;
    const char *p;
    char proto_str[300];
    char *q;
    p = filename;
    q = proto_str;
//for  WIDEVINE & VUDU
    int type;
//#endif

   while (*p != '\0' && *p != ':')
   {
        /* protocols can only contain alphabetic chars */
        if (!isalpha(*p))
            goto file_proto;
        if ((q - proto_str) < sizeof(proto_str) - 1)
            *q++ = *p;
        p++;
   }
    /* if the protocol has length 1, we consider it is a dos drive */
    if (*p == '\0' || is_dos_path(filename)) {
    file_proto:
        strcpy(proto_str, "file");
    } else {
        *q = '\0';
    }

    up = first_protocol;
    while (up != NULL)
   {
        if (!strcmp(proto_str, up->name))
        {
        	type = get_special_protocol_type(filename, up->name);
		if(type == Widevine_PROTOCOL)
        	{
			av_log(NULL,AV_LOG_INFO,"======>[it's Widevine protocol !!!!!!!]<======\n");
        			while(strcmp("wvine", up->name))
        			{
       				 up = up->next;
        			}
        		}
		else if(type == VUDU_PROTOCOL)
		{
			av_log(NULL,AV_LOG_INFO,"======>[it's VUDU protocol !!!!!!!]<======\n");
        		while(strcmp("vudu", up->name))
        		{
       			 up = up->next;
        		}
		}
#ifdef WMDRM		
		else if(type == Netflix_PROTOCOL)
		{
			av_log(NULL,AV_LOG_INFO,"___^ ^___[NFX log]:it's Netflix protocol \n");
			is_it_Netflix = 1;
		}

		else if (type == IPLAY_WMDRM)
		{
			av_log(NULL,AV_LOG_INFO,"======>[it's IPLAY_WMDRM protocol !!!!!!!]<======\n");
			is_it_IPlayerWMDRM = 1;		
		}
#endif
		return url_open_protocol (puc, up, filename, flags);	
        }
/*
// ********Original code************
        if (!strcmp(proto_str, up->name))
            return url_open_protocol (puc, up, filename, flags);
*/
        up = up->next;
    }
    *puc = NULL;
    return AVERROR(ENOENT);
}

int url_read(URLContext *h, unsigned char *buf, int size)
{
    int ret;
    if (h->flags & URL_WRONLY)
        return AVERROR(EIO);
    ret = h->prot->url_read(h, buf, size);
    return ret;
}

int url_read_complete(URLContext *h, unsigned char *buf, int size)
{
    int ret, len;

    len = 0;
    while (len < size) {
       ret = url_read(h, buf+len, size-len);
        if (ret < 1)
            return ret;
        len += ret;
    }
    return len;
}

int url_write(URLContext *h, unsigned char *buf, int size)
{
    int ret;
    if (!(h->flags & (URL_WRONLY | URL_RDWR)))
        return AVERROR(EIO);
    /* avoid sending too big packets */
    if (h->max_packet_size && size > h->max_packet_size)
        return AVERROR(EIO);
    ret = h->prot->url_write(h, buf, size);
    return ret;
}

int64_t url_seek(URLContext *h, int64_t pos, int whence)
{
    int64_t ret;

    if (!h->prot->url_seek)
        return AVERROR(EPIPE);
    ret = h->prot->url_seek(h, pos, whence);
    return ret;
}

int url_close(URLContext *h)
{
    int ret = 0;
    if (!h) return 0; /* can happen when url_open fails */

    if (h->prot->url_close)
        ret = h->prot->url_close(h);
    av_free(h);
    return ret;
}

int url_exist(const char *filename)
{
    URLContext *h;
    if (url_open(&h, filename, URL_RDONLY) < 0)
        return 0;
    url_close(h);
    return 1;
}

int64_t url_filesize(URLContext *h)
{
    int64_t pos, size;

    size= url_seek(h, 0, AVSEEK_SIZE);
    if(size<0){
        pos = url_seek(h, 0, SEEK_CUR);
        if ((size = url_seek(h, -1, SEEK_END)) < 0)
            return size;
        size++;
        url_seek(h, pos, SEEK_SET);
    }
    return size;
}

int url_get_file_handle(URLContext *h)
{
    if (!h->prot->url_get_file_handle)
        return -1;
    return h->prot->url_get_file_handle(h);
}

int url_get_max_packet_size(URLContext *h)
{
    return h->max_packet_size;
}

void url_get_filename(URLContext *h, char *buf, int buf_size)
{
    av_strlcpy(buf, h->filename, buf_size);
}


static int default_interrupt_cb(void)
{
    return 0;
}

void url_set_interrupt_cb(URLInterruptCB *interrupt_cb)
{
    if (!interrupt_cb)
        interrupt_cb = default_interrupt_cb;
    url_interrupt_cb = interrupt_cb;
}

int av_url_read_pause(URLContext *h, int pause)
{
    if (!h->prot->url_read_pause)
        return AVERROR(ENOSYS);
    return h->prot->url_read_pause(h, pause);
}

int64_t av_url_read_seek(URLContext *h,
        int stream_index, int64_t timestamp, int flags)
{
    if (!h->prot->url_read_seek)
        return AVERROR(ENOSYS);
    return h->prot->url_read_seek(h, stream_index, timestamp, flags);
}

int get_special_protocol_type(char *filename, char *name)
{
	int type = 0;
	int fWidevine = 0;
	int j;
    	char proto_str[2800] = {'\0', };
      	if (!strcmp("http", name))
       {
		strcpy(proto_str, filename);
		for(j=0;proto_str[j] != '\0';j++)
		{
			if(proto_str[j] == '=')
			{
				j++;
				if(proto_str[j] == 'W')
				{
					j++;
					if(proto_str[j] == 'V')
					{
						type = Widevine_PROTOCOL;
					}
				}
			}
		}
		if (type == 0)
		{
			for(j=0;proto_str[j] != '\0';j++)
			{
				if(proto_str[j] == '=')
				{
					j++;
					if(proto_str[j] == 'V')
					{
						j++;
						if(proto_str[j] == 'U')
						{
							j++;
							if(proto_str[j] == 'D')
							{
								j++;
								if(proto_str[j] == 'U')
								{
									type = VUDU_PROTOCOL;
								}
							}
						}
					}
				}
			}
		}
 		if (type == 0)
      		{
			for(j=0;proto_str[j] != '\0';j++)
			{
				if(proto_str[j] == '=')
				{
					j++;
					if(proto_str[j] == 'N')
					{
						j++;
						if(proto_str[j] == 'F')
						{
							j++;
							if(proto_str[j] == 'L')
							{
								j++;
								if(proto_str[j] == 'X')
								{
									type = Netflix_PROTOCOL;
								}
							}
						}
					}
				}
			}
		}
#ifdef WMDRM
		if (type == 0)
		{
			char* pProto = "=IPLAYER_WMDRM";
			char* pStr = proto_str;
			av_log(NULL,AV_LOG_INFO,"####################### pStr=%s\n", pStr);
			if (*pStr && strstr(pStr, pProto))
			{
				type = IPLAY_WMDRM;
				av_log(NULL,AV_LOG_INFO,"####################### IPlayer Case ##############\n");
      	}
 		}
#endif
  	}

	return type;
}
