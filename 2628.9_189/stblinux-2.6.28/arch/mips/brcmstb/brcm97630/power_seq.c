#include <linux/autoconf.h>
#include <linux/types.h>

#include <asm/io.h>
#include <asm/mipsregs.h>
#include <asm/addrspace.h>
#include <asm/cacheflush.h>
#include <asm/delay.h>


#include <asm/brcmstb/common/brcmstb.h>
#include <asm/brcmstb/common/power.h>
#include <asm/brcmstb/brcm97630/bchp_ddr23_phy_control_regs.h>
#include <asm/brcmstb/brcm97630/bchp_ddr23_phy_byte_lane_3.h>
#if defined (CONFIG_BCM_VIDE_OFE)
#include <asm/brcmstb/brcm97630/bchp_ofe_armcr4_bridge.h>
#include <linux/libata.h>
#endif


/*
 * ART hardware sequencer
 */
art_ins_t art_pwr_down[] = {
	INS( Delay,	10,	0),
	INS( RdAnd,	BCHP_CLK_PM_CTRL_2 , 0xffffffff),
	INS( WrOr, 	BCHP_CLK_PM_CTRL_2 , BCHP_CLK_PM_CTRL_2_DIS_CPU0_CLK_MASK),

	INS( RdAnd,	BCHP_CLK_PM_CTRL , 0xffffffff),
	INS( WrOr, 	BCHP_CLK_PM_CTRL , BCHP_CLK_PM_CTRL_DIS_CPU0_108M_CLK_MASK),

	INS( RdAnd,	BCHP_CLK_PM_CTRL_216 , 0xffffffff),
	INS( WrOr, 	BCHP_CLK_PM_CTRL_216 , BCHP_CLK_PM_CTRL_216_DIS_CPU0_216_CLK_MASK),

#if 0
	/* turning off BCHP_CLK_PLL1_CTRL_CLOCK_ENA_MASK kills WOL */
	INS( RdAnd,	BCHP_CLK_PLL1_CTRL , ~BCHP_CLK_PLL1_CTRL_CLOCK_ENA_MASK),
	INS( WrOr, 	BCHP_CLK_PLL1_CTRL , 0),

	INS( RdAnd,	BCHP_CLK_PLL1_CTRL , 0xffffffff),
	INS( WrOr, 	BCHP_CLK_PLL1_CTRL , BCHP_CLK_PLL1_CTRL_POWERDOWN_MASK),
#endif

	INS( Halt, 	0, 0),
};

art_ins_t art_pwr_up[] = {
#if 0
	INS( RdAnd,	BCHP_CLK_PLL1_CTRL , ~BCHP_CLK_PLL1_CTRL_POWERDOWN_MASK),
	INS( WrOr, 	BCHP_CLK_PLL1_CTRL , 0),

	INS( RdAnd,	BCHP_CLK_PLL1_CTRL , 0xffffffff),
	INS( WrOr, 	BCHP_CLK_PLL1_CTRL , BCHP_CLK_PLL1_CTRL_CLOCK_ENA_MASK),
#endif

	INS( RdAnd,	BCHP_CLK_PM_CTRL_216 , ~BCHP_CLK_PM_CTRL_216_DIS_CPU0_216_CLK_MASK),
	INS( WrOr, 	BCHP_CLK_PM_CTRL_216 , 0),

	INS( RdAnd,	BCHP_CLK_PM_CTRL, ~BCHP_CLK_PM_CTRL_DIS_CPU0_108M_CLK_MASK),
	INS( WrOr, 	BCHP_CLK_PM_CTRL, 0),

	INS( RdAnd,	BCHP_CLK_PM_CTRL_2 , ~BCHP_CLK_PM_CTRL_2_DIS_CPU0_CLK_MASK),
	INS( WrOr, 	BCHP_CLK_PM_CTRL_2 , 0),

	INS( Delay,	0x80,	0),

	INS( Write, 	BCHP_CLK_CRITICAL_CTRL , BCHP_CLK_CRITICAL_CTRL_MIPS_POWER_INTR_MASK),
	INS( Halt, 	0, 0),
};

struct brcm97635_regs {
	volatile struct ddr23_ctl	*ddr23_ctl_0;
	volatile struct ddr23_ctl	*ddr23_ctl_1;
	volatile struct ddr_phy_control *ddr_phy;
	volatile struct ddr_phy_lane 	*ddr_lanes[NUM_DDR_LANES];
	volatile struct art		*art;
	volatile struct misc		*misc;
	volatile struct clk_control	*clock;
	volatile struct pri_arbiter_control *arbiter;
	volatile struct bsp_glb_ctl	*bsp_glb_ctl;
	volatile struct usb_ctrl	*usb_ctrl;
	reg_t				ddr_divider[2];
	reg_t				ddr_pre_divider[2];
	int				dac_status;
#if defined (CONFIG_BCM_VIDE_OFE)
	unsigned int fe_scratch_1;
	unsigned int fe_reload_physaddr;
	unsigned int fe_reload_windowsize;
	/*
	 * Only used if the OFE magic register
	 * sequence is required
	 */
	unsigned int AHB_Saved_1;
	unsigned int AHB_Saved_2;
	unsigned int AHB_Saved_3;
#endif

} SoC __locked_data = {

	.ddr23_ctl_0	= (volatile struct ddr23_ctl *) REG_TO_K1( BCHP_DDR23_CTL_REGS_0_REVISION),
	.ddr23_ctl_1 	= (volatile struct ddr23_ctl *) REG_TO_K1( BCHP_DDR23_CTL_REGS_1_REVISION),
	.ddr_phy	= (volatile struct ddr_phy_control *) REG_TO_K1( BCHP_DDR23_PHY_CONTROL_REGS_REVISION),
	.ddr_lanes	= {
				(volatile struct ddr_phy_lane *) REG_TO_K1( BCHP_DDR23_PHY_BYTE_LANE_0_REVISION),
				(volatile struct ddr_phy_lane *) REG_TO_K1( BCHP_DDR23_PHY_BYTE_LANE_1_REVISION),
				(volatile struct ddr_phy_lane *) REG_TO_K1( BCHP_DDR23_PHY_BYTE_LANE_2_REVISION),
				(volatile struct ddr_phy_lane *) REG_TO_K1( BCHP_DDR23_PHY_BYTE_LANE_3_REVISION),
			},

	.arbiter	= (volatile struct pri_arbiter_control *) REG_TO_K1( BCHP_PRI_ARB_CONTROL_REGS_CONC_CTL),
	.art		= (volatile struct art *) REG_TO_K1( BCHP_ART_REVISION),
	.misc		= (volatile struct misc *) REG_TO_K1( BCHP_MISC_MISC_REVISION_ID),
	.clock		= (volatile struct clk_control *) REG_TO_K1( BCHP_CLK_REVISION),
	.bsp_glb_ctl	= (volatile struct bsp_glb_ctl *) REG_TO_K1( BCHP_BSP_GLB_CONTROL_GLB_REVISION),
	.usb_ctrl	= (volatile struct usb_ctrl *) REG_TO_K1( BCHP_USB_CTRL_BRT_CTL_1),

};


 __locked_data int  power_bypass_ddr = 0; 
 __locked_data int  power_bypass_bsp = 0;
