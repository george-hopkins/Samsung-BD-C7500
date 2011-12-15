#include <stdio.h>
#include <unistd.h>
#include "openssl/bio.h" 
#include "openssl/ssl.h" 
#include "openssl/err.h"

#include <sys/time.h>

#include "avformat.h"

BIO *			t_pBio; 
SSL_CTX *		t_pCtx; 
extern int bSocketSslOpenState = 0;
int m_Socket;
extern int proto_SslOpen(char* pURL, int port) {

	SSL * ssl; 
	struct timeval starttime,endtime;
	starttime.tv_sec = 0;
	endtime.tv_sec = 0;
	int ret = -1;

	if(bSocketSslOpenState == 1) {
		return -1;
	}
	//printf("[ssl] SslOpen  CALLED = %s\n",pURL);
	/* Set up the library */ 
	SSL_library_init();
	ERR_load_BIO_strings(); 
	SSL_load_error_strings(); 
	OpenSSL_add_all_algorithms(); 

	/* Set up the SSL context */ 
   	t_pCtx = SSL_CTX_new(SSLv23_client_method()); 

	
	t_pBio = BIO_new_ssl_connect(t_pCtx); 

	/* Set the SSL_MODE_AUTO_RETRY flag */ 
 
	//BIO_set_ssl_renegotiate_timeout(t_pBio,5);
	BIO_get_ssl(t_pBio, & ssl); 
	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY); 

	/* Create and setup the connection */ 

	BIO_set_conn_hostname(t_pBio, pURL); 
	BIO_set_nbio(t_pBio, 1); 
	
/*
	if(BIO_do_connect(t_pBio) <= 0) { 
		L_PRINT(LCPrint::A_INFOLINK, LCPrint::DEBUGGING, "Error attempting to connect\n"); 
		BIO_free_all(t_pBio); 
		SSL_CTX_free(t_pCtx); 
		return false; 
	} 
 */
	gettimeofday(&starttime , NULL);

	do 
	{ 
		gettimeofday(&endtime , NULL);

		if( (endtime.tv_sec  - starttime.tv_sec) > 10)
		{
			BIO_free_all(t_pBio); 
			SSL_CTX_free(t_pCtx); 
			return -1;
		}
		
		ret = BIO_do_connect(t_pBio); 
		printf("[ssl] BIO_do_connect RET = %d \n",ret );
		if (ret == 0) 
		{ 
			BIO_free_all(t_pBio); 
			SSL_CTX_free(t_pCtx); 
			return -1;
		} 
		else if (ret < 0 && !BIO_should_retry(t_pBio)) 
		{ 
			printf("[ssl] _2_ ret<0 or BIO_should_retry \n");
			BIO_free_all(t_pBio); 
			SSL_CTX_free(t_pCtx); 
			return -1;
		} 

		usleep(100000); //100ms

	}while (ret < 0); 

	if( BIO_get_fd(t_pBio, &m_Socket) <= 0 )
	{
		printf("[HttpsSocketSSL::%s(%d)] Can't get connection fd\n", __FUNCTION__, __LINE__);
		return -1;
	}


	bSocketSslOpenState = 1;
	return 0;
	
}

extern void proto_SslClose(void) 
{
	if(bSocketSslOpenState == 0) {
		return;
	}
	BIO_free_all(t_pBio); 
	SSL_CTX_free(t_pCtx); 
	bSocketSslOpenState = 0;
}

extern int proto_SslSend(char* buffer , unsigned int size) 
{
	if(bSocketSslOpenState == 0) {
		return -1;
	}

	return BIO_write(t_pBio, buffer, size); 
}

extern int proto_SslReceive(char* buffer , unsigned int size,int isheader) {

	if(bSocketSslOpenState == 0) {
		return -1;
	}

	fd_set set;
	struct timeval tv;

	struct timeval starttime,endtime;
	starttime.tv_sec = 0;
	endtime.tv_sec = 0;

	//printf("[proto_SslReceive::START \n");
	
	FD_ZERO( &set );
	FD_SET(m_Socket, &set);

	tv.tv_sec = 15;
	tv.tv_usec = 0;

	int ret;
//	int len;
//	int temp_size;
	gettimeofday(&starttime , NULL);

//	if(isheader)
//	{
		//printf("[proto_SslReceive::HEADER RECEIVE \n");
		do 
		{ 
			if (url_interrupt_cb())
			{
				printf("@@@ call back is called ON in proto_SslReceive\n");
				return AVERROR(EINTR);
			}
			 
			gettimeofday(&endtime , NULL);
			if( (endtime.tv_sec  - starttime.tv_sec) > 30)
			{
				return -99;
			}

			if( !select(m_Socket+1, &set, NULL, NULL, &tv) )
			{
				printf("[HttpsSocketSSL::%s(%d)] ERROR! read() timeout! 15sec\n",__FUNCTION__, __LINE__);
				ret = -1;
			}
			else
			{
				ret = BIO_read(t_pBio, buffer, size);		//ret will be 0, when stream reaches at EOF
			}


		}while (ret < 0); 
/*	}
	else
	{
		printf("[proto_SslReceive::DATA RECEIVE \n");	
		ret =0;
		temp_size=size;

		while (ret < size || len<=0)
		{ 
			gettimeofday(&endtime , NULL);

			if( (endtime.tv_sec  - starttime.tv_sec) > 10)
			{
				return -1;
			}

			
			if( !select(m_Socket+1, &set, NULL, NULL, &tv) )
			{
				printf("[HttpsSocketSSL::%s(%d)] ERROR! read() timeout! \n");
				ret = 0;
			}
			else
			{

				len = BIO_read(t_pBio, buffer, temp_size);		//ret will be 0, when stream reaches at EOF

				if(0>len)
					len =0;
				
				buffer = buffer + len;
				temp_size = temp_size -len;
				
			}
			printf("!!!!![proto_SslReceive::DATA RECEIVE %d, %d, %d,%d\n",buffer,len,ret ,size);	
			ret = ret +len;
		}
		
		printf("[proto_SslReceive::DATA RECEIVE %d, %d\n",ret, size);	
		
	}*/

	return ret;
		
}

