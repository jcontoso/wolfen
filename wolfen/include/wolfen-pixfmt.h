#include <stdint.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <babl/babl.h>
#include <pixman.h>
#include "wolfen-display.h"

#ifndef WOLFEN_PIX_FMT
#define WOLFEN_PIX_FMT

typedef struct {
	/* display */
	WolfenDisplay *display;
	
	/* xlib visual info */
	XVisualInfo xvi;
	long xvi_mask;
	 
	/* wl shm format */
	uint32_t wl_fmt;
	bool wl_fmt_has_transparency;
	
	/* pixman fmt */
	pixman_format_code_t pixman_fmt;
	
	/* IF THESE ARE NULL NO PIXEL CONVERSION NEEDS TO BE DONE */
	const Babl *xvi_babl;
	const Babl *wl_babl;
	const Babl *fish;
} WolfenPixelFmt;

WolfenPixelFmt *wolfen_fmt_from_wl_fmt(WolfenDisplay *display, int core_screen, uint32_t format);
WolfenPixelFmt *wolfen_fmt_from_xvi_for_wl_fmt(WolfenDisplay *display, XVisualInfo *xvi, long xvi_mask, uint32_t format);
void wolfen_fmt_free(WolfenPixelFmt *fmt);

bool wolfen_wl_fmt_has_transparency(uint32_t wl_fmt);

#endif