__locked_text void
ddr_go_standby(void) 
{
	/*
	 * Cloned from 7635.
	 * 7630 HW is different.
	 */
	reg_t	mask;

	/* turn on auto idle */
	SoC.ddr23_ctl_0->params2 |= BCHP_DDR23_CTL_REGS_0_PARAMS2_auto_idle_MASK;
	SoC.ddr23_ctl_1->params2 |= BCHP_DDR23_CTL_REGS_0_PARAMS2_auto_idle_MASK;

	if (power_bypass_ddr)
		return;

	/* Disable memory arbiter refresh clients */
	SoC.arbiter->refresh_ctl_0 &= ~BCHP_PRI_ARB_CONTROL_REGS_REFRESH_CTL_0_enable_MASK;
	SoC.arbiter->refresh_ctl_1 &= ~BCHP_PRI_ARB_CONTROL_REGS_REFRESH_CTL_1_enable_MASK;

	/* Turn off arbiter clock */
	SoC.clock->pm_ctrl_216 |= BCHP_CLK_PM_CTRL_216_DIS_SUN_216_CLK_MASK;

	/* Disable DDR 0 and 1 CLKE lines */
	SoC.ddr23_ctl_0->params2 |= BCHP_DDR23_CTL_REGS_0_PARAMS2_clke_MASK;
	SoC.ddr23_ctl_1->params2 |= BCHP_DDR23_CTL_REGS_0_PARAMS2_clke_MASK;

	/* start refresh cycle */
	SoC.ddr23_ctl_0->refresh_cmd = 0x60;
	SoC.ddr23_ctl_1->refresh_cmd = 0x60;

	/* wait until both DDR RAMs have entered self refresh */
#define ART_DDR_ACTIVE	(BCHP_ART_STATUS_DDR0_ACTIVE_MASK | BCHP_ART_STATUS_DDR1_ACTIVE_MASK)
	while (SoC.art->status &  ART_DDR_ACTIVE) 
		;

	/* Set idle pad controls */
	SoC.ddr_phy->idle_pad_control |= BCHP_DDR23_PHY_CONTROL_REGS_IDLE_PAD_CONTROL_ctl_iddq_MASK;

	mask = BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dm_iddq_MASK
	       | BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dq_iddq_MASK
	       | BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_read_enb_iddq_MASK
	       | BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dqs_iddq_MASK
	       | BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_clk_iddq_MASK;

	SoC.ddr_lanes[0]->idle_pad_control |= mask;
	SoC.ddr_lanes[1]->idle_pad_control |= mask;
	SoC.ddr_lanes[2]->idle_pad_control |= mask;
	SoC.ddr_lanes[3]->idle_pad_control |= mask;
	
	/* save clock dividers */
	SoC.ddr_divider[0] = SoC.ddr_phy->pll_divider;
	SoC.ddr_pre_divider[0] = SoC.ddr_phy->pll_pre_divider;

	/* turn off all DDR clocks and PLLs */
	SoC.ddr_phy->clk_pm_ctrl = 1;

	SoC.ddr_phy->pll_pre_divider = BCHP_DDR23_PHY_CONTROL_REGS_PLL_PRE_DIVIDER_NDIV_PWRDN_MASK;

	/* clock regulators */
	SoC.ddr_phy->clock_reg_control = 1;

	SoC.ddr_lanes[0]->clock_reg_control = 1;
	SoC.ddr_lanes[1]->clock_reg_control = 1;
	SoC.ddr_lanes[2]->clock_reg_control = 1;
	SoC.ddr_lanes[3]->clock_reg_control = 1;

	/* turn off clock pads */
	SoC.ddr_lanes[0]->clock_pad_disable = 1;
	SoC.ddr_lanes[1]->clock_pad_disable = 1;
	SoC.ddr_lanes[2]->clock_pad_disable = 1;
	SoC.ddr_lanes[3]->clock_pad_disable = 1;


	mask = BCHP_DDR23_PHY_CONTROL_REGS_PLL_CONFIG_DIV2_CLK_RESET_MASK
	     | BCHP_DDR23_PHY_CONTROL_REGS_PLL_CONFIG_PWRDN_CH1_MASK
	     | BCHP_DDR23_PHY_CONTROL_REGS_PLL_CONFIG_DRESET_MASK
	     | BCHP_DDR23_PHY_CONTROL_REGS_PLL_CONFIG_ARESET_MASK
	     | BCHP_DDR23_PHY_CONTROL_REGS_PLL_CONFIG_PWRDN_MASK ;

	SoC.ddr_phy->pll_config = mask;

	/* turn off DDR clocks */
	SoC.clock->pm_ctrl |= BCHP_CLK_PM_CTRL_DIS_DDR2_0_108M_CLK_MASK;
	SoC.clock->pm_ctrl_216 |= BCHP_CLK_PM_CTRL_216_DIS_DDR2_0_216_CLK_MASK ;

}

//#define DEBUG_TRACE

#ifdef DEBUG_TRACE
	#define TX_REG  REG_TO_K1(BCHP_UARTA_REG_START)
	#define DBG_PUTC(x) { *tx = x; }
#else
	#define DBG_PUTC(x) /* */
#endif

