/*
 *     Copyright (c) 1999-2008, Broadcom Corporation
 *     All Rights Reserved
 *     Confidential Property of Broadcom Corporation
 *
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
 *
 */

typedef unsigned int reg_t;

typedef struct ddr23_ctl {
	/*
	 * BCHP_DDR23_CTL_REGS_0_REVISION           0x01800000 
	 * BCHP_DDR23_CTL_REGS_1_REVISION           0x01804000
	 */
	reg_t	revision;		/* 0x01800000  DDR23 Controller revision register */
	reg_t	ctl_status;		/* 0x01800004  DDR23 Controller status register */
	reg_t	unused_0x8;		/* 0x01800008  Unused */
	reg_t	unused_0xc;		/* 0x0180000c  Unused */
	reg_t	params1;		/* 0x01800010  DDR23 Controller Configuration Set #1 */
	reg_t	params2;		/* 0x01800014  DDR23 Controller Configuration Set #2 */
	reg_t	params3;		/* 0x01800018  DDR23 Controller Configuration Set #3 */
	reg_t	refresh;		/* 0x0180001c  DDR23 Controller Automated Refresh Configuration */
	reg_t	refresh_cmd;		/* 0x01800020  Host Initiated Refresh Control */
	reg_t	precharge_cmd;		/* 0x01800024  Host Initiated Precharge Control */
	reg_t	load_mode_cmd;		/* 0x01800028  Host Initiated Load Mode Control */
	reg_t	load_emode_cmd;		/* 0x0180002c  Host Initiated Load Extended Mode Control */
	reg_t	load_emode2_cmd;	/* 0x01800030  Host Initiated Load Extended Mode #2 Control */
	reg_t	load_emode3_cmd;	/* 0x01800034  Host Initiated Load Extended Mode #3 Control */
	reg_t	zq_calibrate;		/* 0x01800038  Host Initiated ZQ Calibration Cycle */
	reg_t	cmd_status;		/* 0x0180003c  Host Command Interface Status */
	reg_t	latency;		/* 0x01800040  DDR2 Controller Access Latency Control */
	reg_t	semaphore0;		/* 0x01800044  Semaphore Register #0 */
	reg_t	semaphore1;		/* 0x01800048  Semaphore Register #1 */
	reg_t	semaphore2;		/* 0x0180004c  Semaphore Register #2 */
	reg_t	semaphore3;		/* 0x01800050  Semaphore Register #3 */
	reg_t	unused_0x54;		/* 0x01800054  Unused */
	reg_t	scratch;		/* 0x01800058  Scratch Register */
	reg_t	unused_0x5c;		/* 0x0180005c  Unused */
	reg_t	stripe_width;		/* 0x01800060  Stripe Width */
	reg_t	unused_0x64;		/* 0x01800064  Unused */
	reg_t	unused_0x68;		/* 0x01800068  Unused */
	reg_t	unused_0x6c;		/* 0x0180006c  Unused */
	reg_t	stripe_height_0;	/* 0x01800070  Stripe Height for picture buffers 0 through 23 */
	reg_t	stripe_height_1;	/* 0x01800074  Stripe Height for picture buffers 24 through 27 */
	reg_t	stripe_height_2;	/* 0x01800078  Stripe Height for picture buffers 28 through 31 */
	reg_t	unused_0x7c;		/* 0x0180007c  Unused */
	reg_t	unused_0x80;		/* 0x01800080  Unused */
	reg_t	unused_0x84;		/* 0x01800084  Unused */
	reg_t	unused_0x88;		/* 0x01800088  Unused */
	reg_t	unused_0x8c;		/* 0x0180008c  Unused */
	reg_t	unused_0x90;		/* 0x01800090  Unused */
	reg_t	unused_0x94;		/* 0x01800094  Unused */
	reg_t	unused_0x98;		/* 0x01800098  Unused */
	reg_t	unused_0x9c;		/* 0x0180009c  Unused */
	reg_t	lbist_cntrl;		/* 0x018000a0  Logic-Bist Control and Status */
	reg_t	lbist_seed;		/* 0x018000a4  Logic-Bist PRPG Seed Value */
	reg_t	lbist_status;		/* 0x018000a8  Logic-Bist Control and Status */
	reg_t	unused_0xac;		/* 0x018000ac  Unused */
	reg_t	vdl_ctl;		/* 0x018000b0  Dynamic VDL Changes Control */
	reg_t	vdl_adr_base;		/* 0x018000b4  Dynamic VDL Changes Base Address */
	reg_t	vdl_adr_end;		/* 0x018000b8  Dynamic VDL Changes End Address */
	reg_t	unused_0xbc;		/* 0x018000bc  Unused */
	reg_t	pmon_ctl;		/* 0x018000c0  Performance Monitoring Control */
	reg_t	pmon_period;		/* 0x018000c4  Performance Monitoring Period Control */
	reg_t	pmon_cycle_cnt;		/* 0x018000c8  Performance Monitoring Active Cycles Count */
	reg_t	pmon_idle_cnt;		/* 0x018000cc  Performance Monitoring Idle Cycles Count */
	reg_t	pmon_rd_cnt1;		/* 0x018000d0  Performance Monitoring Data Channel #1 Read Accesses Count */
	reg_t	pmon_rd_cnt2;		/* 0x018000d4  Performance Monitoring Data Channel #2 Read Accesses Count */
	reg_t	pmon_rd_cnt3;		/* 0x018000d8  Performance Monitoring Data Channel #3 Read Accesses Count */
	reg_t	pmon_wr_cnt1;		/* 0x018000dc  Performance Monitoring Data Channel #1 Write Accesses Count */
	reg_t	pmon_wr_cnt2;		/* 0x018000e0  Performance Monitoring Data Channel #2 Write Accesses Count */
	reg_t	pmon_wr_cnt3;		/* 0x018000e4  Performance Monitoring Data Channel #3 Write Accesses Count */
	reg_t	ddr_tm;			/* 0x018000e8  RAM Macro TM Control */
} ddr23_ctl_t;

