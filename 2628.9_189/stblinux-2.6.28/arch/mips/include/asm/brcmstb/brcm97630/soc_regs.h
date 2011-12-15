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
	reg_t	phybist_cntrl;		/* 0x01800090  DDR Phy-Bist Control and Status */
	reg_t	phybist_seed;		/* 0x01800094  DDR Phy-Bist PRPG Seed Value */
	reg_t	phybist_ctl_status;	/* 0x01800098  DDR Phy-Bist Address & Control Status */
	reg_t	phybist_dq_status;	/* 0x0180009c  DDR Phy-Bist DQ Status */
	reg_t	phybist_misc_status;	/* 0x018000a0  DDR Phy-Bist Miscellaneous Status */
	reg_t	unused_0xa4;		/* 0x018000a4  Unused */
	reg_t	unused_0xa8;		/* 0x018000a8  Unused */
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
	reg_t	unused_0xec;		/* 0x018000ec  Unused */
	reg_t	update_vdl;		/* 0x018000f0  RAM Macro TM Control */
} ddr23_ctl_t;

typedef struct ddr_phy_control {
	reg_t	revision;		/* 0x01805000  Address & Control revision register */
	reg_t	clk_pm_ctrl;		/* 0x01805004  PHY clock power management control register */
	reg_t	unused_0x8;		/* 0x01805008  Unused */
	reg_t	unused_0xc;		/* 0x0180500c  Unused */
	reg_t	pll_status;		/* 0x01805010  PHY PLL status register */
	reg_t	pll_config;		/* 0x01805014  PHY PLL configuration register */
	reg_t	pll_pre_divider;	/* 0x01805018  PHY PLL pre-divider control register */
	reg_t	pll_divider;		/* 0x0180501c  PHY PLL divider control register */
	reg_t	pll_control1;		/* 0x01805020  PHY PLL analog control register #1 i_pll_ctrl[31:0] see PLL spec */
	reg_t	pll_control2;		/* 0x01805024  PHY PLL analog control register #2 i_pll_ctrl[37:32] see PLL spec */
	reg_t	pll_ss_en;		/* 0x01805028  PHY PLL spread spectrum config register */
	reg_t	pll_ss_cfg;		/* 0x0180502c  PHY PLL spread spectrum config register */
	reg_t	static_vdl_override;	/* 0x01805030  Address & Control VDL static override control register */
	reg_t	dynamic_vdl_override;	/* 0x01805034  Address & Control VDL dynamic override control register */
	reg_t	idle_pad_control;	/* 0x01805038  Idle mode SSTL pad control register */
	reg_t	zq_pvt_comp_ctl;	/* 0x0180503c  PVT Compensation control and status register */
	reg_t	drive_pad_ctl;		/* 0x01805040  SSTL pad drive characteristics control register */
	reg_t	clock_reg_control;	/* 0x01805044  Clock Regulator control register */
	reg_t	unused_0x48;		/* 0x01805048  Unused */
	reg_t	unused_0x4c;		/* 0x0180504c  Unused */
	reg_t	unused_0x50;		/* 0x01805050  Unused */
	reg_t	unused_0x54;		/* 0x01805054  Unused */
	reg_t	unused_0x58;		/* 0x01805058  Unused */
	reg_t	virtual_vtt_control;	/* 0x0180505c  Virtual VTT Control and Status register */
	reg_t	virtual_vtt_status;	/* 0x01805060  Virtual VTT Control and Status register */
	reg_t	virtual_vtt_connections;/* 0x01805064  Virtual VTT Connections register */
	reg_t	virtual_vtt_override;	/* 0x01805068  Virtual VTT Override register */
	reg_t	unused_0x6c;		/* 0x0180506c  Unused */
	reg_t	standby_pwr_1;		/* 0x01805070  Standby Power control register #1 */
	reg_t	unused_0x74;		/* 0x01805074  Unused */
	reg_t	unused_0x78;		/* 0x01805078  Unused */
	reg_t	unused_0x7c;		/* 0x0180507c  Unused */
	reg_t	standby_pwr_2;		/* 0x01805080  Standby Power control register #2 */
} ddr_phy_control_t;

#define NUM_DDR_LANES	8

/* from the rdb */

