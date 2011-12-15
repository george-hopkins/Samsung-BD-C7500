/*
 *
 * Copyright (c) 2002-2005 Broadcom Corporation 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
*/
/***********************************************************************/
/*                                                                     */
/*   MODULE:  intEMAC_map.h (was 6345_map.h)                           */
/*   DATE:    96/12/19                                                 */
/*   PURPOSE: Define addresses of major hardware components of         */
/*            the internal MAC component of the 7110, which are not    */
/*            defined by bcm711x_map.h                                 */
/*                                                                     */
/***********************************************************************/
#ifndef __INTEMAC_MAP_H
#define __INTEMAC_MAP_H

#ifdef __cplusplus
extern "C" {
#endif


#ifndef __ASSEMBLY__

/*
** DMA Channel Configuration
*/
typedef struct DmaChannel {
  unsigned long cfg;                    /* (00) assorted configuration */
#define         DMA_BURST_HALT  0x00000004  /* idle after finish current memory burst */
#define         DMA_PKT_HALT    0x00000002  /* idle after an EOP flag is detected */
#define         DMA_ENABLE      0x00000001  /* set to enable channel */
  unsigned long intStat;                /* (04) interrupts control and status */
  unsigned long intMask;                /* (08) interrupts mask */
#define         DMA_BUFF_DONE   0x00000001  /* buffer done */
#define         DMA_DONE        0x00000002  /* packet xfer complete */
#define         DMA_NO_DESC     0x00000004  /* no valid descriptors */
  unsigned long maxBurst;               /* (0C) max burst length permitted */
  unsigned long descPtr;				/* (10) iudma base descriptor pointer */

  /* Unused words */
  unsigned long resv[27];
} DmaChannel;

/*
** DMA State RAM (1 .. 16)
*/
typedef struct DmaStateRam {
  unsigned long baseDescPtr;            /* (00) descriptor ring start address */
  unsigned long state_data;             /* (04) state/bytes done/ring offset */
  unsigned long desc_len_status;        /* (08) buffer descriptor status and len */
  unsigned long desc_base_bufptr;       /* (0C) buffer descrpitor current processing */
} DmaStateRam;

/*
** DMA Registers
*/
typedef struct DmaRegs {
#define IUDMA_ENABLE           0x00000001
#define DMA_FLOWC_CH1_EN        0x00000002
/*
THT: Not defined on 7038 and 7318
#define DMA_FLOWC_CH3_EN        0x00000004
#define DMA_NUM_CHS_MASK        0x0f000000
#define DMA_NUM_CHS_SHIFT       24
#define DMA_FLOWCTL_MASK        0x30000000
#define DMA_FLOWCTL_CH1         0x10000000
#define DMA_FLOWCTL_CH3         0x20000000
#define DMA_FLOWCTL_SHIFT       28
*/
    unsigned long controller_cfg;              /* (00) controller configuration */

    /* Flow control */
    unsigned long flowctl_ch1_thresh_lo;       /* (04) EMAC1 RX DMA channel */
    unsigned long flowctl_ch1_thresh_hi;       /* (08) EMAC1 RX DMA channel */
    unsigned long flowctl_ch1_alloc;           /* (0C) EMAC1 RX DMA channel */
#define IUDMA_CH1_FLOW_ALLOC_FORCE 0x80000000 /* Bit 31 */
    
    unsigned long enet_iudma_rev;              /* (10) Enet rev             */
    unsigned long enet_iudma_tstctl;           /* (14) Enet test control    */
    unsigned long enet_iudma_pci_irq_sts;      /* (18) Enet pci intr status */
    unsigned long enet_iudma_pci_irq_msk;      /* (1C) Enet pci intr mask   */
    unsigned long enet_iudma_r5k_irq_sts;      /* (20) Enet r5k intr status */
    unsigned long enet_iudma_r5k_irq_msk;      /* (24) Enet r5k intr mask   */
    unsigned long enet_iudma_diag_ctl;         /* (28) Enet diag control   */
    unsigned long enet_iudma_diag_rdbk;        /* (2C) Enet diag readback   */

    /* new registers added in 7405b0 */
    unsigned long enet_iudma_mii_select;       /* (30) Enet PHY select */
    unsigned long resv0[3];
    unsigned long enet_iudma_desc_alloc;       /* (40) Enet RX desc allocation */
    unsigned long enet_iudma_desc_thres;       /* (44) Enet RX desc threshold */
    unsigned long enet_iudma_desc_timeout;     /* (48) Enet RX desc timeout */
    unsigned long enet_iudma_desc_irq_sts;     /* (4c) Enet RX desc irq status */
    unsigned long enet_iudma_desc_irq_msk;     /* (50) Enet RX desc irq mask */
	
    /* Unused words */
    unsigned long resv1[43];

    /* Per channel registers/state ram */
    DmaChannel chcfg[2];                   /* (100) Channel configuration */  
} DmaRegs;

/*
** DMA Buffer 
*/
typedef struct DmaDesc {
  unsigned long length_status;                 /* in bytes of data in buffer */
#define          DMA_DESC_USEFPM    0x80000000
#define          DMA_DESC_MULTICAST 0x40000000
#define          DMA_DESC_BUFLENGTH 0x0fff0000
//  unsigned short status;                 /* buffer status */
#define          DMA_OWN        0x8000  /* cleared by DMA, set by SW */
#define          DMA_EOP        0x4000  /* last buffer in packet */
#define          DMA_SOP        0x2000  /* first buffer in packet */
#define          DMA_WRAP       0x1000  /* */
#define          DMA_APPEND_CRC 0x0100

/* EMAC Descriptor Status definitions */
#define          EMAC_MISS      0x0080  /* framed address recognition failed (promiscuous) */
#define          EMAC_BRDCAST   0x0040  /* DA is Broadcast */
#define          EMAC_MULT      0x0020  /* DA is multicast */
#define          EMAC_LG        0x0010  /* frame length > RX_LENGTH register value */
#define          EMAC_NO        0x0008  /* Non-Octet aligned */
#define          EMAC_RXER      0x0004  /* RX_ERR on MII while RX_DV assereted */
#define          EMAC_CRC_ERROR 0x0002  /* CRC error */
#define          EMAC_OV        0x0001  /* Overflow */

/* HDLC Descriptor Status definitions */
#define          DMA_HDLC_TX_ABORT      0x0100
#define          DMA_HDLC_RX_OVERRUN    0x4000
#define          DMA_HDLC_RX_TOO_LONG   0x2000
#define          DMA_HDLC_RX_CRC_OK     0x1000
#define          DMA_HDLC_RX_ABORT      0x0100

  unsigned long address;                /* address of data */
} DmaDesc;


/*
** EMAC transmit MIB counters
*/
typedef struct EmacTxMib {
  unsigned long tx_good_octets;         /* (200) good byte count */
  unsigned long tx_good_pkts;           /* (204) good pkt count */
  unsigned long tx_octets;              /* (208) good and bad byte count */
  unsigned long tx_pkts;                /* (20c) good and bad pkt count */
  unsigned long tx_broadcasts_pkts;     /* (210) good broadcast packets */
  unsigned long tx_multicasts_pkts;     /* (214) good mulitcast packets */
  unsigned long tx_len_64;              /* (218) RMON tx pkt size buckets */
  unsigned long tx_len_65_to_127;       /* (21c) */
  unsigned long tx_len_128_to_255;      /* (220) */
  unsigned long tx_len_256_to_511;      /* (224) */
  unsigned long tx_len_512_to_1023;     /* (228) */
  unsigned long tx_len_1024_to_max;     /* (22c) */
  unsigned long tx_jabber_pkts;         /* (230) > 1518 with bad crc */
  unsigned long tx_oversize_pkts;       /* (234) > 1518 with good crc */
  unsigned long tx_fragment_pkts;       /* (238) < 63   with bad crc */
  unsigned long tx_underruns;           /* (23c) fifo underrun */
  unsigned long tx_total_cols;          /* (240) total collisions in all tx pkts */
  unsigned long tx_single_cols;         /* (244) tx pkts with single collisions */
  unsigned long tx_multiple_cols;       /* (248) tx pkts with multiple collisions */
  unsigned long tx_excessive_cols;      /* (24c) tx pkts with excessive cols */
  unsigned long tx_late_cols;           /* (250) tx pkts with late cols */
  unsigned long tx_defered;             /* (254) tx pkts deferred */
  unsigned long tx_carrier_lost;        /* (258) tx pkts with CRS lost */
  unsigned long tx_pause_pkts;          /* (25c) tx pause pkts sent */
#define NumEmacTxMibVars        24
} EmacTxMib;

/*
** EMAC receive MIB counters
*/
typedef struct EmacRxMib {
  unsigned long rx_good_octets;         /* (280) good byte count */
  unsigned long rx_good_pkts;           /* (284) good pkt count */
  unsigned long rx_octets;              /* (288) good and bad byte count */
  unsigned long rx_pkts;                /* (28c) good and bad pkt count */
  unsigned long rx_broadcasts_pkts;     /* (290) good broadcast packets */
  unsigned long rx_multicasts_pkts;     /* (294) good mulitcast packets */
  unsigned long rx_len_64;              /* (298) RMON rx pkt size buckets */
  unsigned long rx_len_65_to_127;       /* (29c) */
  unsigned long rx_len_128_to_255;      /* (2a0) */
  unsigned long rx_len_256_to_511;      /* (2a4) */
  unsigned long rx_len_512_to_1023;     /* (2a8) */
  unsigned long rx_len_1024_to_max;     /* (2ac) */
  unsigned long rx_jabber_pkts;         /* (2b0) > 1518 with bad crc */
  unsigned long rx_oversize_pkts;       /* (2b4) > 1518 with good crc */
  unsigned long rx_fragment_pkts;       /* (2b8) < 63   with bad crc */
  unsigned long rx_missed_pkts;         /* (2bc) missed packets */
  unsigned long rx_crc_align_errs;      /* (2c0) both or either */
  unsigned long rx_undersize;           /* (2c4) < 63   with good crc */
  unsigned long rx_crc_errs;            /* (2c8) crc errors (only) */
  unsigned long rx_align_errs;          /* (2cc) alignment errors (only) */
  unsigned long rx_symbol_errs;         /* (2d0) pkts with RXERR assertions (symbol errs) */
  unsigned long rx_pause_pkts;          /* (2d4) MAC control, PAUSE */
  unsigned long rx_nonpause_pkts;       /* (2d8) MAC control, not PAUSE */
#define NumEmacRxMibVars        23
} EmacRxMib;

typedef struct EmacRegisters {
  unsigned long rxControl;              /* (00) receive control */
#define          EMAC_PM_REJ    0x80    /*      - reject DA match in PMx regs */
#define          EMAC_UNIFLOW   0x40    /*      - accept cam match fc */
#define          EMAC_FC_EN     0x20    /*      - enable flow control */
#define          EMAC_LOOPBACK  0x10    /*      - loopback */
#define          EMAC_PROM      0x08    /*      - promiscuous */
#define          EMAC_RDT       0x04    /*      - ignore transmissions */
#define          EMAC_ALL_MCAST 0x02    /*      - ignore transmissions */
#define          EMAC_NO_BCAST  0x01    /*      - ignore transmissions */


  unsigned long rxMaxLength;            /* (04) receive max length */
  unsigned long txMaxLength;            /* (08) transmit max length */
  unsigned long unused1[1];
  unsigned long mdioFreq;               /* (10) mdio frequency */
#define          EMAC_MII_PRE_EN 0x00000080 /* prepend preamble sequence */
#define          EMAC_MDIO_PRE   0x00000080  /*      - enable MDIO preamble */
#define          EMAC_MDC_FREQ   0x0000007f  /*      - mdio frequency */

  unsigned long mdioData;               /* (14) mdio data */
#define          MDIO_WR        0x50020000 /*   - write framing */
#define          MDIO_RD        0x60020000 /*   - read framing */
#define          MDIO_PMD_SHIFT  23
#define          MDIO_REG_SHIFT  18

  unsigned long intMask;                /* (18) int mask */
  unsigned long intStatus;              /* (1c) int status */
#define          EMAC_FLOW_INT  0x04    /*      - flow control event */
#define          EMAC_MIB_INT   0x02    /*      - mib event */
#define          EMAC_MDIO_INT  0x01    /*      - mdio event */

  unsigned long unused2[3];
  unsigned long config;                 /* (2c) config */
#define          EMAC_ENABLE    0x001   /*      - enable emac */
#define          EMAC_DISABLE   0x002   /*      - disable emac */
#define          EMAC_SOFT_RST  0x004   /*      - soft reset */
#define          EMAC_SOFT_RESET 0x004  /*      - emac soft reset */
#define          EMAC_EXT_PHY   0x008   /*      - external PHY select */

  unsigned long txControl;              /* (30) transmit control */
#define          EMAC_FD        0x001   /*      - full duplex */
#define          EMAC_FLOWMODE  0x002   /*      - flow mode */
#define          EMAC_NOBKOFF   0x004   /*      - no backoff in  */
#define          EMAC_SMALLSLT  0x008   /*      - small slot time */

  unsigned long txThreshold;            /* (34) transmit threshold */
  unsigned long mibControl;             /* (38) mib control */
#define          EMAC_NO_CLEAR  0x001   /* don't clear on read */

  unsigned long unused3[7];

  unsigned long pm0DataLo;              /* (58) perfect match 0 data lo */
  unsigned long pm0DataHi;              /* (5C) perfect match 0 data hi (15:0) */
  unsigned long pm1DataLo;              /* (60) perfect match 1 data lo */
  unsigned long pm1DataHi;              /* (64) perfect match 1 data hi (15:0) */
  unsigned long pm2DataLo;              /* (68) perfect match 2 data lo */
  unsigned long pm2DataHi;              /* (6C) perfect match 2 data hi (15:0) */
  unsigned long pm3DataLo;              /* (70) perfect match 3 data lo */
  unsigned long pm3DataHi;              /* (74) perfect match 3 data hi (15:0) */
  unsigned long pm4DataLo;              /* (78) perfect match 0 data lo */
  unsigned long pm4DataHi;              /* (7C) perfect match 0 data hi (15:0) */
  unsigned long pm5DataLo;              /* (80) perfect match 1 data lo */
  unsigned long pm5DataHi;              /* (84) perfect match 1 data hi (15:0) */
  unsigned long pm6DataLo;              /* (88) perfect match 2 data lo */
  unsigned long pm6DataHi;              /* (8C) perfect match 2 data hi (15:0) */
  unsigned long pm7DataLo;              /* (90) perfect match 3 data lo */
  unsigned long pm7DataHi;              /* (94) perfect match 3 data hi (15:0) */
#define          EMAC_CAM_V   0x10000  /*      - cam index */
#define          EMAC_CAM_VALID 0x00010000

  unsigned long unused4[90];            /* (98-1fc) */

  EmacTxMib     tx_mib;                 /* (200) emac tx mib */
  unsigned long unused5[8];             /* (260-27c) */

  EmacRxMib     rx_mib;                 /* (280) rx mib */

} EmacRegisters;

/* register offsets for subrouting access */
#define EMAC_RX_CONTROL         0x00
#define EMAC_RX_MAX_LENGTH      0x04
#define EMAC_TX_MAC_LENGTH      0x08
#define EMAC_MDIO_FREQ          0x10
#define EMAC_MDIO_DATA          0x14
#define EMAC_INT_MASK           0x18
#define EMAC_INT_STATUS         0x1C
#ifndef INCLUDE_DOCSIS_APP
/* Does not exist in the internal EMAC core int he bcm7110  */
#define EMAC_CAM_DATA_LO        0x20
#define EMAC_CAM_DATA_HI        0x24
#define EMAC_CAM_CONTROL        0x28
#endif
#define EMAC_CONTROL            0x2C
#define EMAC_TX_CONTROL         0x30
#define EMAC_TX_THRESHOLD       0x34
#define EMAC_MIB_CONTROL        0x38

#endif /* __ASSEMBLY__ */

#define EMAC_RX_CHAN               0
#define EMAC_TX_CHAN               1



#ifdef __cplusplus
}
#endif

#endif

