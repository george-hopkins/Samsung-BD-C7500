/*
#************************************************************************
#* Coprocessor 0 Index Register Bits
#************************************************************************
# No known functionality (32-bit register)
*/

/*
#************************************************************************
#* Coprocessor 0 Entry Register Bits
#************************************************************************
# No known functionality (32-bit register)
*/

/*
#************************************************************************
#* Coprocessor 0 BvAddr Register Bits
#************************************************************************
# Contains offending address (32-bit register)
*/

/*
#************************************************************************
#* Coprocessor 0 Status Register Bits
#************************************************************************

# Notes: COP2 is forced, then allowed to be overwritten
#               writing a '1' to bit 20 force bit 20 to '0' but no effect
*/
#define BCM_CP0_SR_COP3              (1<<31)
#define BCM_CP0_SR_COP2              (1<<30)
#define BCM_CP0_SR_COP1              (1<<29)
#define BCM_CP0_SR_COP0              (1<<28)
#define BCM_CP0_SR_IST               (1<<23)
#define BCM_CP0_SR_BEV               (1<<22)
#define BCM_CP0_SR_SWC               (1<<17)
#define BCM_CP0_SR_ISC               (1<<16)
#define BCM_CP0_SR_KRNL              (1<<1)
#define BCM_CP0_SR_IE                (1<<0)
#define BCM_CP0_SR_IM5               (1<<15)
#define BCM_CP0_SR_IM4               (1<<14)
#define BCM_CP0_SR_IM3               (1<<13)
#define BCM_CP0_SR_IM2               (1<<12)
#define BCM_CP0_SR_IM1               (1<<11)
#define BCM_CP0_SR_IM0               (1<<10)
#define BCM_CP0_SR_SWM1              (1<<9)
#define BCM_CP0_SR_SWM0              (1<<8)

/*
#************************************************************************
#* Coprocessor 0 Cause Register Bits
#************************************************************************
# Notes: 5:2 hold exception cause
# Notes: 29:28 hold Co-processor Number reference by Coproc unusable excptn
# Notes: 7:6, 1:0, 27:15, 30 ***UNUSED***
*/
#define BCM_CP0_CR_BD                        (1<<31)
#define BCM_CP0_CR_EXTIRQ4                   (1<<14)
#define BCM_CP0_CR_EXTIRQ3                   (1<<13)
#define BCM_CP0_CR_EXTIRQ2                   (1<<12)
#define BCM_CP0_CR_EXTIRQ1                   (1<<11)
#define BCM_CP0_CR_EXTIRQ0                   (1<<10)
#define BCM_CP0_CR_SW1                       (1<<9)
#define BCM_CP0_CR_SW0                       (1<<8)
#define BCM_CP0_CR_EXC_CAUSE_MASK            (0xf << 2)
#define BCM_CP0_CR_EXC_COP_MASK              (0x3 << 28)

/*
#************************************************************************
#* Coprocessor 0 EPC Register Bits
#************************************************************************
# Contains PC or PC-4 for resuming program after exception (32-bit register)
*/

/*
#************************************************************************
#* Coprocessor 0 PrID Register Bits
#************************************************************************
# Notes: Company Options=0
#        Company ID=0
#        Processor ID = 0xa
#        Revision = 0xa
*/

/*
#************************************************************************
#* Coprocessor 0 Debug Register Bits
#************************************************************************
# Notes: Bits [29:13],[11], [9], [6] read as zero
*/
#define BCM_CP0_DBG_BRDLY           (0x1 << 31)
#define BCM_CP0_DBG_DBGMD           (0x1 << 30)
#define BCM_CP0_DBG_EXSTAT          (0x1 << 12)
#define BCM_CP0_DBG_BUS_ERR         (0x1 << 10)
#define BCM_CP0_DBG_1STEP           (0x1 << 8)
#define BCM_CP0_DBG_JTGRST          (0x1 << 7)
#define BCM_CP0_DBG_PBUSBRK         (0x1 << 5)
#define BCM_CP0_DBG_IADBRK          (0x1 << 4)
#define BCM_CP0_DBG_DABRKST         (0x1 << 3)
#define BCM_CP0_DBG_DABRKLD         (0x1 << 2)
#define BCM_CP0_DBG_SDBBPEX         (0x1 << 1)
#define BCM_CP0_DBG_SSEX            (0x1 << 0)

