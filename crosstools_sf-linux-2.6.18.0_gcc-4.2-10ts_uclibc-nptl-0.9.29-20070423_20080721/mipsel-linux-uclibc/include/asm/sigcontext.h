/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996, 1997, 1999 by Ralf Baechle
 * Copyright (C) 1999 Silicon Graphics, Inc.
 */
#ifndef _ASM_SIGCONTEXT_H
#define _ASM_SIGCONTEXT_H

#include <asm/sgidefs.h>

#if _MIPS_SIM == _MIPS_SIM_ABI32

/*
 * Keep this struct definition in sync with the sigcontext fragment
 * in arch/mips/tools/offset.c
 */
struct sigcontext {
	unsigned int		sc_regmask;	/* Unused */
	unsigned int		sc_status;
	unsigned long long	sc_pc;
	unsigned long long	sc_regs[32];
	unsigned long long	sc_fpregs[32];
	unsigned int		sc_ownedfp;	/* Unused */
	unsigned int		sc_fpc_csr;
	unsigned int		sc_fpc_eir;	/* Unused */
	unsigned int		sc_used_math;
	unsigned int		sc_dsp;		/* dsp status, was sc_ssflags */
	unsigned long long	sc_mdhi;
	unsigned long long	sc_mdlo;
	unsigned long		sc_hi1;		/* Was sc_cause */
	unsigned long		sc_lo1;		/* Was sc_badvaddr */
	unsigned long		sc_hi2;		/* Was sc_sigset[4] */
	unsigned long		sc_lo2;
	unsigned long		sc_hi3;
	unsigned long		sc_lo3;
};

#endif /* _MIPS_SIM == _MIPS_SIM_ABI32 */

#if _MIPS_SIM == _MIPS_SIM_ABI64 || _MIPS_SIM == _MIPS_SIM_NABI32

/*
 * Keep this struct definition in sync with the sigcontext fragment
 * in arch/mips/tools/offset.c
 *
 * Warning: this structure illdefined with sc_badvaddr being just an unsigned
 * int so it was changed to unsigned long in 2.6.0-test1.  This may break
 * binary compatibility - no prisoners.
 * DSP ASE in 2.6.12-rc4.  Turn sc_mdhi and sc_mdlo into an array of four
 * entries, add sc_dsp and sc_reserved for padding.  No prisoners.
 */
struct sigcontext {
	unsigned long	sc_regs[32];
	unsigned long	sc_fpregs[32];
	unsigned long	sc_mdhi;
	unsigned long	sc_hi1;
	unsigned long	sc_hi2;
	unsigned long	sc_hi3;
	unsigned long	sc_mdlo;
	unsigned long	sc_lo1;
	unsigned long	sc_lo2;
	unsigned long	sc_lo3;
	unsigned long	sc_pc;
	unsigned int	sc_fpc_csr;
	unsigned int	sc_used_math;
	unsigned int	sc_dsp;
	unsigned int	sc_reserved;
};


#endif /* _MIPS_SIM == _MIPS_SIM_ABI64 || _MIPS_SIM == _MIPS_SIM_NABI32 */

#endif /* _ASM_SIGCONTEXT_H */
