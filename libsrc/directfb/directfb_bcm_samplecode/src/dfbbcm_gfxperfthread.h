
typedef struct GfxPerfThreadRoutineContext *GfxPerfThreadRoutineContext_t;

/* For now */
typedef struct {
    pthread_t               thread;
    GfxPerfThreadRoutineContext_t threadRoutineContext;
} GfxPerfThread_t;

DFBResult GfxPerfThreadCreate(IDirectFB * dfb, GfxPerfThread_t * pGfxPerfThread);
DFBResult GfxPerfThreadStart(GfxPerfThread_t * pGfxPerfThread);
DFBResult GfxPerfThreadStop(GfxPerfThread_t * pGfxPerfThread);
DFBResult GfxPerfThreadDelete(GfxPerfThread_t * pGfxPerfThread);
