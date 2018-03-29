/* -*- Mode: c; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <getopt.h>             /* for getopt_long */

#include "pjconfig.h"
#include "pjbase.h"
#include "pj.h"
#include "graphics.h"

#include <fcntl.h>              /* O_RDONLY/... */
#include <termios.h>            /* VINTR/VEOF/... */


#define MAX_SOURCE_BUF (1024*64)
#define MOUSE_DEVICE_PATH "/dev/input/mouse0"


typedef struct {
    const char *path;
    time_t last_modify_time;
} SourceObject;

struct PJContext_ {
    Graphics *graphics;
    Graphics_LAYOUT layout_backup;
    int is_fullscreen;
    int use_backbuffer;
    struct {
        int x, y;
        int fd;
    } mouse;
    double fade;
    double time_origin;
    unsigned int frame;         /* TODO: move to graphics */
    struct {
        int debug;
        int render_time;
    } verbose;
    struct {
        int numer;
        int denom;
    } scaling;
};


#define PJDebug(pj, printf_arg) ((pj)->verbose.debug ? (printf printf_arg) : 0)


static double GetCurrentTimeInMilliSecond(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

// 10 minute le plus proche
static double GetOriginTimeInMilliSecond(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec/600)*600 * 1000.0;
}

static int GetLastFileModifyTime(const char *file_path, time_t *out_mod_time)
{
    struct stat sb;
    if (stat(file_path, &sb) == -1) {
        return 1;
    }
    *out_mod_time = sb.st_mtime;
    return 0;
}

/* SourceObject */
static SourceObject *SourceObject_Create(const char *path)
{
    SourceObject *so;
    so = malloc(sizeof(*so));
    so->path = path;
    so->last_modify_time = 0;
    return so;
}

static void SourceObject_Delete(void *so)
{
    free(so);
}

/* PJContext */
int PJContext_Construct(PJContext *pj)
{
    int scaling_numer, scaling_denom;
    scaling_numer = 1;
    scaling_denom = 2;
    pj->graphics = Graphics_Create(Graphics_LAYOUT_RIGHT_TOP,
                                   scaling_numer, scaling_denom);
    if (!pj->graphics) {
        fprintf(stderr, "Graphics Initialize failed:\r\n");
        fprintf(stderr, " maybe GPU memory allocation failed\r\n");
        fprintf(stderr, " modify /boot/config.txt\r\n");
        fprintf(stderr, "  'gpu_mem_256' or 'gpu_mem_512' assign to more than today:)\r\n");
        fprintf(stderr, " see: http://elinux.org/RPiconfig#Memory\r\n");
        return 2;
    }

    pj->layout_backup = Graphics_LAYOUT_FULLSCREEN;
    pj->is_fullscreen = 0;
    pj->use_backbuffer = 0;
    pj->fade=1.0;
    pj->mouse.x = 0;
    pj->mouse.y = 0;
    pj->mouse.fd = open(MOUSE_DEVICE_PATH, O_RDONLY | O_NONBLOCK);
    pj->time_origin = GetOriginTimeInMilliSecond();
    pj->frame = 0;
    pj->verbose.render_time = 0;
    pj->verbose.debug = 1;
    pj->scaling.numer = scaling_numer;
    pj->scaling.denom = scaling_denom;
	printf("Origin time is %lf\n",pj->time_origin);
    return 0;
}

void PJContext_Destruct(PJContext *pj)
{
    int i;
    RenderLayer *layer;

    if (pj->mouse.fd >= 0) {
        close(pj->mouse.fd);
    }
    for (i = 0; (layer = Graphics_GetRenderLayer(pj->graphics, i)) != NULL; i++) {
        SourceObject_Delete(RenderLayer_GetAux(layer));
    }
    Graphics_Delete(pj->graphics);
}

static int PJContext_ChangeLayout(PJContext *pj, Graphics_LAYOUT layout)
{
    Graphics_SetLayout(pj->graphics, layout);
    Graphics_ApplyLayoutChange(pj->graphics);

    {
        int width, height;
        Graphics_GetWindowSize(pj->graphics, &width, &height);
        printf("size = %dx%d px\r\n", width, height);
    }
    return 0;
}