__locked_text void
ddr_go_active(void) 
{
	/*
	 * Cloned from 7635.
	 * 7630 HW is different.
	 */
	reg_t mask;

#ifdef DEBUG_TRACE
	volatile unsigned int  *tx;
	tx = (volatile unsigned int *)TX_REG;
#endif

	if (power_bypass_ddr)
		return;

	/* turn on DDR clocks */
	SoC.clock->pm_ctrl &= ~BCHP_CLK_PM_CTRL_DIS_DDR2_0_108M_CLK_MASK;
	SoC.clock->pm_ctrl_216 &= ~BCHP_CLK_PM_CTRL_216_DIS_DDR2_0_216_CLK_MASK;

	DELAY(1);
	DBG_PUTC( '1');

	/* restart PLL */
	SoC.ddr_phy->pll_config =  0x8000000C; // ARESET, DRESET, DIV2 CLK RESET

	/* restore clock dividers */
	SoC.ddr_phy->pll_divider = SoC.ddr_divider[0];
	SoC.ddr_phy->pll_pre_divider = SoC.ddr_pre_divider[0];

	DELAY(5);
	DBG_PUTC( '2');

	SoC.ddr_phy->pll_config =  0x80000008; // DRESET, DIV2 CLK RESET

	/* poll for PLL lock */
	while ((SoC.ddr_phy->pll_status & BCHP_DDR23_PHY_CONTROL_REGS_PLL_STATUS_LOCK_MASK) == 0)
			DELAY(1);
	DBG_PUTC( '4');
	/* clear DRESET */
	SoC.ddr_phy->pll_config =  0x80000000; //  DIV2 CLK RESET

	DBG_PUTC( '5');

	/* clock regulators */
	SoC.ddr_phy->clock_reg_control = 0;

	SoC.ddr_lanes[0]->clock_reg_control = 0;
	SoC.ddr_lanes[1]->clock_reg_control = 0;
	SoC.ddr_lanes[2]->clock_reg_control = 0;
	SoC.ddr_lanes[3]->clock_reg_control = 0;

	DBG_PUTC( '6');

	/* turn on DDR clock pads. */
	SoC.ddr_lanes[0]->clock_pad_disable = 0;
	SoC.ddr_lanes[1]->clock_pad_disable = 0;
	SoC.ddr_lanes[2]->clock_pad_disable = 0;
	SoC.ddr_lanes[3]->clock_pad_disable = 0;
	
	/* DDR CLK enable */
	SoC.ddr_phy->clk_pm_ctrl = 0;

	/* turn on DDR I/O pads. */
	SoC.ddr_phy->idle_pad_control &= ~BCHP_DDR23_PHY_CONTROL_REGS_IDLE_PAD_CONTROL_ctl_iddq_MASK;

	mask = BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dm_iddq_MASK
	       | BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dq_iddq_MASK
	       | BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_read_enb_iddq_MASK
	       | BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_dqs_iddq_MASK
	       | BCHP_DDR23_PHY_BYTE_LANE_3_IDLE_PAD_CONTROL_clk_iddq_MASK;

	SoC.ddr_lanes[0]->idle_pad_control &= ~mask;
	SoC.ddr_lanes[1]->idle_pad_control &= ~mask;
	SoC.ddr_lanes[2]->idle_pad_control &= ~mask;
	SoC.ddr_lanes[3]->idle_pad_control &= ~mask;

	/* deassert DIV2 CLK RESET */
	SoC.ddr_phy->pll_config =  0;
	
	/* Turn on arbiter clock */
	SoC.clock->pm_ctrl_216 &= ~BCHP_CLK_PM_CTRL_216_DIS_SUN_216_CLK_MASK;

	DELAY(50);

	/* enable memory arbiter refresh clients */
	SoC.arbiter->refresh_ctl_0 |= BCHP_PRI_ARB_CONTROL_REGS_REFRESH_CTL_0_enable_MASK;
	SoC.arbiter->refresh_ctl_1 |= BCHP_PRI_ARB_CONTROL_REGS_REFRESH_CTL_1_enable_MASK;

	/* 
	 * take DRAM out of self refresh mode by enablng DDR 0 and 1 CLE lines
	 */

	SoC.ddr23_ctl_0->params2 &= ~BCHP_DDR23_CTL_REGS_0_PARAMS2_clke_MASK;
	SoC.ddr23_ctl_1->params2 &= ~BCHP_DDR23_CTL_REGS_0_PARAMS2_clke_MASK;

	/* wait until both DDR have exited self refresh */
	while ((SoC.art->status &  ART_DDR_ACTIVE)  != ART_DDR_ACTIVE) 
		DELAY(1);

	DBG_PUTC( '8');
}

#define INPUTBUF2_START	0x180
__locked_text void
bsp_standby(void)
{
	/* 
	 * can not shut down BSP unless DRAM is shut down 
	 */
	volatile reg_t *input_buf_2;
	
	if (power_bypass_bsp)
		return;

	SoC.bsp_glb_ctl->glb_ordy = BCHP_BSP_GLB_CONTROL_GLB_ORDY_CMD_ODRY2_MASK;

	DELAY(50);
	while ((SoC.bsp_glb_ctl->glb_irdy & BCHP_BSP_GLB_CONTROL_GLB_IRDY_CMD_IDRY2_MASK) == 0)
		DELAY(10);


	input_buf_2 = (volatile reg_t *)REG_TO_K1( BCHP_BSP_CMDBUF_DMEMi_ARRAY_BASE + INPUTBUF2_START);

	input_buf_2[0] = 0x00000010;
	input_buf_2[1] = 0x00000098;
	input_buf_2[2] = 0x00000000;
	input_buf_2[3] = 0x0000004b;
	input_buf_2[4] = 0x00000004;
	input_buf_2[5] = 0x00000000;

	SoC.bsp_glb_ctl->glb_iload2 = 1;

	DELAY(100);
	while ((SoC.bsp_glb_ctl->glb_oload2 & 1) == 0)
			DELAY(100);


	SoC.bsp_glb_ctl->glb_ordy = 0;

	SoC.bsp_glb_ctl->glb_host_intr_status = 0;
	SoC.bsp_glb_ctl->glb_oload2 = 0;
	SoC.bsp_glb_ctl->glb_ordy = BCHP_BSP_GLB_CONTROL_GLB_ORDY_CMD_ODRY2_MASK;

	/* wait for BSP to go idle */
	DELAY(100);
	while ((SoC.art->status & BCHP_ART_STATUS_AEGIS_RDY_MASK) == 0) {
		DELAY(100);
	}
	

	/* Turn off BSP clocks */
	SoC.clock->pm_ctrl	|= BCHP_CLK_PM_CTRL_DIS_BSP_108M_CLK_MASK;
	SoC.clock->pm_ctrl_216	|= BCHP_CLK_PM_CTRL_216_DIS_BSP_216_CLK_MASK;
	SoC.clock->pm_ctrl_1	|= BCHP_CLK_PM_CTRL_1_DIS_BSP_9M_CLK_MASK;
}
	
