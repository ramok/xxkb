/* -*- tab-width: 4; c-basic-offset: 4; -*- */
/*
 * xxkb.c
 *
 *     Main module of the xxkb program.
 *
 *     Copyright (c) 1999-2003, by Ivan Pascal <pascal@tsu.ru>
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/Xlibint.h>
#include <X11/Xatom.h>

#include "wlist.h"
#include "xxkb.h"

#define BASE(w)	(w & base_mask)

#ifdef XT_RESOURCE_SEARCH
#include <X11/IntrinsicP.h>
static XtAppContext app_cont;
#endif

XXkbConfig conf;

Display *dpy;
int scr;
GC gc;
XkbEvent ev;

Window RootWin, MainWin, icon, win, focused, base_mask;
int revert, grp;
Atom take_focus_atom, wm_del_win;

WInfo def_info, *info, *tmp_info;
kbdState def_state;
XFocusChangeEvent focused_event;
XErrorHandler DefErrHandler;

/* Forward declaration */
static ListAction GetWindowAction(Window w);
static char* GetWindowIdent(Window appwin, MatchType type);
static MatchType GetTypeFromState(unsigned int state);
static void IgnoreWindow(WInfo *info, MatchType type);

int
main(int argc, char ** argv)
{
	int  xkbEventType, xkbError, reason_rtrn, mjr, mnr;
	Bool fout_flag = False;
	Geometry geom;
	XWMHints	*wm_hints;
	XSizeHints	*size_hints;
	XClassHint	*class_hints;

	/* Lets begin */
	dpy = XkbOpenDisplay("", &xkbEventType, &xkbError, NULL, NULL, &reason_rtrn); 
	if (!dpy) {
		warnx("Can't connect to X-server: %s", getenv("DISPLAY"));
		switch (reason_rtrn) {
		case XkbOD_BadLibraryVersion :
		case XkbOD_BadServerVersion :
			warnx("xxkb was compiled with XKB version %d.%02d",
				  XkbMajorVersion,XkbMinorVersion);
			warnx("But %s uses incompatible version %d.%02d",
				  reason_rtrn == XkbOD_BadLibraryVersion ? "Xlib" : "Xserver",
				  mjr,mnr);
			break;

		case XkbOD_ConnectionRefused :
			warnx("Connection refused");
			break;

		case XkbOD_NonXkbServer:
			warnx("XKB extension not present");
			break;

		default:
			warnx("Unknown error %d from XkbOpenDisplay", reason_rtrn);
			break;
		}
		exit(1);
	}    

	scr = DefaultScreen(dpy);
	RootWin = RootWindow(dpy, scr);
	base_mask = ~(dpy->resource_mask);
	take_focus_atom = XInternAtom(dpy, "WM_TAKE_FOCUS", True);
	wm_del_win = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	DefErrHandler = XSetErrorHandler((XErrorHandler) ErrHandler);
 
	focused_event.type = FocusIn;
	focused_event.display = dpy;

	/* My configuration*/
	memset(&conf, 0, sizeof(conf));

#ifdef XT_RESOURCE_SEARCH
	app_cont = XtCreateApplicationContext();
	XtDisplayInitialize(app_cont, dpy, APPNAME, APPNAME, NULL, 0, &argc, argv);
#endif
	if (GetConfig(dpy, &conf) != 0) {
		warnx("Unable to initialize");
		return;
	}

	/* My MAIN window */
	geom = conf.mainwindow.geometry;
	if (geom.mask & (XNegative|YNegative)) {
		int x,y;
		unsigned int width, height, bord, dep;
		Window rwin;
		XGetGeometry(dpy, RootWin, &rwin, &x, &y, &width, &height, &bord, &dep);
		if (geom.mask & XNegative)
			geom.x = width + geom.x - geom.width;
		if (geom.mask & YNegative)
			geom.y = height + geom.y - geom.height;
	}

	MainWin = XCreateSimpleWindow(dpy, RootWin, geom.x, geom.y,
								  geom.width, geom.height, 0,
								  BlackPixel(dpy, scr), WhitePixel(dpy, scr));

	XStoreName(dpy, MainWin, APPNAME);
	XSetCommand(dpy, MainWin, argv, argc);

	/* WMHints */
	wm_hints = XAllocWMHints();
	if (wm_hints == NULL) errx(1, "Unable to allocate WM hints");
	wm_hints->window_group = MainWin;
	wm_hints->input = False;
	wm_hints->flags = InputHint | WindowGroupHint;
	XSetWMHints(dpy, MainWin, wm_hints);

	/* ClassHint */
	class_hints = XAllocClassHint();
	if (class_hints == NULL) errx(1, "Unable to allocate class hints");
	class_hints->res_name  = APPNAME;
	class_hints->res_class = APPNAME;
	XSetClassHint(dpy, MainWin, class_hints);
	XFree(class_hints);

	/* SizeHints */
	if (geom.mask & (XValue|YValue)) {
		size_hints = XAllocSizeHints();
		if (size_hints == NULL) errx(1, "Unable to allocate size hints");
		size_hints->x = geom.x;
		size_hints->y = geom.y;
		size_hints->flags = USPosition;
		XSetNormalHints(dpy, MainWin, size_hints);
		XFree(size_hints);
	}

	/* to fix: fails if mainwindow geometry was not read */
	XSetWMProtocols(dpy, MainWin, &wm_del_win, 1);

	/* Show window ? */
	if (conf.controls & WMaker) {
		icon = XCreateSimpleWindow(dpy, MainWin, geom.x, geom.y,
								   geom.width, geom.height, 0,
								   BlackPixel(dpy, scr), WhitePixel(dpy, scr));

		wm_hints->icon_window = icon;
		wm_hints->initial_state = WithdrawnState;
		wm_hints->flags |= StateHint | IconWindowHint;
		XSetWMHints(dpy, MainWin, wm_hints);
	}
	else
		icon = (Window) 0;

	XFree(wm_hints);

	if (conf.tray_type) {
		Atom r;
		int data = 1;
		if (! strcmp(conf.tray_type, "KDE") ||
		    ! strcmp(conf.tray_type, "GNOME") ) {
			r = XInternAtom(dpy, "KWM_DOCKWINDOW", False);
			XChangeProperty(dpy, MainWin, r, r, 32, 0,
							(unsigned char *)&data, 1);
		} else if (! strcmp(conf.tray_type, "KDE2")) {
			r = XInternAtom(dpy, "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", False);
			XChangeProperty(dpy, MainWin, r, XA_WINDOW, 32, 0,
							(unsigned char *)&data, 1);
		}
	}

	if (conf.controls & Main_enable)
		XMapWindow(dpy, MainWin);

	/* What events we want */
	XkbSelectEventDetails(dpy, XkbUseCoreKbd, XkbStateNotify,
						  XkbAllStateComponentsMask, XkbGroupStateMask);
	if (conf.controls & When_create)
		XSelectInput(dpy, RootWin, SubstructureNotifyMask);

	XSelectInput(dpy, MainWin, ExposureMask | ButtonPressMask);
	if (icon)
		XSelectInput(dpy, icon, ExposureMask | ButtonPressMask);

	getGC(MainWin, &gc);

	/* set current defaults */
	def_state.group = conf.Base_group;
	def_state.alt = conf.Alt_group;  

	def_info.win = icon ? icon : MainWin;
	def_info.button = 0;
	def_info.state = def_state;

	Reset();

	if (conf.controls & When_start) {
		int num; 
		Window rwin, parent, *children, *child, app;
		XQueryTree(dpy, RootWin, &rwin, &parent, &children, &num);
		child = children;
		while (num != NULL) {
			app = (Window) 0;
			GetAppWindow(*child, &app);
			if (app != NULL)
				AddWindow(app, *child);
			child++;
			num--;
		}

		XFree(children);
		XGetInputFocus(dpy, &focused, &revert);
		info = win_find(focused);
		if (info == NULL) info = &def_info;
	}

	/* Main Loop */
	while (1) {
		XNextEvent(dpy, &ev.core);

		if (ev.type == xkbEventType) {
			switch (ev.any.xkb_type) { 
			case XkbStateNotify :
				grp = ev.state.locked_group;

				if ((conf.controls & When_change) && !fout_flag) {
					XGetInputFocus(dpy, &focused, &revert);
					if ((focused == None) || (focused == PointerRoot))
						break;
					if (focused != info->win) {
						tmp_info = AddWindow(focused, focused);
						if (tmp_info != NULL) {
							info = tmp_info; info->state.group = grp;
						}
					}
				}
				fout_flag = False;
	  
				if ((conf.controls & Two_state) && ev.state.keycode) {
					int g_min, g_max;
					if (conf.Base_group < info->state.alt) {
						g_min = conf.Base_group; g_max = info->state.alt;
					}
					else { g_max = conf.Base_group; g_min = info->state.alt;}
					if ((grp > g_min) && (grp < g_max)) {
						XkbLockGroup(dpy, XkbUseCoreKbd, g_max);
						break;
					}
					if ((grp < g_min) || (grp > g_max)) {
						XkbLockGroup(dpy, XkbUseCoreKbd, g_min);
						break;
					}
				}
				info->state.group = grp;
				if ((conf.controls & Two_state) &&
				    (grp != conf.Base_group) && (grp != info->state.alt))
					info->state.alt = grp;

				if (info->button != NULL)
					button_update(info->button, gc, grp);
				win_update(MainWin, gc, grp);
				if (icon != NULL)
					win_update(icon, gc, grp);
				if (conf.controls & Bell_enable)
					XBell(dpy, conf.Bell_percent);
				break;

			default:
				break;
			}
		} /* xkb events */
		else
			switch (ev.type) {          /* core events */
			case Expose:	/* Update our window or button */
				if (ev.core.xexpose.count != 0) break;
				win = ev.core.xexpose.window;
				if ((win == MainWin) || (icon && (win == icon)))
					win_update(win, gc, info->state.group);
				else {
					tmp_info = button_find(win);
					if (tmp_info)
						button_update(win, gc, tmp_info->state.group);
				}
				break;

			case ButtonPress:
				win = ev.core.xbutton.window;
				switch (ev.core.xbutton.button) {
				case Button1:
					if ((win == info->button) || (win == MainWin) ||
					    (icon && (win == icon))) {
						if (conf.controls & Two_state) {
							if (info->state.group == conf.Base_group)
								XkbLockGroup(dpy, XkbUseCoreKbd, info->state.alt);
							else
								XkbLockGroup(dpy, XkbUseCoreKbd, conf.Base_group);
						}
						else {
							if (conf.controls & But1_reverse)
								XkbLockGroup(dpy, XkbUseCoreKbd, info->state.group - 1);
							else
								XkbLockGroup(dpy, XkbUseCoreKbd, info->state.group + 1);
						}
					}
					break;

				case Button3:
					if ((win == info->button) || (win == MainWin) ||
					    (icon && (win == icon))) {
						if (conf.controls & But3_reverse)
							XkbLockGroup(dpy, XkbUseCoreKbd, info->state.group - 1);
						else
							XkbLockGroup(dpy, XkbUseCoreKbd, info->state.group + 1);
					}
					break;

				case Button2:
					if ((win != MainWin) && (win != icon)) {
						if (conf.controls & Button_delete) {
							XDestroyWindow(dpy, win);
							if (conf.controls & Forget_window) {
								MatchType type;

								type = GetTypeFromState(ev.core.xbutton.state);
								if (type == -1)
									break;

								if (win == info->button) {
									IgnoreWindow(info, type);
									Reset();
								} else {
									tmp_info = button_find(win);
									if (tmp_info == NULL)
										break;

									IgnoreWindow(tmp_info, type);
								}
							}
						}
						break;
					}
					if (conf.controls & Main_delete)
						Terminate();
				}
				break;

			case FocusIn:
				info = win_find(ev.core.xfocus.window);
				if (info == NULL) {
					warnx("Oops. FocusEvent from unknown window\n");
					info = &def_info;
				}

				if (info->ignore == NULL)
					XkbLockGroup(dpy, XkbUseCoreKbd, info->state.group);
				else {
					def_info.state.group = info->state.group;
					info = &def_info;
				}
				break;

			case FocusOut:
				if (conf.controls & Focus_out) {
					tmp_info = info;
					info = &def_info;
					info->state.group = conf.Base_group; /*???*/
					if (tmp_info->state.group != conf.Base_group) {
						fout_flag = True;
						XkbLockGroup(dpy, XkbUseCoreKbd, info->state.group);
					}
				}
				break;

			case ReparentNotify:
				win = ev.core.xreparent.window;
				if (win == MainWin ||
				    ev.core.xreparent.parent == RootWin ||
				    BASE(ev.core.xreparent.parent) == BASE(win) ||
				    ev.core.xreparent.override_redirect == TRUE ) break;

				AddWindow(win, ev.core.xreparent.parent);
				break;

			case DestroyNotify:
				if (ev.core.xdestroywindow.event == RootWin)
					break;

				win = ev.core.xdestroywindow.window;
				tmp_info = win_find(win);
				if (tmp_info != NULL) {
					win_free(win);
					if (tmp_info == info) Reset();
					break;
				}

				tmp_info = button_find(win);
				if (tmp_info != NULL) tmp_info->button = 0;
				break;

			case ConfigureNotify:
				if (!ev.core.xconfigure.above
					|| !(conf.controls & Button_enable))
					break;
				tmp_info = button_find(ev.core.xconfigure.above);
				if (tmp_info != NULL)
					XRaiseWindow(dpy, ev.core.xconfigure.above);
				break;

			case PropertyNotify:
				win = ev.core.xproperty.window;
				tmp_info = win_find(win);
				if (tmp_info == NULL) {
					Window rwin, parent, *childrens, *child, app;
					int num;
					XQueryTree(dpy, win, &rwin, &parent, &childrens, &num);
					AddWindow(win, parent);
				}
				break;

			case ClientMessage:
				win = ev.core.xclient.window;
				if (((win == MainWin) || (win == icon))
				    && ev.core.xclient.data.l[0] == wm_del_win)
					Terminate();
				break;

			case CreateNotify:
			case NoExpose:
			case UnmapNotify:
			case MapNotify:
			case MappingNotify:
			case GravityNotify:
				/* Ignore these events */
				break;

			default:
				warnx("Unknown event %d", ev.type);
				break;
			}
	}

	return(0);
}

