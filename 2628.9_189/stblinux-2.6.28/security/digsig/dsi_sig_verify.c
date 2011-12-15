/*
 * Digital Signature (DigSig)
 *
 * dsi_sig_verify.c
 *
 * Copyright (C) 2003 Ericsson, Inc
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 * Authors: Axelle Apvrille
 *          David Gordon Aug 2003 
 * modifs:  Makan Pourzandi Sep 2003 
 *          Chris Wright    Sep 2004
 */

#include <linux/string.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/crypto.h>
#include <linux/version.h>
#include <linux/scatterlist.h>

#include "dsi.h"
#include "dsi_debug.h"
#include "dsi_sig_verify.h"
#include "gnupg/cipher/rsa-verify.h"

/*
 * Public key format: 2 MPIs
 * 2 bytes: length of 1st MPI (ie. 0x0400)
 * n bytes: MPI
 * 2 bytes: length of 2nd MPI (ie. 0x0006)
 * n bytes: MPI (ie. 0x29)
 */

#define TVMEMSIZE	4096

int gDigestLength[] = { /* SHA-1 */ 0x14 };
static struct crypto_hash *sha1_tfm;
MPI digsig_public_key[] = {MPI_NULL, MPI_NULL};


/******************************************************************************
                             Internal functions 
******************************************************************************/

int
digsig_rsa_bsign_verify(unsigned char *sha_cat, int length,
		     unsigned char *signed_hash);

static int digsig_sha1_init(SIGCTX * ctx);

static void digsig_sha1_update(SIGCTX * ctx, char *buf, int buflen);

static int digsig_sha1_final(SIGCTX * ctx, char *digest);


/******************************************************************************
Description : 
Parameters  : 
  hashalgo the identifier of the hash algorithm to use (HASH_SHA1 for instance)
  signalgo sign algorithm identifier. Ex. SIGN_RSA
Return value: a signature context valid for this signature only
******************************************************************************/

SIGCTX *digsig_sign_verify_init(int hashalgo, int signalgo)
{
	SIGCTX *ctx;

	/* allocating signature context */
	ctx = kmalloc(sizeof(SIGCTX), GFP_KERNEL);
	if (!ctx) {
		DSM_ERROR("Cannot allocate ctx\n");
		goto err;
	}


	/* checking hash algorithm is known */
	switch (hashalgo) {
	case HASH_SHA1:
		if (digsig_sha1_init(ctx)) {
			DSM_ERROR("Initializing SHA1 failed\n");
			goto err;
		}
		break;
	default:
		DSM_ERROR("Unknown hash algo\n");
		goto err;
	}

	/* checking sign algo is known */
	switch (signalgo) {
	case SIGN_RSA:
		break;
	default:
		DSM_ERROR("Unknown sign algo\n");
		goto err;
	}
	ctx->digestAlgo = hashalgo;
	ctx->signAlgo = signalgo;
	return ctx;

err:
	if (ctx) {
		kfree(ctx);
	}
	
	return NULL;
}

/******************************************************************************
Description : 
Parameters  : 
  ctx
  buf data to sign (part of a bigger buffer)
  buflen length of buf
Return value: 0 normally
******************************************************************************/
int digsig_sign_verify_update(SIGCTX * ctx, char *buf, int buflen)
{
	if (ctx == NULL)
		return -EINVAL;

	switch (ctx->digestAlgo) {
	case HASH_SHA1:
		digsig_sha1_update(ctx, buf, buflen);
		break;
	default:
		DSM_ERROR("%s: Unknown hash algo\n", __FUNCTION__);
		return -EINVAL;
	}

	return 0;
}

/******************************************************************************
Description : This is where the RSA signature verification actually takes place
Parameters  : 
Return value: 
******************************************************************************/
int
digsig_sign_verify_final(SIGCTX * ctx, int siglen /* PublicKey */ ,
		      unsigned char *signed_hash)
{
	char *digest;
	int rc = -ENOMEM;

	digest = kmalloc (gDigestLength[ctx->digestAlgo], DIGSIG_SAFE_ALLOC);
	if (!digest) {
		DSM_ERROR ("kmalloc failed in %s for digest\n", __FUNCTION__);
		goto err;
	}
	/* TO DO: check the length of the signature: it should be equal to the length
	   of the modulus */
	if ((rc = digsig_sha1_final(ctx, digest)) < 0) {
		DSM_ERROR
		    ("%s: Cannot finalize hash algorithm\n", __FUNCTION__);
		goto err;
	}

	rc = -EINVAL;
	if (siglen < gDigestLength[ctx->digestAlgo])
		goto err;

	switch (ctx->digestAlgo) {
	case SIGN_RSA:
		rc = digsig_rsa_bsign_verify(digest,
					  gDigestLength[ctx->digestAlgo],
					  signed_hash);
		break;
	default:
		DSM_ERROR
		    ("Unsupported cipher algorithm in binary digital signature verification\n");
	}

	/* free everything */
	/* do not free SHA1-TFM. For optimization, we choose always to use the same one */
err:
	kfree (digest);
	return rc;
}

