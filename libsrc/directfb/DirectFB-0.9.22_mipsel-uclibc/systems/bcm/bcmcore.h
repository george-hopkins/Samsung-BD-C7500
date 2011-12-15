/**
 * Copyright (c) 2005-2009, Broadcom Corporation
 *    All Rights Reserved
 *    Confidential Property of Broadcom Corporation
 *
 * THIS SOFTWARE MAY ONLY BE USED SUBJECT TO AN EXECUTED SOFTWARE LICENSE
 * AGREEMENT  BETWEEN THE USER AND BROADCOM.  YOU HAVE NO RIGHT TO USE OR
 * EXPLOIT THIS MATERIAL EXCEPT SUBJECT TO THE TERMS OF SUCH AN AGREEMENT.
 */
#ifndef __BCMCORE_H__
#define __BCMCORE_H__

#include <core/coretypes.h>

#include <core/system.h>

#include <fusion/call.h>
#include <fusion/reactor.h>

#ifdef BCMUSESLINUXINPUTS
#include "../fbdev/vt.h"
#endif

#include <bdvd.h>

#include "bcmmixer.h"
#include "bdvd_bcc.h"

/**
 * bcmgraphics_context_shared is the part of bcmgraphics_context
 * that is allocated in shared memory for multi-app support.
 */
typedef struct {
    /* This is for multi app support, if we ever support that */
    FusionSkirmish           lock; /* For concurrency on the bsettop api fcts */
    FusionCall               call; /* Main call handler for Remote Procedure Calls (RPC), only for Multi anyway */
} BCMCoreContextShared;

/* Eventually this info should be shared if possible in case
 * we do add multi-app core support
 */

typedef struct {
    BCMCoreLayerType          bcmcore_type;
    BCMMixerHandle            bcmcore_mixer;
    char *                    layer_name;
    bool                      premultiplied;
} BCMCoreLayerCreateSettings;

typedef struct {
    BCMCoreLayerHandle handle;
    BCMMixerHandle        mixer;
    unsigned int          mixed_layer_id;
    BCMCoreLayerHandle    layer_locked_ext;
} BCMCoreContextLayer;

#define BCM_MAX_DISPLAY_WINDOWS 3

/**
 * bcmgraphics_context is a structure containing handles
 * used by the BCMCore module.
 */
typedef struct {
    BCMCoreContextShared      *shared;

    CoreDFB                   *core;

    bdvd_display_h             primary_display_handle;
    bdvd_display_h             secondary_display_handle;

    bdvd_video_format_e        video_format;

    /* This is temporary, for the 7440Ax, will make a structure to
     * assign statically the ids.
     */
    BCMCoreGraphicsWindowContext  primary_graphics_window[BCM_MAX_DISPLAY_WINDOWS];
    BCMCoreGraphicsWindowContext  secondary_graphics_window;
    BCMCoreVgraphicsWindowContext vgraphics_window;

    bdvd_graphics_constraints_t graphics_constraints;
    bool                        waitforflip;

    bdvd_graphics_compositor_h driver_compositor;
    bdvd_graphics_compositor_h mixer_compositor;

    bdvd_bcc_h      driver_bcc_pg;
    bdvd_bcc_h      driver_bcc_textst;
    bdvd_bcc_h      driver_bcc_ig;
    bdvd_bcc_h      driver_bcc_other;
    bdvd_sid_handle driver_sid;
    bdvd_dtk_h      driver_fgx;

    void * (*system_video_memory_virtual)( unsigned int offset );

    struct {
        pthread_mutex_t     mutex;
        pthread_mutexattr_t mutex_attr;
        BCMCoreContextLayer layer[MAX_LAYERS];
    } bcm_layers;

    BCMMixerHandle             mixers[BCM_MIXER_L];

    bool hdi_mixer_issue_clearrect;

#ifdef BCMUSESLINUXINPUTS
    VirtualTerminal           *vt;
#endif

	bool MemoryMode256MB;
	bool DualHeap;
} BCMCoreContext;

#define DSCAPS_BCC  (DSCAPS_COPY_BLIT_IG | DSCAPS_COPY_BLIT_OTHR | \
                     DSCAPS_COPY_BLIT_PG | DSCAPS_COPY_BLIT_TEXTST)

#endif /* __BCMCORE_H__ */