__locked_text void
bsp_active(void)
{
	/* 
	 * can not shut down BSP unless DRAM is shut down 
	 */
	int delay_count;

	if (power_bypass_bsp)
		return;

	/* Turn on BSP clocks */
	SoC.clock->pm_ctrl	&= ~BCHP_CLK_PM_CTRL_DIS_BSP_108M_CLK_MASK;
	SoC.clock->pm_ctrl_216	&= ~BCHP_CLK_PM_CTRL_216_DIS_BSP_216_CLK_MASK;
	SoC.clock->pm_ctrl_1	&= ~BCHP_CLK_PM_CTRL_1_DIS_BSP_9M_CLK_MASK;

	for (delay_count = 1000000; delay_count > 0; --delay_count)
			;

	/* issue BSP interrupt */
	SoC.clock->critical_ctrl =  BCHP_CLK_CRITICAL_CTRL_BSP_POWER_INTR_MASK;
	SoC.clock->critical_ctrl =  0;
}

void 
avd_standby(void)
{
	SoC.clock->pm_ctrl	|= BCHP_CLK_PM_CTRL_DIS_AVD0_108M_CLK_MASK;
	SoC.clock->pm_ctrl_216	|= BCHP_CLK_PM_CTRL_216_DIS_AVD0_216_CLK_MASK;
	SoC.clock->pm_ctrl_2	|= BCHP_CLK_PM_CTRL_2_DIS_AVD0_CORE_CLK_MASK;

	SoC.clock->alt_pll_ctrl &= ~BCHP_CLK_ALT_PLL_CTRL_CLOCK_ENA_MASK;
	SoC.clock->alt_pll_ctrl |= BCHP_CLK_ALT_PLL_CTRL_POWERDOWN_MASK;

}

void
avd_active(void)
{
	SoC.clock->alt_pll_ctrl &= ~BCHP_CLK_ALT_PLL_CTRL_POWERDOWN_MASK;
	SoC.clock->alt_pll_ctrl |= BCHP_CLK_ALT_PLL_CTRL_CLOCK_ENA_MASK;

	SoC.clock->pm_ctrl	&= ~BCHP_CLK_PM_CTRL_DIS_AVD0_108M_CLK_MASK;
	SoC.clock->pm_ctrl_216	&= ~BCHP_CLK_PM_CTRL_216_DIS_AVD0_216_CLK_MASK;
	SoC.clock->pm_ctrl_2	&= ~BCHP_CLK_PM_CTRL_2_DIS_AVD0_CORE_CLK_MASK;
}

void
m2mc_standby(void)
{
	SoC.clock->pm_ctrl	|= BCHP_CLK_PM_CTRL_DIS_GFX_108M_CLK_MASK;
	SoC.clock->pm_ctrl_216	|= BCHP_CLK_PM_CTRL_216_DIS_GFX_216_CLK_MASK;
}

void
m2mc_active(void)
{
	SoC.clock->pm_ctrl	&= ~BCHP_CLK_PM_CTRL_DIS_GFX_108M_CLK_MASK;
	SoC.clock->pm_ctrl_216	&= ~BCHP_CLK_PM_CTRL_216_DIS_GFX_216_CLK_MASK;
}

#define XPT_MISC (  BCHP_CLK_PM_CTRL_1_DIS_XPT_81M_CLK_MASK \
		 |  BCHP_CLK_PM_CTRL_1_DIS_XPT_54M_CLK_MASK \
		 |  BCHP_CLK_PM_CTRL_1_DIS_XPT_27M_CLK_MASK \
		 |  BCHP_CLK_PM_CTRL_1_DIS_XPT_40P5M_CLK_MASK \
		 |  BCHP_CLK_PM_CTRL_1_DIS_XPT_20P25M_CLK_MASK ); 

void
xpt_standby(void)
{
	SoC.clock->pm_ctrl	|= BCHP_CLK_PM_CTRL_DIS_XPT_108M_CLK_MASK;
	SoC.clock->pm_ctrl_216	|= BCHP_CLK_PM_CTRL_216_DIS_XPT_216_CLK_MASK;
	SoC.clock->pm_ctrl_1	|= XPT_MISC;
}

void
xpt_active(void)
{
	SoC.clock->pm_ctrl	&= ~BCHP_CLK_PM_CTRL_DIS_XPT_108M_CLK_MASK;
	SoC.clock->pm_ctrl_216	&= ~BCHP_CLK_PM_CTRL_216_DIS_XPT_216_CLK_MASK;
	SoC.clock->pm_ctrl_1	&= ~XPT_MISC;
}

void
shift_standby(void)
{
	SoC.clock->pm_ctrl	|= BCHP_CLK_PM_CTRL_DIS_SHIFT_108M_CLK_MASK;
	SoC.clock->pm_ctrl_216	|= BCHP_CLK_PM_CTRL_216_DIS_SHIFT_216_CLK_MASK;
}

void
shift_active(void)
{
	SoC.clock->pm_ctrl	&= ~BCHP_CLK_PM_CTRL_DIS_SHIFT_108M_CLK_MASK;
	SoC.clock->pm_ctrl_216	&= ~BCHP_CLK_PM_CTRL_216_DIS_SHIFT_216_CLK_MASK;

}

#define VIDEO_108M_MASK ( BCHP_CLK_PM_CTRL_DIS_VEC_108M_CLK_MASK \
	| BCHP_CLK_PM_CTRL_DIS_BVNM_108M_CLK_MASK \
	| BCHP_CLK_PM_CTRL_DIS_BVNE_108M_CLK_MASK \
	| BCHP_CLK_PM_CTRL_DIS_HDMI_108M_CLK_MASK )


#define VIDEO_216M_MASK ( BCHP_CLK_PM_CTRL_216_DIS_BVNE_216_CLK_MASK \
	| BCHP_CLK_PM_CTRL_216_DIS_BVNM_216_CLK_MASK \
	| BCHP_CLK_PM_CTRL_216_DIS_VEC_216_CLK_MASK )

#define VIDEO_162M_MASK (BCHP_CLK_PM_CTRL_2_DIS_VEC_DAC_162M_CLK_MASK \
	| BCHP_CLK_PM_CTRL_2_DIS_VEC_162M_CLK_MASK \
	| BCHP_CLK_PM_CTRL_2_DIS_HEXDAC_162M_CLK_MASK )