/******************************************************************************
Description : 
   Call this only at the end of the whole program when you do not want to use 
   signature verification again.
Parameters  : 
Return value: 
******************************************************************************/
void digsig_sign_verify_free(void)
{
	if (sha1_tfm) {
		crypto_free_hash(sha1_tfm);
		sha1_tfm = NULL;
	}
	/* this might cause unpredictable behavior if structures are refering to this,
	   their pointer might suddenly become NULL, might need a usage count associated */
}

/******************************************************************************
Description : 
   Initialize public key
Parameters  : 
Return value:
******************************************************************************/

int digsig_init_pkey(const char read_par, unsigned char *raw_public_key, int mpi_size)
{
	int nread;

	switch (read_par) {

	case 'n':
		DSM_PRINT(DEBUG_SIGN, "Reading raw_public_key_n!\n");
		nread = mpi_size;
		digsig_public_key[0] =
			mpi_read_from_buffer(raw_public_key, &nread, 0);
		break;
	case 'e':
		DSM_PRINT(DEBUG_SIGN, "Reading raw_public_key_e!\n");
		nread = mpi_size;
		digsig_public_key[1] =
			mpi_read_from_buffer(raw_public_key, &nread, 0);
		break;
	}

	return 0;
}

/*
Description:
    Initialize public key from internal hardcoded arrays.
Parameters  : 
Return value:
*/

void dump_key(char *msg, unsigned char *key, int len)
{
	int i;
	printk(KERN_DEBUG "%s: \n" KERN_DEBUG, msg);
	for (i = 0; i < len; i++) {
		printk("    0x%02x", key[i]);
		if (i % 8 == 7)
			printk("\n" KERN_DEBUG);
	}
	printk("\n");

}

/* this struct gets loaded into the .digsig_keys section.
 * The section is created in arch/mips/kernel/vmlinux.lds.S
 * Adding anything else to the section will probably screw up the
 * tool which inserts the keys into the kernel.
 * The sizes must match the sizes in the tool.
 */
#define N_KEY_MAX       260
#define E_KEY_MAX       16

#define  __digsig_keys_sec	__attribute__ ((__section__ (".digsig_keys")))

__digsig_keys_sec
struct {
	int		valid;
	unsigned int	n_key_max;
	unsigned int	e_key_max;
	unsigned char	n_key[N_KEY_MAX];
	unsigned char	e_key[E_KEY_MAX];
} digsig_keys = { 0xcafe1234,
		 N_KEY_MAX,
		 E_KEY_MAX,
		 {0xde, 0xad, 0xbe, 0xef },
		 {0xa5, 0x5a}};

void digsig_init_pkey_internal(void)
{
	int buf_size;

	//printk(KERN_DEBUG "digsig.keys_valid 0x%08x\n\n", digsig_keys.valid);
	//dump_key("n_key", digsig_keys.n_key, 130);
	//dump_key("e_key", digsig_keys.e_key, 5);

	if (digsig_keys.valid != 1) {
		printk(KERN_NOTICE "DIGSIG: no valid key, digsig security disabled\n");
		return;
	}

	DSM_PRINT(DEBUG_SIGN, "Reading raw_public_key_n!\n");
	/* 
	 * key size in bits encoded in first two bytes, big endian.
         * round key size up and convert to bytes.
	 * add two bytes for the size field
	 */
	buf_size = digsig_keys.n_key[0] << 8 | digsig_keys.n_key[1];
	buf_size = (buf_size + 7)/8;
	buf_size += 2;	
	//printk(KERN_INFO "%s: n_key size %d\n", __FUNCTION__, buf_size);
	digsig_public_key[0] =
		mpi_read_from_buffer(digsig_keys.n_key, &buf_size, 0);

	DSM_PRINT(DEBUG_SIGN, "Reading raw_public_key_e!\n");
	buf_size = digsig_keys.e_key[0] << 8 | digsig_keys.e_key[1];
	buf_size = (buf_size + 7)/8;
	buf_size += 2;	
	//printk(KERN_INFO "%s: e_key size %d\n", __FUNCTION__, buf_size);
	digsig_public_key[1] =
		mpi_read_from_buffer(digsig_keys.e_key, &buf_size, 0);

	g_init = 1;
	printk(KERN_NOTICE "DIGSIG: digsig security enabled\n");
}

/******************************************************************************
Description : 
   Performs RSA verification of signature contained in binary
Parameters  : 
Return value: 0 - RSA signature is valid
              1 - RSA signature is invalid
              -1 - an error occured
******************************************************************************/

