/* -*- Mode: c; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#ifndef INCLUDED_PJ_H
#define INCLUDED_PJ_H
#include <stddef.h>
typedef struct PJContext_ PJContext;

int PJContext_HostInitialize(void);
void PJContext_HostDeinitialize(void);

size_t PJContext_InstanceSize(void);
int PJContext_Construct(PJContext *pj);
void PJContext_Destruct(PJContext *pj);
int PJContext_ParseArgs(PJContext *pj, int argc, const char *argv[]);
int PJContext_PrepareMainLoop(PJContext *pj);
int PJContext_MainLoop(PJContext *pj);

int PJContext_SwitchFullscreen(PJContext *pj);
int PJContext_AppendLayer(PJContext *pj, const char *path);

int PJContext_SetFade(PJContext *pj, double fade);


void PJContext_SetImageTexture( PJContext *p, int elemSize1, int elemSize, int cols, int rows, unsigned char *data );
void PJContext_SetImage2Texture( PJContext *p, int elemSize1, int elemSize, int cols, int rows, unsigned char *data );
void PJContext_SetLutTextureHi( PJContext *p, int elemSize1, int elemSize, int cols, int rows, unsigned char *data );
void PJContext_SetLutTextureLow( PJContext *p, int elemSize1, int elemSize, int cols, int rows, unsigned char *data );

int PJContext_UpdateMousePosition(PJContext *pj,int *mousex,int *mousey,int *w,int *h);
static int PJContext_HandleKeyboadEvent(PJContext *pj);


int PJContext_ChangeLayerShader(PJContext *pj,const char *fname,int layerNum);


void PJContext_GetSurfaceContextDisplay( PJContext *pj, void *pSurface, void *pContext, void *pDisplay );
void PJContext_GetVideoTextureNum( PJContext *pj, void *ptex );







#endif