#define BCHP_MISC_DAC_BG_PWRDN (BCHP_MISC_DAC_BG_CTRL_REG_PWRDN_CORE_MASK \
	| BCHP_MISC_DAC_BG_CTRL_REG_PWRDN_AUX_MASK \
	| BCHP_MISC_DAC_BG_CTRL_REG_PWRDN_REFAMP_MASK )


#define DISABLE_DAC(x)  \
	if ((SoC.misc->dac##x##_ctrl_reg & BCHP_MISC_DAC##x##_CTRL_REG_PWRDN_CORE_MASK) == 0) {\
		SoC.misc->dac##x##_ctrl_reg |= BCHP_MISC_DAC##x##_CTRL_REG_PWRDN_CORE_MASK; \
		SoC.dac_status |= (1 << x); \
	} else \
		SoC.dac_status &= ~(1 << x); 
		

#define ENABLE_DAC(x)  \
	if (SoC.dac_status & (1 << x)) { \
		SoC.misc->dac##x##_ctrl_reg &= ~BCHP_MISC_DAC##x##_CTRL_REG_PWRDN_CORE_MASK; \
		SoC.dac_status &= ~(1 << x); \
	}
		

void
video_standby(void)
{

	if (( SoC.misc->dac_bg_ctrl_reg  & BCHP_MISC_DAC_BG_PWRDN) == 0) {
		SoC.misc->dac_bg_ctrl_reg |= BCHP_MISC_DAC_BG_PWRDN;
		SoC.dac_status |= 1;
	} else
		SoC.dac_status &= ~1;

	DISABLE_DAC(1);
	DISABLE_DAC(2);
	DISABLE_DAC(3);
	DISABLE_DAC(4);
	DISABLE_DAC(5);
	DISABLE_DAC(6);

	SoC.clock->pm_ctrl     |= VIDEO_108M_MASK;
	SoC.clock->pm_ctrl_216 |= VIDEO_216M_MASK;
	SoC.clock->pm_ctrl_2   |= VIDEO_162M_MASK;
}

void
video_active(void)
{
	SoC.clock->pm_ctrl     &= ~VIDEO_108M_MASK;
	SoC.clock->pm_ctrl_216 &= ~VIDEO_216M_MASK;
	SoC.clock->pm_ctrl_2   &= ~VIDEO_162M_MASK;

	ENABLE_DAC(1);
	ENABLE_DAC(2);
	ENABLE_DAC(3);
	ENABLE_DAC(4);
	ENABLE_DAC(5);
	ENABLE_DAC(6);

	if (SoC.dac_status & 1)  {
		SoC.misc->dac_bg_ctrl_reg &= ~BCHP_MISC_DAC_BG_PWRDN;
		SoC.dac_status &= ~1;
	}

}


#define AUDIO_108M_MASK ( BCHP_CLK_PM_CTRL_DIS_RPTD1_108M_CLK_MASK \
	| BCHP_CLK_PM_CTRL_DIS_RPTD0_108M_CLK_MASK \
	| BCHP_CLK_PM_CTRL_DIS_AIO_108M_CLK_MASK )


#define AUDIO_216M_MASK ( BCHP_CLK_PM_CTRL_216_DIS_RPTD0_216_CLK_MASK \
	| BCHP_CLK_PM_CTRL_216_DIS_RPTD1_216_CLK_MASK )

#define AUDIO_MASK (BCHP_CLK_PM_CTRL_2_DIS_RAP0_DSP_PROG_CLK_MASK | BCHP_CLK_PM_CTRL_2_DIS_RAP1_DSP_PROG_CLK_MASK)

void
audio_standby(void)
{
	SoC.clock->pm_ctrl     |= AUDIO_108M_MASK;
	SoC.clock->pm_ctrl_2   |= AUDIO_MASK;
	SoC.clock->pm_ctrl_216 |= AUDIO_216M_MASK;

}

void
audio_active(void)
{
	SoC.clock->pm_ctrl     &= ~AUDIO_108M_MASK;
	SoC.clock->pm_ctrl_2   &= ~AUDIO_MASK;
	SoC.clock->pm_ctrl_216 &= ~AUDIO_216M_MASK;
}

void 
audio_plls_standby(void)
{
	SoC.clock->aud0_ctrl &= ~BCHP_CLK_AUD0_CTRL_CLOCK_ENA_MASK;
	SoC.clock->aud0_ctrl |= BCHP_CLK_AUD0_CTRL_POWERDOWN_MASK;

}

void
audio_plls_active(void)
{
	SoC.clock->aud0_ctrl &= ~BCHP_CLK_AUD0_CTRL_POWERDOWN_MASK;
	SoC.clock->aud0_ctrl |= BCHP_CLK_AUD0_CTRL_CLOCK_ENA_MASK;

}

void
aio_standby(void)
{
	SoC.clock->pm_ctrl     |= BCHP_CLK_PM_CTRL_DIS_AIO_108M_CLK_MASK;
	SoC.clock->pm_ctrl_216 |= BCHP_CLK_PM_CTRL_216_DIS_AIO_216_CLK_MASK;
	SoC.clock->pm_ctrl_2   |= BCHP_CLK_PM_CTRL_2_DIS_AIO_162M_CLK_MASK;
}

void
aio_active(void)
{
	SoC.clock->pm_ctrl     &= ~BCHP_CLK_PM_CTRL_DIS_AIO_108M_CLK_MASK;
	SoC.clock->pm_ctrl_216 &= ~BCHP_CLK_PM_CTRL_216_DIS_AIO_216_CLK_MASK;
	SoC.clock->pm_ctrl_2   &= ~BCHP_CLK_PM_CTRL_2_DIS_AIO_162M_CLK_MASK;
}

#define HDMI_162M_MASK (BCHP_CLK_PM_CTRL_2_DIS_HDMI_VEC_162M_CLK_MASK \
	| BCHP_CLK_PM_CTRL_2_DIS_HDMI_MAX_CLK_MASK )


void
hdmi_standby(void)
{

	SoC.clock->pm_ctrl_1 |= BCHP_CLK_PM_CTRL_1_ENA_HDMI_LOW_PWR_MASK;
	SoC.clock->pm_ctrl_2 |= HDMI_162M_MASK;
	SoC.clock->pm_ctrl   |= BCHP_CLK_PM_CTRL_DIS_HDMI_108M_CLK_MASK;

	if (power_pm_test) 
	    SoC.clock->pm_ctrl_1 |= BCHP_CLK_PM_CTRL_1_DIS_HDMI_27M_CLK_MASK;
}


