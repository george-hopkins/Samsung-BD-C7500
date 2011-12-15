/*---------------------------------------------------------------------------

    Copyright (c) 2001-2007 Broadcom Corporation                 /\
                                                          _     /  \     _
    _____________________________________________________/ \   /    \   / \_
                                                            \_/      \_/  

 Copyright (c) 2007 Broadcom Corporation
 All rights reserved.
 
 Redistribution and use of this software in source and binary forms, with or
 without modification, are permitted provided that the following conditions
 are met:
 
 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
 
 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
 
 * Neither the name of Broadcom Corporation nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission of Broadcom Corporation.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.

 File: pmlib.c

 Description:
 Power management API for Broadcom STB/DTV peripherals

    when        who         what
    -----       ---         ----
    20071030    cernekee    initial version
 ------------------------------------------------------------------------- */

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pmlib.h>

struct brcm_pm_priv
{
	struct brcm_pm_state	last_state;
};

#define BUF_SIZE	64
#define MAX_ARGS	16

#define SYS_USB_STAT	"/sys/devices/platform/brcm-pm/usb"
#define SYS_ENET_STAT	"/sys/devices/platform/brcm-pm/enet"
#define SYS_SATA_STAT	"/sys/devices/platform/brcm-pm/sata"
#define SYS_DDR_STAT	"/sys/devices/platform/brcm-pm/ddr"
#define SYS_TP1_STAT	"/sys/devices/system/cpu/cpu1/online"
#define SYS_BASE_FREQ	"/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq"
#define SYS_CUR_FREQ	"/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"
#define DHCPCD_PIDFILE	"/etc/config/dhcpc/dhcpcd-eth0.pid"
#define DHCPCD_PATH	"/bin/dhcpcd"
#define IFCONFIG_PATH	"/bin/ifconfig"
#define HDPARM_PATH	"/sbin/hdparm"
#define SATA_RESCAN_GLOB "/sys/class/scsi_host/host*/scan"
#define SATA_DEVICE_GLOB "/sys/class/scsi_device/*/device/block:*"
#define SATA_DELETE_GLOB "/sys/class/scsi_device/*/device/delete"

static int sysfs_get(char *path, unsigned int *out)
{
	FILE *f;
	unsigned int tmp;

	f = fopen(path, "r");
	if(! f)
		return(-1);
	if(fscanf(f, "%u", &tmp) != 1)
	{
		fclose(f);
		return(-1);
	}
	*out = tmp;
	fclose(f);
	return(0);
}

static int sysfs_set(char *path, int in)
{
	FILE *f;
	char buf[BUF_SIZE];

	f = fopen(path, "w");
	if(! f)
		return(-1);
	sprintf(buf, "%u", in);
	if(! fputs(buf, f) < 0)
	{
		fclose(f);
		return(-1);
	}
	fclose(f);
	return(0);
}

static int sysfs_set_string(char *path, const char *in)
{
	FILE *f;

	f = fopen(path, "w");
	if(! f)
		return(-1);
	if(! fputs(in, f) < 0)
	{
		fclose(f);
		return(-1);
	}
	fclose(f);
	return(0);
}

static int run(char *prog, ...)
{
	va_list ap;
	int status, i = 1;
	pid_t pid;
	char *args[MAX_ARGS], *a;

	va_start(ap, prog);

	pid = fork();
	if(pid < 0)
		return(-1);
	
	if(pid != 0)
	{
		wait(&status);
		va_end(ap);
		return(WEXITSTATUS(status) ? -1 : 0);
	}

	/* child */
	args[0] = prog;
	do
	{
		a = va_arg(ap, char *);
		args[i++] = a;
	} while(a);

	execv(prog, args);
	_exit(1);

	va_end(ap);	/* never reached */
	return(0);
}

void *brcm_pm_init(void)
{
	struct brcm_pm_priv *ctx;

	ctx = (void *)malloc(sizeof(*ctx));
	if(! ctx)
		goto bad;

	if(sysfs_get(SYS_BASE_FREQ,
		(unsigned int *)&ctx->last_state.cpu_base) != 0)
	{
		/* cpufreq not supported on this platform */
		ctx->last_state.cpu_base = 0;
	}
	
	if(brcm_pm_get_status(ctx, &ctx->last_state) != 0)
		goto bad_free;
	
	return(ctx);

bad_free:
	free(ctx);
bad:
	return(NULL);
}

void brcm_pm_close(void *vctx)
{
	free(vctx);
}

