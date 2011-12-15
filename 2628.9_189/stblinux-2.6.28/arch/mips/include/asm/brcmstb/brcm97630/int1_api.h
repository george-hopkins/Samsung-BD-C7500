#ifndef _INT1_API_H
#define _INT1_API_H


#ifdef __cplusplus
extern "C" {
#endif


/***************************************************************************
 *  Logical Level 1 Interrupt IDs
 **************************************************************************/

/* Level 1 Interrupt IDs */        
/*  reserved					0x03f */
#define INT1_ID_EXT_USB_OHCI_0_CPU_INTR		0x03e
#define INT1_ID_EXT_USB_EHCI_CPU_INTR		0x03d
#define INT1_ID_UPG_108_IRQ_CPU_INTR		0x03c
#define INT1_ID_XPT_FE_CPU_INTR			0x03b
#define INT1_ID_XPT_PCR_CPU_INTR		0x03a
#define INT1_ID_RPTD1_CPU_INTR			0x039
/* reserved					0x038 */
#define INT1_ID_MEMARB_CPU_INTR			0x037
/* reserved					0x036 */
#define INT1_ID_IPI1_CPU_INTR			0x035
#define INT1_ID_IPI0_CPU_INTR			0x034
/* reserved					0x033 */
#define INT1_ID_MIPS0_CPU_INTR			0x032
#define INT1_ID_FGX_CPU_INTR			0x031
#define INT1_ID_ARB_RTS_CPU_INTR		0x030
#define INT1_ID_ARB_CRIT_CPU_INTR		0x02f
#define INT1_ID_ARB_ARCH_CPU_INTR		0x02e
#define INT1_ID_FGT_TRB_CPU_INTR		0x02d
#define INT1_ID_XPT_MSG_STAT_CPU_INTR		0x02c
#define INT1_ID_OFE_IDE_CPU_INTR		0x02b
/* reserved					0x02a */
/* reserved					0x029 */
#define INT1_ID_EXT_IRQ_4_CPU_INTR		0x028
#define INT1_ID_EXT_IRQ_3_CPU_INTR		0x027
#define INT1_ID_EXT_IRQ_2_CPU_INTR		0x026
#define INT1_ID_EXT_IRQ_1_CPU_INTR		0x025
#define INT1_ID_EXT_IRQ_0_CPU_INTR		0x024
/* reserved					0x023 */
#define INT1_ID_OFE_PROC_CPU_INTR		0x022
/* reserved					0x021 */
/* reserved					0x020 */

#define INT1_ID_XPT_RAV_CPU_INTR		0x01f
#define INT1_ID_AVD0_CPU_INTR			0x01e
/* reserved					0x01d */
#define INT1_ID_BVNF_CPU_INTR_3			0x01c
/* reserved					0x01b */
#define INT1_ID_USB_CPU_INTR			0x01a
#define INT1_ID_UPG_UART2_CPU_INTR		0x019
#define INT1_ID_UPG_UART1_CPU_INTR		0x018
#define INT1_ID_SUN_CPU_INTR			0x017
/* reserved					0x016 */
#define INT1_ID_UPG_UART0_CPU_INTR		0x015
#define INT1_ID_UPG_SPI_CPU_INTR		0x014
#define INT1_ID_UPG_BSC_CPU_INTR		0x013
#define INT1_ID_UPG_CPU_INTR			0x012
#define INT1_ID_UPG_TMR_CPU_INTR		0x011
#define INT1_ID_BVNG_CPU_INTR			0x010
#define INT1_ID_ENET_CPU_INTR			0x00f
#define INT1_ID_BVNF_CPU_INTR_2			0x00e
#define INT1_ID_BVNF_CPU_INTR_1			0x00d
#define INT1_ID_BVNF_CPU_INTR_0			0x00c
#define INT1_ID_BVNB_CPU_INTR			0x00b
#define INT1_ID_VEC_CPU_INTR			0x00a
#define INT1_ID_RPTD_CPU_INTR			0x009
#define INT1_ID_HDMI_CPU_INTR			0x008
#define INT1_ID_GFX_CPU_INTR			0x007
#define INT1_ID_AIO_CPU_INTR			0x006
#define INT1_ID_BSP_CPU_INTR			0x005
/* reserved					0x004 */
#define INT1_ID_XPT_MSG_CPU_INTR		0x003
#define INT1_ID_XPT_OVFL_CPU_INTR		0x002
#define INT1_ID_XPT_STATUS_CPU_INTR		0x001
#define INT1_ID_HIF_CPU_INTR			0x000


/***************************************************************************
 *  Level 1 ISR Type Definition
 **************************************************************************/
typedef void (*FN_L1_ISR) (void *, int);


/***************************************************************************
 *  Level 1 Interrupt Control Structure
 **************************************************************************/

#ifndef _ASMLANGUAGE


typedef struct Int1Control {
unsigned long        			IntrW0Status;
#define XPT_RAV_CPU_INTR		0x80000000
#define AVD0_CPU_INTR_0			0x40000000
/* reserved 				0x20000000 */
#define BVNF_CPU_INTR_3			0x10000000
#define reserved00			0x08000000
#define USB_CPU_INTR			0x04000000
#define UPG_UART2_CPU_INTR		0x02000000
#define UPG_UART1_CPU_INTR		0x01000000
#define SUN_CPU_INTR			0x00800000
#define reserved01			0x00400000
#define UPG_UART0_CPU_INTR		0x00200000
#define UPG_SPI_CPU_INTR		0x00100000
#define UPG_BSC_CPU_INTR		0x00080000
#define UPG_CPU_INTR			0x00040000
#define UPG_TMR_CPU_INTR		0x00020000
#define BVNG_CPU_INTR			0x00010000
#define ENET_CPU_INTR			0x00008000
#define BVNF_CPU_INTR_2			0x00004000
#define BVNF_CPU_INTR_1			0x00002000
#define BVNF_CPU_INTR_0			0x00001000
#define BVNB_CPU_INTR			0x00000800
#define VEC_CPU_INTR			0x00000400
#define RPTD0_CPU_INTR			0x00000200
#define HDMI_CPU_INTR			0x00000100
#define GFX_CPU_INTR			0x00000080
#define AIO_CPU_INTR			0x00000040
#define BSP_CPU_INTR			0x00000020
#define reserved02			0x0000001c
#define XPT_STATUS_CPU_INTR	 	0x00000002
#define HIF_CPU_INTR			0x00000001

unsigned long        			IntrW1Status;
#define USB_OHCI_1_CPU_INTR	 	0x80000000
#define USB_OHCI_0_CPU_INTR	 	0x40000000
#define USB_EHCI_CPU_INTR		0x20000000
#define UPG_108_IRQ_CPU_INTR	 	0x10000000
#define XPT_FE_CPU_INTR			0x08000000
#define XPT_PCR_CPU_INTR		0x04000000
#define RPTD1_CPU_INTR			0x02000000
#define USB_EHCI_1_CPU_INTR	 	0x01000000
#define MEMARB_CPU_INTR			0x00800000
#define reserved10			0x00400000
#define IPI1_CPU_INTR			0x00200000
#define IPI0_CPU_INTR			0x00100000
#define reserved11			0x00080000
#define MIPS0_CPU_INTR			0x00040000
#define FGX_CPU_INTR			0x00020000
#define ARB_RTS_CPU_INTR		0x00010000
#define ARB_CRIT_CPU_INTR		0x00008000
#define ARB_ARCH_CPU_INTR		0x00004000
#define FGT_TRB_CPU_INTR		0x00002000
#define reserved12			0x00001000
#define OFE_IDE_CPU_INTR		0x00000800
/* reserved				0x00000400 */
/* reserved 				0x00000200 */
#define EXT_IRQ_4_CPU_INTR		0x00000100
#define EXT_IRQ_3_CPU_INTR		0x00000080
#define EXT_IRQ_2_CPU_INTR		0x00000040
#define EXT_IRQ_1_CPU_INTR		0x00000020
#define EXT_IRQ_0_CPU_INTR		0x00000010
/* reserved				0x00000008 */
#define OFE_PROC_CPU_INTR		0x00000004
#define reserved13			0x00000002
#define PCI_INTA_0_CPU_INTR	 	0x00000001

  unsigned long        IntrW0MaskStatus;
  unsigned long        IntrW1MaskStatus;
  unsigned long        IntrW0MaskSet;
  unsigned long        IntrW1MaskSet;
  unsigned long        IntrW0MaskClear;
  unsigned long        IntrW1MaskClear;
} Int1Control;                                


#endif /* _ASMLANGUAGE */


/***************************************************************************
 *  Level 1 Interrupt Function Prototypes
 **************************************************************************/
extern void CPUINT1_SetInt1Control(Int1Control *int1c);
extern unsigned long CPUINT1_GetInt1ControlAddr(void);
extern void CPUINT1_Isr(void);
extern void CPUINT1_Disable(unsigned long intId);
extern void CPUINT1_Enable(unsigned long intId);
extern int  CPUINT1_ConnectIsr(unsigned long intId, FN_L1_ISR pfunc,
                               void *param1, int param2);


#ifdef __cplusplus
}
#endif

#endif /* _INT1_API_H */