void
hdmi_active(void)
{

	SoC.clock->pm_ctrl_1 &= ~BCHP_CLK_PM_CTRL_1_ENA_HDMI_LOW_PWR_MASK;
	SoC.clock->pm_ctrl_2 &= ~HDMI_162M_MASK;
	SoC.clock->pm_ctrl   &= ~BCHP_CLK_PM_CTRL_DIS_HDMI_108M_CLK_MASK;

	if (power_pm_test) 
	    SoC.clock->pm_ctrl_1 &= ~BCHP_CLK_PM_CTRL_1_DIS_HDMI_27M_CLK_MASK;
}

void
enet_standby(int wol)
{
	volatile unsigned int *tstctl;

	if (wol == 0) {
                /* disable phy */
                tstctl = REG_TO_K1(BCHP_ENET_TOP_IUDMA_ENET_TSTCTL);
                *tstctl |= BCHP_ENET_TOP_IUDMA_ENET_TSTCTL_disable_ephy_MASK;

		SoC.clock->pm_ctrl_1	|= BCHP_CLK_PM_CTRL_1_DIS_ENET_25M_CLK_MASK;
	}

	SoC.clock->pm_ctrl	|= BCHP_CLK_PM_CTRL_DIS_ENET_108M_CLK_MASK;
	SoC.clock->pm_ctrl_216	|= BCHP_CLK_PM_CTRL_216_DIS_ENET_216_CLK_MASK;
}

void
enet_active(int wol)
{
	volatile unsigned int *tstctl;

	SoC.clock->pm_ctrl	&= ~BCHP_CLK_PM_CTRL_DIS_ENET_108M_CLK_MASK;
	SoC.clock->pm_ctrl_216	&= ~BCHP_CLK_PM_CTRL_216_DIS_ENET_216_CLK_MASK;

	if (wol == 0) {
		SoC.clock->pm_ctrl_1	&= ~BCHP_CLK_PM_CTRL_1_DIS_ENET_25M_CLK_MASK;

                /* enable  phy */
                tstctl = REG_TO_K1(BCHP_ENET_TOP_IUDMA_ENET_TSTCTL);
                *tstctl &= ~BCHP_ENET_TOP_IUDMA_ENET_TSTCTL_disable_ephy_MASK;
	}
}

void
hif_standby(void)
{

	SoC.clock->pm_ctrl	|= BCHP_CLK_PM_CTRL_DIS_HIF_108M_CLK_MASK;
	SoC.clock->pm_ctrl_216	|= BCHP_CLK_PM_CTRL_216_DIS_HIF_216_CLK_MASK;
}

void
hif_active(void)
{

	SoC.clock->pm_ctrl	&= ~BCHP_CLK_PM_CTRL_DIS_HIF_108M_CLK_MASK;
	SoC.clock->pm_ctrl_216	&= ~BCHP_CLK_PM_CTRL_216_DIS_HIF_216_CLK_MASK;
}

__locked_text void
sundry_standby(void)
{
#if 0	/*
	 * need clocks for jtag debug
	 * Consumes approx 1 mw
	 */
	SoC.clock->pm_ctrl_1	|= ( BCHP_CLK_PM_CTRL_1_DIS_BSP_9M_CLK_MASK
				   | BCHP_CLK_PM_CTRL_1_DIS_JTAG_9M_CLK_MASK );
#endif

#if 0
	SoC.clock->pm_ctrl_1	|=  BCHP_CLK_PM_CTRL_1_DIS_SUN_27M_CLK_MASK;
#endif

	SoC.clock->pm_ctrl_1	&= ~BCHP_CLK_PM_CTRL_1_DIS_SUN_27M_LOW_PWR_MASK;
//	SoC.clock->pm_ctrl_1	&= ~BCHP_CLK_PM_CTRL_1_DIS_SUN_108M_LOW_PWR_MASK;
	
}

__locked_text void
sundry_active(void)
{
	SoC.clock->pm_ctrl_1	|=  BCHP_CLK_PM_CTRL_1_DIS_SUN_108M_LOW_PWR_MASK;
	SoC.clock->pm_ctrl_1	|=  BCHP_CLK_PM_CTRL_1_DIS_SUN_27M_LOW_PWR_MASK;

#if 0
	SoC.clock->pm_ctrl_1	&=  ~BCHP_CLK_PM_CTRL_1_DIS_SUN_27M_CLK_MASK;
#endif

	SoC.clock->pm_ctrl_1	&= ~( BCHP_CLK_PM_CTRL_1_DIS_BSP_9M_CLK_MASK
				   | BCHP_CLK_PM_CTRL_1_DIS_JTAG_9M_CLK_MASK );

}
void
sdio_standby(void)
{
	SoC.clock->pm_ctrl_2   |= BCHP_CLK_PM_CTRL_2_DIS_SDIO_CLK_MASK;
}

void
sdio_active(void)
{
	SoC.clock->pm_ctrl_2   &= ~BCHP_CLK_PM_CTRL_2_DIS_SDIO_CLK_MASK;
}