static int PJContext_Relayout(PJContext *pj, int forward)
{
    Graphics_LAYOUT layout;
    pj->is_fullscreen = 0;
    layout = Graphics_GetLayout(Graphics_GetCurrentLayout(pj->graphics), forward);
    return PJContext_ChangeLayout(pj, layout);
}

static int PJContext_NextLayout(PJContext *pj)
{
    return PJContext_Relayout(pj, 1);
}

static int PJContext_PreviousLayout(PJContext *pj)
{
    return PJContext_Relayout(pj, 0);
}

int PJContext_SwitchFullscreen(PJContext *pj)
{
    if (pj->is_fullscreen) {
        pj->is_fullscreen = 0;
        return PJContext_ChangeLayout(pj, pj->layout_backup);
    } else {
        pj->is_fullscreen = 1;
        pj->layout_backup = Graphics_GetCurrentLayout(pj->graphics);
        return PJContext_ChangeLayout(pj, Graphics_LAYOUT_FULLSCREEN);
    }
}

static int PJContext_SwitchBackbuffer(PJContext *pj)
{
    pj->use_backbuffer ^= 1;
    Graphics_SetBackbuffer(pj->graphics, pj->use_backbuffer);
    return Graphics_ApplyOffscreenChange(pj->graphics);
}

static int PJContext_ChangeScaling(PJContext *pj, int add)
{
    pj->scaling.denom += add;
    if (pj->scaling.denom <= 0) {
        pj->scaling.denom = 1;
    }
    if (pj->scaling.denom >= 16) {
        pj->scaling.denom = 16;
    }
    Graphics_SetWindowScaling(pj->graphics, pj->scaling.numer, pj->scaling.denom);
    Graphics_ApplyWindowScalingChange(pj->graphics);
    {
        int width, height;
        Graphics_GetSourceSize(pj->graphics, &width, &height);
        printf("offscreen size = %dx%d px, scaling = %d/%d\r\n",
               width, height, pj->scaling.numer, pj->scaling.denom);
    }
    return 0;
}

static int PJContext_ReloadAndRebuildShadersIfNeed(PJContext *pj)
{
    int i;
    RenderLayer *layer;

    /* PJDebug(pj, ("PJContext_ReloadAndRebuildShadersIfNeed\r\n")); */
    for (i = 0; (layer = Graphics_GetRenderLayer(pj->graphics, i)) != NULL; i++) {
        time_t t;
        SourceObject *so;
        so = RenderLayer_GetAux(layer);
        if (GetLastFileModifyTime(so->path, &t)) {
            fprintf(stderr, "file open failed: %s\r\n", so->path);
            continue;
        }
        if (so->last_modify_time != t) {
		printf("** rebuild shader.\n");
            FILE *fp;
            fp = fopen(so->path, "r");
            if (fp == NULL) {
                fprintf(stderr, "file open failed: %s\r\n", so->path);
            } else {
                size_t len;
                char code[MAX_SOURCE_BUF]; /* hmm.. */
                errno = 0;
                len = fread(code, 1, sizeof(code), fp);
                /* TODO: handle errno */
                if (ferror(fp) != 0) {
                    PJDebug(pj, ("ferror = %d\r\n", ferror(fp)));
                }
                fclose(fp);
                if (errno != 0) {
                    PJDebug(pj, ("errno = %d\r\n", errno));
                }
                PJDebug(pj, ("update: %s\r\n", so->path));
                RenderLayer_UpdateShaderSource(layer, code, (int)len);
                so->last_modify_time = t;
                Graphics_BuildRenderLayer(pj->graphics, i);
            }
        }
    }
    return 0;
}