typedef struct ddr_phy_control {
	/*
	BCHP_DDR23_PHY_CONTROL_REGS_0_REVISION   0x01802000
	BCHP_DDR23_PHY_CONTROL_REGS_1_REVISION   0x01806000
	*/
	reg_t	revision;		/* 0x01802000  Address & Control revision register */
	reg_t	clk_pm_ctrl;		/* 0x01802004  PHY clock power management control register */
	reg_t	unused_0x8;		/* 0x01802008  Unused */
	reg_t	unused_0xc;		/* 0x0180200c  Unused */
	reg_t	pll_status;		/* 0x01802010  PHY PLL status register */
	reg_t	pll_config;		/* 0x01802014  PHY PLL configuration register */
	reg_t	pll_pre_divider;	/* 0x01802018  PHY PLL pre-divider control register */
	reg_t	pll_divider;		/* 0x0180201c  PHY PLL divider control register */
	reg_t	pll_control1;		/* 0x01802020  PHY PLL analog control register #1 */
	reg_t	pll_control2;		/* 0x01802024  PHY PLL analog control register #2 */
	reg_t	pll_ss_en;		/* 0x01802028  PHY PLL spread spectrum config register */
	reg_t	pll_ss_cfg;		/* 0x0180202c  PHY PLL spread spectrum config register */
	reg_t	static_vdl_override;	/* 0x01802030  Address & Control VDL static override control register */
	reg_t	dynamic_vdl_override;	/* 0x01802034  Address & Control VDL dynamic override control register */
	reg_t	idle_pad_control;	/* 0x01802038  Idle mode SSTL pad control register */
	reg_t	zq_pvt_comp_ctl;	/* 0x0180203c  PVT Compensation control and status register */
	reg_t	drive_pad_ctl;		/* 0x01802040  SSTL pad drive characteristics control register */
	reg_t	clock_reg_control;	/* 0x01802044  Clock Regulator control register */
} ddr_phy_control_t;

#define NUM_DDR_LANES	8

/* from the rdb */
#define BCHP_DDR23_PHY_BYTE_LANE_0_0_REVISION    0x01802400 /* Byte lane revision register */
#define BCHP_DDR23_PHY_BYTE_LANE_0_1_REVISION    0x01806400 /* Byte lane revision register */
#define BCHP_DDR23_PHY_BYTE_LANE_1_0_REVISION    0x01802300 /* Byte lane revision register */
#define BCHP_DDR23_PHY_BYTE_LANE_1_1_REVISION    0x01806300 /* Byte lane revision register */
#define BCHP_DDR23_PHY_BYTE_LANE_2_0_REVISION    0x01802200 /* Byte lane revision register */
#define BCHP_DDR23_PHY_BYTE_LANE_2_1_REVISION    0x01806200 /* Byte lane revision register */
#define BCHP_DDR23_PHY_BYTE_LANE_3_0_REVISION    0x01802100 /* Byte lane revision register */
#define BCHP_DDR23_PHY_BYTE_LANE_3_1_REVISION    0x01806100 /* Byte lane revision register */

