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

#include "xxkb.h"
#include "wlist.h"

static WInfo *winlist = NULL, *last = NULL;

WInfo*
win_find(Window w)
{
	WInfo *pt = winlist;
	while (pt != NULL) {
		if (pt->win == w) break;
		pt = pt->next;
	}
	return pt;
}

WInfo*
button_find(Window w)
{
	WInfo *pt = winlist;
	while (pt != NULL) {
		if (pt->button == w) break;
		pt = pt->next;
	}
	return pt;
}

void
win_update(Window win, XXkbConfig *conf, GC gc, int group, int win_x, int win_y)
{
	if (win && conf->mainwindow.pictures[group])
		XCopyArea(dpy, conf->mainwindow.pictures[group], win, gc,
				  0, 0,
				  conf->mainwindow.geometry.width,
				  conf->mainwindow.geometry.height,
				  win_x, win_y);
}

void
button_update(Window win, XXkbConfig *conf, GC gc, int group)
{
	if (win && conf->button.pictures[group])
		XCopyArea(dpy, conf->button.pictures[group], win, gc,
				  0, 0,
				  conf->button.geometry.width,
				  conf->button.geometry.height,
				  0, 0);
}

WInfo*
win_add(Window w, kbdState *state)
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

void
win_free(Window w)
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

void
win_free_list()
{
	WInfo *pt = winlist, *tmp;

	while (pt != NULL) {
		tmp = pt->next;
		if (pt->button) XDestroyWindow(dpy, pt->button);
		free(pt);
		pt = tmp;  
	}
}
