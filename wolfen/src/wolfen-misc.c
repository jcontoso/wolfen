#include <stdlib.h>
#include <math.h>
#include <X11/Xlib.h>
#include "wolfen-misc.h"

int wolfen_digit_count(int n) {
	if (n) {
		return floor(log10(abs(n)))+1;
	} else {
		return 1;		
	}
}

void wolfen_xfree(void *p) {
	if (p) {
		XFree(p);
	}
}
