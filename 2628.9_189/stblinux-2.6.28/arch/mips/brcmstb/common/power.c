

#include <linux/autoconf.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>

#include <asm/io.h>
#include <asm/mipsregs.h>
#include <asm/addrspace.h>
#include <asm/cacheflush.h>
#include <asm/delay.h>
#include <asm/atomic.h>

#include <asm/brcmstb/common/brcmstb.h>
#include <asm/brcmstb/common/power.h>

#include <linux/console.h>
#include <linux/cpu.h>
#include <linux/pm.h>
#include <linux/freezer.h>
#include <linux/suspend.h>
#include <linux/vmstat.h>
#include <linux/syscalls.h>
#include <linux/ftrace.h>
#include <linux/major.h>
#include <linux/power_mgmt.h>


int power_debug = 0;
int power_standby_cnt = 0;
int power_wol;
int power_pm_test;

static int __init set_pm_test(char *p)
{
	power_pm_test = 1;
	return 0;
}
early_param("pm_test", set_pm_test);

#ifdef module_param_named
module_param_named(debug, power_debug, int, 0444);
#else
MODULE_PARM (power_debug, "i");
#endif
MODULE_PARM_DESC (power_debug, "enable debug");

__locked_text void __power_down(void);
__locked_text void __power_up(void);
static void power_down(void);
static void power_up(void);

void enet_active(int wol);
void enet_standby(int wol);

extern void plat_irq_dispatch(void);

#ifdef CONFIG_USB_EHCI_HCD
extern void ehci_hcd_init(void);
extern void ehci_hcd_cleanup(void);
#endif /* CONFIG_USB_EHCI_HCD */
#ifdef CONFIG_USB_OHCI_HCD
extern void ohci_hcd_mod_init(void);
extern void ohci_hcd_mod_exit(void);
#endif /* CONFIG_USB_OHCI_HCD */


typedef enum power_states  {
	Active = 0,
	Dark,		/* AV clocks off, DDR RAM  active */
	Standby
} power_states_t;

struct {
	power_states_t	state;
	atomic_t	ref_cnt;
	int		dev_major;
	unsigned long	sp;
	unsigned long	gp;
	unsigned long	w0_mask;		/* HIF CPU level 1 intr w0 mask status */
	unsigned long	w1_mask;		/* HIF CPU level 1 intr w1 mask status */
	unsigned long	cpu_mask;		/* MIPS intr mask */
	power_event_t	event;			/* current event state */
	unsigned int	early;			/* early wakeup */
	reg_t		wake_counter;		/* wakeup counter */
} power __locked_data = {0, {0}, 0, 0};		/* initialize to force into Data segment */


//#define DEBUG

/*
 * /proc interface section
 */

static int
power_read_proc( char	*page,
		 char	**start,
		 off_t	off,
		 int	count,
		 int	*eof,
		 void	*data)
{
	int len;
	unsigned int value;

	len = 0;
	*eof = 1;

	if (off)
		return(0);

	value = (unsigned int) data;
	 
#if 0
	printk("%s page %p  start %p off 0x%08lx, count %d\n", 
		__FUNCTION__, page, *start, off, count);
#endif

        len += sprintf(page + len, "0x%x\n", power_standby_cnt);

        return len;
}

static int
power_write_proc(struct file	*file,
		 const char __user *buffer,
		 unsigned long	count,
		 void		*data)
{
	unsigned int value, tmp;

	int full_count, num_args;
	full_count = count;

	value = (unsigned int) data;
	 
	num_args = sscanf(buffer,"0x%x\n", &tmp);

	if (num_args == 0)
		num_args = sscanf(buffer,"%d\n", &tmp);

#if 0
	printk("%s value 0x%08x data 0x%08x\n",
		__FUNCTION__, value, tmp);
	printk("count: %d len %d\n", full_count, num_args);
#endif

	if (num_args != 1)
		return(full_count);

	power_standby_cnt = tmp;

	while (power_standby_cnt) {
		power_standby_cnt--;
		power.event.mask = PWR_UART_0 | PWR_UART_1 | PWR_UART_2;
		power_down();
		power.event.mask = 0;
	}

	return(full_count);
}



/*
 * watch dog timer support
 */
struct timers {
	unsigned int	int_status;
	unsigned int	cpu_int_enable;
	unsigned int	timer_ctrl[4];
	unsigned int	timer_stat[4];
	unsigned int	wd_timeout;
	unsigned int	wd_cmd;
	unsigned int	wd_reset_count;
	unsigned int	wd_reset_status;
	unsigned int	pci_int_enable;
	unsigned int	wd_control;
	};

static void
start_watchdog(void)
{
	volatile struct timers *tp;
	volatile int *stc;

	stc = (volatile int *)BCM_PHYS_TO_K1(BCHP_SUN_TOP_CTRL_RESET_CTRL);

	*stc |= BCHP_SUN_TOP_CTRL_RESET_CTRL_watchdog_reset_enable_MASK;

	tp = (volatile struct timers *) BCM_PHYS_TO_K1(TIMR_ADR_BASE);
	
	tp->wd_timeout = 0x7fffffff;	/* 38 seconds */
	tp->wd_cmd = 0xff00;
	tp->wd_cmd = 0x00ff;

}

