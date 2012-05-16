
/*

 SDL_rotozoom - rotozoomer

 LGPL (c) A. Schiffler

*/

#ifndef _SDL_rotozoom_h
#define _SDL_rotozoom_h

#include <math.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef M_PI
#define M_PI	3.141592654
#endif

#include "SDL.h"

/* ---- Defines */

#define SMOOTHING_OFF		0
#define SMOOTHING_ON		1

/* ---- Structures */

    typedef struct tColorRGBA {
	Uint8 r;
	Uint8 g;
	Uint8 b;
	Uint8 a;
    } tColorRGBA;

    typedef struct tColorY {
	Uint8 y;
    } tColorY;


/* ---- Prototypes */

#define DLLINTERFACE

/* 
 
 rotozoomSurface()

 Rotates and zoomes a 32bit or 8bit 'src' surface to newly created 'dst' surface.
 'angle' is the rotation in degrees. 'zoom' a scaling factor. If 'smooth' is 1
 then the destination 32bit surface is anti-aliased. If the surface is not 8bit
 or 32bit RGBA/ABGR it will be converted into a 32bit RGBA format on the fly.

*/

    DLLINTERFACE SDL_Surface *rotozoomSurface(SDL_Surface * src, double angle, double zoom, int smooth);

    DLLINTERFACE SDL_Surface *rotozoomSurfaceXY
    (SDL_Surface * src, double angle, double zoomx, double zoomy, int smooth);

/* Returns the size of the target surface for a rotozoomSurface() call */

    DLLINTERFACE void rotozoomSurfaceSize(int width, int height, double angle, double zoom, int *dstwidth,
					  int *dstheight);

    DLLINTERFACE void rotozoomSurfaceSizeXY
    (int width, int height, double angle, double zoomx, double zoomy, 
     int *dstwidth, int *dstheight);

/* 
 
 zoomSurface()

 Zoomes a 32bit or 8bit 'src' surface to newly created 'dst' surface.
 'zoomx' and 'zoomy' are scaling factors for width and height. If 'smooth' is 1
 then the destination 32bit surface is anti-aliased. If the surface is not 8bit
 or 32bit RGBA/ABGR it will be converted into a 32bit RGBA format on the fly.

*/

    DLLINTERFACE SDL_Surface *zoomSurface(SDL_Surface * src, double zoomx, double zoomy, int smooth);

/* Returns the size of the target surface for a zoomSurface() call */

    DLLINTERFACE void zoomSurfaceSize(int width, int height, double zoomx, double zoomy, int *dstwidth, int *dstheight);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif				/* _SDL_rotozoom_h */