int PJContext_ChangeLayerShader(PJContext *pj,const char *fname,int layerNum)
{
    int i;
    RenderLayer *layer;

    layer = Graphics_GetRenderLayer(pj->graphics, layerNum);
	if( layer==NULL ) {
			printf("ChangeLayerShader: illegal layer=%d\n",layerNum);
			return -1;
	}

	time_t t;
	SourceObject *so;
	so = RenderLayer_GetAux(layer);

	// pas tres safe...
	so->path=fname;

	FILE *fp;
	fp = fopen(so->path, "r");
	if (fp == NULL) {
		fprintf(stderr, "file open failed: %s\r\n", so->path);
	} else {
		size_t len;
		char code[MAX_SOURCE_BUF]; /* hmm.. */
		errno = 0;
		len = fread(code, 1, sizeof(code), fp);
		/* TODO: handle errno */
		if (ferror(fp) != 0) {
			PJDebug(pj, ("ferror = %d\r\n", ferror(fp)));
		}
		fclose(fp);
		if (errno != 0) {
			PJDebug(pj, ("errno = %d\r\n", errno));
		}
		PJDebug(pj, ("update: %s\r\n", so->path));
		RenderLayer_UpdateShaderSource(layer, code, (int)len);
		//so->last_modify_time = t;
		Graphics_BuildRenderLayer(pj->graphics, layerNum);
	}
    return 0;
}


// return 1 if mouse was moved
// fill the variables only if changed, and non NULL
int PJContext_UpdateMousePosition(PJContext *pj,int *mousex,int *mousey,int *w,int *h)
{
    int dx, dy;
    unsigned char buf[3];       /* /dev/input/mouse is 3 bytes units */
    size_t len;
    int err;
        int width, height;


	// init la base...
        Graphics_GetWindowSize(pj->graphics, &width, &height);
	if( mousex ) *mousex=pj->mouse.x;
	if( mousey ) *mousey=pj->mouse.y;
	if( w ) *w=width;
	if( h ) *h=height;


    if (pj->mouse.fd < 0) {
        return 0;
    }

    /* /dev/input/mouse spec
        see: http://www.unixresources.net/linux/lf/45/archive/00/00/09/60/96038.html
    */

    errno = 0;
    len = read(pj->mouse.fd, &buf, sizeof(buf));
    if (len < sizeof(buf)) {
        return 0;
    }
    err = errno;
    if (err == EWOULDBLOCK) {
        /* ok... try again next time */
        errno = 0;
        return 0;
    }
    if (err != 0) {
        if( err!=19 ) printf("error on mouse-read: code %d(%s)\r\n", err, strerror(err));
        errno = 0;
        return 0;
    }
    if (buf[0] & 0x08) {
        /* ok */
    } else {
        return 0;
    }

    /* button */
    if (buf[0] & 0x03) {
        /* TODO? */
    }

    /* movement */
    if (buf[0] & 0x10) { /* left */
        dx = buf[1] - 256;
    } else {            /* right */
        dx = buf[1];
    }
    if (buf[0] & 0x20) { /* down */
        dy = buf[2] - 256;
    } else {            /* up */
        dy = buf[2];
    }

    pj->mouse.x += dx;
    pj->mouse.y += dy;
    {
        /* fix mouse position */
        //Graphics_GetWindowSize(pj->graphics, &width, &height);
        if (pj->mouse.x < 0) {
            pj->mouse.x = 0;
        }
        if (pj->mouse.y < 0) {
            pj->mouse.y = 0;
        }
        if (pj->mouse.x >= width) {
            pj->mouse.x = width;
        }
        if (pj->mouse.y >= height) {
            pj->mouse.y = height;
        }
    }

	if( mousex ) *mousex=pj->mouse.x;
	if( mousey ) *mousey=pj->mouse.y;
	if( w ) *w=width;
	if( h ) *h=height;

        printf("mouse is (%d,%d) w=%d h=%d\n",pj->mouse.x,pj->mouse.y,width,height);
	return 1;
}

static void PJContext_SetUniforms(PJContext *pj)
{
    double t;
    double fade;
    double mouse_x, mouse_y;
    int width, height;

    t = GetCurrentTimeInMilliSecond() - pj->time_origin;
    
    fade=pj->fade;

    Graphics_GetWindowSize(pj->graphics, &width, &height);
    mouse_x = (double)pj->mouse.x / width;
    mouse_y = (double)pj->mouse.y / height;

    Graphics_SetUniforms(pj->graphics, t / 1000.0,fade,
                         mouse_x, mouse_y, drand48());
}

