/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 * Copyright (C) 1997, 1998, 1999, 2000 Ralf Baechle ralf@gnu.org
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 2002 MIPS Technologies, Inc.  All rights reserved.
 */
#include <linux/autoconf.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/mmzone.h>

#include <asm/cpu.h>
#include <asm/bootinfo.h>
#include <asm/mmu_context.h>
#include <asm/pgtable.h>
#include <asm/system.h>
#include <asm/tlbdebug.h>

extern void build_tlb_refill_handler(void);

/*
 * Make sure all entries differ.  If they're not different
 * MIPS32 will take revenge ...
 */
#define UNIQUE_ENTRYHI(idx) (CKSEG0 + ((idx) << (PAGE_SHIFT + 1)))

/* Atomicity and interruptability */
#ifdef CONFIG_MIPS_MT_SMTC

#include <asm/smtc.h>
#include <asm/mipsmtregs.h>

#define ENTER_CRITICAL(flags) \
	{ \
	unsigned int mvpflags; \
	local_irq_save(flags);\
	mvpflags = dvpe()
#define EXIT_CRITICAL(flags) \
	evpe(mvpflags); \
	local_irq_restore(flags); \
	}
#else

#define ENTER_CRITICAL(flags) local_irq_save(flags)
#define EXIT_CRITICAL(flags) local_irq_restore(flags)

#endif /* CONFIG_MIPS_MT_SMTC */


#ifdef DPRINTK
#undef DPRINTK
#endif
#if 0
#define DPRINTK printk
#else
#define DPRINTK(...) do { } while(0)
#endif



#if defined( CONFIG_MIPS_BCM_NDVD ) || defined (CONFIG_CPU_SB1)
#if defined(CONFIG_MIPS_BCM7440B0)
#include <asm/brcmstb/brcm97440b0/bchp_pci_cfg.h>
#include <asm/brcmstb/brcm97440b0/boardmap.h>

#elif defined(CONFIG_MIPS_BCM7405A0)
#include <asm/brcmstb/brcm97405a0/bchp_pci_cfg.h>
#include <asm/brcmstb/brcm97405a0/boardmap.h>

#elif defined(CONFIG_MIPS_BCM7405B0)
#include <asm/brcmstb/brcm97405b0/bchp_pci_cfg.h>
#include <asm/brcmstb/brcm97405b0/boardmap.h>

#elif defined(CONFIG_MIPS_BCM7601)
#include <asm/brcmstb/brcm97601/bchp_pci_cfg.h>
#include <asm/brcmstb/brcm97601/boardmap.h>

#elif defined(CONFIG_MIPS_BCM7630)
#include <asm/brcmstb/brcm97630/boardmap.h>

#elif defined(CONFIG_MIPS_BCM7635)
#include <asm/brcmstb/brcm97635/bchp_pci_cfg.h>
#include <asm/brcmstb/brcm97635/boardmap.h>

#elif defined (CONFIG_CPU_SB1)

#else
#error "Unsupported Chip revision"
#endif


#define PG_4K		0x00001000
#define PG_16K		0x00004000
#define PG_64K		0x00010000
#define PG_256K		0x00040000
#define PG_1M		0x00100000
#define PG_4M		0x00400000
#define PG_16M		0x01000000
#define PG_64M		0x04000000
#define PG_256M		0x10000000


#define UNCACHED	0x2
#define CACHED		0x3



struct tlb_entry {
	unsigned long	paddr;			/* physical address */
	unsigned long	vaddr;			/* virtual adderss */
	unsigned long	pg_size;
	unsigned int	cache_mode;
	unsigned long	num_entries;
	char *name;				/* region name */
};

struct tlb_entry initial_tlb[] = {

#if defined (CONFIG_MIPS_BRCM76XX)

#ifdef CONFIG_PCI
/*	virt				phys		size	region
 *   0xf0000000 - 0xf1000000	0xf0000000 - 0xf1000000	32	PCI IO Space
 *   0xd0000000 - 0xd8000000	0xd0000000 - 0xd8000000 128	PCI Mem Window
 */

    {
	paddr:		CPU2PCI_CPU_PHYS_IO_WIN_BASE, 
	vaddr: 		PCI_IO_WIN_BASE,
	pg_size:	PG_16M,
	cache_mode:	UNCACHED,
	num_entries:	1,
	name:		 "PCI IO Space"
    },