#if defined (CONFIG_BCM_VIDE_OFE)
void
ofe_standby(int do_magic)
{
	unsigned int ruAlive;

	/*
	 * Prior to entry to this routine, PM code has invoked
	 * the suspend function of the vide_ofe driver. The driver
	 * suspend function has issued an ATA SLEEP command to the
	 * FE firmware, which has performed all of PM standby
	 * functions of the FE, including shutting down the OFE
	 * logic and all analog/digital power outputs.
	 */

	printk(KERN_DEBUG "%s: ENTER\n", __FUNCTION__);

	/*
	 * Save crucial registers.
	 * - scratch_1: CFE stores the chip rev in
	 *              Scratch1 register (checked
	 *              during probe).
	 * - dram_base: Need correct firmare window
	 * - dram_size: parameters to restore around
	 *              FE reset.
	 */
	SoC.fe_scratch_1         = *(volatile unsigned int *)REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_REG_SCRATCH_1);
	SoC.fe_reload_physaddr   = *(volatile unsigned int *)REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_REG_DRAM_OFFSET);
	SoC.fe_reload_windowsize = *(volatile unsigned int *)REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_REG_DRAM_RANGE);
	ruAlive                  = *(volatile unsigned int *)REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_REG_SEMAPHORE_2);
	printk(KERN_DEBUG "%s: Save chip rev:          0x%08x\n",
	       __FUNCTION__, SoC.fe_scratch_1);
	printk(KERN_DEBUG "%s: Save Reload PhysAddr:   0x%08x\n",
	       __FUNCTION__, SoC.fe_reload_physaddr);
	printk(KERN_DEBUG "%s: Save Reload WindowSize: 0x%08x\n",
	       __FUNCTION__, SoC.fe_reload_windowsize);
	printk(KERN_DEBUG "%s: Current ARM RUAlive:     0x%08x\n",
	       __FUNCTION__, ruAlive);

	/*
	 * Place ARM into reset
	 */
	printk(KERN_DEBUG "%s: Reset ARM\n", __FUNCTION__);
	*(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_REG_BRIDGE_CTL) |=
		BCHP_OFE_ARMCR4_BRIDGE_REG_BRIDGE_CTL_arm_soft_rst_MASK;

	/*
	 * Assert ARM_HALT_N (0 = ARM_HALT_N asserted)
	 */
	printk(KERN_DEBUG "%s: Assert ARM_HALT_N\n", __FUNCTION__);
	*(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_REG_ARM_HALT) = 0;

	if (ruAlive) {
		/*
		 * Clear the ARM RUAlive register
		 */
		printk(KERN_DEBUG "%s: Clear ARM RUAlive register\n", __FUNCTION__);
		*(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_REG_SEMAPHORE_2) = 0;
	}

	if (do_magic) {
		/*
		 * Perform the magic AHB Master Indirect register sequence
		 * - Should not be necessary if FE firmware does its job
		 *   correctly as part of SLEEP command execution. Leaving
		 *   this code here, but compiled out, just in case.
		 */
		printk(KERN_DEBUG "%s: Perform AHB Master Indirect register magic\n", __FUNCTION__);
		*(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_AHB_MASTER_REG_START) = 0x10000708;
		*(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_AHB_MASTER_REG_START + 4) = 0x02;
		SoC.AHB_Saved_1 = *(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_AHB_MASTER_REG_END);
		*(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_AHB_MASTER_REG_END) = 0xffffffff;

		*(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_AHB_MASTER_REG_START) = 0x100009f1;
		*(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_AHB_MASTER_REG_START + 4) = 0;
		SoC.AHB_Saved_2 = *(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_AHB_MASTER_REG_END);
		*(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_AHB_MASTER_REG_END) = 0x1;

		*(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_AHB_MASTER_REG_START) = 0x10000700;
		*(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_AHB_MASTER_REG_START + 4) = 0x02;
		SoC.AHB_Saved_3 = *(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_AHB_MASTER_REG_END);
		*(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_AHB_MASTER_REG_END) = 0xffffffff;
	}

	/*
	 * Stop clocks
	 */
	printk(KERN_DEBUG "%s: Stop clocks\n", __FUNCTION__);
	SoC.clock->pm_ctrl	|= (BCHP_CLK_PM_CTRL_DIS_OFE_DATAPATH_108M_CLK_MASK
				    | BCHP_CLK_PM_CTRL_DIS_OFE_DISCCTRL_108M_CLK_MASK);

	SoC.clock->pm_ctrl_216	|= (BCHP_CLK_PM_CTRL_216_DIS_OFE_DATAPATH_216_CLK_MASK
				    | BCHP_CLK_PM_CTRL_216_DIS_OFE_DISCCTRL_216_CLK_MASK);

	SoC.clock->pm_ctrl_2	|= (BCHP_CLK_PM_CTRL_2_DIS_OFE_DATAPATH_27M_CLK_MASK
				    | BCHP_CLK_PM_CTRL_2_DIS_OFE_DISCCTRL_27M_CLK_MASK
				    | BCHP_CLK_PM_CTRL_2_DIS_OFE_DATAPATH_67M_CLK_MASK
				    | BCHP_CLK_PM_CTRL_2_DIS_OFE_DISCCTRL_67M_CLK_MASK
				    | BCHP_CLK_PM_CTRL_2_DIS_OFE_DISCCTRL_162M_CLK_MASK);

	printk(KERN_DEBUG "%s: DONE\n", __FUNCTION__);
}

