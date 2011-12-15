
#include <linux/autoconf.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/proc_fs.h>

#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/crypto.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
#include <linux/scatterlist.h>
#endif



#include <asm/io.h>
#include <asm/mipsregs.h>
#include <asm/addrspace.h>
#include <asm/scatterlist.h>
#include <asm/delay.h>
#include <asm/brcmstb/common/brcmstb.h>


#include "crypt.h"

int debug = 0;


#ifdef module_param_named
module_param_named(debug, debug, int, 0444);
#else
MODULE_PARM (debug, "i");
#endif
MODULE_PARM_DESC (debug, "enable debug");

//#define DEBUG
//#define TEST_CODE
#ifdef TEST_CODE 

int buf_size = 32;
int test_buf_size;

#ifdef module_param_named
module_param_named(buf_size, buf_size, int, 0444);
#else
MODULE_PARM (buf_size, "i");
#endif
MODULE_PARM_DESC (buf_size, "buffer size ");


/* --------- Start test code  --------- */
#define	N_TEST_BUF	4
unsigned char *test_buf[N_TEST_BUF];
unsigned char digest_sw[32];	/* actually only need 20 bytes for 160 bits */
unsigned char digest_hw[32];

int
init_test_buf(void)
{
	int i, j;
	unsigned short *sp;

	test_buf_size = buf_size << 10;

	for (i = 0; i < N_TEST_BUF; i++) {
	    test_buf[i] = kmalloc(test_buf_size, GFP_KERNEL | GFP_DMA);

	    if (test_buf[i] == NULL) {
		    printk("%s: kmalloc failed\n", __FUNCTION__);
		    for (j = i-1; j >= 0; j--)
			if (test_buf[j])
				kfree(test_buf[j]);
		    return(1);
	    }
	}

	for (j = 0; j < N_TEST_BUF; j++) {
	    sp = (unsigned short *)test_buf[j];

	    for (i = 0; i < test_buf_size/2; i++)
		    sp[i] = (unsigned short)( i + (0x3553 << j));
	    }
	return(0);
}

