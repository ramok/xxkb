/* -*- tab-width: 4; c-basic-offset: 4; -*- */

typedef struct {
	int	group;
	int	alt;
} kbdState;

typedef struct __WInfo {
	struct __WInfo	*next;
	Window		win;
	Window		button;
	kbdState	state;
	int		ignore;
} WInfo;

WInfo* win_add(Window w, kbdState *state);

WInfo* win_find(Window win);
WInfo* button_find(Window button);

void win_update(Window win, XXkbConfig *conf, GC gc, int group, int win_x, int win_y);
void button_update(Window win, XXkbConfig *conf, GC gc, int group);

void win_free(Window w);
void win_free_list(void);
