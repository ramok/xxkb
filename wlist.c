/* -*- tab-width: 4; c-basic-offset: 4; -*- */
/*
 * wlist.c
 *
 *     This module provides some helper functions to deal with
 *     windows.
 *
 *     Copyright (c) 1999-2003, by Ivan Pascal <pascal@tsu.ru>
 */

#include <X11/Xlib.h>

#include "wlist.h"
#include "xxkb.h"

extern Display *dpy;
extern XXkbConfig conf;

WInfo* win_find(w)
	Window w;
{
	WInfo *pt = winlist;
	while (pt != NULL) {
		if (pt->win == w) break;
		pt = pt->next;
	}
	return pt;
}

WInfo* button_find(w)
	Window w;
{
	WInfo *pt = winlist;
	while (pt != NULL) {
		if (pt->button == w) break;
		pt = pt->next;
	}
	return pt;
}

void
win_update(win, gc, group)
	Window	win;
	GC		gc;
	int		group;
{
	if (win && conf.pictures[group])
		XCopyArea(dpy, conf.pictures[group], win, gc,
				  0, 0,
				  conf.main_geom.width, conf.main_geom.height,
				  0, 0);
}

void
button_update(win, gc, group)
	Window	win;
	GC		gc;
	int		group;
{
	if (win && conf.pictures[group + MAX_GROUP])
		XCopyArea(dpy, conf.pictures[group + MAX_GROUP], win, gc,
				  0, 0,
				  conf.but_geom.width, conf.but_geom.height,
				  0, 0);
}

WInfo* win_add(w, state)
	Window w;
	kbdState* state;
{
	WInfo *pt;
	pt = (WInfo*) malloc(sizeof(WInfo));
	if (pt != NULL) {
		pt->win = w;
		pt->button = 0;
		memcpy((void*) &pt->state, (void*) state, sizeof(kbdState));
		pt->next = 0;
		if (last != NULL) last->next = pt;
		if (winlist == NULL) winlist = pt;
		last = pt;
	}
	return pt;
}

void   win_free(w)
	Window w;
{
	WInfo *pt = winlist, *prev = NULL;

	while (pt != NULL) {
		if (pt->win == w) break;
		prev = pt;
		pt = pt->next;
	}
	if (pt == NULL) {
		warnx("Window not in list");
		return;
	}
  
	if (prev == NULL) winlist = pt->next;
	else prev->next = pt->next;

	if (last == pt) last = prev;

	free(pt);
	return;
}

void   win_free_list()
{
	WInfo *pt = winlist, *tmp;

	while (pt != NULL) {
		tmp = pt->next;
		if (pt->button) XDestroyWindow(dpy, pt->button);
		free(pt);
		pt = tmp;  
	}
}