void
free_test_buf(void)
{
	int i;

	for (i = 0; i < N_TEST_BUF; i++) 
	    if (test_buf[i])
		    kfree(test_buf[i]);
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
void
do_sha1_sw(void)
{
	struct hash_desc hash;
	struct scatterlist sg;
	int i, rc;

	hash.tfm = crypto_alloc_hash("sha1", 0, CRYPTO_ALG_ASYNC);
	if (hash.tfm == NULL) {
		printk("%s: crypto_alloc_tfm failed\n", __FUNCTION__);
		return;
	}
	hash.flags = 0;

	rc = crypto_hash_init(&hash);
	if (rc) {
		printk(KERN_ERR "%s  Error initializing crypto hash; rc = [%d]\n",
			__FUNCTION__, rc);
		crypto_free_hash(hash.tfm);
		return;
	}

	for (i = 0; i < N_TEST_BUF; i++) {
		sg_init_one(&sg, test_buf[i], test_buf_size);
		rc = crypto_hash_update(&hash, &sg, test_buf_size);
		if (rc) {
			printk(KERN_ERR "%s  Error updating crypto hash; rc = [%d]\n",
				__FUNCTION__, rc);
			crypto_free_hash(hash.tfm);
			return;
		}
	}

	rc = crypto_hash_final(&hash, digest_sw);
	if (rc) 
		printk(KERN_ERR "%s  Error finalizing crypto hash; rc = [%d]\n",
			__FUNCTION__, rc);
	crypto_free_hash(hash.tfm);

}
#else
static struct crypto_tfm *sha1_tfm;
void
do_sha1_sw(void)
{
	struct scatterlist sg[1];
	int i, rc;

	sha1_tfm = crypto_alloc_tfm("sha1", 0);
	if (sha1_tfm == NULL) {
		printk("%s: crypto_alloc_tfm failed\n", __FUNCTION__);
		return;
	}
	rc = crypto_digest_init(sha1_tfm);
	if (rc) {
		printk(KERN_ERR "%s  Error initializing crypto hash; rc = [%d]\n",
			__FUNCTION__, rc);
	}

	for (i = 0; i < N_TEST_BUF; i++) {
	    sg[0].page   =  virt_to_page((unsigned long) test_buf[i]);
	    sg[0].offset = (unsigned int)test_buf[i] & ~PAGE_MASK;
	    sg[0].length = test_buf_size;

	    crypto_digest_update(sha1_tfm, sg, 1);
	}

	crypto_digest_final(sha1_tfm, digest_sw);
	crypto_free_tfm(sha1_tfm);

}
#endif


/* --------- End test code  ---------  */
#endif

void
digest_dump(char *msg, unsigned char *dp)
{
	int i;

	printk("%s: %s\n", __FUNCTION__, msg);
	for (i = 0; i < 20; i++) {
		printk("0x%02x  ", dp[i]);
		if (i % 4 == 3)
			printk("\n");
	}
	printk("\n");


}



#define SHA_CONTEXT	2	/* 0, 1, or 2 */
#define DMA_CTLR	1	/* 0 or 1 */

#define NUM_DMA_DESC	4
typedef struct  sharf_control {
	volatile sharf_csr_t	*csr;
	volatile sharf_intr_t	*intr;
	volatile sharf_dma_t	*dma_regs[2];
	sharf_dma_desc_t	*dma_desc[2];	/* ping pong between them */
	unsigned long 	 	dma_desc_paddr[2];
	int			state;
	struct semaphore	busy;
	struct semaphore	wait_intr;
	spinlock_t		lock;
} sharf_t;


#define 		SHA_BUSY	0x01
#define 		SHA_IO		0x02
#define 		SHA_DONE	0x04
#define 		SHA_ERROR	0x08

sharf_t sharf;

static irqreturn_t 
brcm_sharf_intr(int arg, void *foo)
{
	volatile sharf_csr_t	*csr;
	volatile sharf_dma_t	*dma;
	volatile sharf_intr_t	*intr;
	sharf_dma_desc_t	*desc;
	int			do_print;
	unsigned int		intr_status;
	int			ours;

	csr  = sharf.csr;
	intr = sharf.intr;
	desc = sharf.dma_desc[0];
	dma  = sharf.dma_regs[DMA_CTLR];


#ifdef DEBUG
	do_print = 1;
#else
	do_print = 0;
#endif

	//printk("%s\n", __func__);

	if ((sharf.state & SHA_IO) == 0)  {
	    /* no IO in progress */
	    //printk("%s No SHA_IO flag\n", __func__);
	    return IRQ_NONE;
	}
	
	/* 
	 * Check and clear only our interrupts 
	 * There's a small timing hole if this isn't our interrupt,
	 * but our IO operation completes after we read the status.
	 * So check the dma status regardless of the interrupt status.
	 */

	ours = 0;
	intr_status = intr->status; 

	if (intr_status & ( MEM_DMA1_DONE | FAIL2_INTR )) {
	    intr->status_clear =  MEM_DMA1_DONE | FAIL2_INTR ;
	    ours = 1;
	}

	if ((dma->status & STATUS_DMA_BUSY) == 0)  {
	    ours = 1;
	} 
#ifdef DEBUG
	else {
	    printk("%s:  Interrupt but still busy: \n", __func__);
	    do_print = 1;
	}
#endif

	/* should we ignore ERROR_INTR | GR_BRIDGE_ERROR completely ?? */

	if (!ours && ((intr_status & ( ERROR_INTR | GR_BRIDGE_ERROR )) == 0)) {
	    /* not our interrupt */
	    return IRQ_NONE;
	}

	/*
	 * don't worry about the FAIL2_INTR bit.
	 * There's no digest to compare
	 */
	if ((intr_status & FAIL2_INTR) &&  csr->error_status) {
	    printk("%s:  SHARF Error: FAIL2"
		"intr status 0x%08x sharf status 0x%08x sharf error_status 0x%08x\n", 
			__func__,
			intr_status,
			csr->status,
			csr->error_status);
	    do_print = 1;
	}

	if ((intr_status & ERROR_INTR) &&  csr->error_status) {
	    printk("%s:  SHARF Error: ERROR"
			"  intr status 0x%08x sharf status 0x%08x sharf error_status 0x%08x\n", 
			__func__,
			intr_status,
			csr->status,
			csr->error_status);
	    do_print = 1;
	    sharf.state |= SHA_ERROR;
	}

	if (intr_status & GR_BRIDGE_ERROR) {
	    /* io failed for some reason */
	    printk("%s:  GR Bridge Error:"
			" intr status 0x%08x sharf status 0x%08x sharf error_status 0x%08x\n", 
			__func__,
			intr_status,
			csr->status,
			csr->error_status);
	    do_print = 1;
	    sharf.state |= SHA_ERROR;
	}



	if (do_print) {
	    printk("%s:  dma status 0x%08x desc 0x%08x  current 0x%08x byte 0x%08x\n",
		__func__,
		dma->status,
		dma->desc_addr,
		dma->current_desc,
		dma->current_byte);

	    printk("%s: interrupt status 0x%08x, sharf status 0x%08x error 0x%08x\n",
		__func__,
		intr_status,
		csr->status,
		csr->error_status);
	}



	if (ours) {	 /* success */
	    sharf.state |= SHA_DONE;
	    sharf.state &=  ~SHA_IO;
	    up(&sharf.wait_intr);
	}

	/*
	 * Is there an interrupt pending for dma channel 0 ?
	 * If so, return IRQ_NONE so interrupt processing will continue;
	 * Don't clear the ERROR bits, in case they indidate a channel 0 error.
	 */

	if (intr->status & ( MEM_DMA0_DONE | FAIL0_INTR | FAIL1_INTR ))
		return IRQ_NONE;

	intr->status_clear = intr->status & (  ERROR_INTR | GR_BRIDGE_ERROR );
	return IRQ_HANDLED;
}


#define REG_TO_K1(x) ( (volatile unsigned int *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET + (x)) )

void
brcm_hw_init(void)
{
	int i, error;


	spin_lock_init(&sharf.lock);

	sharf.csr = (sharf_csr_t *) REG_TO_K1(BCHP_SHARF_TOP_REVISION);
	sharf.dma_regs[0] = (sharf_dma_t *)REG_TO_K1(BCHP_SHARF_MEM_DMA0_FIRST_DESC);
	sharf.dma_regs[1] = (sharf_dma_t *)REG_TO_K1(BCHP_SHARF_MEM_DMA1_FIRST_DESC);
	sharf.intr = (sharf_intr_t *)REG_TO_K1(SHIFT_INTR2_STATUS);




	for (i = 0; i < 2; i++) {
	    sharf.dma_desc[i] = kmalloc(sizeof(sharf_dma_desc_t) * NUM_DMA_DESC,
						GFP_KERNEL | GFP_DMA);
	    if (sharf.dma_desc[i]  == NULL) {
		printk(KERN_ERR "%s: kmalloc for sharf.dma_desc failed\n", __FUNCTION__);
		break;
	    }

	    sharf.dma_desc_paddr[i] = virt_to_phys(sharf.dma_desc[i]);
#ifdef DEBUG
	    printk("%s: desc[%x] virt 0x%08x phys 0x%08lx\n",
		__FUNCTION__,
		i,
		(unsigned int)sharf.dma_desc[i],
		sharf.dma_desc_paddr[i]);
#endif
	}	

	sharf.csr->status = 0;

#ifdef DEBUG
	printk("%s: csr @ 0x%p dma_regs[0] 0x%p dma_regs[1] 0x%p \n",
		__FUNCTION__,
		sharf.csr,
		sharf.dma_regs[0],
		sharf.dma_regs[1]);

	printk("%s: revision: 0x%08x status 0x%08x error_status 0x%08x\n",
		__FUNCTION__,
		sharf.csr->revision,
		sharf.csr->status,
		sharf.csr->error_status);
#endif

	sema_init(&sharf.busy, 1);	/* unlocked */
	sema_init(&sharf.wait_intr, 0);	/* locked */

	if ( (error = request_irq( BCM_LINUX_SHARF_IRQ,
		brcm_sharf_intr,
		IRQF_DISABLED | IRQF_SHARED,
		"sharf",
		(void *)&sharf)) )
	{
	    printk(KERN_ERR "%s: request_irq failed for BCM_LINUX_SHARF_IRQ %d\n", __func__, error);
	}
}

void
read_context(unsigned char *digest, int which)
{
	volatile sharf_csr_t	*csr;
	int i;
	unsigned int xp[5];

	csr  = sharf.csr;

	switch (which) {
	  case 0:
	    for (i = 0; i < 5; i++)
		    xp[i] = __be32_to_cpu(csr->sha_ctx_0_state[4-i]);
	    break;

	  case 1:
	    for (i = 0; i < 5; i++)
		    xp[i] = __be32_to_cpu(csr->sha_ctx_1_state[4-i]);
	    break;

	  case 2:
	    for (i = 0; i < 5; i++)
		    xp[i] = __be32_to_cpu(csr->sha_ctx_2_state[4-i]);
	    break;

	   default:
		BUG();
		break;
	}

	memcpy(digest, xp, sizeof(xp));
}

#define MODE_UPDATE	0
#define MODE_FIRST	1
#define MODE_LAST	2

int
brcm_gen_digest( unsigned char *buf, int len, unsigned char *digest, int  mode)
{

	volatile sharf_csr_t	*csr;
	volatile sharf_intr_t	*intr;
	volatile sharf_dma_t	*dma;
	sharf_dma_desc_t	*desc;
	int 			error;
	unsigned long		ipl;

#ifdef DEBUG
	int 			i;
	printk("%s: enter: buf 0x%p len 0x%x mode %d\n",
			__FUNCTION__,  buf, len, mode);
#endif
	csr  = sharf.csr;
	intr = sharf.intr;
	desc = sharf.dma_desc[0];
	dma  = sharf.dma_regs[DMA_CTLR];

	if (mode & MODE_FIRST) {
	    while (sharf.state & SHA_BUSY)  {
		/* DigSig is already single threaded */
		printk(KERN_ERR "%s: busy\n", __FUNCTION__);
		down(&sharf.busy);
	    }
	    sharf.state |= SHA_BUSY;
	}
	

	/*
	 * build a one element dma descriptor
	 */
	desc[0].read_addr = virt_to_phys(buf);
	desc[0].write_addr = 0;

	desc[0].xfer = len & XFER_SIZE_MASK;
	desc[0].xfer |= XFER_LAST | XFER_INTR_ENABLE;			

	desc[0].next = 0xdeadbe00;
	desc[0].next |= NEXT_READ_MODE_LE;

	desc[0].index = 0x0;

	desc[0].flags = (SHA_CONTEXT << FLAGS_CONTEXT_SHIFT);
	desc[0].flags |= FLAGS_MODE_SHA1;
	desc[0].flags |= FLAGS_SG_ENABLE;

	if (mode &  MODE_FIRST)
	    desc[0].flags |= FLAGS_SG_SCRAM_START;

	if (mode & MODE_LAST)
	    desc[0].flags |= FLAGS_SG_SCRAM_END;
		

	desc[0].resv1 = 0;
	desc[0].resv2 = 0;


	dma_cache_wback((unsigned long)desc, sizeof(desc));
	dma_cache_wback((unsigned long)buf, len);

#ifdef DEBUG
	{
	    unsigned int		*xp;
	    printk("buf 0x%p len 0x%x digest 0x%p\n",
		    buf, len, digest);
	    xp = (unsigned int *)desc;
	    for (i = 0; i < 8; i++)
		    printk("desc[%d] 0x%08x\n", i, xp[i]);
	}
#endif

#ifdef DEBUG
	printk("%s: dma status 0x%08x desc 0x%08x  current 0x%08x byte 0x%08x\n",
		    __func__,
		    dma->status,
		    dma->desc_addr,
		    dma->current_desc,
		    dma->current_byte);

	printk("%s: sharf status 0x%08x error 0x%08x\n\n",
		    __func__,
		    csr->status,
		    csr->error_status);
	
#endif
	/* 
	 * start the IO
	 * Clear old failure status;
	 * Enable the interrupts
	 */
	
	spin_lock_irqsave(&sharf.lock, ipl);

	sharf.csr->status &= ~STATUS_FAIL_CTX_2;
	intr->mask_clear =  MEM_DMA1_DONE | FAIL2_INTR | ERROR_INTR | GR_BRIDGE_ERROR;


	sharf.state &=  ~(SHA_ERROR | SHA_DONE);
	sharf.state |=  SHA_IO;

	dma->desc_addr = sharf.dma_desc_paddr[0];
	dma->control = 0;
	dma->control = CONTROL_RUN;

	spin_unlock_irqrestore(&sharf.lock, ipl);

	/*
	 * wait for IO to finish
	 */
	down(&sharf.wait_intr);

	error = sharf.state;
	sharf.state &=  ~(SHA_ERROR | SHA_DONE);


	/*
	 * extract the digest from context
	 */
	if (digest)
	    read_context(digest, SHA_CONTEXT);

	if (mode & MODE_LAST) {
	    sharf.state &= ~SHA_BUSY;
	    /* wakeup any waiters */
	    up(&sharf.busy);
	}

#ifdef DEBUG
	if (mode & MODE_LAST)
		digest_dump("HW", digest);
#endif
	return(1);
}
#ifdef TEST_CODE
void
do_test(void)
{
	int i, mode;

	if (init_test_buf())
		return;
	do_sha1_sw();
	digest_dump("sw", digest_sw);

	do_sha1_sw();
	digest_dump("sw", digest_sw);

	/* just sync up to the clock  for testing*/
//	set_current_state(TASK_INTERRUPTIBLE);
//	schedule_timeout(1);
	
	for (i = 0; i < N_TEST_BUF; i++) {
	    mode = 0;
	    if (i == 0)
		mode |= MODE_FIRST;
	    if (i == N_TEST_BUF -1)
		mode |= MODE_LAST;
	    brcm_gen_digest(test_buf[i], test_buf_size , digest_hw, mode);
	}

	digest_dump("hw", digest_hw);

	free_test_buf();

}
#endif

void digsig_module_register(int (*hw_sha)(unsigned char *, int, unsigned char *, int));
void digsig_module_unregister(int (*hw_sha)(unsigned char *, int, unsigned char *, int));

int __init
brcm_crypt_init(void)
{

        printk(KERN_INFO "%s: SHA-1 HW assist for DigSig \n", __FUNCTION__);

	brcm_hw_init();

#ifdef TEST_CODE
	do_test();
#else
	digsig_module_register(brcm_gen_digest);
#endif

	return(0);
}

void __exit
brcm_crypt_exit(void)
{
	int i;


        printk(KERN_INFO "%s exiting\n", __FUNCTION__);
#ifndef TEST_CODE
	digsig_module_unregister(brcm_gen_digest);
#endif

	free_irq(BCM_LINUX_SHARF_IRQ, (void *)&sharf);

	for (i = 0; i < 2; i++) 
	    if (sharf.dma_desc[i])
		kfree(sharf.dma_desc[i]);
}

module_init(brcm_crypt_init);
module_exit(brcm_crypt_exit);

MODULE_LICENSE("GPL");