int digsig_rsa_bsign_verify(unsigned char *hash_format, int length,
			 unsigned char *signed_hash)
{
	int rc = 0, cmp;
	MPI hash, data;
	unsigned nread = DIGSIG_ELF_SIG_SIZE;
	int nframe;
	unsigned char sig_class;
	unsigned char sig_timestamp[SIZEOF_UNSIGNED_INT];
	int i;
	SIGCTX *ctx = NULL;
	unsigned char *new_sig;

	new_sig = kmalloc(gDigestLength[HASH_SHA1], DIGSIG_SAFE_ALLOC);
	if (!new_sig) {
		DSM_ERROR ("kmalloc failed in %s for new_sig\n", __FUNCTION__);
		return -ENOMEM;
	}

	/* Get MPI of signed data from .sig file/section */
	nread = DIGSIG_ELF_SIG_SIZE;

	data = mpi_read_from_buffer(signed_hash + DIGSIG_RSA_DATA_OFFSET, 
				    &nread, 0);
	if (!data) {
		kfree(new_sig);
		return -EINVAL;
	}

	/* Get MPI for hash */
	/* bsign modif - file hash - gpg modif */
	/* bsign modif: add bsign greet at beginning */
	/* gpg modif:   add class and timestamp at end */

	ctx = kmalloc(sizeof(SIGCTX), GFP_KERNEL);
	if (!ctx) {
		DSM_ERROR("Cannot allocate ctx\n");
		mpi_free (data);
		kfree (new_sig);
		return -ENOMEM;
	}


	digsig_sha1_init(ctx);

	sig_class = signed_hash[DIGSIG_RSA_CLASS_OFFSET];
	sig_class &= 0xff;

	for (i = 0; i < SIZEOF_UNSIGNED_INT; i++) {
		sig_timestamp[i] =
		    signed_hash[DIGSIG_RSA_TIMESTAMP_OFFSET + i] & 0xff;
	}

	digsig_sha1_update(ctx, DIGSIG_BSIGN_STRING, DIGSIG_BSIGN_GREET_SIZE);
	digsig_sha1_update(ctx, hash_format, SHA1_DIGEST_LENGTH);
	digsig_sha1_update(ctx, &sig_class, 1);
	digsig_sha1_update(ctx, sig_timestamp, SIZEOF_UNSIGNED_INT);

	if ((rc = digsig_sha1_final(ctx, new_sig)) < 0) {
		DSM_ERROR
		    ("internal_rsa_verify_final Cannot finalize hash algorithm\n");
		mpi_free(data);
		kfree(ctx);
		return rc;
	}

	nframe = mpi_get_nbits(digsig_public_key[0]);
	hash = do_encode_md(new_sig, nframe);

	if (hash == MPI_NULL) {
		DSM_PRINT(DEBUG_SIGN, "mpi creation failed\\n");
	}

	/* Do RSA verification */
	cmp = rsa_verify(hash, &data, digsig_public_key);
	rc = cmp ? -EPERM : 0;

	mpi_free(hash);
	mpi_free(data);
	kfree(ctx);
	kfree (new_sig);
	return rc;
}

/* 
 * wrapper functions for kernel/module.c:module_sig_verify
 */
int digsig_rsa_bsign_module_verify(unsigned char *digest,
			 unsigned char *signed_hash)
{
	return(digsig_rsa_bsign_verify(digest, 512, signed_hash + DIGSIG_BSIGN_INFOS));
}

int digsig_security_enabled(void)
{
	return(digsig_keys.valid == 1);
}


/******************************************************************************
Description : 
   initialisation of hash with sha1
Parameters  : 
Return value: 0 for successful allocation, -1 for failed
******************************************************************************/

static int digsig_sha1_init(SIGCTX * ctx)
{

	if (ctx == NULL)
		return -1;

	if (sha1_tfm == NULL)

		sha1_tfm = crypto_alloc_hash("sha1", 0, CRYPTO_ALG_ASYNC);
	ctx->hash.tfm = sha1_tfm;
	if (IS_ERR(ctx->hash.tfm)) {
		DSM_ERROR("tfm allocation failed\n");
		return -1;
	}

	crypto_hash_init(&ctx->hash);
	return 0;
}


/******************************************************************************
Description : Portability layer function. 
Parameters  : 
Return value: void. 
******************************************************************************/

static void digsig_sha1_update(SIGCTX * ctx, char *buf, int buflen)
{


	sg_init_one(ctx->sg, buf, buflen);

	crypto_hash_update(&ctx->hash, ctx->sg, buflen);
}

/******************************************************************************
Description : Portability layer function. 
Parameters  : 
Return value: 0 for successful allocation, -EINVAL for failed
******************************************************************************/

static int digsig_sha1_final(SIGCTX * ctx, char *digest)
{
	/* TO DO: check the length of the signature: it should be equal to the length
	   of the modulus */

	if (ctx == NULL)
		return -EINVAL;

	crypto_hash_final(&ctx->hash, digest);
	return 0;
}