void
Reset()
{
	info = &def_info;
	info->state = def_state;
	XkbLockGroup(dpy, XkbUseCoreKbd, conf.Base_group);
}

void
Terminate()
{
	int i;
	win_free_list();
	XFreeGC(dpy,gc);

	for (i = 0; i < MAX_GROUP; i++) {
		if (conf.mainwindow.pictures[i] != NULL)
			XFreePixmap(dpy, conf.mainwindow.pictures[i]);
		if (conf.button.pictures[i] != NULL)
			XFreePixmap(dpy, conf.button.pictures[i]);
	}

	if (icon) XDestroyWindow(dpy, icon);
	XDestroyWindow(dpy, MainWin);
#ifdef XT_RESOURCE_SEARCH
	XtCloseDisplay(dpy);
#else
	XCloseDisplay(dpy);
#endif

	exit(0);
}

void
getGC(Window win, GC * gc)
{
	unsigned long valuemask = 0; /* No data in ``values'' */
	XGCValues values;
	*gc = XCreateGC(dpy, win, valuemask, &values);
/*	XSetForeground(dpy, *gc, BlackPixel(dpy, scr)); */
}

WInfo*
AddWindow(Window win, Window parent)
{
	int ignore = 0;
	WInfo *info;
	Status stat;
	ListAction action;
	XWindowAttributes attr;

	/* properties can be unappropriate at this moment */
	/* so we need posibility to redecide when them will be changed*/
	XSelectInput(dpy, win, PropertyChangeMask);

	/* don't deal with windows that never get a focus */
	if (!ExpectInput(win))
		return (WInfo*) 0;

	action = GetWindowAction(win);
	if ( ((action & Ignore) && !(conf.controls & Ignore_reverse)) ||
	     (!(action & Ignore) && (conf.controls & Ignore_reverse)))
		ignore = 1;

	info = win_find(win);
	if (info == NULL) {
		info = win_add(win, &def_state);
		if (info == NULL) return (WInfo*) 0;
		if (action & AltGrp) info->state.alt = action & GrpMask;
		if (action & InitAltGrp) info->state.group = info->state.alt;
	}

	XGetInputFocus(dpy, &focused, &revert);
	XSelectInput(dpy, win, FocusChangeMask | StructureNotifyMask | PropertyChangeMask);
	if (focused == win) {
		focused_event.window = win;
		XSendEvent(dpy, MainWin, 0, 0, (XEvent *) &focused_event);
	}

	info->ignore = ignore;
	if ((conf.controls & Button_enable) && (!info->button) && !ignore )
		info->button = MakeButton(parent);

/* to be sure that window still exists */
	stat = XGetWindowAttributes(dpy, win, &attr);
	if (stat == 0) {
		/* failed */
		win_free(win);
		return (WInfo*) 0;
	}

	return info;
}

