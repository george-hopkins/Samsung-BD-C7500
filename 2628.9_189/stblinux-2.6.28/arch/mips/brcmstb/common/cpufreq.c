/*---------------------------------------------------------------------------

    Copyright (c) 2001-2007 Broadcom Corporation                 /\
                                                          _     /  \     _
    _____________________________________________________/ \   /    \   / \_
                                                            \_/      \_/  
    
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 2 as
 published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

    File: cpufreq.c

    Description: 
    cpufreq driver for Broadcom MIPS

    when        who         what
    -----       ---         ----
    20070109    cernekee    ripped from powernow-k6.c
 ------------------------------------------------------------------------- */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/ioport.h>
#include <linux/slab.h>

#include <asm/timex.h>
#include <asm/io.h>
#include <asm/mipsregs.h>

/*
 * NOTE: To enable this module for a new platform:
 *
 * 1) Add any SoC-specific support in here; ensure that MAX_FREQ is correct
 * 2) Edit SYS_SUPPORTS_CPUFREQ dependencies in arch/mips/Kconfig
 * 3) Update bcm*defconfig in arch/mips/kernel/configs/
 */

#if defined(CONFIG_MIPS_BCM7401C0)
#define MAX_FREQ 300000
#elif defined(CONFIG_MIPS_BCM7400D0) || defined(CONFIG_MIPS_BCM7405) \
	|| defined(CONFIG_MIPS_BCM7335)
#define MAX_FREQ 400000
#else
#error cpufreq is not supported on this platform.
#endif

static struct cpufreq_frequency_table clock_tbl[] = {
	{0, MAX_FREQ},
	{1, MAX_FREQ / 2},
	{2, MAX_FREQ / 4},
	{3, MAX_FREQ / 8},
	{0, CPUFREQ_TABLE_END},
};

static int current_freq = MAX_FREQ;

/**
 * brcm_set_state - set the CPU clock divider
 * @best_i: clock_tbl[best_i] is the target speed
 *
 *   Tries to change the CPU clock divider
 */
static void brcm_set_state (unsigned int best_i)
{
	struct cpufreq_freqs    freqs;

	freqs.old = current_freq;
	freqs.new = clock_tbl[best_i].frequency;
	freqs.cpu = 0; /* cpufreq is UP only driver */

	cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);

	/* see MIPS datasheet, CP0 register $22 */
#if defined(CONFIG_MIPS_BCM7401C0)
	write_c0_diag4((read_c0_diag4() & ~0x01c00000) |
		(clock_tbl[best_i].index << 23) | (0 << 22));
#elif defined(CONFIG_MIPS_BCM7400D0) || defined(CONFIG_MIPS_BCM7405) \
	|| defined(CONFIG_MIPS_BCM7335)
	write_c0_diag5((read_c0_diag5() & ~0x70000000) |
		(clock_tbl[best_i].index << 29) | (0 << 28));
#endif

	current_freq = clock_tbl[best_i].frequency;

	cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);

	return;
}


/**
 * brcm_verify - verifies a new CPUfreq policy
 * @policy: new policy
 *
 * Policy must be within lowest and highest possible CPU Frequency,
 * and at least one possible state must be within min and max.
 */
static int brcm_verify(struct cpufreq_policy *policy)
{
	return cpufreq_frequency_table_verify(policy, &clock_tbl[0]);
}


/**
 * brcm_setpolicy - sets a new CPUFreq policy
 * @policy: new policy
 * @target_freq: the target frequency
 * @relation: how that frequency relates to achieved frequency (CPUFREQ_RELATION_L or CPUFREQ_RELATION_H)
 *
 * sets a new CPUFreq policy
 */
static int brcm_target (struct cpufreq_policy *policy,
			       unsigned int target_freq,
			       unsigned int relation)
{
	unsigned int    newstate = 0;

	if (cpufreq_frequency_table_target(policy, &clock_tbl[0], target_freq, relation, &newstate))
		return -EINVAL;

	brcm_set_state(newstate);

	return 0;
}


static int brcm_cpu_init(struct cpufreq_policy *policy)
{
	int result;

	if (policy->cpu != 0)
		return -ENODEV;

	/* cpuinfo and default policy values */
	policy->governor = CPUFREQ_DEFAULT_GOVERNOR;
	policy->cpuinfo.transition_latency = CPUFREQ_ETERNAL;
	policy->cur = current_freq;

	result = cpufreq_frequency_table_cpuinfo(policy, clock_tbl);
	if (result)
		return (result);

	cpufreq_frequency_table_get_attr(clock_tbl, policy->cpu);

	return 0;
}


static int brcm_cpu_exit(struct cpufreq_policy *policy)
{
	brcm_set_state(0);	/* full speed */
	cpufreq_frequency_table_put_attr(policy->cpu);
	return 0;
}

static unsigned int brcm_get(unsigned int cpu)
{
	return(current_freq);
}

static struct freq_attr* brcm_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};

static struct cpufreq_driver brcm_driver = {
	.verify		= brcm_verify,
	.target		= brcm_target,
	.init		= brcm_cpu_init,
	.exit		= brcm_cpu_exit,
	.get		= brcm_get,
	.name		= "cpufreq",
	.owner		= THIS_MODULE,
	.attr		= brcm_attr,
};


/**
 * brcm_cpufreq_init - initializes the BRCM cpufreq driver.
 *
 *   Initializes the cpufreq support. Returns -ENODEV on unsupported
 * devices, -EINVAL or -ENOMEM on problems during initiatization, and zero
 * on success.
 */
static int __init brcm_cpufreq_init(void)
{
	if (cpufreq_register_driver(&brcm_driver)) {
		return -EINVAL;
	}

	return 0;
}


/**
 * brcm_cpufreq_exit - unregisters cpufreq support
 *
 *   Unregisters cpufreq support.
 */
static void __exit brcm_cpufreq_exit(void)
{
	cpufreq_unregister_driver(&brcm_driver);
}


MODULE_AUTHOR ("Broadcom Corporation");
MODULE_DESCRIPTION ("cpufreq driver for brcmstb.");
MODULE_LICENSE ("GPL");

module_init(brcm_cpufreq_init);
module_exit(brcm_cpufreq_exit);