enum ddr_lanes{
	Lane_0_0 = 0,
	Lane_0_1,
	Lane_1_0,
	Lane_1_1,
	Lane_2_0,
	Lane_2_1,
	Lane_3_0,
	Lane_3_1,
};
	
typedef struct ddr_phy_lane {
	reg_t	revision;		/* 0x01802400  Byte lane revision register */
	reg_t	vdl_calibrate;		/* 0x01802404  Byte lane VDL calibration control register */
	reg_t	vdl_status;		/* 0x01802408  Byte lane VDL calibration status register */
	reg_t	unused_0xc;		/* 0x0180240c  Unused */
	reg_t	vdl_override_0;		/* 0x01802410  Read DQSP VDL static override control register */
	reg_t	vdl_override_1;		/* 0x01802414  Read DQSN VDL static override control register */
	reg_t	vdl_override_2;		/* 0x01802418  Read Enable VDL static override control register */
	reg_t	vdl_override_3;		/* 0x0180241c  Write data and mask VDL static override control register */
	reg_t	vdl_override_4;		/* 0x01802420  Read DQSP VDL dynamic override control register */
	reg_t	vdl_override_5;		/* 0x01802424  Read DQSN VDL dynamic override control register */
	reg_t	vdl_override_6;		/* 0x01802428  Read Enable VDL dynamic override control register */
	reg_t	vdl_override_7;		/* 0x0180242c  Write data and mask VDL dynamic override control register */
	reg_t	read_control;		/* 0x01802430  Byte Lane read channel control register */
	reg_t	read_fifo_status;	/* 0x01802434  Read fifo status register */
	reg_t	read_fifo_clear;	/* 0x01802438  Read fifo status clear register */
	reg_t	idle_pad_control;	/* 0x0180243c  Idle mode SSTL pad control register */
	reg_t	drive_pad_ctl;		/* 0x01802440  SSTL pad drive characteristics control register */
	reg_t	clock_pad_disable;	/* 0x01802444  Clock pad disable register */
	reg_t	wr_preamble_mode;	/* 0x01802448  Write cycle preamble control register */
	reg_t	clock_reg_control;	/* 0x0180244c  Clock Regulator control register */
} ddr_phy_lane_t;

typedef struct art {
	reg_t	revision;		/* 0x00041000  Autonomous Register Transfer (ART) Revision ID */
	reg_t	config;			/* 0x00041004  Various Configuration Settings */
	reg_t	status;			/* 0x00041008  General Hardware Status */
	reg_t	unused_0xc;		/* 0x0004100c  Unused */
	reg_t	wakeup_control;		/* 0x00041010  ART Wakeup Event Control */
	reg_t	sleep_control;		/* 0x00041014  ART Sleep Event Control */
	reg_t	sequence_trigger;	/* 0x00041018  ART Sequence Trigger Manual Control */
	reg_t	sequence_status;	/* 0x0004101c  ART Sequencer Status */
	reg_t	codestore_pointer;	/* 0x00041020  Address for Access to Code Store Memory */
	reg_t	codestore_data;		/* 0x00041024  Data Word for Access to Code Store Memory */
	reg_t	codestore_tm;		/* 0x00041028  Code Store Memory Test Mode Control */
	reg_t	debug;			/* 0x0004102c  ART Debug Output */
} art_t;