int brcm_pm_get_status(void *vctx, struct brcm_pm_state *st)
{
	struct brcm_pm_priv *ctx = vctx;
	int tmp;

	/* read status from /proc */

	if(sysfs_get(SYS_USB_STAT, (unsigned int *)&st->usb_status) != 0) {
		st->usb_status = BRCM_PM_UNDEF;
	}
	if(sysfs_get(SYS_ENET_STAT, (unsigned int *)&st->enet_status) != 0) {
		st->enet_status = BRCM_PM_UNDEF;
	}
	if(sysfs_get(SYS_SATA_STAT, (unsigned int *)&st->sata_status) != 0) {
		st->sata_status = BRCM_PM_UNDEF;
	}
	if(sysfs_get(SYS_DDR_STAT, (unsigned int *)&st->ddr_timeout) != 0) {
		st->ddr_timeout = BRCM_PM_UNDEF;
	}
	if(sysfs_get(SYS_TP1_STAT, (unsigned int *)&st->tp1_status) != 0) {
		st->tp1_status = BRCM_PM_UNDEF;
	}

	if(ctx->last_state.cpu_base)
	{
		if((sysfs_get(SYS_CUR_FREQ, (unsigned int *)&tmp) != 0) ||
		   (tmp == 0))
		{
			return(-1);
		}
		
		st->cpu_base = ctx->last_state.cpu_base;
		st->cpu_divisor = st->cpu_base / tmp;
	} else {
		st->cpu_base = 0;
		st->cpu_divisor = -1;
	}

	if(st != &ctx->last_state)
		memcpy(&ctx->last_state, st, sizeof(*st));

	return(0);
}

static int sata_rescan_hosts(void)
{
	glob_t g;
	int i, ret = 0;

	if(glob(SATA_RESCAN_GLOB, GLOB_NOSORT, NULL, &g) != 0)
		return(-1);

	for(i = 0; i < (int)g.gl_pathc; i++)
		ret |= sysfs_set_string(g.gl_pathv[i], "0 - 0");
	globfree(&g);

	return(ret);
}

static int sata_spindown_devices(void)
{
	glob_t g;
	int i, ret = 0;

	/* NOTE: if there are no devices present, it is not an error */
	if(glob(SATA_DEVICE_GLOB, GLOB_NOSORT, NULL, &g) != 0)
		return(0);

	for(i = 0; i < (int)g.gl_pathc; i++)
	{
		char *devname = rindex(g.gl_pathv[i], ':');
		char buf[BUF_SIZE];
		
		if(! devname)
			return(-1);
		snprintf(buf, BUF_SIZE, "/dev/%s", devname + 1);

		/* ignore return value as some devices will fail */
		run(HDPARM_PATH, "-y", buf, NULL);
	}
	globfree(&g);

	return(ret);
}

static int sata_delete_devices(void)
{
	glob_t g;
	int i, ret = 0;

	if(glob(SATA_DELETE_GLOB, GLOB_NOSORT, NULL, &g) != 0)
		return(0);

	for(i = 0; i < (int)g.gl_pathc; i++)
		ret |= sysfs_set(g.gl_pathv[i], 1);
	globfree(&g);

	return(ret);
}

int brcm_pm_set_status(void *vctx, struct brcm_pm_state *st)
{
	struct brcm_pm_priv *ctx = vctx;
	int ret = 0;

#define CHANGED(element) \
	((st->element != BRCM_PM_UNDEF) && \
	 (st->element != ctx->last_state.element))

	
	if(CHANGED(usb_status))
	{
		ret |= sysfs_set(SYS_USB_STAT, st->usb_status ? 1 : 0);
		ctx->last_state.usb_status = st->usb_status;
	}

	if(CHANGED(enet_status))
	{
		if(st->enet_status)
		{
			ret |= run(DHCPCD_PATH, "-H", "eth0", NULL);
		} else {
			unsigned int pid;

			if(sysfs_get(DHCPCD_PIDFILE, &pid) == 0)
				kill(pid, SIGTERM);
			ret |= run(IFCONFIG_PATH, "eth0", "down", NULL);
			ret |= sysfs_set(SYS_ENET_STAT, 0);
		}
		ctx->last_state.enet_status = st->enet_status;
	}

	if(CHANGED(sata_status))
	{
		if(st->sata_status)
		{
			ret |= sata_rescan_hosts();
		} else {
			ret |= sata_spindown_devices();
			ret |= sata_delete_devices();
			ret |= sysfs_set(SYS_SATA_STAT, 0);
		}
		ctx->last_state.sata_status = st->sata_status;
	}

	if(CHANGED(tp1_status))
	{
		ret |= sysfs_set(SYS_TP1_STAT, st->tp1_status);
	}

	if(CHANGED(cpu_divisor))
	{
		if(! st->cpu_divisor || ! ctx->last_state.cpu_base)
		{
			ret |= -1;
		} else {
			unsigned int freq;

			freq = ctx->last_state.cpu_base / st->cpu_divisor;
			ret |= sysfs_set(SYS_CUR_FREQ, freq);
		}
		ctx->last_state.cpu_divisor = st->cpu_divisor;
	}

	if(CHANGED(ddr_timeout))
	{
		ret |= sysfs_set(SYS_DDR_STAT, st->ddr_timeout);
	}

#undef CHANGED

	return(ret);
}