    {
	paddr:		CPU2PCI_CPU_PHYS_MEM_WIN_BASE,
	vaddr:		PCI_MEM_WIN_BASE,
	pg_size:	PG_64M,
	cache_mode:	UNCACHED,
	num_entries:	1,
	name:		 "PCI Mem Window"
    },

#endif

#elif defined (CONFIG_MIPS_BCM7405)

/*	virt				phys		size	region
 *   0xf0000000 - 0xf060000b	0xf0000000 - 0xf060000b	6	PCI I/O
 *   0xd0000000 - 0xdfffffff	0xd0000000 - 0xdfffffff	256	PCI Mem Window
 *   0xe0000000 - 0xefffffff	0x20000000 - 0x2fffffff	256	Memory
 */

    {
	paddr:		PCI_IO_WIN_BASE, 
	vaddr: 		PCI_IO_WIN_BASE,
	pg_size:	PG_4M,
	cache_mode:	UNCACHED,
	num_entries:	1,
	name:		 "PCI IO Space"
    },

    {
	paddr:		CPU2PCI_CPU_PHYS_MEM_WIN_BASE,
	vaddr:		PCI_MEM_WIN_BASE,
	pg_size:	PG_64M,
	cache_mode:	UNCACHED,
	num_entries:	2,
	name:		 "PCI Mem Window"
    },


#elif defined (CONFIG_CPU_SB1)
	/* nothing needed */
#else
#error __FILE__ "unsupported platform"
#endif

	/* end of table */
	{0, 0, 0, 0, 0, 0}
};


/*
 * just convert the page size to the cpu page mask
 */
unsigned long
pg_size_to_cpu_mask(unsigned long page_size)
{
	unsigned long cpu_pg_mask;

	switch(page_size) {
	    case PG_4K:
		cpu_pg_mask = PM_4K;
		break;

	    case PG_16K:
		cpu_pg_mask = PM_16K;
		break;

	    case PG_64K:
		cpu_pg_mask = PM_64K;
		break;

	    case PG_256K:
		cpu_pg_mask = PM_256K;
		break;

	    case PG_1M:
		cpu_pg_mask = PM_1M;
		break;

	    case PG_4M:
		cpu_pg_mask = PM_4M;
		break;

	    case PG_16M:
		cpu_pg_mask = PM_16M;
		break;

	    case PG_64M:
		cpu_pg_mask = PM_64M;
		break;

	    case PG_256M:
		cpu_pg_mask = PM_256M;
		break;
	
	    default:
		cpu_pg_mask = 0;
		BUG();
		break;
	}
	
	return(cpu_pg_mask);
}

#define PADDR_TO_PFN(x) ((x) >> (6) & 0x3fffffc0)
#define PAGE_DIRTY	0x4
#define PAGE_VALID	0x2
#define PAGE_GLOBAL	0x1

void
write_tlb_entry( struct tlb_entry *tp, int	index )
{

	    unsigned long  cpu_pg_mask;
	    unsigned long pfn0, pfn1, pfn_increment;
	    unsigned long vpn, lo0, lo1;
	    int i;

	    cpu_pg_mask = pg_size_to_cpu_mask(tp->pg_size);
	    write_c0_pagemask(cpu_pg_mask);
	    mtc0_tlbw_hazard();

//	    printk("vaddr 0x%08lx paddr 0x%08lx pg_size 0x%08lx  cache 0x%x entries %ld name %s\n",
//		tp->vaddr, tp->paddr, tp->pg_size, tp->cache_mode, tp->num_entries, tp->name);

	    /*
	     * try to catch page alignment errors in the table entry
	     */
	    if (tp->vaddr & (tp->pg_size -1)) {
		printk(KERN_ERR "%s: %s: address 0x%08lx not on page boundary 0x%08lx\n",
			__FUNCTION__, tp->name, tp->vaddr, tp->pg_size);
		BUG();
	    }

	    if (tp->paddr & (tp->pg_size -1)) {
		printk(KERN_ERR "%s: %s: address 0x%08lx not on page boundary 0x%08lx\n",
			__FUNCTION__, tp->name, tp->paddr, tp->pg_size);
		BUG();
	    }

	    vpn = tp->vaddr;
	
	    pfn0 = PADDR_TO_PFN(tp->paddr);
	    pfn1 = PADDR_TO_PFN(tp->paddr + tp->pg_size);
	    pfn_increment = (PADDR_TO_PFN(tp->pg_size << 1));		/* two pages at a time */

	
	    lo0 = pfn0 | (tp->cache_mode << 3) | PAGE_DIRTY | PAGE_VALID | PAGE_GLOBAL;
	    lo1 = pfn1 | (tp->cache_mode << 3) | PAGE_DIRTY | PAGE_VALID | PAGE_GLOBAL;

	

	    for (i = 0; i < tp->num_entries; i++ ) {
		printk(KERN_INFO "%s: vpn=%08lx paddr 0x%08lx 0x%08lx page_size 0x%lx\n",
			tp->name, vpn, (lo0 & ~0x1f) << 6, (lo1 & ~0x1f) << 6, tp->pg_size);
		printk(KERN_INFO "    lo0 0x%08lx, lo1 0x%08lx, index %d\n",
			lo0, lo1, index);

		write_c0_entrylo0(lo0);
		write_c0_entrylo1(lo1);
		mtc0_tlbw_hazard();

		write_c0_entryhi(vpn);
		write_c0_index(index);
		mtc0_tlbw_hazard();

		tlb_write_indexed();
		mtc0_tlbw_hazard();

		vpn += tp->pg_size << 1;
		lo0 += pfn_increment;
		lo1 += pfn_increment;
		index++;
	    }
}

