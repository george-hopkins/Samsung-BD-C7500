
/*
 * Broadcom power management for DVD SoC devices
 */


/*
 * power events interrupts
 */

#define PWR_UART_0	0x0001
#define PWR_UART_1	0x0002
#define PWR_UART_2	0x0004
#define PWR_UART_3	0x0008
#define PWR_IRR		0x0010
#define PWR_TIMER	0x0020
#define PWR_GPIO	0x0040

#define PWR_SUNDRY_MASK 0x00ff

#define PWR_ENET	0x1000
#define PWR_HDMI	0x2000

#define PWR_EVENT_MASK	0xffff

enum power_events {
	INIT = 1,
	GO_ACTIVE,
	GO_STANDBY,		/* implies a wait */
	GO_DARK,
	GO_SHUTDOWN,		/* reboot */
	SET_MASK,
	GET_MASK,
};

typedef struct power_event {
	enum power_events	type;
	unsigned int		id;
	unsigned int		events;	/* pending events */
	unsigned int		mask;	/* event enabled mask */
	unsigned int		timeout;/* wakeup after timeout seconds */
} power_event_t;

#define PW_MGMT_MSG	_IOWR('P', 1, power_event_t)	/* power event message */
