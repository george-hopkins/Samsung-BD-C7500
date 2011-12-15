/*
 * Copyright (C) 2001-2009 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed with the hope that it will be useful, but without
 * any warranty; without even the implied warranty of merchantability or
 * fitness for a particular purpose.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#include <linux/config.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/smp.h>

#include <asm/io.h>


#include <asm/addrspace.h>


int debug = 0;

#ifdef module_param_named
module_param_named(debug, debug, int, 0444);
#else
MODULE_PARM (debug, "i");
#endif
MODULE_PARM_DESC (debug, "enable debug");

static char *videotype;

#ifdef module_param_named
module_param_named(videotype, videotype, charp , 0444);
#else
MODULE_PARM (videotype, "string");
#endif
MODULE_PARM_DESC (videotype, "videotype string");



static struct proc_dir_entry *hwdebug_root, *pc_root;
#define NR_COUNTERS	2
/*
 * The performance counters are internal to the MIPS cpu core and are accessed
 * by coprocessor 0 instructions.
 * In order to read or write the values on a non-boot cpu, a message is sent to that cpu.
 * it saves the results in memory, which must be cache line aligned to avoid trashing the values
 * on non-coherent cache systems (7440b0, 7440c0, 7443, ...)
 */

struct register_values {
	unsigned long 	control_read;
	unsigned long 	counter_read;
	unsigned long 	control_write;
	unsigned long	counter_write;
};

struct register_values pc_values[NR_CPUS][NR_COUNTERS] __cacheline_aligned;

/*
 * the /proc entries for the performance counters
 */
struct perf_counter_entry {
	struct proc_dir_entry	*control;
	struct proc_dir_entry	*counter;
};
	

struct cpu_counter_entry {
	struct proc_dir_entry		 *root;
	struct perf_counter_entry	 pc[NR_COUNTERS];
};

struct cpu_counter_entry	cpu_pc[NR_CPUS];

#define read_perf_cntr(select)  \
({					\
	unsigned int __res;		\
	__asm__ __volatile__(		\
	".set\tmips32\n\t"		\
	"mfc0 \t%0, $25, " #select "\n"	\
	: "=r" (__res));		\
					\
	__res;				\
})



void pc_read_register(unsigned int value)
{

	int cpu, reg, counter;
	unsigned int data;
	unsigned int index;

	/* cpu:8 register set:7 control/count: 1 */
	cpu = value >> 8;
	counter = value & 1;
	reg = (value & 0xe) >> 1;

	index = value & 0xf;


	switch (index) {
	    case 0:
		data = read_perf_cntr(0);
		break;

	    case 1:
		data = read_perf_cntr(1);
		break;

	    case 2:
		data = read_perf_cntr(2);
		break;

	    case 3:
		data = read_perf_cntr(3);
		break;
	
	    default:
	    	data = 0xffffffff;
		break;
	}

#if 0
	printk("%s value 0x%08x cpu 0x%x reg 0x%x counter %d data 0x%08x\n",
		__FUNCTION__, value, cpu, reg, counter, data);
#endif

	if (counter) 
		pc_values[cpu][reg].counter_read = data;
	else
		pc_values[cpu][reg].control_read = data;

	dma_cache_wback((unsigned int)pc_values, sizeof(pc_values));


}


int pc_read_proc(	char	*page,
			char	**start,
			off_t	off,
			int	count,
			int	*eof,
			void	*data)
{
	int len  = 0;
	int cpu, reg, counter;
	unsigned int value;
	unsigned int reg_data;
	

	*eof = 1;

	if (off)
		return(0);

	value = (unsigned int) data;
	 
	/* cpu:8 register set:7 control/count: 1 */
	counter = value & 1;
	reg = (value & 0xe) >> 1;
	cpu = value >> 8;

#if 0
	printk("%s page %p  start %p off 0x%08x, count %d\n", 
		__FUNCTION__, page, *start, off, count);

#endif

#if 0
        len += sprintf(page + len, " cpu %d register set %d %s \n",
		cpu, reg, counter?"counter":"control");
#endif

	if (cpu == 0) {
		pc_read_register(value);
	}
#if defined(CONFIG_SMP) || defined(CONFIG_ASMP)
	else {
		dma_cache_wback_inv((unsigned int)pc_values, sizeof(pc_values));
		smp_call_function((void (*)(void *))pc_read_register, (void *)value, 1, 1);
	}
#endif

	if (counter) 
		reg_data = pc_values[cpu][reg].counter_read;
	else
		reg_data = pc_values[cpu][reg].control_read;

        len += sprintf(page + len, " 0x%08x \n", reg_data);


        return len;
}


#define write_perf_cntr(select, value)  \
do {					\
	__asm__ __volatile__(		\
	".set\tmips32\n\t"		\
	"mtc0 \t%0, $25, " #select "\n"	\
	: : "r" ((unsigned int)value)); \
					\
} while (0)



