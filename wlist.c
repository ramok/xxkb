#include <X11/Xlib.h>
#include "wlist.h"

extern Display *dpy;

WInfo* win_find(w)
	Window w;
{
	WInfo *pt = winlist;
	while (pt) {
		if (pt->win == w) break;
		pt = pt->next;
	}
	return pt;
}

WInfo* button_find(w)
	Window w;
{
	WInfo *pt = winlist;
	while (pt) {
		if (pt->button == w) break;
		pt = pt->next;
	}
	return pt;
}

WInfo* win_add(w, state)
	Window w; kbdState* state;
{
	WInfo *pt;
	pt = (WInfo*) malloc(sizeof(WInfo));
	if (pt) {
		pt->win = w;
		pt->button = 0;
		memcpy((void*) &pt->state, (void*) state, sizeof(kbdState));
		pt->next = 0;
		if (last) last->next = pt;
		if (!winlist) winlist = pt;
		last = pt;
	}
	return pt;
}

void   win_free(w)
	Window w;
{
	WInfo *pt = winlist, *prev = NULL;

	while (pt) {
		if (pt->win == w) break;
		prev = pt;
		pt = pt->next;
	}
	if (!pt) { printf("not in list!\n"); return; }
  
	if (!prev) winlist = pt->next;
	else prev->next = pt->next;

	if (last == pt) last = prev;

	free((void*) pt);
	return;
}

void   win_free_list()
{
	WInfo *pt = winlist, *tmp;

	while (pt) {
		tmp = pt->next;
		if (pt->button) XDestroyWindow(dpy, pt->button);
		free((void*) pt);
		pt = tmp;  
	}
}