/*
 * initialize 'wired' tlb entries.
 * Some of these may be needed for memory init code on platforms
 * with memory not addressable in KSEG 0/1.
 *
 * Interrupts should be disabled.
 * The VM system is not yet running.
 * The page size isn't even set yet.
 * Assume no global configuration variables are initialized
 */

void
early_tlb_init(void)
{
	struct tlb_entry *tp;
	unsigned long  index, tlb_size;
	unsigned long  config;

	/*
         * check that the TLB exists 
	 */
	config = read_c0_config();
	if (((config >> 7) & 3) != 1)
		panic("No TLB present");

	config = read_c0_config1();
	tlb_size = ((config >> 25) & 0x3f) + 1;

	printk(KERN_INFO "%s: tlb size %ld\n", __FUNCTION__, tlb_size);

	/* 
	 * initialize entire TLB to uniqe virtual addresses
	 * but with the PAGE_VALID bit not set
	 */
	write_c0_wired(0);
	write_c0_pagemask(PM_DEFAULT_MASK);
	write_c0_framemask(0);

	write_c0_entrylo0(0);	/* not _PAGE_VALID */
	write_c0_entrylo1(0);

	for (index = 0; index < tlb_size; index++) {
		/* Make sure all entries differ. */
		write_c0_entryhi(UNIQUE_ENTRYHI(index+32));
		write_c0_index(index);
		mtc0_tlbw_hazard();
		tlb_write_indexed();
	}

	tlbw_use_hazard();
	
	//dump_tlb(0, tlb_size -1 );

	/*
         * write wired entries according to the table above
	 */

	index = 0;

	for (tp = initial_tlb; tp->vaddr; tp++) {
	    write_tlb_entry(tp, index);
	    index += tp->num_entries;
	}

	write_c0_wired(index);

	//dump_tlb_wired();
}




#endif
#if defined(CONFIG_CPU_LOONGSON2)
/*
 * LOONGSON2 has a 4 entry itlb which is a subset of dtlb,
 * unfortrunately, itlb is not totally transparent to software.
 */
#define FLUSH_ITLB write_c0_diag(4);

#define FLUSH_ITLB_VM(vma) { if ((vma)->vm_flags & VM_EXEC)  write_c0_diag(4); }

#else

#define FLUSH_ITLB
#define FLUSH_ITLB_VM(vma)

#endif

void local_flush_tlb_all(void)
{
	unsigned long flags;
	unsigned long old_ctx;
	int entry;

	ENTER_CRITICAL(flags);
	/* Save old context and create impossible VPN2 value */
	old_ctx = read_c0_entryhi();
	write_c0_entrylo0(0);
	write_c0_entrylo1(0);

	entry = read_c0_wired();

	/* Blast 'em all away. */
	while (entry < current_cpu_data.tlbsize) {
		/* Make sure all entries differ. */
		write_c0_entryhi(UNIQUE_ENTRYHI(entry));
		write_c0_index(entry);
		mtc0_tlbw_hazard();
		tlb_write_indexed();
		entry++;
	}
	tlbw_use_hazard();
	write_c0_entryhi(old_ctx);
	FLUSH_ITLB;
	EXIT_CRITICAL(flags);
}

