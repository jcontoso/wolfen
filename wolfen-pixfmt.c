#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <wayland-server.h>
#include <babl/babl.h>
#include "wolfen-pixfmt.h"

typedef enum {
	WOLFEN_PIXEL_FMT_STRICT,
	WOLFEN_PIXEL_FMT_LOOSE,
	WOLFEN_PIXEL_FMT_VERY_LOOSE,
	WOLFEN_PIXEL_FMT_SUPER_LOOSE
} WolfenPixelFmtLooseness;

long wolfen_wlfmt2xvi_strict(uint32_t format, XVisualInfo *xvi, int core_screen) {
	long xvi_flags;
	
	xvi_flags = VisualScreenMask;
	xvi->screen = core_screen;
	switch (format) {
		case WL_SHM_FORMAT_ARGB8888:
			xvi_flags = VisualClassMask | VisualDepthMask | VisualRedMaskMask | VisualGreenMaskMask	| VisualBitsPerRGBMask | VisualBlueMaskMask | VisualScreenMask;
			xvi->class = TrueColor;
			xvi->depth = 32; 
			xvi->red_mask = 16711680;
			xvi->green_mask = 65280;
			xvi->blue_mask = 255;
			xvi->bits_per_rgb = 8;
			break;
		case WL_SHM_FORMAT_XRGB8888:
			/* how do 24bit visuals work? is it stored as a 32bit int with padding or as 3 8bit ints? */
			xvi_flags = VisualClassMask | VisualDepthMask | VisualRedMaskMask | VisualGreenMaskMask	| VisualBitsPerRGBMask | VisualBlueMaskMask | VisualScreenMask;
			xvi->class = TrueColor;
			xvi->depth = 24; 
			xvi->red_mask = 16711680;
			xvi->green_mask = 65280;
			xvi->blue_mask = 255;
			xvi->bits_per_rgb = 8;
			break;
		default:
			puts("unsupported format");
	}	

	return xvi_flags;
}

long wolfen_wlfmt2xvi_loose(uint32_t format, XVisualInfo *xvi, bool even_looser, int core_screen) {
	long xvi_flags;
	
	xvi_flags = VisualScreenMask;
	xvi->screen = core_screen;
	switch (format) {
		case WL_SHM_FORMAT_ARGB8888:
			xvi_flags = VisualClassMask | VisualDepthMask;
			xvi->screen = core_screen;
			xvi->class = TrueColor;
			if (!even_looser) {
				xvi->depth = 32; 
			} else {
				xvi->depth = 24; 
			}
			break;
		case WL_SHM_FORMAT_XRGB8888:
			xvi_flags = VisualClassMask | VisualDepthMask;
			xvi->class = TrueColor;
			xvi->depth = 24 /*24?*/; 
			break;
		default:
			puts("unsupported format");
	}	

	return xvi_flags;
}


long wolfen_get_best_xvi(Display *display, int core_screen, XVisualInfo *xvi) {
	/*actually do this properly */
	xvi->screen = core_screen;
	xvi->visualid = XVisualIDFromVisual(DefaultVisual(display, core_screen));
	xvi->depth = DefaultDepth(display, core_screen);
	return VisualScreenMask | VisualDepthMask | VisualIDMask;
}

long wolfen_xvi_template_from_wl_fmt(Display *display, int core_screen, uint32_t format, XVisualInfo *xvi_out, WolfenPixelFmtLooseness looseness) {
	switch (looseness) {
		case WOLFEN_PIXEL_FMT_STRICT:
			return wolfen_wlfmt2xvi_strict(format, xvi_out, core_screen);
			break;
		case WOLFEN_PIXEL_FMT_LOOSE:
			return wolfen_wlfmt2xvi_loose(format, xvi_out, false, core_screen);
			break;
		case WOLFEN_PIXEL_FMT_VERY_LOOSE:
			return wolfen_wlfmt2xvi_loose(format, xvi_out, true, core_screen);
			break;
		case WOLFEN_PIXEL_FMT_SUPER_LOOSE:
			return wolfen_get_best_xvi(display, core_screen, xvi_out);
			break;
		default:
			puts("What the heck?!");
	}	
}