#define BCHP_DDR23_PHY_BYTE_LANE_0_REVISION      0x01805400 /* Byte lane revision register */
#define BCHP_DDR23_PHY_BYTE_LANE_1_REVISION      0x01805300 /* Byte lane revision register */
#define BCHP_DDR23_PHY_BYTE_LANE_2_REVISION      0x01805200 /* Byte lane revision register */
#define BCHP_DDR23_PHY_BYTE_LANE_3_REVISION      0x01805100 /* Byte lane revision register */


enum ddr_lanes{
	Lane_0 = 0,
	Lane_1,
	Lane_2,
	Lane_3,
};
	
typedef struct ddr_phy_lane {
	reg_t	revision;		/* 0x01805100  Byte lane revision register */
	reg_t	vdl_calibrate;		/* 0x01805104  Byte lane VDL calibration control register */
	reg_t	vdl_status;		/* 0x01805108  Byte lane VDL calibration status register */
	reg_t	unused_0xc;		/* 0x0180510c  Unused */
	reg_t	vdl_override_0;		/* 0x01805110  Read DQSP VDL static override control register */
	reg_t	vdl_override_1;		/* 0x01805114  Read DQSN VDL static override control register */
	reg_t	vdl_override_2;		/* 0x01805118  Read Enable VDL static override control register */
	reg_t	vdl_override_3;		/* 0x0180511c  Write data and mask VDL static override control register */
	reg_t	vdl_override_4;		/* 0x01805120  Read DQSP VDL dynamic override control register */
	reg_t	vdl_override_5;		/* 0x01805124  Read DQSN VDL dynamic override control register */
	reg_t	vdl_override_6;		/* 0x01805128  Read Enable VDL dynamic override control register */
	reg_t	vdl_override_7;		/* 0x0180512c  Write data and mask VDL dynamic override control register */
	reg_t	read_control;		/* 0x01805130  Byte Lane read channel control register */
	reg_t	read_fifo_status;	/* 0x01805134  Read fifo status register */
	reg_t	read_fifo_clear;	/* 0x01805138  Read fifo status clear register */
	reg_t	idle_pad_control;	/* 0x0180513c  Idle mode SSTL pad control register */
	reg_t	drive_pad_ctl;		/* 0x01805140  SSTL pad drive characteristics control register */
	reg_t	clock_pad_disable;	/* 0x01805144  Clock pad disable register */
	reg_t	wr_preamble_mode;	/* 0x01805148  Write cycle preamble control register */
	reg_t	clock_reg_control;	/* 0x0180514c  Clock Regulator control register */
} ddr_phy_lane_t;

typedef struct art {
	reg_t	revision;		/* 0x00071000  Autonomous Register Transfer (ART) Revision ID */
	reg_t	config;			/* 0x00071004  Various Configuration Settings */
	reg_t	status;			/* 0x00071008  General Hardware Status */
	reg_t	unused_0xc;		/* 0x0007100c  Unused */
	reg_t	wakeup_control;		/* 0x00071010  ART Wakeup Event Control */
	reg_t	sleep_control;		/* 0x00071014  ART Sleep Event Control */
	reg_t	sequence_trigger;	/* 0x00071018  ART Sequence Trigger Manual Control */
	reg_t	sequence_status;	/* 0x0007101c  ART Sequencer Status */
	reg_t	codestore_pointer;	/* 0x00071020  Address for Access to Code Store Memory */
	reg_t	codestore_data;		/* 0x00071024  Data Word for Access to Code Store Memory */
	reg_t	codestore_tm;		/* 0x00071028  Code Store Memory Test Mode Control */
	reg_t	debug;			/* 0x0007102c  ART Debug Output */
} art_t;

