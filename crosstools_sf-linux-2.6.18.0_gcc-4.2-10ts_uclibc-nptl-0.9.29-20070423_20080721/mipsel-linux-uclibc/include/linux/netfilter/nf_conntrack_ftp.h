#ifndef _NF_CONNTRACK_FTP_H
#define _NF_CONNTRACK_FTP_H
/* FTP tracking. */

/* This enum is exposed to userspace */
enum ip_ct_ftp_type
{
	/* PORT command from client */
	IP_CT_FTP_PORT,
	/* PASV response from server */
	IP_CT_FTP_PASV,
	/* EPRT command from client */
	IP_CT_FTP_EPRT,
	/* EPSV response from server */
	IP_CT_FTP_EPSV,
};


#endif /* _NF_CONNTRACK_FTP_H */