/* All entries common to a mm share an asid.  To effectively flush
   these entries, we just bump the asid. */
void local_flush_tlb_mm(struct mm_struct *mm)
{
	int cpu;

	preempt_disable();

	cpu = smp_processor_id();

	if (cpu_context(cpu, mm) != 0) {
		drop_mmu_context(mm, cpu);
	}

	preempt_enable();
}

void local_flush_tlb_range(struct vm_area_struct *vma, unsigned long start,
	unsigned long end)
{
	struct mm_struct *mm = vma->vm_mm;
	int cpu = smp_processor_id();

	if (cpu_context(cpu, mm) != 0) {
		unsigned long flags;
		int size;

		ENTER_CRITICAL(flags);
		size = (end - start + (PAGE_SIZE - 1)) >> PAGE_SHIFT;
		size = (size + 1) >> 1;
		if (size <= current_cpu_data.tlbsize/2) {
			int oldpid = read_c0_entryhi();
			int newpid = cpu_asid(cpu, mm);

			start &= (PAGE_MASK << 1);
			end += ((PAGE_SIZE << 1) - 1);
			end &= (PAGE_MASK << 1);
			while (start < end) {
				int idx;

				write_c0_entryhi(start | newpid);
				start += (PAGE_SIZE << 1);
				mtc0_tlbw_hazard();
				tlb_probe();
				tlb_probe_hazard();
				idx = read_c0_index();
				write_c0_entrylo0(0);
				write_c0_entrylo1(0);
				if (idx < 0)
					continue;
				/* Make sure all entries differ. */
				write_c0_entryhi(UNIQUE_ENTRYHI(idx));
				mtc0_tlbw_hazard();
				tlb_write_indexed();
			}
			tlbw_use_hazard();
			write_c0_entryhi(oldpid);
		} else {
			drop_mmu_context(mm, cpu);
		}
		FLUSH_ITLB;
		EXIT_CRITICAL(flags);
	}
}

void local_flush_tlb_kernel_range(unsigned long start, unsigned long end)
{
	unsigned long flags;
	int size;

	ENTER_CRITICAL(flags);
	size = (end - start + (PAGE_SIZE - 1)) >> PAGE_SHIFT;
	size = (size + 1) >> 1;
	if (size <= current_cpu_data.tlbsize / 2) {
		int pid = read_c0_entryhi();

		start &= (PAGE_MASK << 1);
		end += ((PAGE_SIZE << 1) - 1);
		end &= (PAGE_MASK << 1);

		while (start < end) {
			int idx;

			write_c0_entryhi(start);
			start += (PAGE_SIZE << 1);
			mtc0_tlbw_hazard();
			tlb_probe();
			tlb_probe_hazard();
			idx = read_c0_index();
			write_c0_entrylo0(0);
			write_c0_entrylo1(0);
			if (idx < 0)
				continue;
			/* Make sure all entries differ. */
			write_c0_entryhi(UNIQUE_ENTRYHI(idx));
			mtc0_tlbw_hazard();
			tlb_write_indexed();
		}
		tlbw_use_hazard();
		write_c0_entryhi(pid);
	} else {
		local_flush_tlb_all();
	}
	FLUSH_ITLB;
	EXIT_CRITICAL(flags);
}

void local_flush_tlb_page(struct vm_area_struct *vma, unsigned long page)
{
	int cpu = smp_processor_id();

	if (cpu_context(cpu, vma->vm_mm) != 0) {
		unsigned long flags;
		int oldpid, newpid, idx;

		newpid = cpu_asid(cpu, vma->vm_mm);
		page &= (PAGE_MASK << 1);
		ENTER_CRITICAL(flags);
		oldpid = read_c0_entryhi();
		write_c0_entryhi(page | newpid);
		mtc0_tlbw_hazard();
		tlb_probe();
		tlb_probe_hazard();
		idx = read_c0_index();
		write_c0_entrylo0(0);
		write_c0_entrylo1(0);
		if (idx < 0)
			goto finish;
		/* Make sure all entries differ. */
		write_c0_entryhi(UNIQUE_ENTRYHI(idx));
		mtc0_tlbw_hazard();
		tlb_write_indexed();
		tlbw_use_hazard();

	finish:
		write_c0_entryhi(oldpid);
		FLUSH_ITLB_VM(vma);
		EXIT_CRITICAL(flags);
	}
}

