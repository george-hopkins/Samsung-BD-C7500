
#if defined(CONFIG_MIPS_BCM7601)

#include <asm/brcmstb/brcm97601/bchp_art.h>
#include <asm/brcmstb/brcm97601/bchp_bsp_cmdbuf.h>
#include <asm/brcmstb/brcm97601/bchp_bsp_glb_control.h>
#include <asm/brcmstb/brcm97601/bchp_clk.h>
#include <asm/brcmstb/brcm97601/bchp_clk_vcxo.h>
#include <asm/brcmstb/brcm97601/bchp_ddr23_ctl_regs_0.h>
#include <asm/brcmstb/brcm97601/bchp_ddr23_ctl_regs_1.h>
#include <asm/brcmstb/brcm97601/bchp_ddr23_phy_control_regs_0.h>
#include <asm/brcmstb/brcm97601/bchp_ddr23_phy_control_regs_1.h>
#include <asm/brcmstb/brcm97601/bchp_pri_arb_control_regs.h>
#include <asm/brcmstb/brcm97601/bchp_vcxo_ctl_misc.h>
#include <asm/brcmstb/brcm97601/bchp_misc.h>
/* A0 has wakeup_ctrl2, B0 has wakeup_intr2 */
#include <asm/brcmstb/brcm97601/bchp_wakeup_ctrl2.h>
#include <asm/brcmstb/brcm97601/bchp_wakeup_intr2.h>
#include <asm/brcmstb/brcm97601/bchp_usb_ctrl.h>
#include <asm/brcmstb/brcm97601/soc_regs.h>

/*
 * work around the name changes between 7601 A0, B0, and 763x
 */
#undef BCHP_WAKEUP_CTRL2_STATUS
#define BCHP_WAKEUP_CTRL2_STATUS BCHP_WAKEUP_INTR2_STATUS

#elif defined(CONFIG_MIPS_BCM7635)

#include <asm/brcmstb/brcm97635/bchp_art.h>
#include <asm/brcmstb/brcm97635/bchp_bsp_cmdbuf.h>
#include <asm/brcmstb/brcm97635/bchp_bsp_glb_control.h>
#include <asm/brcmstb/brcm97635/bchp_clk.h>
#include <asm/brcmstb/brcm97635/bchp_clk_vcxo.h>
#include <asm/brcmstb/brcm97635/bchp_ddr23_ctl_regs_0.h>
#include <asm/brcmstb/brcm97635/bchp_ddr23_ctl_regs_1.h>
#include <asm/brcmstb/brcm97635/bchp_ddr23_phy_control_regs_0.h>
#include <asm/brcmstb/brcm97635/bchp_ddr23_phy_control_regs_1.h>
#include <asm/brcmstb/brcm97635/bchp_pri_arb_control_regs.h>
#include <asm/brcmstb/brcm97635/bchp_vcxo_ctl_misc.h>
#include <asm/brcmstb/brcm97635/bchp_misc.h>
#include <asm/brcmstb/brcm97635/bchp_wakeup_ctrl2.h>
#include <asm/brcmstb/brcm97635/bchp_usb_ctrl.h>
#include <asm/brcmstb/brcm97635/soc_regs.h>

#elif defined(CONFIG_MIPS_BCM7630)

#include <asm/brcmstb/brcm97630/bchp_art.h>
#include <asm/brcmstb/brcm97630/bchp_bsp_cmdbuf.h>
#include <asm/brcmstb/brcm97630/bchp_bsp_glb_control.h>
#include <asm/brcmstb/brcm97630/bchp_clk.h>
#include <asm/brcmstb/brcm97630/bchp_ddr23_ctl_regs_0.h>
#include <asm/brcmstb/brcm97630/bchp_ddr23_ctl_regs_1.h>
#include <asm/brcmstb/brcm97630/bchp_ddr23_phy_control_regs.h>
#include <asm/brcmstb/brcm97630/bchp_pri_arb_control_regs.h>
#include <asm/brcmstb/brcm97630/bchp_misc.h>
#include <asm/brcmstb/brcm97630/bchp_wakeup_ctrl2.h>
#include <asm/brcmstb/brcm97630/bchp_usb_ctrl.h>
#include <asm/brcmstb/brcm97630/soc_regs.h>

#else
#error "No power management support for chip"
#endif

#ifdef  REG_TO_K1
#undef REG_TO_K1
#endif

#define REG_TO_K1(x) ( (volatile unsigned int *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET + (x)) )
#define BREG(x)	(BCHP_##x)


/*
 * ART Instruction
 * May be one word or two.
 */
typedef struct art_ins{
	unsigned int word_0;	/* opcode and parameter 1 */
	unsigned int word_1;	/* Parameter 2 */
} art_ins_t;

#define INS(op, p1, p2) {((op << 28) | (p1 & 0xfffffff)), p2}

extern art_ins_t art_pwr_down[];
extern art_ins_t art_pwr_up[];


typedef struct clk_pm_wakeup {
	unsigned int wakeup_status;
	unsigned int wakeup_mask;
	unsigned int critical_ctrl;
	unsigned int critical_status;
} clk_pm_wakeup_t;


#define WAKE2_IRR	BCHP_WAKEUP_CTRL2_STATUS_IRR_MASK
#define WAKE2_TIMER	BCHP_WAKEUP_CTRL2_STATUS_WAKEUP_TIMER_MASK
#define WAKE2_UART0	BCHP_WAKEUP_CTRL2_STATUS_UART0_MASK
#define WAKE2_UART1	BCHP_WAKEUP_CTRL2_STATUS_UART1_MASK
#define WAKE2_UART2	BCHP_WAKEUP_CTRL2_STATUS_UART2_MASK
#define WAKE2_GIO	BCHP_WAKEUP_CTRL2_STATUS_GIO_MASK


enum op_codes {
	Halt = 0,
	Delay,
	Write,
	WrOr,
	RdAnd,
	PollNotSet,
	PollSet,
};



/*
 * sections for text and data that need to be locked into cache.
 * sections are defined in mips/kernel/vmlinux.ld.S
 */

#define  __locked_text     __attribute__ ((__section__ (".locked.text")))
#define  __locked_data     __attribute__ ((__section__ (".locked.data")))

extern __locked_data art_ins_t ddr_standby[];
extern __locked_data art_ins_t ddr_active[];

__locked_text void ddr_go_active(void);
__locked_text void ddr_go_standby(void);
void bsp_active(void);
void bsp_standby(void);
void usb_active(void);
void usb_standby(void);
void hif_standby(void);
void hif_active(void);
__locked_text void sundry_standby(void);
__locked_text void sundry_active(void);

extern int power_wol;
extern int power_pm_test;


/*
 * microsecond delay loop --
 *   based on 500 mhz clock and 3 clock cycles per loop
 */
#define DELAY(x) { __delay(x * 166); }

