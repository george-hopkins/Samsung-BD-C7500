#ifndef __ASM_BUG_H
#define __ASM_BUG_H

#include <asm/sgidefs.h>

#ifdef CONFIG_BUG

#include <asm/break.h>

extern void do_bug(const char *);

#define BUG()								\
do {									\
	printk("BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __FUNCTION__); \
	do_bug(__FUNCTION__); \
} while (0)

//	__asm__ __volatile__("nop\n\tnop\n\twait\n\tnop\n\tnop\n" : : "i" (BRK_BUG));	
//	__asm__ __volatile__("nop\n\tnop\n\tbreak %0" : : "i" (BRK_BUG));

#define HAVE_ARCH_BUG

#if (_MIPS_ISA > _MIPS_ISA_MIPS1)

#if 0
#define BUG_ON(condition)						\
do {									\
	__asm__ __volatile__("tne $0, %0, %1"				\
			     : : "r" (condition), "i" (BRK_BUG));	\
} while (0)

#else

#define BUG_ON(condition) do { if (unlikely(condition)) do_bug(__FUNCTION__); } while(0)

#endif

#define HAVE_ARCH_BUG_ON

#endif /* _MIPS_ISA > _MIPS_ISA_MIPS1 */

#endif

#include <asm-generic/bug.h>

#endif /* __ASM_BUG_H */