typedef struct clk_control  {

	reg_t	revision;		/* 0x00070000  clock_gen Revision register */
	reg_t	pm_ctrl;		/* 0x00070004  Software power management control to turn off 108 MHz clocks */
	reg_t	pm_ctrl_1;		/* 0x00070008  Software power management control to turn off various clocks */
	reg_t	pm_ctrl_2;		/* 0x0007000c  Software power management control to turn off or select various clocks */
	reg_t	pm_ctrl_216;		/* 0x00070010  Software power management control to turn off 216 MHz clocks */
	reg_t	unused_0x14;		/* 0x00070014  Unused */
	reg_t	unused_0x18;		/* 0x00070018  Unused */
	reg_t	wakeup_status;		/* 0x0007001c  Hardware power management wakeup status. */
	reg_t	wakeup_mask;		/* 0x00070020  Hardware power management wakeup masks. */
	reg_t	critical_ctrl;		/* 0x00070024  Hardware power management critical control. */
	reg_t	critical_status;		/* 0x00070028  Hardware power management critical status. */
	reg_t	unused_0x2c;		/* 0x0007002c  Unused */
	reg_t	unused_0x30;		/* 0x00070030  Unused */
	reg_t	unused_0x34;		/* 0x00070034  Unused */
	reg_t	unused_0x38;		/* 0x00070038  Unused */
	reg_t	regulator_2p5_volts;		/* 0x0007003c  2.5V Regulator Voltage Adjustment */
	reg_t	temp_mon_ctrl;		/* 0x00070040  Temperature monitor control. */
	reg_t	temp_mon_status;		/* 0x00070044  Temperature monitor status. */
	reg_t	unused_0x48;		/* 0x00070048  Unused */
	reg_t	unused_0x4c;		/* 0x0007004c  Unused */
	reg_t	unused_0x50;		/* 0x00070050  Unused */
	reg_t	unused_0x54;		/* 0x00070054  Unused */
	reg_t	unused_0x58;		/* 0x00070058  Unused */
	reg_t	unused_0x5c;		/* 0x0007005c  Unused */
	reg_t	unused_0x60;		/* 0x00070060  Unused */
	reg_t	unused_0x64;		/* 0x00070064  Unused */
	reg_t	unused_0x68;		/* 0x00070068  Unused */
	reg_t	unused_0x6c;		/* 0x0007006c  Unused */
	reg_t	scratch;		/* 0x00070070  clock_gen  Scratch register */
	reg_t	unused_0x74;		/* 0x00070074  Unused */
	reg_t	unused_0x78;		/* 0x00070078  Unused */
	reg_t	unused_0x7c;		/* 0x0007007c  Unused */
	reg_t	unused_0x80;		/* 0x00070080  Unused */
	reg_t	unused_0x84;		/* 0x00070084  Unused */
	reg_t	unused_0x88;		/* 0x00070088  Unused */
	reg_t	unused_0x8c;		/* 0x0007008c  Unused */
	reg_t	unused_0x90;		/* 0x00070090  Unused */
	reg_t	unused_0x94;		/* 0x00070094  Unused */
	reg_t	unused_0x98;		/* 0x00070098  Unused */
	reg_t	unused_0x9c;		/* 0x0007009c  Unused */
	reg_t	unused_0xa0;		/* 0x000700a0  Unused */
	reg_t	unused_0xa4;		/* 0x000700a4  Unused */
	reg_t	unused_0xa8;		/* 0x000700a8  Unused */
	reg_t	unused_0xac;		/* 0x000700ac  Unused */
	reg_t	unused_0xb0;		/* 0x000700b0  Unused */
	reg_t	unused_0xb4;		/* 0x000700b4  Unused */
	reg_t	unused_0xb8;		/* 0x000700b8  Unused */
	reg_t	unused_0xbc;		/* 0x000700bc  Unused */
	reg_t	unused_0xc0;		/* 0x000700c0  Unused */
	reg_t	unused_0xc4;		/* 0x000700c4  Unused */
	reg_t	unused_0xc8;		/* 0x000700c8  Unused */
	reg_t	unused_0xcc;		/* 0x000700cc  Unused */
	reg_t	unused_0xd0;		/* 0x000700d0  Unused */
	reg_t	unused_0xd4;		/* 0x000700d4  Unused */
	reg_t	unused_0xd8;		/* 0x000700d8  Unused */
	reg_t	unused_0xdc;		/* 0x000700dc  Unused */
	reg_t	unused_0xe0;		/* 0x000700e0  Unused */
	reg_t	unused_0xe4;		/* 0x000700e4  Unused */
	reg_t	unused_0xe8;		/* 0x000700e8  Unused */
	reg_t	unused_0xec;		/* 0x000700ec  Unused */
	reg_t	unused_0xf0;		/* 0x000700f0  Unused */
	reg_t	unused_0xf4;		/* 0x000700f4  Unused */
	reg_t	unused_0xf8;		/* 0x000700f8  Unused */
	reg_t	unused_0xfc;		/* 0x000700fc  Unused */
	reg_t	pll0_ctrl;		/* 0x00070100  Main PLL0 CML powerdown */
	reg_t	unused_0x104;		/* 0x00070104  Unused */
	reg_t	unused_0x108;		/* 0x00070108  Unused */
	reg_t	unused_0x10c;		/* 0x0007010c  Unused */
	reg_t	pll0_ebi_div;		/* 0x00070110  Main PLL0 channel 4 EBI output clock divider settings */
	reg_t	unused_0x114;		/* 0x00070114  Unused */
	reg_t	unused_0x118;		/* 0x00070118  Unused */
	reg_t	pll0_lock_cnt;		/* 0x0007011c  Main PLL0 Lock Counter */
	reg_t	pll1_ctrl;		/* 0x00070120  Main PLL1 reset, enable, and powerdown */
	reg_t	unused_0x124;		/* 0x00070124  Unused */
	reg_t	pll1_mips_div;		/* 0x00070128  Main PLL1 channel 1 MIPS clock divider settings */
	reg_t	pll1_ofe_div;		/* 0x0007012c  Main PLL1 channel 3 OFE clock divider settings */
	reg_t	pll1_sdio_div;		/* 0x00070130  Main PLL1 channel 4 SDIO clock divider settings */
	reg_t	pll1_rap_div;		/* 0x00070134  Main PLL1 channel 5 RAP clock divider settings */
	reg_t	pll1_avd_div;		/* 0x00070138  Main PLL1 channel 6 AVD clock divider settings */
	reg_t	pll1_lock_cnt;		/* 0x0007013c  Main PLL1 Lock Counter */
	reg_t	alt_pll_ctrl;		/* 0x00070140  Alternate PLL reset, enable, powerdown, and control */
	reg_t	alt_pll_ctrl_lo;		/* 0x00070144  Alternate PLL control low 32 bits */
	reg_t	alt_pll_div;		/* 0x00070148  Alternate PLL main and channel 1 (MIPS) divider settings */
	reg_t	alt_pll_div2;		/* 0x0007014c  Alternate PLL channel 2 (RAP) divider settings */
	reg_t	alt_pll_div3;		/* 0x00070150  Alternate PLL channel 3 (spare) divider settings */
	reg_t	unused_0x154;		/* 0x00070154  Unused */
	reg_t	unused_0x158;		/* 0x00070158  Unused */
	reg_t	alt_pll_lock_cnt;		/* 0x0007015c  Alternate PLL Lock Counter */
	reg_t	vcxo0_ctrl;		/* 0x00070160  VCXO 0 PLL reset, enable, powerdown, and mode */
	reg_t	vcxo0_ctrl_lo;		/* 0x00070164  VCXO 0 PLL control low 32 bits */
	reg_t	vcxo0_div;		/* 0x00070168  VCXO 0 PLL divider settings */
	reg_t	unused_0x16c;		/* 0x0007016c  Unused */
	reg_t	unused_0x170;		/* 0x00070170  Unused */
	reg_t	unused_0x174;		/* 0x00070174  Unused */
	reg_t	unused_0x178;		/* 0x00070178  Unused */
	reg_t	vcxo0_lock_cnt;		/* 0x0007017c  VCXO 0 PLL Lock Counter */
	reg_t	aud0_ctrl;		/* 0x00070180  Audio PLL 0 reset, enable, and powerdown */
	reg_t	unused_0x184;		/* 0x00070184  Unused */
	reg_t	unused_0x188;		/* 0x00070188  Unused */
	reg_t	unused_0x18c;		/* 0x0007018c  Unused */
	reg_t	unused_0x190;		/* 0x00070190  Unused */
	reg_t	unused_0x194;		/* 0x00070194  Unused */
	reg_t	unused_0x198;		/* 0x00070198  Unused */
	reg_t	aud0_lock_cnt;		/* 0x0007019c  Audio PLL 0 Lock Counter */
	reg_t	unused_0x1a0;		/* 0x000701a0  Unused */
	reg_t	unused_0x1a4;		/* 0x000701a4  Unused */
	reg_t	unused_0x1a8;		/* 0x000701a8  Unused */
	reg_t	unused_0x1ac;		/* 0x000701ac  Unused */
	reg_t	unused_0x1b0;		/* 0x000701b0  Unused */
	reg_t	unused_0x1b4;		/* 0x000701b4  Unused */
	reg_t	unused_0x1b8;		/* 0x000701b8  Unused */
	reg_t	unused_0x1bc;		/* 0x000701bc  Unused */
	reg_t	unused_0x1c0;		/* 0x000701c0  Unused */
	reg_t	unused_0x1c4;		/* 0x000701c4  Unused */
	reg_t	unused_0x1c8;		/* 0x000701c8  Unused */
	reg_t	unused_0x1cc;		/* 0x000701cc  Unused */
	reg_t	unused_0x1d0;		/* 0x000701d0  Unused */
	reg_t	unused_0x1d4;		/* 0x000701d4  Unused */
	reg_t	unused_0x1d8;		/* 0x000701d8  Unused */
	reg_t	unused_0x1dc;		/* 0x000701dc  Unused */
	reg_t	unused_0x1e0;		/* 0x000701e0  Unused */
	reg_t	unused_0x1e4;		/* 0x000701e4  Unused */
	reg_t	unused_0x1e8;		/* 0x000701e8  Unused */
	reg_t	unused_0x1ec;		/* 0x000701ec  Unused */
	reg_t	unused_0x1f0;		/* 0x000701f0  Unused */
	reg_t	unused_0x1f4;		/* 0x000701f4  Unused */
	reg_t	unused_0x1f8;		/* 0x000701f8  Unused */
	reg_t	unused_0x1fc;		/* 0x000701fc  Unused */
	reg_t	pll_lock;		/* 0x00070200  PLL lock status */
	reg_t	pll_lock_cntr_reset;		/* 0x00070204  PLL Lock Counter Resets */
	reg_t	pll_test_sel;		/* 0x00070208  PLL core test select */
	reg_t	unused_0x20c;		/* 0x0007020c  Unused */
	reg_t	unused_0x210;		/* 0x00070210  Unused */
	reg_t	unused_0x214;		/* 0x00070214  Unused */
	reg_t	unused_0x218;		/* 0x00070218  Unused */
	reg_t	unused_0x21c;		/* 0x0007021c  Unused */
	reg_t	unused_0x220;		/* 0x00070220  Unused */
	reg_t	unused_0x224;		/* 0x00070224  Unused */
	reg_t	unused_0x228;		/* 0x00070228  Unused */
	reg_t	unused_0x22c;		/* 0x0007022c  Unused */
	reg_t	unused_0x230;		/* 0x00070230  Unused */
	reg_t	unused_0x234;		/* 0x00070234  Unused */
	reg_t	unused_0x238;		/* 0x00070238  Unused */
	reg_t	unused_0x23c;		/* 0x0007023c  Unused */
	reg_t	unused_0x240;		/* 0x00070240  Unused */
	reg_t	unused_0x244;		/* 0x00070244  Unused */
	reg_t	unused_0x248;		/* 0x00070248  Unused */
	reg_t	unused_0x24c;		/* 0x0007024c  Unused */
	reg_t	unused_0x250;		/* 0x00070250  Unused */
	reg_t	unused_0x254;		/* 0x00070254  Unused */
	reg_t	unused_0x258;		/* 0x00070258  Unused */
	reg_t	unused_0x25c;		/* 0x0007025c  Unused */
	reg_t	unused_0x260;		/* 0x00070260  Unused */
	reg_t	unused_0x264;		/* 0x00070264  Unused */
	reg_t	unused_0x268;		/* 0x00070268  Unused */
	reg_t	unused_0x26c;		/* 0x0007026c  Unused */
	reg_t	unused_0x270;		/* 0x00070270  Unused */
	reg_t	unused_0x274;		/* 0x00070274  Unused */
	reg_t	unused_0x278;		/* 0x00070278  Unused */
	reg_t	unused_0x27c;		/* 0x0007027c  Unused */
	reg_t	unused_0x280;		/* 0x00070280  Unused */
	reg_t	unused_0x284;		/* 0x00070284  Unused */
	reg_t	unused_0x288;		/* 0x00070288  Unused */
	reg_t	unused_0x28c;		/* 0x0007028c  Unused */
	reg_t	unused_0x290;		/* 0x00070290  Unused */
	reg_t	unused_0x294;		/* 0x00070294  Unused */
	reg_t	unused_0x298;		/* 0x00070298  Unused */
	reg_t	unused_0x29c;		/* 0x0007029c  Unused */
	reg_t	unused_0x2a0;		/* 0x000702a0  Unused */
	reg_t	unused_0x2a4;		/* 0x000702a4  Unused */
	reg_t	unused_0x2a8;		/* 0x000702a8  Unused */
	reg_t	unused_0x2ac;		/* 0x000702ac  Unused */
	reg_t	unused_0x2b0;		/* 0x000702b0  Unused */
	reg_t	unused_0x2b4;		/* 0x000702b4  Unused */
	reg_t	unused_0x2b8;		/* 0x000702b8  Unused */
	reg_t	unused_0x2bc;		/* 0x000702bc  Unused */
	reg_t	unused_0x2c0;		/* 0x000702c0  Unused */
	reg_t	unused_0x2c4;		/* 0x000702c4  Unused */
	reg_t	unused_0x2c8;		/* 0x000702c8  Unused */
	reg_t	unused_0x2cc;		/* 0x000702cc  Unused */
	reg_t	unused_0x2d0;		/* 0x000702d0  Unused */
	reg_t	unused_0x2d4;		/* 0x000702d4  Unused */
	reg_t	unused_0x2d8;		/* 0x000702d8  Unused */
	reg_t	unused_0x2dc;		/* 0x000702dc  Unused */
	reg_t	unused_0x2e0;		/* 0x000702e0  Unused */
	reg_t	unused_0x2e4;		/* 0x000702e4  Unused */
	reg_t	unused_0x2e8;		/* 0x000702e8  Unused */
	reg_t	unused_0x2ec;		/* 0x000702ec  Unused */
	reg_t	unused_0x2f0;		/* 0x000702f0  Unused */
	reg_t	unused_0x2f4;		/* 0x000702f4  Unused */
	reg_t	unused_0x2f8;		/* 0x000702f8  Unused */
	reg_t	unused_0x2fc;		/* 0x000702fc  Unused */
	reg_t	gpio_pad_ctrl;		/* 0x00070300  GPIO pad control */
	reg_t	sdio_pad_ctrl;		/* 0x00070304  SDIO pad control */
	reg_t	dvo_pad_ctrl;		/* 0x00070308  DVO pad control */
	reg_t	i2c_pad_ctrl;		/* 0x0007030c  I2C pad control */
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
	reg_t	perf_mon_access_0b;	/* 0x0040bb54  Performance Monitor Client Access bits 55:32 for ddr interface #0 */
	reg_t	perf_mon_access_1a;	/* 0x0040bb58  Performance Monitor Client Access bits 31:0 for ddr interface #1 */
	reg_t	perf_mon_access_1b;	/* 0x0040bb5c  Performance Monitor Client Access bits 55:32 for ddr interface #1 */
	reg_t	master_ctl;		/* 0x0040bb60  Master Control */
} pri_arbiter_control_t;

