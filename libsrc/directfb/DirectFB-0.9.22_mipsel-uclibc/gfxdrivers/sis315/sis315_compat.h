#ifndef _SIS315_COMPAT_H
#define _SIS315_COMPAT_H

#include <linux/fb.h>

#ifndef FB_ACCEL_SIS_GLAMOUR_2
#define FB_ACCEL_SIS_GLAMOUR_2  40	/* SiS 315, 650, 740            */
#endif
#ifndef FB_ACCEL_SIS_XABRE
#define FB_ACCEL_SIS_XABRE      41	/* SiS 330 ("Xabre")            */
#endif

#include <linux/sisfb.h>

/* ioctl to enable/disable panning auto-maximize (like nomax parameter) */
#ifndef SISFB_GET_AUTOMAXIMIZE
#define SISFB_GET_AUTOMAXIMIZE	_IOR('n',0xFA,__u32)
#endif
#ifndef SISFB_SET_AUTOMAXIMIZE
#define SISFB_SET_AUTOMAXIMIZE	_IOW('n',0xFA,__u32)
#endif

/* think KERNEL_VERSION(a,b,c) */
#ifndef SISFB_VERSION
#define SISFB_VERSION(a,b,c)	(((a) << 16) + ((b) << 8) + (c))
#endif

#endif /* _SIS315_COMPAT_H */