Window
MakeButton(Window parent)
{
	Window button, rwin;
	int x, y;
	unsigned int width, height, bord, dep;
	XSetWindowAttributes attr;
	Geometry geom = conf.button.geometry;

	parent = GetGrandParent(parent);
	if (parent == (Window) 0)
		return (Window) 0;

	XGetGeometry(dpy, parent, &rwin, &x, &y, &width, &height, &bord, &dep);
	x = (geom.mask & XNegative)? width  + geom.x - geom.width  : geom.x;
	y = (geom.mask & YNegative)? height + geom.y - geom.height : geom.y;

	if ((geom.width > width) || (geom.height > height))
		return 0 ;

	button = XCreateSimpleWindow(dpy, parent,
								 x, y, geom.width, geom.height, 0,
								 BlackPixel(dpy, scr), WhitePixel(dpy, scr));

	attr.override_redirect = True;
	attr.win_gravity = geom.gravity;

	XChangeWindowAttributes(dpy, button, CWWinGravity|CWOverrideRedirect, &attr);
	XSelectInput(dpy, parent, SubstructureNotifyMask);
	XSelectInput(dpy, button, ExposureMask | ButtonPressMask);
	XMapRaised(dpy, button);

	return button;
}

Window
GetGrandParent(Window w)
{
	Window rwin, parent, *child;
	int num;

	while (1) {
		if (!XQueryTree(dpy, w, &rwin, &parent, &child, &num))
			return (Window) 0;
		XFree(child);
		if (parent == rwin) return w;
		w = parent;
	}
}