typedef struct clk_control  {
	reg_t	revision;		/* 0x00040000  clock_gen Revision register */
	reg_t	pm_ctrl;		/* 0x00040004  Software power management control to turn off 108 MHz clocks */
	reg_t	pm_ctrl_1;		/* 0x00040008  Software power management control to turn off various clocks */
	reg_t	pm_ctrl_2;		/* 0x0004000c  Software power management control to turn off or select various clocks */
	reg_t	pm_ctrl_216;		/* 0x00040010  Software power management control to turn off 216 MHz clocks */
	reg_t	unused_0x14;		/* 0x00040014  Unused */
	reg_t	unused_0x18;		/* 0x00040018  Unused */
	reg_t	wakeup_status;		/* 0x0004001c  Hardware power management wakeup status. */
	reg_t	wakeup_mask;		/* 0x00040020  Hardware power management wakeup masks. */
	reg_t	critical_ctrl;		/* 0x00040024  Hardware power management critical control. */
	reg_t	critical_status;	/* 0x00040028  Hardware power management critical status. */
	reg_t	unused_0x2c;		/* 0x0004002c  Unused */
	reg_t	unused_0x30;		/* 0x00040030  Unused */
	reg_t	unused_0x34;		/* 0x00040034  Unused */
	reg_t	unused_0x38;		/* 0x00040038  Unused */
	reg_t	regulator_2p5_volts;	/* 0x0004003c  2.5V Regulator Voltage Adjustment */
	reg_t	temp_mon_ctrl;		/* 0x00040040  Temperature monitor control. */
	reg_t	temp_mon_status;	/* 0x00040044  Temperature monitor status. */
	reg_t	unused_0x48;		/* 0x00040048  Unused */
	reg_t	unused_0x4c;		/* 0x0004004c  Unused */
	reg_t	unused_0x50;		/* 0x00040050  Unused */
	reg_t	unused_0x54;		/* 0x00040054  Unused */
	reg_t	unused_0x58;		/* 0x00040058  Unused */
	reg_t	unused_0x5c;		/* 0x0004005c  Unused */
	reg_t	unused_0x60;		/* 0x00040060  Unused */
	reg_t	unused_0x64;		/* 0x00040064  Unused */
	reg_t	unused_0x68;		/* 0x00040068  Unused */
	reg_t	unused_0x6c;		/* 0x0004006c  Unused */
	reg_t	scratch;		/* 0x00040070  clock_gen  Scratch register */
} clk_control_t;

typedef struct pri_arbiter_control {
	reg_t	conc_ctl;		/* 0x0040bb00  Client concentrator bus (CCB) address space control */
	reg_t	refresh_ctl_0;		/* 0x0040bb04  Refresh client control for ddr interface #0 */
	reg_t	refresh_ctl_1;		/* 0x0040bb08  Refresh client control for ddr interface #1 */
	reg_t	wr_timeout_ctl_0;	/* 0x0040bb0c  Write timeout control for ddr interface #0 */
	reg_t	wr_timeout_ctl_1;	/* 0x0040bb10  Write timeout control for ddr interface #1 */
	reg_t	wr_timeout_sts_0;	/* 0x0040bb14  Write timeout status for ddr interface #0 */
	reg_t	wr_timeout_sts_1;	/* 0x0040bb18  Write timeout status for ddr interface #1 */
	reg_t	wr_timeout_clr_0;	/* 0x0040bb1c  Write timeout status clear for ddr interface #0 */
	reg_t	wr_timeout_clr_1;	/* 0x0040bb20  Write timeout status clear for ddr interface #1 */
	reg_t	wr_timeout_bad_conc;	/* 0x0040bb24  Write timeout bad concentrators */
	reg_t	perf_mon_ctl;		/* 0x0040bb28  Performance Monitor Control */
	reg_t	perf_mon_clr;		/* 0x0040bb2c  Performance Monitor Clear Counts */
	reg_t	perf_mon_events_0;	/* 0x0040bb30  Performance Monitor Events for ddr interface #0 */
	reg_t	perf_mon_events_1;	/* 0x0040bb34  Performance Monitor Events for ddr interface #1 */
	reg_t	perf_mon_sum_0;		/* 0x0040bb38  Performance Monitor Sum of Latencies for ddr interface #0 */
	reg_t	perf_mon_sum_1;		/* 0x0040bb3c  Performance Monitor Sum of Latencies for ddr interface #1 */
	reg_t	perf_mon_size_sum_0;	/* 0x0040bb40  Performance Monitor Sum of Request Sizes for ddr interface #0 */
	reg_t	perf_mon_size_sum_1;	/* 0x0040bb44  Performance Monitor Sum of Request Sizes for ddr interface #1 */
	reg_t	perf_mon_max_0;		/* 0x0040bb48  Performance Monitor Maximum Latency for ddr interface #0 */
	reg_t	perf_mon_max_1;		/* 0x0040bb4c  Performance Monitor Maximum Latency for ddr interface #1 */
	reg_t	perf_mon_access_0a;	/* 0x0040bb50  Performance Monitor Client Access bits 31:0 for ddr interface #0 */
	reg_t	perf_mon_access_0b;	/* 0x0040bb54  Performance Monitor Client Access bits 61:32 for ddr interface #0 */
	reg_t	perf_mon_access_1a;	/* 0x0040bb58  Performance Monitor Client Access bits 31:0 for ddr interface #1 */
	reg_t	perf_mon_access_1b;	/* 0x0040bb5c  Performance Monitor Client Access bits 61:32 for ddr interface #1 */
} pri_arbiter_control_t;