/*
#************************************************************************
#* Coprocessor 0 DBEXCPC Register Bits
#************************************************************************
# Debug Exception Program Counter (32-bits)
*/

/*
#************************************************************************
#* Coprocessor 0 PROCCFG Register Bits
#************************************************************************
# Select 0
*/
#define BCM_CP0_CFG1EN              (0x1 << 31) 
#define BCM_CP0_BE                  (0x1 << 15)
#define BCM_CP0_MIPS32MSK           (0x3 << 13) /* 0 = MIPS32 Arch */
#define BCM_CP0_ARMSK               (0x7 << 10) /* Architecture Rev 0 */
#define BCM_CP0_MMUMSK              (0x7 << 7)  /* 0 no MMU */
#define BCM_CP0_K0Coherency         (0x7 << 0)  /* 0 no Coherency */
#define BCM_CP0_K0Uncached          (0x2 << 0)  /* 2 = Uncached */
#define BCM_CP0_K0WriteThrough      (0x1 << 0)  /* 0 = Cached, Dcache write thru */
#define BCM_CP0_K0Writeback         (0x3)       /* 0 = Cached, Dcache write back */

/*
# Select 1
#  Bit  31:   unused
#  Bits 30:25 MMU Size (Num TLB entries-1)
#  Bits 24:22 ICache sets/way (2^n * 64)
#  Bits 21:19 ICache Line size (2^(n+1) bytes) 0=No Icache
#  Bits 18:16 ICache Associativity (n+1) way                    
#  Bits 15:13 DCache sets/way (2^n * 64)
#  Bits 12:10 DCache Line size (2^(n+1) bytes) 0=No Dcache
#  Bits 9:7   DCache Associativity (n+1) way                    
#  Bits 6:4   unused
#  Bit  3:    1=At least 1 watch register
#  Bit  2:    1=MIPS16 code compression implemented
#  Bit  1:    1=EJTAG implemented                   
#  Bit  0:    1=FPU implemented                   
*/
#define BCM_CP0_CFG_ISMSK      (0x7 << 22)
#define BCM_CP0_CFG_ISSHF      22
#define BCM_CP0_CFG_ILMSK      (0x7 << 19)
#define BCM_CP0_CFG_ILSHF      19
#define BCM_CP0_CFG_IAMSK      (0x7 << 16)
#define BCM_CP0_CFG_IASHF      16
#define BCM_CP0_CFG_DSMSK      (0x7 << 13)
#define BCM_CP0_CFG_DSSHF      13
#define BCM_CP0_CFG_DLMSK      (0x7 << 10)
#define BCM_CP0_CFG_DLSHF      10
#define BCM_CP0_CFG_DAMSK      (0x7 << 7)
#define BCM_CP0_CFG_DASHF      7

/*
#************************************************************************
#* Coprocessor 0 Config Register Bits
#************************************************************************
*/
#define BCM_CP0_CFG_ICSHEN         (0x1 << 31)
#define BCM_CP0_CFG_DCSHEN         (0x1 << 30)


/*
#************************************************************************
#* KSEG Mapping Definitions and Macro's
#************************************************************************
*/
#define BCM_K0BASE          0x80000000
#define BCM_K0SIZE          0x20000000
#define BCM_K1BASE          0xa0000000
#define BCM_K1SIZE          0x20000000
#define BCM_K2BASE          0xc0000000

#define BCM_PHYS_TO_K0(x)   ((x) | 0x80000000)
#define BCM_PHYS_TO_K1(x)   ((x) | 0xa0000000)
#define BCM_K0_TO_PHYS(x)   ((x) & 0x1fffffff)
#define BCM_K1_TO_PHYS(x)   (BCM_K0_TO_PHYS(x))
#define BCM_K0_TO_K1(x)     ((x) | 0x20000000)
#define BCM_K1_TO_K0(x)     ((x) & 0xdfffffff)

