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

WInfo* win_find(Window w);
WInfo* button_find(Window button);
WInfo* win_add(Window w, kbdState *state);
void   win_free(Window w);
void   win_free_list(void);