static void
stop_watchdog(void)
{
	volatile struct timers *tp;

	tp = (volatile struct timers *) BCM_PHYS_TO_K1(TIMR_ADR_BASE);
	
	tp->wd_cmd = 0xee00;
	tp->wd_cmd = 0x00ee;
}

/*
 * Cache ops
 */

#if defined(cache_op)
# undef cache_op
#endif

#define Lock_I			0x1c
#define Lock_D			0x1d
#define Hit_Invalidate_I	0x10
#define Hit_Writeback_Inv_D	0x15

#define cache_op(op,addr)						\
	__asm__ __volatile__(						\
	"	.set	noreorder				\n"	\
	"	.set	mips3\n\t				\n"	\
	"	cache	%0, %1					\n"	\
	"	.set	mips0					\n"	\
	"	.set	reorder"					\
	:								\
	: "i" (op), "m" (*(unsigned char *)(addr)))

/* 
 * linux has a define for this
*/
#define CACHE_LINE_SIZE	32

/*
 * these functions assume there's only a primary cache and
 * no secondary cache
 */
static void
cache_lock_I(unsigned long start, int len)
{
	unsigned long addr, end;

#ifdef DEBUG
	printk("%s addr 0x%08lx len 0x%x\n", __FUNCTION__, start, len);
#endif
	addr = start & ~(CACHE_LINE_SIZE - 1);
	end = (start + len - 1) & ~(CACHE_LINE_SIZE - 1);

	do {
		cache_op(Lock_I, addr);
		if (addr == end)
			break;
		addr += CACHE_LINE_SIZE;
	} while (1);
}

static void
cache_unlock_I(unsigned long start, int len)
{
	unsigned long addr, end;

#ifdef DEBUG
	printk("%s addr 0x%08lx len 0x%x\n", __FUNCTION__, start, len);
#endif
	addr = start & ~(CACHE_LINE_SIZE - 1);
	end = (start + len - 1) & ~(CACHE_LINE_SIZE - 1);

	do {
		cache_op(Hit_Invalidate_I, addr);
		if (addr == end)
			break;
		addr += CACHE_LINE_SIZE;
	} while (1);
}

static void
cache_lock_D(unsigned long start, int len)
{
	unsigned long addr, end;

#ifdef DEBUG
	printk("%s addr 0x%08lx len 0x%x\n", __FUNCTION__, start, len);
#endif
	addr = start & ~(CACHE_LINE_SIZE - 1);
	end = (start + len - 1) & ~(CACHE_LINE_SIZE - 1);

	do {
		cache_op(Lock_D, addr);
		if (addr == end)
			break;
		addr += CACHE_LINE_SIZE;
	} while (1);
}

static void
cache_unlock_D(unsigned long start, int len)
{
	unsigned long addr, end;

#ifdef DEBUG
	printk("%s addr 0x%08lx len 0x%x\n", __FUNCTION__, start, len);
#endif
	addr = start & ~(CACHE_LINE_SIZE - 1);
	end = (start + len - 1) & ~(CACHE_LINE_SIZE - 1);

	do {
		cache_op(Hit_Writeback_Inv_D, addr);
		if (addr == end)
			break;
		addr += CACHE_LINE_SIZE;
	} while (1);
}


/*
 * ART
 */

static art_ins_t art_test[] = {
	INS( Delay,	0x400,	0),
	INS( RdAnd,	BCHP_CLK_REVISION , 0xffffffff),
	INS( WrOr, 	BCHP_CLK_SCRATCH , 0),
	INS( Halt, 	0, 0),
};

static void
art_clear_seq(void)
{
	volatile art_t *art;
	int i;

	art = (volatile art_t *)REG_TO_K1(BCHP_ART_REVISION);

	art->codestore_pointer = 0;

	for (i = 0; i < 256; i++) 
	    art->codestore_data = 0;
}

static void
art_load_seq(art_ins_t *seq, unsigned int index)
{

	volatile art_t *art;
	int i;

	art = (volatile art_t *)REG_TO_K1(BCHP_ART_REVISION);

	art->codestore_pointer = index;	/* mask ? */

	for (i = 0; ; i++) {
	    art->codestore_data = seq[i].word_0;
	
	    if ((seq[i].word_0 >> 28) > Delay)
		art->codestore_data = seq[i].word_1;

	    /* check for end of sequence */
	    if (seq[i].word_0 == Halt)
			break;
	} 

	art->codestore_data = 0;
}

static void
art_dump_seq(unsigned int index, unsigned int count)
{
	volatile art_t *art;
	int i;
	unsigned int word;

	art = (volatile art_t *)REG_TO_K1(BCHP_ART_REVISION);

	art->codestore_pointer = index;	/* mask ? */

	for (i = 0; i < count; i++) {
		word = art->codestore_data;
		printk(KERN_WARNING "%s: %d: 0x%x\n", __FUNCTION__, i, word);
	} 
}