/*
 * This one is only used for pages with the global bit set so we don't care
 * much about the ASID.
 */
void local_flush_tlb_one(unsigned long page)
{
	unsigned long flags;
	int oldpid, idx;

	ENTER_CRITICAL(flags);
	oldpid = read_c0_entryhi();
	page &= (PAGE_MASK << 1);
	write_c0_entryhi(page);
	mtc0_tlbw_hazard();
	tlb_probe();
	tlb_probe_hazard();
	idx = read_c0_index();
	write_c0_entrylo0(0);
	write_c0_entrylo1(0);
	if (idx >= 0) {
		/* Make sure all entries differ. */
		write_c0_entryhi(UNIQUE_ENTRYHI(idx));
		mtc0_tlbw_hazard();
		tlb_write_indexed();
		tlbw_use_hazard();
	}
	write_c0_entryhi(oldpid);
	FLUSH_ITLB;
	EXIT_CRITICAL(flags);
}

/*
 * We will need multiple versions of update_mmu_cache(), one that just
 * updates the TLB with the new pte(s), and another which also checks
 * for the R4k "end of page" hardware bug and does the needy.
 */
void __update_tlb(struct vm_area_struct * vma, unsigned long address, pte_t pte)
{
	unsigned long flags;
	pgd_t *pgdp;
	pud_t *pudp;
	pmd_t *pmdp;
	pte_t *ptep;
	int idx, pid;

	/*
	 * Handle debugger faulting in for debugee.
	 */
	if (current->active_mm != vma->vm_mm)
		return;

	ENTER_CRITICAL(flags);

	pid = read_c0_entryhi() & ASID_MASK;
	address &= (PAGE_MASK << 1);
	write_c0_entryhi(address | pid);
	pgdp = pgd_offset(vma->vm_mm, address);
	mtc0_tlbw_hazard();
	tlb_probe();
	tlb_probe_hazard();
	pudp = pud_offset(pgdp, address);
	pmdp = pmd_offset(pudp, address);
	idx = read_c0_index();
	ptep = pte_offset_map(pmdp, address);

#if defined(CONFIG_64BIT_PHYS_ADDR) && defined(CONFIG_CPU_MIPS32)
	write_c0_entrylo0(ptep->pte_high);
	ptep++;
	write_c0_entrylo1(ptep->pte_high);
#else
	write_c0_entrylo0(pte_val(*ptep++) >> 6);
	write_c0_entrylo1(pte_val(*ptep) >> 6);
#endif
	mtc0_tlbw_hazard();
	if (idx < 0)
		tlb_write_random();
	else
		tlb_write_indexed();
	tlbw_use_hazard();
	FLUSH_ITLB_VM(vma);
	EXIT_CRITICAL(flags);
}

#if 0
static void r4k_update_mmu_cache_hwbug(struct vm_area_struct * vma,
				       unsigned long address, pte_t pte)
{
	unsigned long flags;
	unsigned int asid;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int idx;

	ENTER_CRITICAL(flags);
	address &= (PAGE_MASK << 1);
	asid = read_c0_entryhi() & ASID_MASK;
	write_c0_entryhi(address | asid);
	pgdp = pgd_offset(vma->vm_mm, address);
	mtc0_tlbw_hazard();
	tlb_probe();
	tlb_probe_hazard();
	pmdp = pmd_offset(pgdp, address);
	idx = read_c0_index();
	ptep = pte_offset_map(pmdp, address);
	write_c0_entrylo0(pte_val(*ptep++) >> 6);
	write_c0_entrylo1(pte_val(*ptep) >> 6);
	mtc0_tlbw_hazard();
	if (idx < 0)
		tlb_write_random();
	else
		tlb_write_indexed();
	tlbw_use_hazard();
	EXIT_CRITICAL(flags);
}
#endif