static void PJContext_Render(PJContext *pj)
{
    if (pj->verbose.render_time) {
        double t, ms;
        t = GetCurrentTimeInMilliSecond();
        Graphics_Render(pj->graphics);
        ms = GetCurrentTimeInMilliSecond() - t;
        printf("render time: %.1f ms (%.0f fps)    \r", ms, 1000.0 / ms);
    } else {
        Graphics_Render(pj->graphics);
    }
}

static void PJContext_AdvanceFrame(PJContext *pj)
{
    pj->frame += 1;
}

// return 1 if bad, 0 if good
static int PJContext_Update(PJContext *pj)
{
    //if( PJContext_ReloadAndRebuildShadersIfNeed(pj )) { return 1; }
    //PJContext_UpdateMousePosition(pj);
    PJContext_SetUniforms(pj);
    PJContext_Render(pj);
    PJContext_AdvanceFrame(pj);
    return 0;
}

static void PrintHelp(void)
{
    printf("Key:\r\n");
    printf("  t        FPS printing\r\n");
    printf("  f        switch to fullscreen mode\r\n");
    printf("  < or >   layout change\r\n");
    printf("  [ or ]   offscreen scaling\r\n");
    printf("  b        backbuffer ON/OFF\r\n");
    printf("  q        exit\r\n");
}

static int PJContext_HandleKeyboadEvent(PJContext *pj)
{
    return 0;
}

int PJContext_PrepareMainLoop(PJContext *pj)
{
	// a faire: tester les retours de ces deux fonctions...
    Graphics_AllocateOffscreen(pj->graphics);    
    PJContext_ReloadAndRebuildShadersIfNeed(pj);
	return(0);
}

// return 1 si error, 0 si ok
int PJContext_MainLoop(PJContext *pj)
{
    //for (;;) {
#ifdef SKIP
        switch (getchar()) {
        case 'Q':
        case 'q':
        case VEOF:      /* Ctrl+d */
        case VINTR:     /* Ctrl+c */
        case 0x7f:      /* Ctrl+c */
        case 0x03:      /* Ctrl+c */
        case 0x1b:      /* ESC */
            printf("\r\nexit\r\n");
            goto goal;
        case 'f':
        case 'F':
            if (PJContext_SwitchFullscreen(pj)) {
                printf("error\r\n");
                goto goal;
            }
            break;
        case '>':
            if (PJContext_NextLayout(pj)) {
                printf("error\r\n");
                goto goal;
            }
            break;
        case '<':
            if (PJContext_PreviousLayout(pj)) {
                printf("error\r\n");
                goto goal;
            }
            break;
        case ']':
            PJContext_ChangeScaling(pj, 1);
            break;
        case '[':
            PJContext_ChangeScaling(pj, -1);
            break;
        case 't':
        case 'T':
            pj->verbose.render_time ^= 1;
            printf("\r\n");
            break;
        case 'b':
            PJContext_SwitchBackbuffer(pj);
            printf("backbuffer %s\r\n", pj->use_backbuffer ? "ON": "OFF");
            break;
        case '?':
            PrintHelp();
        default:
            break;
        }
#endif
        if( PJContext_Update(pj)==0 ) { return(0); }
   // }
  goal:
    return(1);
}

int PJContext_SetFade(PJContext *pj, double fade) {
	pj->fade=fade;
}

int PJContext_AppendLayer(PJContext *pj, const char *path)
{
    SourceObject *so;
    FILE *fp;
    char code[MAX_SOURCE_BUF];
    size_t len;
    PJDebug(pj, ("PJContext_AppendLayer: %s\r\n", path));
    fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "file open failed: %s\r\n", path);
        return 1;
    }
    len = fread(code, 1, sizeof(code), fp);
    fclose(fp);
    so = SourceObject_Create(path);
    Graphics_AppendRenderLayer(pj->graphics, code, (int)len, (void *)so);
    return 0;
}