typedef struct wakeup_intr2 {
	reg_t	status;			/* 0x00400f80  Wake-Up Status Register */
	reg_t	set;			/* 0x00400f84  Wake-Up Set Register */
	reg_t	clear;			/* 0x00400f88  Wake-Up Clear Register */
	reg_t	mask_status;		/* 0x00400f8c  Wake-Up Mask Status Register */
	reg_t	mask_set;		/* 0x00400f90  Wake-Up Mask Set Register */
	reg_t	mask_clear;		/* 0x00400f94  Wake-Up Mask Clear Register */
} wakeup_intr2_t;

typedef struct misc {
	reg_t	misc_revision_id;	/* 0x00180900  Revision ID Register */
	reg_t	delay_reg;		/* 0x00180904  DELAY Register for DACs */
	reg_t	out_mux;		/* 0x00180908  Output muxing */
	reg_t	dac0_1_const;		/* 0x0018090c  DAC0 & DAC1 Const Vals */
	reg_t	dac2_3_const;		/* 0x00180910  DAC2 & DAC3 Const Vals */
	reg_t	dac4_5_const;		/* 0x00180914  DAC4 & DAC5 Const Vals */
	reg_t	out_ctrl;		/* 0x00180918  Output Control */
	reg_t	sec_co_ch0_sa_status;	/* 0x0018091c  SEC Component-Only Channel 0 SA Status Register */
	reg_t	sec_co_ch1_sa_status;	/* 0x00180920  SEC Component-Only Channel 1 SA Status Register */
	reg_t	sec_co_ch2_sa_status;	/* 0x00180924  SEC Component-Only Channel 2 SA Status Register */
	reg_t	sec_co_sa_config;	/* 0x00180928  SEC Component-Only SA Config Register */
	reg_t	unused_0x2c;		/* 0x0018092c  Unused */
	reg_t	unused_0x30;		/* 0x00180930  Unused */
	reg_t	sync_delay_reg;		/* 0x00180934  DELAY Register for Digital Sync */
	reg_t	dvi_sa_config;		/* 0x00180938  DVI SA Configuration Register */
	reg_t	dvi_sa_status;		/* 0x0018093c  DVI SA Status Register */ 
	reg_t	prim_master_sel;	/* 0x00180940  Master Select for Primary VEC path */
	reg_t	sec_master_sel;		/* 0x00180944  Master Select for Secondary VEC path */
	reg_t	unused_0x48;		/* 0x00180948  Unused */
	reg_t	dvi_master_sel;		/* 0x0018094c  Master Select for DVI VEC path */
	reg_t	unused_0x50;		/* 0x00180950  Unused */
	reg_t	dac_bg_ctrl_reg;	/* 0x00180954  HEPDAC Bandgap Control Register */
	reg_t	dac1_ctrl_reg;		/* 0x00180958  HEPDAC1 Control Register */
	reg_t	dac2_ctrl_reg;		/* 0x0018095c  HEPDAC2 Control Register */
	reg_t	dac3_ctrl_reg;		/* 0x00180960  HEPDAC3 Control Register */
	reg_t	dac4_ctrl_reg;		/* 0x00180964  HEPDAC4 Control Register */
	reg_t	dac5_ctrl_reg;		/* 0x00180968  HEPDAC5 Control Register */
	reg_t	dac6_ctrl_reg;		/* 0x0018096c  HEPDAC6 Control Register */
	reg_t	unused_0x70;		/* 0x00180970  Unused */
	reg_t	dvi_dithering;		/* 0x00180974  DVI Dithering Register */
	reg_t	prim_ch0_sa_status;	/* 0x00180978  PRIM Channel 0 SA Status Register */
	reg_t	prim_ch1_sa_status;	/* 0x0018097c  PRIM Channel 1 SA Status Register */
	reg_t	prim_ch2_sa_status;	/* 0x00180980  PRIM Channel 2 SA Status Register */
	reg_t	prim_sa_config;		/* 0x00180984  PRIM SA Config Register */
} misc_t;