void
ofe_active(void)
{
	printk(KERN_DEBUG "%s: ENTER\n", __FUNCTION__);

	/*
	 * Restart clocks
	 */
	printk(KERN_DEBUG "%s: Restart clocks\n", __FUNCTION__);
	SoC.clock->pm_ctrl	&= ~(BCHP_CLK_PM_CTRL_DIS_OFE_DATAPATH_108M_CLK_MASK
				     | BCHP_CLK_PM_CTRL_DIS_OFE_DISCCTRL_108M_CLK_MASK);

	SoC.clock->pm_ctrl_216	&= ~(BCHP_CLK_PM_CTRL_216_DIS_OFE_DATAPATH_216_CLK_MASK
				     | BCHP_CLK_PM_CTRL_216_DIS_OFE_DISCCTRL_216_CLK_MASK);

	SoC.clock->pm_ctrl_2	&= ~(BCHP_CLK_PM_CTRL_2_DIS_OFE_DATAPATH_27M_CLK_MASK
				     | BCHP_CLK_PM_CTRL_2_DIS_OFE_DISCCTRL_27M_CLK_MASK
				     | BCHP_CLK_PM_CTRL_2_DIS_OFE_DATAPATH_67M_CLK_MASK
				     | BCHP_CLK_PM_CTRL_2_DIS_OFE_DISCCTRL_67M_CLK_MASK
				     | BCHP_CLK_PM_CTRL_2_DIS_OFE_DISCCTRL_162M_CLK_MASK);
#if 0
	/*
	 * Restore the magic AHB Master Indirect registers
	 * - Should not be necessary if FE firmware does its job
	 *   correctly as part of wakeup from SLEEP command execution
	 *   (by way of FE hard reset). Leaving this code here, but
	 *   compiled out, just in case.
	 */
	printk(KERN_DEBUG "%s: Restore AHB Master Indirect registers\n", __FUNCTION__);
	*(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_AHB_MASTER_REG_START) = 0x10000708;
	*(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_AHB_MASTER_REG_START + 4) = 0x02;
	*(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_AHB_MASTER_REG_END) = SoC.AHB_Saved_1;

	*(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_AHB_MASTER_REG_START) = 0x100009f1;
	*(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_AHB_MASTER_REG_START + 4) = 0;
	*(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_AHB_MASTER_REG_END) = SoC.AHB_Saved_2;

	*(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_AHB_MASTER_REG_START) = 0x10000700;
	*(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_AHB_MASTER_REG_START + 4) = 0x02;
	*(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_AHB_MASTER_REG_END) = SoC.AHB_Saved_3;
#endif


	/*
	 * Enable the required ARM control bits
	 */
	printk(KERN_DEBUG "%s: Enable ARM Control Bits\n", __FUNCTION__);
	*(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_REG_ARM_CTL) |=
		(BCHP_OFE_ARMCR4_BRIDGE_REG_ARM_CTL_INITRAMA_MASK |
		 BCHP_OFE_ARMCR4_BRIDGE_REG_ARM_CTL_CR4_DBGEN_MASK |
		 BCHP_OFE_ARMCR4_BRIDGE_REG_ARM_CTL_DAP_DBGEN_MASK);
	
	/*
	 * Re-install the FE firmware base address,
	 * ARM window size and Scratch_1 register (Product
	 * Revision loaded by CFE at boot time.
	 */
	printk(KERN_DEBUG "%s: Restore chip rev:          0x%08x\n",
	       __FUNCTION__, SoC.fe_scratch_1);
	printk(KERN_DEBUG "%s: Restore Reload PhysAddr:   0x%08x\n",
	       __FUNCTION__, SoC.fe_reload_physaddr);
	printk(KERN_DEBUG "%s: Restore Reload WindowSize: 0x%08x\n",
	       __FUNCTION__, SoC.fe_reload_windowsize);
	*(volatile unsigned int *)REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_REG_SCRATCH_1)   = SoC.fe_scratch_1;
	*(volatile unsigned int *)REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_REG_DRAM_OFFSET) = SoC.fe_reload_physaddr;
	*(volatile unsigned int *)REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_REG_DRAM_RANGE)  = SoC.fe_reload_windowsize;

	/*
	 * Deassert ARM_HALT_N (1 = ARM_HALT_N deasserted)
	 */
	printk(KERN_DEBUG "%s: Deassert ARM_HALT_N, delay 10 msec\n", __FUNCTION__);
	*(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_REG_ARM_HALT) = 1;
	msleep(10);

	/*
	 * Take the ARM out of reset
	 */
	printk(KERN_DEBUG "%s: Take ARM out of reset\n", __FUNCTION__);
	*(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_REG_BRIDGE_CTL) &=
		~BCHP_OFE_ARMCR4_BRIDGE_REG_BRIDGE_CTL_arm_soft_rst_MASK;

	printk(KERN_DEBUG "%s: Final FE Bridge Status: 0x%08x\n",
	       __FUNCTION__,
	       *(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_REG_BRIDGE_STS));
	printk(KERN_DEBUG "%s: Final FE Bridge Control: 0x%08x\n",
	       __FUNCTION__,
	       *(volatile unsigned int *) REG_TO_K1(BCHP_OFE_ARMCR4_BRIDGE_REG_BRIDGE_CTL));

	printk(KERN_DEBUG "%s: DONE\n", __FUNCTION__);
}
#endif // CONFIG_BCM_VIDE_OFE


/*
 * These routines are called from the driver and not from the
 * all_active() and all_standby() routines because their cores
 * need to be active until late suspend.
 */
void
usb_active(void)
{
	SoC.clock->pm_ctrl_216	&= ~BCHP_CLK_PM_CTRL_216_DIS_USB_216_CLK_MASK;
	SoC.clock->pm_ctrl	&= ~BCHP_CLK_PM_CTRL_DIS_USB_108M_CLK_MASK;
	SoC.usb_ctrl->pll_ctl_1	&= ~BCHP_USB_CTRL_PLL_CTL_1_XTAL_PWRDWN_MASK;
	SoC.usb_ctrl->pll_ctl_1	|= BCHP_USB_CTRL_PLL_CTL_1_PLL_PWRDWNB_MASK;

	SoC.usb_ctrl->utmi_ctl_1 |= ( BCHP_USB_CTRL_UTMI_CTL_1_PHY_PWDNB_MASK
				    | BCHP_USB_CTRL_UTMI_CTL_1_UTMI_PWDNB_MASK
				    | BCHP_USB_CTRL_UTMI_CTL_1_PHY1_PWDNB_MASK
				    | BCHP_USB_CTRL_UTMI_CTL_1_UTMI1_PWDNB_MASK );
}

void
usb_standby(void)
{
	SoC.usb_ctrl->utmi_ctl_1&= ~( BCHP_USB_CTRL_UTMI_CTL_1_PHY_PWDNB_MASK
				    | BCHP_USB_CTRL_UTMI_CTL_1_UTMI_PWDNB_MASK
				    | BCHP_USB_CTRL_UTMI_CTL_1_PHY1_PWDNB_MASK
				    | BCHP_USB_CTRL_UTMI_CTL_1_UTMI1_PWDNB_MASK );

	SoC.usb_ctrl->pll_ctl_1	&= ~BCHP_USB_CTRL_PLL_CTL_1_PLL_PWRDWNB_MASK;
	SoC.usb_ctrl->pll_ctl_1	|= BCHP_USB_CTRL_PLL_CTL_1_XTAL_PWRDWN_MASK;
	SoC.clock->pm_ctrl	|= BCHP_CLK_PM_CTRL_DIS_USB_108M_CLK_MASK;
	SoC.clock->pm_ctrl_216	|= BCHP_CLK_PM_CTRL_216_DIS_USB_216_CLK_MASK;
}


void
all_active(void)
{
	printk(KERN_DEBUG "%s enter\n", __func__);

	if (power_pm_test) {
	    xpt_active();
	    avd_active();
	    m2mc_active();
	    shift_active();
	    audio_plls_active();
	    audio_active();
	    aio_active();
	    video_active();
	    hdmi_active();
	}

	usb_active();
	sdio_active();

#if defined (CONFIG_BCM_VIDE_OFE)
	ofe_active();
#endif

	printk(KERN_DEBUG "%s exit\n", __func__);
}


void
all_standby(void)
{

	printk(KERN_DEBUG "%s enter\n", __func__);
	
	if (power_pm_test) {
	    hdmi_standby();
	    video_standby();
	    aio_standby();
	    audio_standby();
	    audio_plls_standby();
	    shift_standby();
	    m2mc_standby();
	    avd_standby();
	    xpt_standby();
	}

	usb_standby();
	sdio_standby();

#if defined (CONFIG_BCM_VIDE_OFE)
	ofe_standby(0);
#endif
	printk(KERN_DEBUG "%s exit\n", __func__);
}