static void
art_dump_regs(void)
{
	volatile art_t *art;

	art = (volatile art_t *)REG_TO_K1(BCHP_ART_REVISION);

	printk(KERN_WARNING "wake cntrl  sleep_cntrl  trigger status\n"
		KERN_WARNING "0x%08x 0x%08x  0x%08x  0x%08x \n",
		art->wakeup_control,
		art->sleep_control,
		art->sequence_trigger,
		art->sequence_status);
}

static void
art_run_test(void)
{
	volatile art_t *art;

	volatile int *clk_rev;
	volatile int *clk_scratch;

	art_load_seq(art_test, 0);
	art_dump_seq(0,10);

	clk_rev = (volatile int *)REG_TO_K1(BCHP_CLK_REVISION);
	clk_scratch = (volatile int *)REG_TO_K1(BCHP_CLK_SCRATCH);

	printk(KERN_WARNING "clk revision  0x%08x  scratch 0x%08x\n", *clk_rev, *clk_scratch);

	art = (volatile art_t *)REG_TO_K1(BCHP_ART_REVISION);
	
	art_dump_regs();
	art->sleep_control = 0x80000000;
	art->sequence_trigger = 1;

	schedule_timeout_interruptible(10);

	art_dump_regs();
	printk(KERN_WARNING "clk revision  0x%08x  scratch 0x%08x\n", *clk_rev, *clk_scratch);
}

static void
art_init(void)
{

#ifdef DEBUG
	art_clear_seq();
	art_run_test();
#endif

	art_clear_seq();
	art_load_seq(art_pwr_down, 0);
	//art_dump_seq(0, 32);
	art_load_seq(art_pwr_up, 0x20);
	//art_dump_seq(0x20, 32);
}
/* 
 * power management section
 */
#define read_cycle_cntr()  \
({					\
	unsigned int __res;		\
	__asm__ __volatile__(		\
	".set\tmips32r2\n\t"		\
	"mfc0 \t%0, $9, 0 \n"		\
	: "=r" (__res));		\
					\
	__res;				\
})



extern char _slocked_text[], _elocked_text[];
extern char _slocked_data[], _elocked_data[];



extern void rollback_handle_int(void);
extern void handle_int(void), handle_int_end(void);
extern unsigned int kernelsp[NR_CPUS];

extern void all_active(void);
extern void all_standby(void);

__locked_data void  (*power_up_intr)(void);

//#define DEBUG_TRACE

#ifdef DEBUG_TRACE
	#define TX_REG  REG_TO_K1(BCHP_UARTA_REG_START)

	#define DBG_PUTC(x) {*tx = x; }
#else
	#define DBG_PUTC(x) /* */
#endif


typedef struct wake_timer {
	reg_t	event;
	reg_t	counter;
	reg_t	alarm;
	reg_t	prescaller;
} wake_timer_t;

/*
 * lock/unlock all the pieces needed for suspend/resume
 */
static void
cache_lock_all(void)
{

	/* cache lock power down/up code and data*/
	cache_lock_I((unsigned long)_slocked_text, _elocked_text - _slocked_text);
	cache_lock_D((unsigned long)_slocked_data, _elocked_data - _slocked_data);

	/*
	 * plat_irq_dispatch is a jump target in handle_int.
	 * The branch prediction logic in the cpu prefeteches
	 * the target plat_irq_dispatch.
	 * Lock one cache line
	 */
	cache_lock_I((unsigned long)plat_irq_dispatch, 32);	

	/*
	 * cache lock kernel stack pointer save location
	 * this is access at the very beginning of the interrupt.
	 * It might not need to be locked because of the frequency of access,
	 * but why take the chance.
	 */
	cache_lock_D((unsigned long)kernelsp, NR_CPUS * sizeof (void *));

	/* cache lock stack and thread info struct */
	cache_lock_D(power.gp,  THREAD_SIZE);

	/*
	 *  interrupt vector  
	 *  0x200
	 */
	cache_lock_I(0x80000200, 0x1f);

	/* something is fetching in this range */
	cache_lock_D(0x80000000, 0x1f);

	/* interrupt handler */
	cache_lock_I((unsigned long)rollback_handle_int,
			(unsigned long)handle_int_end - (unsigned long)rollback_handle_int);

}

static void
cache_unlock_all(void)
{
	/* cache unlock power down/up code and data*/
	cache_unlock_I((unsigned long)_slocked_text, _elocked_text - _slocked_text);
	cache_unlock_D((unsigned long)_slocked_data, _elocked_data - _slocked_data);

	cache_unlock_I((unsigned long)plat_irq_dispatch, 32);

	/* cache unlock kernel stack pointer save location */
	cache_unlock_D((unsigned long)kernelsp, NR_CPUS * sizeof (void *));
	/* cache unlock stack and thread info struct */
	cache_unlock_D(power.gp,  THREAD_SIZE);

	/* interrupt vector */
	cache_unlock_I(0x80000200, 0x1f);
	cache_unlock_D(0x80000000, 0x1f);

	/* interrupt handler */
	cache_unlock_I((unsigned long)rollback_handle_int,
			(unsigned long)handle_int_end - (unsigned long)rollback_handle_int);
}