typedef struct clk_vcxo_ctl {
	reg_t	misc;			/* 0x00040810 VCXO block output clock selection */
} clk_vcxo_ctl_t;

typedef struct vcxo_ctl {
	reg_t	pll0_ctrl;		/* 0x00041200  Main PLL0 reset, enable, and powerdown */
	reg_t	pci_div;		/* 0x00041204  Main PLL0 PCI out divider settings */
	reg_t	pll0_lock_cnt;		/* 0x00041208  Main PLL0 Lock Counter */
	reg_t	pll1_ctrl;		/* 0x0004120c  Main PLL1 reset, enable, and powerdown */
	reg_t	ofe_div;		/* 0x00041210  Main PLL1 OFE out divider settings */
	reg_t	sdio_div;		/* 0x00041214  Main PLL1 SDIO out divider settings */
	reg_t	sc_div;			/* 0x00041218  Main PLL1 SC out divider settings */
	reg_t	pll1_lock_cnt;		/* 0x0004121c  Main PLL1 Lock Counter */
	reg_t	mips_ctrl;		/* 0x00041220  MIPS PLL reset, enable, powerdown, and control */
	reg_t	mips_div;		/* 0x00041224  MIPS PLL divider settings */
	reg_t	mips_lock_cnt;		/* 0x00041228  MIPS PLL Lock Counter */
	reg_t	mips_pll_ctrl_lo;	/* 0x0004122c  MIPS pll_ctrl low 32 bits */
	reg_t	auddsp_ctrl;		/* 0x00041230  Audio DSP PLL reset, enable, powerdown, and control */
	reg_t	auddsp_div;		/* 0x00041234  Audio DSP PLL divider settings */
	reg_t	auddsp_lock_cnt;	/* 0x00041238  Audio DSP PLL Lock Counter */
	reg_t	auddsp_pll_ctrl_lo;	/* 0x0004123c  Audio DSP pll_ctrl low 32 bits */
	reg_t	avd_ctrl;		/* 0x00041240  AVD PLL reset, enable, powerdown, and control */
	reg_t	avd_div;		/* 0x00041244  AVD PLL divider settings */
	reg_t	avd_lock_cnt;		/* 0x00041248  AVD PLL Lock Counter */
	reg_t	avd_pll_ctrl_lo;	/* 0x0004124c  AVD pll_ctrl low 32 bits */
	reg_t	vc0_ctrl;		/* 0x00041250  VCXO 0 reset, enable, powerdown, and mode */
	reg_t	vc0_div;		/* 0x00041254  VCXO 0 PLL divider settings */
	reg_t	vc0_lock_cnt;		/* 0x00041258  VCXO 0 Lock Counter */
	reg_t	vc0_pll_ctrl_lo;	/* 0x0004125c  VCXO 0 pll_ctrl low 32 bits */
	reg_t	ac0_ctrl;		/* 0x00041260  Audio PLL 0 reset, enable, and powerdown */
	reg_t	ac0_lock_cnt;		/* 0x00041264  Audio PLL 0 Lock Counter */
	reg_t	unused_0x68;		/* 0x00041268  Unused */
	reg_t	unused_0x6c;		/* 0x0004126c  Unused */
	reg_t	unused_0x70;		/* 0x00041270  Unused */
	reg_t	unused_0x74;		/* 0x00041274  Unused */
	reg_t	unused_0x78;		/* 0x00041278  Unused */
	reg_t	unused_0x7c;		/* 0x0004127c  Unused */
	reg_t	lock;			/* 0x00041280  PLL lock status */
	reg_t	lock_cntr_reset;	/* 0x00041284  PLL Lock Counter Resets */
	reg_t	unused_0x88;		/* 0x00041288  Unused */
	reg_t	unused_0x8c;		/* 0x0004128c  Unused */
	reg_t	pll_test_sel;		/* 0x00041290  PLL core test select */
	reg_t	unused_0x94;		/* 0x00041294  Unused */
	reg_t	unused_0x98;		/* 0x00041298  Unused */
	reg_t	unused_0x9c;		/* 0x0004129c  Unused */
	reg_t	gpio_pad_ctrl;		/* 0x000412a0  GPIO pad control */
	reg_t	dvo_pad_ctrl;		/* 0x000412a4  DVO pad control */
	reg_t	i2c_pad_ctrl;		/* 0x000412a8  I2C pad control */
	reg_t	ofe_pad_ctrl;		/* 0x000412ac  OFE LVDS pad control */
} vcxo_ctl_t;


