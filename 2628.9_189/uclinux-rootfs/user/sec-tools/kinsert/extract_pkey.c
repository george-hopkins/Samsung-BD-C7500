    /*
     * Distributed Security Module (DSM)
     *
     * Simple extractor of MPIs for e and n in gpg exported keys (or pubrings
     * with only one key apparently)
     * Exported keys come from gpg --export.
     * Output is meant to be copy pasted into kernel code until better mechanism.
     *      
     * Copyright (C) 2002-2003 Ericsson, Inc
     *
     *      This program is free software; you can redistribute it and/or modify
     *      it under the terms of the GNU General Public License as published by
     *      the Free Software Foundation; either version 2 of the License, or
     *      (at your option) any later version.
     * 
     * Author: David Gordon Aug 2003 
     * Modifs: Vincent Roy  Sep 2003 
     *         Chris Wright Sep 2004
     *         Axelle Apvrille Feb 2005
     */
    /*
     * This code was extracted from the extract_pkey.c code supplied with DigSig.
     * Extract keys into key buffers.
     */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DIGSIG_ELF_SIG_SIZE       512	/* this is a redefinition */
#define DIGSIG_PKEY_N_OFFSET        8	/* offset for pkey->n */
#define DIGSIG_MPI_MAX_SIZE       512	/* maximum MPI size in BYTES */
    /*
     * Prototype definition 
     */
void            usage();
int             check_pubkey(int fd);
int             getMPI(int fd, unsigned char *key);

extern int debug;

extern char *progname;

void dump_key(unsigned char *msg, unsigned char *key, int len)
{
    int i;

    if (debug == 0)
	return;

    printf("%s len %d\n", msg, len);

    for (i = 0; i < len; i++) {
	printf("0x%02X, ", key[i] & 0xff);
	if (i % 8 == 7)
	    printf("\n");
    }
    printf("\n");
}

/**
 * One argument expected: GPG public key file
 */
int
extract_key(int fd)
{
    extern unsigned char  *n_key, *e_key;
    extern int      n_key_len, e_key_len;


    if (check_pubkey(fd) == -1) {
	fprintf(stderr,"%s: check_pubkey failed\n", progname);
	return(1);
    }


    n_key = (unsigned char *) malloc(DIGSIG_MPI_MAX_SIZE + 1);
    if (n_key == 0) {
	fprintf(stderr, "%s: could not alloc key memory", progname);
	perror(" ");
	return(1);
    }

    e_key = (unsigned char *) malloc(DIGSIG_MPI_MAX_SIZE + 1);
    if (n_key == 0) {
	fprintf(stderr, "%s: could not alloc key memory", progname);
	perror(" ");
	return(1);
    }

    /*
     * read MPI n 
     */
    if ((n_key_len = getMPI(fd, n_key)) == -1) {
	fprintf(stderr, "%s: can't read n_key", progname);
	return(1);
    }
    dump_key("n_key", n_key, n_key_len);

    /*
     * read MPI e 
     */
    if ((e_key_len = getMPI(fd, e_key)) == -1) {
	fprintf(stderr, "%s: can't read e_key", progname);
	return(1);
    }
    dump_key("e_key", e_key, e_key_len);

    return(0);
}


/**
 * Checks public key file format. This should be an OpenPGP message containing an RSA public
 * key.
 *
 * TODO: check all errors
 *
 * @param fd the public key file descriptor (file must be readable)
 *
 * @return -1 if error
 *         0  if successful, the file pointer has moved just at the beginning of the first MPI.
 */
int
check_pubkey(int fd)
{
    unsigned char   c;
    unsigned int    header_len;
    const int       RSA_ENCRYPT_OR_SIGN = 1;
    const int       RSA_SIGN = 3;
    const int       CREATION_TIME_LENGTH = 4;

    /*
     * reading packet tag byte: a public key should be : 100110xx 
     */
    read(fd, &c, 1);
    if (!(c & 0x80)) {
	printf("Bad packet tag byte: this is not an OpenPGP message.\n");
	return -1;
    }
    if (c & 0x40) {
	printf("Packet tag: new packet headers are not supported\n");
	return -1;
    }
    if (!(c & 0x18)) {
	printf("Packet tag: this is not a public key\n");
	return -1;
    }

    switch (c & 0x03) {
    case 0:
	header_len = 2;
	break;
    case 1:
	header_len = 3;
	break;
    case 2:
	header_len = 5;
	break;
    case 3:
    default:
	printf
	    ("Packet tag: indefinite length headers are not supported\n");
	return -1;
    }

    /*
     * skip the rest of the header 
     */
    /*
     * printf("Header len:%d\n",header_len); 
     */
    header_len--;
    lseek(fd, header_len, SEEK_CUR);

    /*
     * Version 4 public key message formats contain: 1 byte for the
     * version 4 bytes for key creation time 1 byte for public key
     * algorithm id MPI n MPI e 
     */
    read(fd, &c, 1);
    /*
     * printf("Version %d\n",c); 
     */
    if (c != 4) {
	printf("Public key message: unsupported version\n");
	return -1;
    }

    lseek(fd, CREATION_TIME_LENGTH, SEEK_CUR);	/* skip packet
							 * creation time */
    read(fd, &c, 1);
    if (c != RSA_ENCRYPT_OR_SIGN && c != RSA_SIGN) {
	printf
	    ("Public key message: this is not an RSA key, or signatures not allowed\n");
	return -1;
    }


    return 0;			/* OK */
}

/**
 * Retrieves an MPI and sends it to DigSig kernel module. 
 * The file must be set at the beginning of the MPI.
 *
 * TO DO: test for read/write errors.
 *
 * @param fd the public key file. Must be readable and pointing on the beginning
 * of an MPI.
 *
 * @param key buffer to hold the key.
 *
 * @return len if successful, and then the file points just after the MPI.
 */
int
getMPI(int fd, unsigned char *key)
{
    unsigned char   c;
    unsigned int    len;
    int             i;
    int             key_offset = 0;


    if (key == NULL) {
	printf("%s: no key buffer\n", __FUNCTION__);
	return -1;
    }

    /*
     * first two bytes represent MPI's length in BITS 
     */
    read(fd, &c, 1);
    key[key_offset++] = c;
    len = c << 8;

    read(fd, &c, 1);
    key[key_offset++] = c;
    len |= c;
    len = (len + 7) / 8;	/* Bit to Byte conversion */

    /*
     * read the MPI 
     */
    read(fd, &key[key_offset], len);
    key_offset += len;



    return key_offset;
}