/*
 * Thou shalt not make any calls to any functions not locked in cache
 * from functions locked in cache.  Even referening a function like printk
 * in a path not taken may cause a cpu lockup because the cpu does branch 
 * prediction on targets when the branch is prefetched and before the branch
 * is executed.
 */

/*
 * check for any wakeup events and capture in power.event.events
 */
__locked_text int
check_wakeup_events(int debug)
{
	volatile clk_pm_wakeup_t	*ck;
	volatile art_t			*art;
	volatile wakeup_intr2_t 	*wake;
	register unsigned int		mask;
	volatile wake_timer_t		*wktmr;


	ck   = (volatile clk_pm_wakeup_t *)REG_TO_K1(BCHP_CLK_WAKEUP_STATUS);
	art  = (volatile art_t *)REG_TO_K1(BCHP_ART_REVISION);
	wake = (wakeup_intr2_t *)REG_TO_K1(BCHP_WAKEUP_CTRL2_STATUS);
	wktmr = (volatile wake_timer_t *)REG_TO_K1(BCHP_WKTMR_REG_START);


	/*
	 * check wakeup events
	 */

	mask = 0;

	if (ck->wakeup_status & BCHP_CLK_WAKEUP_MASK_ENET_MASK)
	    mask |= PWR_ENET;

	if (ck->wakeup_status & BCHP_CLK_WAKEUP_MASK_HDMI_MASK)
	    mask |= PWR_HDMI;

	/* 
	 *  determine which sundry event  happened
	 */
	if (ck->wakeup_status & BCHP_CLK_WAKEUP_MASK_SUNDRY_MASK) {

	    if (wake->status & WAKE2_IRR)
		mask |= PWR_IRR;
		    
	    if (wake->status & WAKE2_TIMER)
		mask |= PWR_TIMER;
		    
	    if (wake->status & WAKE2_UART0)
		mask |= PWR_UART_0;
		    
	    if (wake->status & WAKE2_UART1)
		mask |= PWR_UART_1;
		    
	    if (wake->status & WAKE2_UART2)
		mask |= PWR_UART_2;

	    if (wake->status & WAKE2_GIO) {
		mask |= PWR_GPIO;
	    }
	}

#if 0	/* can only be enabled if RAM  is not disabled  during suspend */
	if (debug && mask {
	    printk(KERN_DEBUG "%s: Sundry: wake_control2 status 0x%08x, mask 0x%08x\n",
		    __FUNCTION__, wake->status, wake->mask_status);

	    printk(KERN_DEBUG "%s: clk_wakeup status 0x%08x enable mask 0x%08x\n",
		    __FUNCTION__, ck->wakeup_status, ck->wakeup_mask);

	    printk(KERN_DEBUG "%s: events 0x%08x mask 0x%08x\n",
		    __FUNCTION__, mask power.event.mask);
		
	}
#endif

	/* clear events from wakeup controller */
	wake->clear = 0xffffffff;

	/* if an event happened, clear timer alarm */
	if (mask & power.event.mask) {
	    wktmr->alarm = 0;
	    wktmr->event = 1;
	}
	

	power.event.events = mask;
	return (mask & power.event.mask);
}

static void
clear_wakeup_events(void)
{
	volatile clk_pm_wakeup_t	*ck;
	volatile art_t			*art;
	volatile wakeup_intr2_t 	*wake;

	ck   = (volatile clk_pm_wakeup_t *)REG_TO_K1(BCHP_CLK_WAKEUP_STATUS);
	art  = (volatile art_t *)REG_TO_K1(BCHP_ART_REVISION);
	wake = (wakeup_intr2_t *)REG_TO_K1(BCHP_WAKEUP_CTRL2_STATUS);



	/*
	 * Mask all wakeup events  from the ART.
	 * clear any pending wakeup events from the ART.
	 */
	wake->mask_clear = 0xffffffff;
	wake->mask_set = 0xff;
	wake->clear = 0xffffffff;

	/*
	 * clear clock  power management wakeup mask
	 */
	ck->wakeup_mask = 0;

	return;
}


__locked_text void
__power_down(void)
{
	volatile art_t			*art;
	volatile wakeup_intr2_t		*wake;
	volatile clk_pm_wakeup_t	*ck;

#ifdef DEBUG_TRACE
	volatile unsigned int  *tx;
	tx = (volatile unsigned int *)TX_REG;
#endif

	art  = (volatile art_t *)REG_TO_K1(BCHP_ART_REVISION);
	wake = (wakeup_intr2_t *)REG_TO_K1(BCHP_WAKEUP_CTRL2_STATUS);
	ck   = (volatile clk_pm_wakeup_t *)REG_TO_K1(BCHP_CLK_WAKEUP_STATUS);

	power.state = Standby;
	power.early = 0;
	power_up_intr = __power_up;

	sundry_standby();
	bsp_standby();
	ddr_go_standby();

	DBG_PUTC( 'P');


	/* clear previous status */
	art->sequence_status = BCHP_ART_SEQUENCE_STATUS_BAD_OPCODE_MASK
				| BCHP_ART_SEQUENCE_STATUS_CODE_STORE_WRAP_MASK
				| BCHP_ART_SEQUENCE_STATUS_EXECUTED_HALT_MASK
				| BCHP_ART_SEQUENCE_STATUS_GOT_STOP_TRIGGER_MASK
				| BCHP_ART_SEQUENCE_STATUS_GOT_START_TRIGGER_MASK
				| BCHP_ART_SEQUENCE_STATUS_GOT_WAKEUP_MASK;


	if (ck->wakeup_mask & ck->wakeup_status) {
		/* a wakeup event already occured */
		check_wakeup_events(0);
		power.early = 1;
		__power_up();
		return;
	} 

	/* enable_sleep | sleep pointer */
	art->sleep_control  = BCHP_ART_SLEEP_CONTROL_ENABLE_SLEEP_MASK;
	/* start sleep sequence */
	art->sequence_trigger = BCHP_ART_SEQUENCE_TRIGGER_SLEEP_START_MASK;

	wake->clear = 0xff;
	/* enable_wakeup | wakeup pointer */
	art->wakeup_control = BCHP_ART_WAKEUP_CONTROL_ENABLE_WAKEUP_MASK | 0x20;

	DBG_PUTC( 'I');
	local_irq_enable();

	/*
	 * now wait.
	 */
	DBG_PUTC( 'W');
#if 1		/* set to zero to test w/o ART */
	__asm__(".set\tmips3\n\t"
	"wait\n\t"
	"nop\n\t nop\n\t nop\n\t nop\n\t"
	".set\tmips0");
#else
	{ 
		unsigned int i;
		
		local_irq_disable();
		for (i = 0; i < 10; i++)
			DELAY(100000);
		__power_up();
		local_irq_enable();
	}
#endif

	DBG_PUTC( 'd');
	local_irq_disable();
	DBG_PUTC( 'X');
	DBG_PUTC( '\n');
	DBG_PUTC( '\r');

}

__locked_text void
__power_up(void)
{
	volatile clk_pm_wakeup_t *ck;
	volatile art_t		 *art;
#ifdef DEBUG_TRACE
	volatile unsigned int  *tx;
	tx = (volatile unsigned int *)TX_REG;
#endif

	ck = (volatile clk_pm_wakeup_t *)REG_TO_K1(BCHP_CLK_WAKEUP_STATUS);
	art = (volatile art_t *)REG_TO_K1(BCHP_ART_REVISION);

#if 1	// out for debug
	art->wakeup_control &= ~0x80000000;
	ck->wakeup_mask = 0;
#endif
	/* clear MIPS CPU interrupt */
	ck->critical_ctrl = 0;

	/* clear interrupt re-vector */
	power_up_intr = NULL;

	DBG_PUTC( 'U');
	ddr_go_active();
	DBG_PUTC( 'D');

	bsp_active();

	DBG_PUTC( 'B');
	sundry_active();

	power_up();
}

/*
 * put memory into a low power state, then idle the cpu via a wait instruction.
 * when an interrupt occurs, it will be handled on the current stack, which is this stack.
 *
 * This function locks all the necessary code into cache.
 * This should be table driven.
 */
void
power_down(void)
{
	unsigned long sp, gp;
	unsigned long flags;
	volatile clk_pm_wakeup_t	*ck;
	volatile art_t			*art;
	volatile wakeup_intr2_t		*wake;
	unsigned int cpu_status;

//	start_watchdog();

	if (power.event.mask == 0) {
		printk(KERN_ERR "%s: attempt to suspend with no events enabled\n", __func__);
		return;
	}

	printk(KERN_DEBUG "%s power.event.mask 0x%08x\n", __func__, power.event.mask);

        __asm__ __volatile__ ("sw $29, %0" : : "m" (sp));
	power.sp = sp;

        __asm__ __volatile__ ("sw $28, %0" : : "m" (gp));
	power.gp = gp;

	ck   = (volatile clk_pm_wakeup_t *)REG_TO_K1(BCHP_CLK_WAKEUP_STATUS);
	art  = (volatile art_t *)REG_TO_K1(BCHP_ART_REVISION);
	wake = (volatile wakeup_intr2_t *)REG_TO_K1(BCHP_WAKEUP_CTRL2_STATUS);

#if 0
	printk(KERN_ERR "%s: wake_control2 status 0x%08x, mask 0x%08x\n",
		__FUNCTION__, wake->status, wake->mask_status);

	printk(KERN_ERR "%s: clk_wakeup status 0x%08x enable mask 0x%08x\n",
		__FUNCTION__, ck->wakeup_status, ck->wakeup_mask);

#endif 

	preempt_disable();

	local_save_flags(flags);
	local_irq_disable();

	enet_standby(power_wol);

	/*
	 * disable all interrupts in the HIF block 
	 */
	power.w0_mask = CPUINT1C->IntrW0MaskStatus;
	power.w1_mask = CPUINT1C->IntrW1MaskStatus;

	CPUINT1C->IntrW0MaskSet = 0xffffffff;
	CPUINT1C->IntrW1MaskSet = 0xffffffff;

	flush_cache_all();

	hif_standby();
	/*
	 * disable cpu interrupts except ART on interrupt  2
	 */
	cpu_status  = read_c0_status();

	/* disable all interrupts, including timers */
	cpu_status  &= ~ST0_IM;	

	/* enable intr 2, for ART wakeup */
	cpu_status  |= STATUSF_IP2;		
	write_c0_status( cpu_status);

	cache_lock_all();

	/*
	 * Mask all wakeup events  from the ART.
	 * clear any pending wakeup events from the ART.
	 */
	wake->mask_clear = 0xffffffff;
	wake->mask_set = 0xff;
	wake->clear = 0xffffffff;

	/*
	 * clear clock  power management wakeup mask
	 */
	ck->wakeup_mask = 0;


	/* 
	 * enable wakeups from sundry block 
	 */


	if (power.event.mask & PWR_IRR)
		wake->mask_clear = WAKE2_IRR;

	if (power.event.mask & PWR_UART_0)
		wake->mask_clear = WAKE2_UART0;
		
	if (power.event.mask & PWR_UART_1)
		wake->mask_clear = WAKE2_UART1;
		
	if (power.event.mask & PWR_UART_2)
		wake->mask_clear = WAKE2_UART2;

	if (power.event.mask &  PWR_GPIO)
		wake->mask_clear = WAKE2_GIO;

	
	if (power.event.mask & PWR_TIMER) {
		register unsigned int	timeout;
		volatile wake_timer_t	*wktmr;

		wktmr = (volatile wake_timer_t *)REG_TO_K1(BCHP_WKTMR_REG_START);

		timeout = power.event.timeout;
		/* avoid instant wakeups */
		if (timeout < 2)
			timeout = 2;

		wktmr->alarm = wktmr->counter + timeout;

		wake->mask_clear = WAKE2_TIMER;
	}


	/*
	 * set clock power management wakeup mask
	 */
	if (power.event.mask & PWR_SUNDRY_MASK)
	    ck->wakeup_mask |= BCHP_CLK_WAKEUP_MASK_SUNDRY_MASK;

	if (power.event.mask & PWR_ENET)
	    ck->wakeup_mask |= BCHP_CLK_WAKEUP_MASK_ENET_MASK;

	if (power.event.mask & PWR_HDMI)
	    ck->wakeup_mask |= BCHP_CLK_WAKEUP_MASK_HDMI_MASK;

	power.state = Standby;

	__power_down();

	power.state = Dark;

	local_irq_restore(flags);
	printk("%s %s: back from __power_down(), %s events 0x%08x\n",
		power.early? KERN_ERR : KERN_DEBUG ,
		__func__,
		power.early? "EARLY": "" ,
		power.event.events);
	preempt_enable();

}

/*
 * Finish the power up sequence.
 *    This is called in interrupt context
 * Release all the locked regions of cache
 */

void
power_up(void)
{
	unsigned int cpu_status;

	power.state = Dark;

	check_wakeup_events(0);
	clear_wakeup_events();

//	stop_watchdog();

	cache_unlock_all();

	sundry_active();
	hif_active();
	enet_active(power_wol); 

	/*
	 * restore interrupt masks in the HIF block
	 */
	CPUINT1C->IntrW0MaskClear = ~power.w0_mask;
	CPUINT1C->IntrW1MaskClear = ~power.w1_mask;

	/*
	 * restore cpu interrupts except clock and counters
	 */
	cpu_status  = read_c0_status();
	cpu_status  &= ~ST0_IM;				
	cpu_status  |= (power.cpu_mask & ~(STATUSF_IP6 | STATUSF_IP7));
	write_c0_status( cpu_status);

//	art_dump_regs();
}


/*
struct platform_suspend_ops {
        int (*valid)(suspend_state_t state);
        int (*begin)(suspend_state_t state);
        int (*prepare)(void);
        int (*enter)(suspend_state_t state);
        void (*finish)(void);
        void (*end)(void);
        void (*recover)(void);
};

*/


static int
power_pm_valid(suspend_state_t state)
{
	printk(KERN_DEBUG "%s: enter\n", __func__);
	return state == PM_SUSPEND_STANDBY || state == PM_SUSPEND_MEM;
}

static int
power_pm_begin(suspend_state_t state)
{
	printk(KERN_DEBUG "%s: enter\n", __func__);
	switch (state) {
	    case PM_SUSPEND_STANDBY:
	    printk(KERN_DEBUG "%s: PM_SUSPEND_STANDBY\n", __func__);
		return 0;

	    case PM_SUSPEND_MEM:
	    printk(KERN_DEBUG "%s: PM_SUSPEND_MEM\n", __func__);
		return 0;

	    default:
		return -EINVAL;
	}

}

static int
power_pm_prepare(void)
{
	printk(KERN_DEBUG "%s: enter\n", __func__);
	return(0);

}

static int
power_pm_enter(suspend_state_t state)
{
	printk(KERN_DEBUG "%s: enter\n", __func__);
	power_down();
	return(0);

}

static void
power_pm_finish(void)
{
	printk(KERN_DEBUG "%s: enter\n", __func__);
}

static void
power_pm_end(void)
{
	printk(KERN_DEBUG "%s: enter\n", __func__);

}

static void
power_pm_recover(void)
{
	printk(KERN_DEBUG "%s: enter\n", __func__);

}

static struct platform_suspend_ops power_pm_ops = {
	.valid		= power_pm_valid,
        .begin		= power_pm_begin,
	.prepare	= power_pm_prepare,
	.enter		= power_pm_enter,
	.finish		= power_pm_finish,
	.end		= power_pm_end,
	.recover	= power_pm_recover,
};



/*
 * disable clock and counter interrupts;
 */
static int timeclock_suspend(struct sys_device *dev, pm_message_t state)
{
	unsigned int cpu_status;
	volatile wake_timer_t		*wktmr;

	cpu_status  = read_c0_status();
	power.cpu_mask = cpu_status & ST0_IM;

	cpu_status  &= ~(STATUSF_IP6 | STATUSF_IP7);

	cpu_status  |= cpu_status;		
	write_c0_status( cpu_status);

	/*
	 * save wakeup counter 
	 */
	wktmr = (volatile wake_timer_t *)REG_TO_K1(BCHP_WKTMR_REG_START);

	power.wake_counter =  wktmr->counter;

	printk(KERN_DEBUG "%s: Clock & Counter interrupts disabled\n", __func__);
	return 0;
}

/*
 * re-enable clock and counter interrupts;
 */
static int timeclock_resume(struct sys_device *dev)
{
	unsigned int cpu_status;
	struct timespec			ts;
	volatile wake_timer_t		*wktmr;

	wktmr = (volatile wake_timer_t *)REG_TO_K1(BCHP_WKTMR_REG_START);

	/*
	 * update clock
	 * Don't really have to worry about the wake timer counter wrapping
	 */
	getnstimeofday(&ts);
	ts.tv_sec += wktmr->counter - power.wake_counter;
	do_settimeofday(&ts);

	cpu_status  = read_c0_status();

	cpu_status |= power.cpu_mask & (STATUSF_IP6 | STATUSF_IP7);

	write_c0_status( cpu_status);
	printk(KERN_DEBUG "%s: Clock & Counter interrupts re-enabled\n", __func__);
	return 0;
}

/* sysfs resume/suspend bits for timekeeping */
static struct sysdev_class timeclock_sysclass = {
        .name           = "timeclock",
        .resume         = timeclock_resume,
        .suspend        = timeclock_suspend,
};

static struct sys_device device_timeclock = {
        .id             = 0,
        .cls            = &timeclock_sysclass,
};

static int __init timeclock_init_device(void)
{
        int error = sysdev_class_register(&timeclock_sysclass);
        if (!error)
                error = sysdev_register(&device_timeclock);
        return error;
}

/*
 * Registration has to be later than the 'timekeeping' registration
 * in kernel/time/timekeeping.c
 * During suspend, the clock interrupts have to be turned off before timekeeping
 * is suspended, and turned back on _after_ timekeeping is resumed.
 */
late_initcall(timeclock_init_device);


int print_names;

static int
power_open(struct inode *inode, struct file *file)
{
	/* enforce exclusive open */

	if (atomic_inc_return(&power.ref_cnt) > 1) {
		atomic_dec(&power.ref_cnt);
		return -EBUSY;
	}

	print_names = 1;

	return(0);
}

static int
power_release(struct inode *inode, struct file *file)
{
	/* release exclusive open */
	atomic_dec(&power.ref_cnt);
	return(0);
}

extern  int power_suspend_devices( suspend_state_t state);
extern  void power_resume_devices( void);

extern  int suspend_prepare( void);
extern  int suspend_enter( suspend_state_t state);
extern  void suspend_finish( void);

static int usb_pm = 1;

static int __init usb_pm_disable(char *p)
{
	usb_pm = 0;
	return 0;
}
early_param("no_usb_pm", usb_pm_disable);

inline void
usb_stop(void)
{
#ifdef CONFIG_USB_EHCI_HCD
	    ehci_hcd_cleanup();
#endif /* CONFIG_USB_EHCI_HCD */
#ifdef CONFIG_USB_OHCI_HCD
	    ohci_hcd_mod_exit();
#endif /* CONFIG_USB_OHCI_HCD */

}

inline void
usb_restart(void)
{
#ifdef CONFIG_USB_EHCI_HCD
	ehci_hcd_init();
#endif /* CONFIG_USB_EHCI_HCD */
#ifdef CONFIG_USB_OHCI_HCD
	ohci_hcd_mod_init();
#endif /* CONFIG_USB_OHCI_HCD */
}

void power_standby(void)
{
	struct pid *pgrp;
	struct task_struct *p;

	if (power.state != Active) 
	    return;


	/*
	 * make all of the threads for this process unfreezable
	 */
	pgrp = task_pgrp(current);

	do_each_pid_thread(pgrp, PIDTYPE_PGID, p) {
		p->flags |= PF_NOFREEZE;
		if (print_names)
			printk(KERN_DEBUG "%s: NOFREEZE %s\n", __func__, p->comm);

	} while_each_pid_thread(pgrp, PIDTYPE_PGID, p);

	print_names = 0;


	printk(KERN_INFO "%s: Syncing filesystems ... ", __func__);
	sys_sync();
	printk("done.\n");

	if (usb_pm)
		usb_stop();

	if (power.event.mask & PWR_ENET)
		power_wol = 1;
	else
		power_wol = 0;

	suspend_prepare();
	power_suspend_devices(PM_SUSPEND_MEM);
	all_standby();
	power.state = Dark;

}

static int
power_ioctl(struct inode *inode,
	    struct file *file,
	    unsigned int cmd,
	    unsigned long arg)
{

	struct power_event event, response;
	int	copy_back_response;

	copy_back_response = 0;

	switch (cmd) {
	    case PW_MGMT_MSG:
	       if (copy_from_user(&event, (char __user *)arg, sizeof(struct power_event)))
		    return -EFAULT;
		break;

	    default:
		return -EINVAL;
		break;
	}

	power.event.type = event.type;

	switch (event.type) {

	    case GO_ACTIVE:
		if (power.state == Dark) {
			all_active();
			power_resume_devices();
			suspend_finish();
			if (usb_pm)
			    usb_restart();
		}
		power.state = Active;
		break;

	    case GO_STANDBY:
		if (power.event.mask == 0)
			return -EINVAL;

		if ((power.event.mask & PWR_TIMER) && (power.event.timeout == 0)) {
			printk(KERN_ERR "%s: TIMER set but timeout is zero \n", __func__);
			return -EINVAL;
		}

		power_standby();

		/* wait for printks to drain */
		schedule_timeout_interruptible(100);

		suspend_enter(PM_SUSPEND_MEM);

		copy_back_response = 1;
		break;

	    case GO_DARK:
		power_standby();
		copy_back_response = 1;
		break;

	    case GO_SHUTDOWN:
		power_standby();
		/* reboot */
		break;

	    case SET_MASK:
		power.event.mask = event.mask & PWR_EVENT_MASK;
		power.event.timeout = event.timeout;
		printk(KERN_DEBUG "%s: SET_MASK user mask 0x%08x power.event.mask 0x%08x timeout 0x%d\n",
			__func__, event.mask, power.event.mask, power.event.timeout);
		copy_back_response = 1;
		break;

	    case GET_MASK:
		copy_back_response = 1;
		break;

	    default:
		break;
	}

	if (copy_back_response) {
	    response = power.event;
	    if (copy_to_user( (char __user *)arg, &response, sizeof(struct power_event)))
		return -EFAULT;
	}

	return(0);
}

static const struct file_operations power_fops = {
        .ioctl   =       power_ioctl,
        .open    =       power_open,
        .release =       power_release,
        .owner   =       THIS_MODULE,
};


#define POWER_MAJOR (DDV_MAJOR  + 1)	/* 40 */
#define MAX_POWER_MINORS	1

static int __init
power_init(void)
{
	struct proc_dir_entry *ep;
	int ret;


	printk(KERN_INFO "%s power_debug %d\n", __FUNCTION__, power_debug);

	ret = register_chrdev( PWR_MGMT_MAJOR, "power", &power_fops);
	if (ret < 0) {
	    printk(KERN_ERR" %s: can not register character device: %d\n",
		__func__, ret);
		return ret;
	}

	power.dev_major = ret;
	
	printk(KERN_INFO "%s: major device number %d\n", __func__, power.dev_major);


	/* create /proc/power */
	ep = create_proc_entry("power", 0600, NULL);
	if (ep) {
		ep->nlink = 1;
		ep->data = 0;
		ep->read_proc = power_read_proc;
		ep->write_proc = power_write_proc;
	} else 
		/* this error isn't fatal */
		printk(KERN_INFO "%s: failed to create /proc/power\n", __func__);


	art_init();

	/*
	 * set default power event mask
	 */
	
	power.event.mask = PWR_UART_0;
	power.state = Active;
	atomic_set(&power.ref_cnt, 0);

	suspend_set_ops(&power_pm_ops);

	return(0);
}

static void __exit
power_exit(void)
{
	printk(KERN_DEBUG "%s\n", __FUNCTION__);
	remove_proc_entry("power", NULL);

	if (power.dev_major >= 0) 
		unregister_chrdev(power.dev_major, "power");
}

module_init(power_init);
module_exit(power_exit);
MODULE_LICENSE("GPL");