void
GetAppWindow(Window win, Window *core)
{
	Window rwin, parent, *children, *child;
	int n;

	if (!XQueryTree(dpy, win, &rwin, &parent, &children, &n))
		return;
	child = children;

	while (n != 0) {
		if (BASE(*child) != BASE(win)) {
			*core = *child;
			break;
		}
		GetAppWindow(*child, core);
		if (*core) break;
		child++;
		n--;
	}

	XFree(children);
}

static Bool
Compare(char *pattern, char *str)
{
	char *i = pattern, *j = str, *sub, *lpos;
	Bool aster = False; 

	do {
		if (*i == '*') { 
			i++;
			if (*i == '\0') return True;
			aster = True; sub = i; lpos = j;
			continue;
		}
		if (*i == *j) {
			i++; j++;
			continue;
		}
		if (*j == '\0')
			return False;

		if (aster) {
			j = ++lpos;
			i = sub;
		} else {
			return False;
		}
	} while (*j || *i);

	return ((*i == *j) ? True : False);
}

static ListAction
searchInList(SearchList *list, char *ident)
{
	int i;
	ListAction ret = 0;

	if (list == NULL)
		return 0;

	for (i = 0; i < list->num; i++) {
		if (Compare(list->idx[i], ident)) {
			ret |= list->action; /*???*/
			break;
		}
	}

	return ret;
}