void __init add_wired_entry(unsigned long entrylo0, unsigned long entrylo1,
	unsigned long entryhi, unsigned long pagemask)
{
	unsigned long flags;
	unsigned long wired;
	unsigned long old_pagemask;
	unsigned long old_ctx;

	ENTER_CRITICAL(flags);
	/* Save old context and create impossible VPN2 value */
	old_ctx = read_c0_entryhi();
	old_pagemask = read_c0_pagemask();
	wired = read_c0_wired();
	write_c0_wired(wired + 1);
	write_c0_index(wired);
	tlbw_use_hazard();	/* What is the hazard here? */
	write_c0_pagemask(pagemask);
	write_c0_entryhi(entryhi);
	write_c0_entrylo0(entrylo0);
	write_c0_entrylo1(entrylo1);
	mtc0_tlbw_hazard();
	tlb_write_indexed();
	tlbw_use_hazard();

	write_c0_entryhi(old_ctx);
	tlbw_use_hazard();	/* What is the hazard here? */
	write_c0_pagemask(old_pagemask);
	local_flush_tlb_all();
	EXIT_CRITICAL(flags);
}

/*
 * Used for loading TLB entries before trap_init() has started, when we
 * don't actually want to add a wired entry which remains throughout the
 * lifetime of the system
 */

static int temp_tlb_entry __cpuinitdata;

__init int add_temporary_entry(unsigned long entrylo0, unsigned long entrylo1,
			       unsigned long entryhi, unsigned long pagemask)
{
	int ret = 0;
	unsigned long flags;
	unsigned long wired;
	unsigned long old_pagemask;
	unsigned long old_ctx;

	ENTER_CRITICAL(flags);
	/* Save old context and create impossible VPN2 value */
	old_ctx = read_c0_entryhi();
	old_pagemask = read_c0_pagemask();
	wired = read_c0_wired();
	if (--temp_tlb_entry < wired) {
		printk(KERN_WARNING
		       "No TLB space left for add_temporary_entry\n");
		ret = -ENOSPC;
		goto out;
	}

	write_c0_index(temp_tlb_entry);
	write_c0_pagemask(pagemask);
	write_c0_entryhi(entryhi);
	write_c0_entrylo0(entrylo0);
	write_c0_entrylo1(entrylo1);
	mtc0_tlbw_hazard();
	tlb_write_indexed();
	tlbw_use_hazard();

	write_c0_entryhi(old_ctx);
	write_c0_pagemask(old_pagemask);
out:
	EXIT_CRITICAL(flags);
	return ret;
}

static void __cpuinit probe_tlb(unsigned long config)
{
	struct cpuinfo_mips *c = &current_cpu_data;
	unsigned int reg;

	/*
	 * If this isn't a MIPS32 / MIPS64 compliant CPU.  Config 1 register
	 * is not supported, we assume R4k style.  Cpu probing already figured
	 * out the number of tlb entries.
	 */
	if ((c->processor_id & 0xff0000) == PRID_COMP_LEGACY)
		return;
#ifdef CONFIG_MIPS_MT_SMTC
	/*
	 * If TLB is shared in SMTC system, total size already
	 * has been calculated and written into cpu_data tlbsize
	 */
	if((smtc_status & SMTC_TLB_SHARED) == SMTC_TLB_SHARED)
		return;
#endif /* CONFIG_MIPS_MT_SMTC */

	reg = read_c0_config1();
	if (!((config >> 7) & 3))
		panic("No TLB present");

	c->tlbsize = ((reg >> 25) & 0x3f) + 1;
}

static int __cpuinitdata ntlb = 0;
static int __init set_ntlb(char *str)
{
	get_option(&str, &ntlb);
	return 1;
}

__setup("ntlb=", set_ntlb);

void __cpuinit tlb_init(void)
{
	unsigned int config = read_c0_config();

	/*
	 * You should never change this register:
	 *   - On R4600 1.7 the tlbp never hits for pages smaller than
	 *     the value in the c0_pagemask register.
	 *   - The entire mm handling assumes the c0_pagemask register to
	 *     be set to fixed-size pages.
	 */
	probe_tlb(config);
	write_c0_pagemask(PM_DEFAULT_MASK);
	temp_tlb_entry = current_cpu_data.tlbsize - 1;

        /* From this point on the ARC firmware is dead.  */
	/* Did I tell you that ARC SUCKS?  */

	if (ntlb) {
		if (ntlb > 1 && ntlb <= current_cpu_data.tlbsize) {
			int wired = current_cpu_data.tlbsize - ntlb;
			write_c0_wired(wired);
			write_c0_index(wired-1);
			printk("Restricting TLB to %d entries\n", ntlb);
		} else
			printk("Ignoring invalid argument ntlb=%d\n", ntlb);
	}
	build_tlb_refill_handler();
}