typedef struct wakeup_intr2 {
	reg_t	status;			/* 0x00406c80  Wake-Up Status Register */
	reg_t	set;			/* 0x00406c84  Wake-Up Set Register */
	reg_t	clear;			/* 0x00406c88  Wake-Up Clear Register */
	reg_t	mask_status;		/* 0x00406c8c  Wake-Up Mask Status Register */
	reg_t	mask_set;		/* 0x00406c90  Wake-Up Mask Set Register */
	reg_t	mask_clear;		/* 0x00406c94  Wake-Up Mask Clear Register */
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
	reg_t	dac_bg_ctrl_reg;	/* 0x00180954  HEXDAC Bandgap Control Register */
	reg_t	dac1_ctrl_reg;		/* 0x00180958  HEXDAC1 Control Register */
	reg_t	dac2_ctrl_reg;		/* 0x0018095c  HEXDAC2 Control Register */
	reg_t	dac3_ctrl_reg;		/* 0x00180960  HEXDAC3 Control Register */
	reg_t	dac4_ctrl_reg;		/* 0x00180964  HEXDAC4 Control Register */
	reg_t	dac5_ctrl_reg;		/* 0x00180968  HEXDAC5 Control Register */
	reg_t	dac6_ctrl_reg;		/* 0x0018096c  HEXDAC6 Control Register */
	reg_t	unused_0x70;		/* 0x00180970  Unused */
	reg_t	dvi_dithering;		/* 0x00180974  DVI Dithering Register */
	reg_t	prim_ch0_sa_status;	/* 0x00180978  PRIM Channel 0 SA Status Register */
	reg_t	prim_ch1_sa_status;	/* 0x0018097c  PRIM Channel 1 SA Status Register */
	reg_t	prim_ch2_sa_status;	/* 0x00180980  PRIM Channel 2 SA Status Register */
	reg_t	prim_sa_config;		/* 0x00180984  PRIM SA Config Register */
} misc_t;

#if 0
typedef struct clk_vcxo_ctl {
	reg_t	misc;			/* 0x00040810 VCXO block output clock selection */
} clk_vcxo_ctl_t;

typedef struct vcxo_ctl {
} vcxo_ctl_t;
#endif


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