static ListAction
searchInPropList(SearchList *list, Window win)
{
	int i, j;
	int prop_num;
	Atom *props, *atoms;
	ListAction ret = 0;

	if (list == NULL)
		return 0;

	atoms = (Atom *) calloc(list->num, sizeof(Atom));
	if (atoms == NULL)
		return 0;

	XInternAtoms(dpy, list->idx, list->num, False, atoms);
	for (i = 0, j = 0; i < list->num; i++) {
		if (atoms[i] != None) {
			j = 1;
			break;
		}
	}
	if (j != 0) {
		props = XListProperties(dpy, win, &prop_num);
		if (props != NULL) {
			for (i = 0; i < list->num; i++) {
				if (atoms[i] != None) {
					for (j = 0; j < prop_num; j++) {
						if (atoms[i] == props[j])
							ret |= list->action;
					}
				}
			}
			XFree(props);
		}
	}
	XFree(atoms);

	return ret;
}


/*
 * GetWindowAction
 * Returns
 *     an action associated with a window.
 */

static ListAction
GetWindowAction(Window w)
{
	ListAction ret = 0;
	XClassHint wm_class;
	char *name;

	if (XGetClassHint(dpy, w, &wm_class)) {
		ret = searchInList(conf.app_lists[0], wm_class.res_class);
		ret |= searchInList(conf.app_lists[1], wm_class.res_name);
		XFree(wm_class.res_name);
		XFree(wm_class.res_class);
	}

	if (XFetchName(dpy, w, &name)) {
		if (name != NULL) {
			ret |= searchInList(conf.app_lists[2], name);
			XFree(name);
		}
	}
	ret |= searchInPropList(conf.app_lists[3], w);

	return ret;
}