int PJContext_ParseArgs(PJContext *pj, int argc, const char *argv[])
{
    int i;
    int layer;
    Graphics *g;

    g = pj->graphics;
    layer = 0;
    for (i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if (strcmp(arg, "--debug") == 0) {
            pj->verbose.debug = 1;
        } else if (strcmp(arg, "--RGB888") == 0) {
            Graphics_SetOffscreenPixelFormat(g, Graphics_PIXELFORMAT_RGB888);
        } else if (strcmp(arg, "--RGBA8888") == 0) {
            Graphics_SetOffscreenPixelFormat(g, Graphics_PIXELFORMAT_RGBA8888);
        } else if (strcmp(arg, "--RGB565") == 0) {
            Graphics_SetOffscreenPixelFormat(g, Graphics_PIXELFORMAT_RGB565);
        } else if (strcmp(arg, "--nearestneighbor") == 0) {
            Graphics_SetOffscreenInterpolationMode(g, Graphics_INTERPOLATION_MODE_NEARESTNEIGHBOR);
        } else if (strcmp(arg, "--bilinear") == 0) {
            Graphics_SetOffscreenInterpolationMode(g, Graphics_INTERPOLATION_MODE_BILINEAR);
        } else if (strcmp(arg, "--wrap-clamp_to_edge") == 0) {
            Graphics_SetOffscreenWrapMode(g, Graphics_WRAP_MODE_CLAMP_TO_EDGE);
        } else if (strcmp(arg, "--wrap-repeat") == 0) {
            Graphics_SetOffscreenWrapMode(g, Graphics_WRAP_MODE_REPEAT);
        } else if (strcmp(arg, "--wrap-mirror_repeat") == 0) {
            Graphics_SetOffscreenWrapMode(g, Graphics_WRAP_MODE_MIRRORED_REPEAT);
        } else if (strcmp(arg, "--backbuffer") == 0) {
            pj->use_backbuffer = 1;
        } else {
            printf("layer %d: %s\r\n", layer, arg);
            PJContext_AppendLayer(pj, arg);
            layer += 1;
        }
    }
    Graphics_SetBackbuffer(g, pj->use_backbuffer);
    Graphics_ApplyOffscreenChange(pj->graphics);
    return (layer == 0) ? 1 : 0;
}

int PJContext_HostInitialize(void)
{
    Graphics_HostInitialize();
    return 0;
}

void PJContext_HostDeinitialize(void)
{
    Graphics_HostDeinitialize();
}

size_t PJContext_InstanceSize(void)
{
    return sizeof(PJContext);
}

int PJContext_Main(PJContext *pj)
{
    PJContext_PrepareMainLoop(pj);
    PJContext_MainLoop(pj);
    return EXIT_SUCCESS;
}


// le uniform "img"
void PJContext_SetImageTexture( PJContext *pj, int elemSize1, int elemSize, int cols, int rows, unsigned char *data )
{
	Graphics_SetImageTexture( pj->graphics, Graphics_TEX_NAME_IMG, elemSize1, elemSize, cols, rows, data );
}

void PJContext_SetImage2Texture( PJContext *pj, int elemSize1, int elemSize, int cols, int rows, unsigned char *data )
{
	Graphics_SetImageTexture( pj->graphics, Graphics_TEX_NAME_IMG2, elemSize1, elemSize, cols, rows, data );
}

void PJContext_SetLutTextureHi( PJContext *pj, int elemSize1, int elemSize, int cols, int rows, unsigned char *data )
{
	Graphics_SetImageTexture( pj->graphics, Graphics_TEX_NAME_LUTHI, elemSize1, elemSize, cols, rows, data );
}

void PJContext_SetLutTextureLow( PJContext *pj, int elemSize1, int elemSize, int cols, int rows, unsigned char *data )
{
	Graphics_SetImageTexture( pj->graphics, Graphics_TEX_NAME_LUTLOW, elemSize1, elemSize, cols, rows, data );
}



void PJContext_GetSurfaceContextDisplay( PJContext *pj, void *pSurface, void *pContext, void *pDisplay ) {
	Graphics_GetSurfaceContextDisplay( pj->graphics, pSurface, pContext, pDisplay );
}


void PJContext_GetVideoTextureNum( PJContext *pj, void *ptex ) {
	Graphics_GetVideoTextureNum( pj->graphics, ptex );
}



