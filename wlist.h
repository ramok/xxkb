/* -*- tab-width: 4; c-basic-offset: 4; -*- */

#include <X11/Xlib.h>

typedef struct {
	int	group;
	int	alt;
} kbdState;

typedef struct _WInfo {
	struct _WInfo	*next;
	Window		win;
	Window		button;
	kbdState	state;
	int		ignore;
} WInfo;

static WInfo *winlist = NULL, *last = NULL;

WInfo* win_add(Window w, kbdState *state);

WInfo* win_find(Window win);
WInfo* button_find(Window button);

void   win_update(Window win, GC gc, int group);
void   button_update(Window win, GC gc, int group);

void   win_free(Window w);
void   win_free_list(void);