Bool
ExpectInput(Window w)
{
	Bool ok = False;
	XWMHints *hints = NULL;

	hints = XGetWMHints(dpy, w);
	if (hints != NULL) {
		if ((hints->flags & InputHint) && hints->input)
			ok = True;
		XFree(hints);
	}

	if (!ok) {
		Atom *protocols;
		Status stat;
		int n, i;

		stat = XGetWMProtocols(dpy, w, &protocols, &n);
		if (stat != 0) {
			/* success */
			for (i = 0; i < n; i++) {
				if (protocols[i] == take_focus_atom) {
					ok = True;
					break;
				}
			}
			XFree(protocols);
		}
	}

	return ok;
}


/*
 * GetWindowIdent
 * Returns
 *     the window identifier, which should be freed by the caller.
 */

static char*
GetWindowIdent(Window appwin, MatchType type)
{
	Status stat;
	XClassHint wm_class;
	char *ident = NULL;

	switch (type) {
	case WMName:
		XFetchName(dpy, appwin, &ident);
		break;

	default:
		stat = XGetClassHint(dpy, appwin, &wm_class);
		if (stat != 0) {
			/* success */
			if (type == WMClassClass) {
				XFree(wm_class.res_name);
				ident = wm_class.res_class;
			} else if (type == WMClassName) {
				XFree(wm_class.res_class);
				ident = wm_class.res_name;
			}
		}
		break;
	}

	return ident;
}


/*
 * IgnoreWindow
 *     Appends a window to the ignore list.
 */

static void
IgnoreWindow(WInfo *info, MatchType type)
{
	char *ident;

	ident = GetWindowIdent(info->win, type);
	if (ident == NULL) {
		XBell(dpy, conf.Bell_percent);
		return;
	}

	AddAppToIgnoreList(&conf, ident, type);
	info->ignore = 1;

	XFree(ident);
}

static MatchType
GetTypeFromState(unsigned int state)
{
	MatchType type;

	switch (state & (ControlMask | ShiftMask)) {
	case 0:
		type = -1;
		break;

	case ControlMask :
		type = WMClassClass;
		break;

	case ShiftMask   :
		type = WMName;
		break;

	case ControlMask | ShiftMask:
		type = WMClassName;
		break;
	} 

	return type;
}

void
ErrHandler(Display *dpy, XErrorEvent *err)
{
	switch (err->error_code) {
	case BadWindow:
	case BadDrawable:
		/* Ignore these errors */
		break;

	default:
		(*DefErrHandler)(dpy, err);
		break;
	}
}
