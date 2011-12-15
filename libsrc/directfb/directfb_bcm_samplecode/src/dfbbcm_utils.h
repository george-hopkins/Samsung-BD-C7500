#ifndef DFBBCM_UTILS_H_
#define DFBBCM_UTILS_H_

#include <directfb.h>
#include <direct/debug.h>

#include <bdvd.h>

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

/* macro for a safe call to DirectFB functions */
#define DFBCHECK(x...)                                                    \
     {                                                                    \
          err = x;                                                        \
          if (err != DFB_OK) {                                            \
               fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ );     \
               DirectFBError( #x, err );                                  \
          }                                                               \
     }

typedef enum {
    ePaletteGenType_red_gradient,
    ePaletteGenType_green_gradient,
    ePaletteGenType_blue_gradient
} ePaletteGenType;

static unsigned int rand_pool = 0x12345678;
static unsigned int rand_add  = 0x87654321;

/* Those two function were stolen from the df_ examples.
 */
static inline unsigned int myrand()
{
     rand_pool ^= ((rand_pool << 7) | (rand_pool >> 25));
     rand_pool += rand_add;
     rand_add  += rand_pool;

     return rand_pool;
}

static inline uint32_t myclock()
{
     struct timeval tv;

     gettimeofday (&tv, NULL);

     return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

/* Timer control routines */
static inline uint32_t mymsleep(uint32_t ms)
{
    struct timespec timeout;
    struct timespec timerem;

    timeout.tv_sec = ms / 1000ul;
    timeout.tv_nsec = (ms % 1000ul) * 1000000ul;

    do {
        if(nanosleep(&timeout, &timerem) == -1) {
            if (errno == EINTR) {
                memcpy(&timeout, &timerem, sizeof(timeout));
            }
            else {
                D_ERROR("mymsleep: %s", strerror(errno));
                return 0;
            }
        }
        else {
            return 0;
        }
    } while (1);
}

DFBEnumerationResult
enum_layers_callback( unsigned int                 id,
                      DFBDisplayLayerDescription   desc,
                      void                        *data );

typedef struct {
     const char            *string;
     DFBSurfacePixelFormat  format;
} FormatString;

extern FormatString format_strings[];

uint32_t
nb_format_strings();

DFBSurfacePixelFormat
convert_string2dspf( const char * format );

const char *
convert_dspf2string( DFBSurfacePixelFormat format );

DFBResult
display_set_video_format(bdvd_display_h display,
                         bdvd_video_format_e video_format);

DFBResult
generate_palette_pattern_entries( IDirectFBPalette *palette, ePaletteGenType gen_type );

static int dx;
static int dy;

/* Ok this may not be the most efficient algorithm, I did
 * that in 5 minutes
 */
static inline void
movement_edgetoedge_crosshair(
    int * x, int * y,
    const DFBRegion * boundaries) {

    bool change_dx = false;
    bool change_dy = false;

    if (*x == boundaries->x1) {
        dx = -1;
        change_dx = true;
    }

    if (*x == boundaries->x2) {
        dx = 1;
        change_dx = true;
    }

    if (*y == boundaries->y1) {
        dy = -1;
        change_dy = true;
    }

    if (*y == boundaries->y2) {
        dy = 1;
        change_dy = true;
    }

    if (!change_dx && !change_dy) {
        goto move;
    }

#if 1 /* Too much code for nothing */

    if (change_dx) {

        /* Trigger a direction change */
        switch (dx) {
            case -1:
                dx = myrand()%2;
                switch (dx) {
                    case 0:
                        if (!change_dy) {
                            if (dy)
                                dx = 0;
                            else
                                dx = 1;
                        }
                        else {
                            dx = 0;
                        }
                    break;
                    case 1:
                        dx = 1;
                    break;
                    default:
                        fprintf(stderr, "Not supposed to happen, got dx %d\n", dx);
                        abort();
                    break;
                }
            break;
            case 0:
                dx = myrand()%2;
                switch (dx) {
                    case 0:
                        dx = -1;
                    break;
                    case 1:
                        dx = 1;
                    break;
                    default:
                        fprintf(stderr, "Not supposed to happen, got dx %d\n", dx);
                        abort();
                    break;
                }
            break;
            case 1:
                dx = myrand()%2;
                switch (dx) {
                    case 0:
                        dx = -1;
                    break;
                    case 1:
                        if (!change_dy) {
                            if (dy)
                                dx = 0;
                            else
                                dx = -1;
                        }
                        else {
                            dx = 0;
                        }
                    break;
                    default:
                        fprintf(stderr, "Not supposed to happen, got dx %d\n", dx);
                        abort();
                    break;
                }
            break;
            default:
                fprintf(stderr, "Not supposed to happen, got dx %d\n", dx);
                abort();
            break;
        }

    }

    if (change_dy) {

        /* Trigger a direction change in y, also check */
        switch (dy) {
            case -1:
                dy = myrand()%2;
                switch (dy) {
                    case 0:
                        if (dx)
                            dy = 0;
                        else
                            dy = 1;
                    break;
                    case 1:
                        dy = 1;
                    break;
                    default:
                        fprintf(stderr, "Not supposed to happen, got dy %d\n", dy);
                        abort();
                    break;
                }
            break;
            case 0:
                dy = myrand()%2;
                switch (dy) {
                    case 0:
                        dy = -1;
                    break;
                    case 1:
                        dy = 1;
                    break;
                    default:
                        fprintf(stderr, "Not supposed to happen, got dy %d\n", dy);
                        abort();
                    break;
                }
            break;
            case 1:
                dy = myrand()%2;
                switch (dy) {
                    case 0:
                        dy = -1;
                    break;
                    case 1:
                        if (dx)
                            dy = 0;
                        else
                            dy = -1;
                    break;
                    default:
                        fprintf(stderr, "Not supposed to happen, got dy %d\n", dy);
                        abort();
                    break;
                }
            break;
            default:
                fprintf(stderr, "Not supposed to happen, got dy %d\n", dy);
                abort();
            break;
        }

    }

#endif

move:

    *x += dx;
    *y += dy;

}

static inline void
movement_edgetoedge_bresenham_scanline(
    int x, int y,
    int * dx, int * dy,
    const DFBRegion * boundaries) {
/* TODO */
}

void dump_mem(uint32_t addr, uint8_t * buffer, uint32_t word_size, int len);

typedef struct {
    pthread_mutex_t cond_mutex;
    pthread_mutexattr_t cond_mutex_attr;
    pthread_cond_t cond;
    pthread_condattr_t cond_attr;
} wakeup_call_t;

DFBResult
init_wakeup_call(wakeup_call_t * wakeup_call);

/* WE NEED TO IMPLEMENT THIS SO THE WAKE UP CALL IS READ
 * PROTECTED BY THE COND MUTEX SOMEHOW
 */
DFBResult
wait_wakeup_call(wakeup_call_t * wakeup_call);

DFBResult
broadcast_wakeup_call(wakeup_call_t * wakeup_call);

DFBResult
uninit_wakeup_call(wakeup_call_t * wakeup_call);

#endif // DFBBCM_UTILS_H_