typedef struct bsp_glb_ctl {
	reg_t	glb_revision;		/* 0x0030b000  Revision Register */
	reg_t	glb_timer;		/* 0x0030b004  Timer */
	reg_t	glb_irdy;		/* 0x0030b008  Input Command Buffer Ready */
	reg_t	glb_ordy;		/* 0x0030b00c  Output Command Buffer Ready */
	reg_t	glb_host_intr_status;	/* 0x0030b010  Host CPU Interrupt Status */
	reg_t	glb_host_intr_en;	/* 0x0030b014  Host CPU Interrupt Enable */
	reg_t	unused_0x18;		/* 0x0030b018  Unused */
	reg_t	unused_0x1c;		/* 0x0030b01c  Unused */
	reg_t	glb_oload2;		/* 0x0030b020  Output Command Buffer 2 Loaded */
	reg_t	glb_oload1;		/* 0x0030b024  Output Command Buffer 1 Loaded */
	reg_t	glb_iload2;		/* 0x0030b028  Input Command Buffer 2 Loaded */
	reg_t	glb_iload1;		/* 0x0030b02c  Input Command Buffer 1 Loaded */
	reg_t	otp_tp_sel;		/* 0x0030b030  OTP Wrapper Test Output Port Selection Register */
	reg_t	host_to_bsp_general_intr; /* 0x0030b034  HOST to BSP Processor
							General Interrupt Trigger Register */
	reg_t	otp_wdata0;		/* 0x0030b038  OTP Write Data */
	reg_t	otp_wdata1;		/* 0x0030b03c  OTP Write Data */
	reg_t	otp_wdata2;		/* 0x0030b040  OTP Write Data */
	reg_t	otp_wdata3;		/* 0x0030b044  OTP Write Data */
	reg_t	otp_wdata4;		/* 0x0030b048  OTP Write Data */
	reg_t	otp_wdata5;		/* 0x0030b04c  OTP Write Data */
	reg_t	otp_wdata6;		/* 0x0030b050  OTP Write Data */
	reg_t	otp_wdata7;		/* 0x0030b054  OTP Write Data */
	reg_t	glb_oload_cr;		/* 0x0030b058  Challenge/Response Output Command Buffer Loaded */
	reg_t	glb_iload_cr;		/* 0x0030b05c  Challenge/Response Input Command Buffer Loaded */
	reg_t	glb_mem0;		/* 0x0030b060  TM Memory Control Register */
	reg_t	glb_mem1;		/* 0x0030b064  TM Memory Control Register */
	reg_t	glb_mem2;		/* 0x0030b068  TM Memory Control Register */
	reg_t	glb_mem3;		/* 0x0030b06c  TM Memory Control Register */
	reg_t	glb_dwnld_err;		/* 0x0030b070  Download Error flags */
	reg_t	glb_dwnld_status;	/* 0x0030b074  Download Status flags */
} bsp_glb_ctl_t;

typedef struct usb_ctrl {
	reg_t brt_ctl_1;		/* 0x00480200 BERT Control 1 Register */
	reg_t brt_ctl_2;		/* 0x00480204 BERT Control 2 Register */
	reg_t brt_stat_1;		/* 0x00480208 BERT Status 1 Register */
	reg_t brt_stat_2;		/* 0x0048020c BERT Status 2 Register */
	reg_t utmi_ctl_1;		/* 0x00480210 UTMI Control Register */
	reg_t test_port_ctl;	/* 0x00480214 Test Port Control Register */
	reg_t pll_ctl_1;		/* 0x00480218 PLL Control Register */
	reg_t ebridge;			/* 0x0048021c Control Register for EHCI Bridge */
	reg_t obridge;			/* 0x00480220 Control Register for OHCI Bridge */
	reg_t fladj_value;		/* 0x00480224 Frame Adjust Value */
	reg_t setup;			/* 0x00480228 Setup Register */
	reg_t mdio;				/* 0x0048022c MDIO Interface Programming Register */
	reg_t mdio2;			/* 0x00480230 MDIO Interface Read Register */
	reg_t usb_simctl;		/* 0x00480234 Simulation Register */
	reg_t spare1;			/* 0x00480238 Spare1 Register for future use */
	reg_t rev_id;			/* 0x0048023c REVISION ID Register */
} usb_ctrl;