const Babl *wolfen_xvi2babl(XVisualInfo *xvi) {
	switch (xvi->class) {
		case TrueColor:
			{
				char *fa;
				char *fb;
				char *fc;
				char *fd;

				if (xvi->red_mask == 4294967295) {
					fa = "R";
				} else if (xvi->green_mask == 4294967295) {
					fa = "G";
				} else if (xvi->blue_mask == 4294967295) {
					fa = "B";
				} else {
					fa = "A";
				}
				
				if (xvi->red_mask == 16711680) {
					fb = "R";
				} else if (xvi->green_mask == 16711680) {
					fb = "G";
				} else if (xvi->blue_mask == 16711680) {
					fb = "B";
				} else {
					fb = "A";
				}

				if (xvi->red_mask == 65280) {
					fc = "R";
				} else if (xvi->green_mask == 65280) {
					fc = "G";
				} else if (xvi->blue_mask == 65280) {
					fc = "B";
				} else {
					fc = "A";
				}
	
				if (xvi->red_mask == 255) {
					fd = "R";
				} else if (xvi->green_mask == 255) {
					fd = "G";
				} else if (xvi->blue_mask == 255) {
					fd = "B";
				} else {
					fd = "A";
				}															
				
				return babl_format_new(babl_model("RGBA"),babl_type("u8"),babl_component(fa), babl_component(fb), babl_component(fc),babl_component(fd), NULL);
			}
			break;
		default:
			puts("What the heck?!");
	}
	
	return NULL;
}

const Babl *wolfen_wl2babl(uint32_t wl_fmt) {
	switch (wl_fmt) {
		case WL_SHM_FORMAT_XRGB8888:
		case WL_SHM_FORMAT_ARGB8888:
				return babl_format_new(babl_model("RGBA"),babl_type("u8"),babl_component("A"), babl_component("R"), babl_component("G"),babl_component("B"), NULL);
			break;
		default:
			puts("What the heck?!");
	}
	return NULL;

}


WolfenPixelFmt *wolfen_fmt_from_wl_fmt(WolfenDisplay *display, int core_screen, uint32_t format) {
	WolfenPixelFmt *ret;
	XVisualInfo xvi_template;
	XVisualInfo *xvis;
	int xvis_count;
	WolfenPixelFmtLooseness looseness;
	
	ret = malloc(sizeof(WolfenPixelFmt));
	ret->display = display;
	ret->wl_fmt = format;
	ret->xvi_babl = ret->wl_babl = ret->fish = NULL;
	xvis = NULL;
	looseness = WOLFEN_PIXEL_FMT_VERY_LOOSE;
	
	while (!xvis) {
		WolfenPixelFmtLooseness tlooseness;
		
		tlooseness = looseness;
		if (looseness > WOLFEN_PIXEL_FMT_SUPER_LOOSE) {
			puts("What the heck?!");
			free(ret);
			return NULL;
		}
		
		ret->xvi_mask = wolfen_xvi_template_from_wl_fmt(display->x_display, core_screen, format, &xvi_template, looseness);
		xvis = XGetVisualInfo(display->x_display, ret->xvi_mask, &xvi_template, &xvis_count);
		if (xvis) {
			looseness = tlooseness;
		} else {
			looseness++;			
		}
	}

	ret->xvi = xvis[0];
	XFree(xvis);
	
	if (looseness != WOLFEN_PIXEL_FMT_STRICT) {
		ret->xvi_babl = wolfen_xvi2babl(&ret->xvi);
		ret->wl_babl = wolfen_wl2babl(format);
		ret->fish = babl_fish(ret->wl_babl, ret->xvi_babl);
	}
	
	return ret;
}

WolfenPixelFmt *wolfen_fmt_from_xvi_for_wl_fmt(WolfenDisplay *display, XVisualInfo *xvi, long xvi_mask, uint32_t format) {
	WolfenPixelFmt *ret;
	bool conv_needed;
	
	ret = malloc(sizeof(WolfenPixelFmt));
	ret->display = display;
	ret->wl_fmt = format;
	ret->xvi_babl = ret->wl_babl = ret->fish = NULL;
	ret->xvi = *xvi;
	ret->xvi_mask = xvi_mask;
	conv_needed = true;
	
	switch (ret->wl_fmt) {
		case WL_SHM_FORMAT_XRGB8888:
		case WL_SHM_FORMAT_ARGB8888:
			if ((ret->xvi_mask & VisualClassMask) && (ret->xvi_mask & VisualDepthMask) && (ret->xvi_mask & VisualRedMaskMask) && (ret->xvi_mask & VisualGreenMaskMask) && (ret->xvi_mask &  VisualBitsPerRGBMask) && (ret->xvi_mask & VisualBlueMaskMask) && (ret->xvi.class == TrueColor) && (ret->xvi.depth == 24 || ret->xvi.depth == 32) && (ret->xvi.red_mask == 16711680)	&& (ret->xvi.green_mask == 65280)	&& (ret->xvi.blue_mask == 255) && (ret->xvi.bits_per_rgb == 8)) {
				conv_needed = false;
			}
			break;
		default:
			puts("What the heck?!");
			free(ret);
			ret = NULL;
	}
	
	if (ret && conv_needed) {
		ret->xvi_babl = wolfen_xvi2babl(&ret->xvi);
		ret->wl_babl = wolfen_wl2babl(format);
		ret->fish = babl_fish(ret->wl_babl, ret->xvi_babl);		
	}

	return ret;
}

void wolfen_fmt_free(WolfenPixelFmt *fmt) {
	if (fmt) {
		free(fmt);
	
	}
}