void pc_write_register(unsigned int value)
{

	int cpu, reg, counter;
	unsigned int data;
	unsigned int index;

	/* cpu:8 register set:7 control/count: 1 */
	cpu = value >> 8;
	counter = value & 1;
	reg = (value & 0xe) >> 1;

	index = value & 0xf;

	dma_cache_wback_inv((unsigned int)pc_values, sizeof(pc_values));

	if (counter) 
		data = pc_values[cpu][reg].counter_write;
	else
		data = pc_values[cpu][reg].control_write;


	switch (index) {
	    case 0:
		write_perf_cntr(0, data);
		break;

	    case 1:
		write_perf_cntr(1, data);
		break;

	    case 2:
		write_perf_cntr(2, data);
		break;

	    case 3:
		write_perf_cntr(3, data);
		break;
	
	    default:
		break;
	}

#if 0
	printk("%s value 0x%08x cpu 0x%x reg 0x%x counter %d data 0x%08x\n",
		__FUNCTION__, value, cpu, reg, counter, data);
#endif


}
int pc_write_proc(	struct file *file,
			const char __user *buffer,
			unsigned long count,
			void *data)
{
	int cpu, reg, counter;
	unsigned int value, tmp;

	int full_count, num_args;
	full_count = count;

	value = (unsigned int) data;
	 
	/* cpu:8 register set:7 control/count: 1 */
	counter = value & 1;
	reg = (value & 0xe) >> 1;
	cpu = value >> 8;

//	printk("%s: <%s>\n", __FUNCTION__, buffer);

	num_args = sscanf(buffer,"0x%x\n", &tmp);

	if (num_args == 0)
		num_args = sscanf(buffer,"%d\n", &tmp);

#if 0
	printk("%s value 0x%08x cpu 0x%x reg 0x%x counter %d data 0x%08x\n",
		__FUNCTION__, value, cpu, reg, counter, tmp);
	printk("count: %d len %d\n", full_count, num_args);
#endif

	if (num_args != 1)
		return(full_count);

	if (counter) 
		pc_values[cpu][reg].counter_write = tmp;
	else
		pc_values[cpu][reg].control_write = tmp;

	if (cpu == 0) {
		pc_write_register(value);
	}
#if defined(CONFIG_SMP) || defined(CONFIG_ASMP)
	else {
		dma_cache_wback_inv((unsigned int)pc_values, sizeof(pc_values));
		smp_call_function((void (*)(void *))pc_write_register, (void *)value, 1, 1);
	}
#endif
	return(full_count);
}

int perf_cnt_init(struct proc_dir_entry *root)
{
	int cpu, j;
	char name[16];
	struct proc_dir_entry *ep;

	printk("%s debug %d\n", __FUNCTION__, debug);

	if (videotype)
		printk("%s videotype <%s>\n", __FUNCTION__, videotype);

	memset(cpu_pc, 0, sizeof (cpu_pc));

	pc_root = proc_mkdir("cpu_perf", root);

	if (pc_root == 0)
		return(0);

	for (cpu = 0; cpu < NR_CPUS; cpu++) {
		sprintf(name,"%d", cpu);
		cpu_pc[cpu].root = proc_mkdir(name, pc_root);

		if (cpu_pc[cpu].root == 0)
			return(0);

		for (j = 0; j < NR_COUNTERS; j++) {
			sprintf(name,"control_%d", j);
			ep = create_proc_entry(name, 0600, cpu_pc[cpu].root);
			if (ep) {
				ep->nlink = 1;
				ep->data = (void *)((cpu << 8) | (j << 1) | 0);
				ep->read_proc = pc_read_proc;
				ep->write_proc = pc_write_proc;
				cpu_pc[cpu].pc[j].control = ep;
			}

			sprintf(name,"counter_%d", j);
			ep = create_proc_entry(name, 0600, cpu_pc[cpu].root);
			if (ep) {
				ep->nlink = 1;
				ep->data = (void *)((cpu << 8) | (j << 1) | 1);
				ep->read_proc = pc_read_proc;
				ep->write_proc = pc_write_proc;
				cpu_pc[cpu].pc[j].control = ep;
			}
		}
		 
	}





	return(0);
}

void perf_cnt_exit(struct proc_dir_entry *root)
{
	int cpu, j;
	char name[16];

	if (root == 0)
		return;

	if (pc_root == 0)
		return;


	for (cpu = 0; cpu < NR_CPUS; cpu++) {
		if (cpu_pc[cpu].root == 0)
			continue;

		for (j = 0; j < NR_COUNTERS; j++) {
			sprintf(name,"control_%d", j);
			remove_proc_entry(name, cpu_pc[cpu].root);

			sprintf(name,"counter_%d", j);
			remove_proc_entry(name, cpu_pc[cpu].root);
		}

		sprintf(name,"%d", cpu);
		remove_proc_entry(name, pc_root);
	}		

	remove_proc_entry("cpu_perf", root);

}
int __init hwdebug_init(void)
{

	/* create /proc/hwdebug */
        hwdebug_root = proc_mkdir("hwdebug", NULL);

        if (hwdebug_root == 0)
                return(0);

	perf_cnt_init(hwdebug_root);
	return(0);
	
}

void __exit hwdebug_exit(void)
{
	printk("%s\n", __FUNCTION__);
	perf_cnt_exit(hwdebug_root);
}

module_init(hwdebug_init);
module_exit(hwdebug_exit);
MODULE_LICENSE("GPL");

