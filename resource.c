/* -*- tab-width: 4; -*- */

#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <X11/XKBlib.h>

#include "wlist.h"
#include "xxkb.h"

#ifdef XT_RESOURCE_SEARCH
#include <X11/IntrinsicP.h>
#endif

void err_malloc(void);
#define ERR_MALLOC	err_malloc()

/* Gravity names */
struct {
	char *name;
	int  len;
	int  res;
}GravityTab[] = {{"NorthEast", 9, NorthEastGravity},
		 {"NorthWest", 9, NorthWestGravity},
		 {"North",     5, NorthGravity},
		 {"SouthEast", 9, SouthEastGravity},
		 {"SouthWest", 9, SouthWestGravity},
		 {"South",     5, SouthGravity},
		 {"East",      4, EastGravity},
		 {"West",      4, WestGravity},
		 {"Center",    6, CenterGravity},		
		 {NULL, 0, 0}};

/* app_list attributes */
struct {
	char      *name;
	MatchType type;
} MatchLookup[] = {{"wm_class_class", WMClassClass},
                   {"wm_class_name",  WMClassName},
                   {"wm_name",        WMName},
                   {"property",       Prop}
};
#define match_num sizeof(MatchLookup)/sizeof(MatchLookup[0])

struct {
	char       *name;
	ListAction action;
	int        group;
} ActionLookup[] = {{"start_alt",  InitAltGrp, 0},
		    {"alt_group1", AltGrp,     0},
		    {"alt_group2", AltGrp,     1},
		    {"alt_group3", AltGrp,     2},
		    {"alt_group4", AltGrp,     3},
		    {"ignore",    Ignore,     0}
};
#define action_num sizeof(ActionLookup)/sizeof(ActionLookup[0])

/* keep temporary for compatibility */
char  *ignoreMatch[] = {"wm_class.class", "wm_class.name", "wm_name"};

/* Defaults */
Geometry def_but_geom = { XNegative, -75, 7, 15, 15,  NorthEastGravity},
	def_main_geom = {0, 0, 0, 48, 48, 0};

typedef struct {
	char  *name;
	char  *def;
	int   flag;
} CtrlRes;

CtrlRes ControlsTable [] = {
	{"add_when_start",		"yes", When_start},
	{"add_when_create",		"yes", When_create}, 
	{"add_when_change",		"no",  When_change},
	{"focusout",			"no",  Focus_out},
	{"two_state",			"yes", Two_state},
	{"button_delete",		"yes", Button_delete},
	{"button_delete_and_forget",	"yes", Forget_window},
	{"mainwindow_delete",		"yes", Main_delete}
};
#define FlagsNum sizeof(ControlsTable)/sizeof(ControlsTable[0])

char *MainXpmDflt[] = {"en48.xpm", "ru48.xpm", "", ""};
char *ButXpmDflt[]  = {"en15.xpm", "ru15.xpm", "", ""};

const char *AppName = "XXkb";
const char *Yes   = "yes";
const char *No    = "no";

int load_image(Display *dpy, char* name, Pixmap *map);

void ParseConfig(db, class, prefix, name, type, def_val, value)
	XrmDatabase db;
	char *class, *prefix, *name, *def_val;
	ResType type;
	void *value;
{
	XrmValue val; char *type_ret, *fullname, *s; Bool res = False;

	if (db) {
		fullname = malloc(strlen(prefix) + strlen(name) + 2);
		if (!fullname) ERR_MALLOC;
		sprintf(fullname, "%s.%s", prefix, name);
		res = XrmGetResource(db, fullname, class, &type_ret, &val);
		free(fullname);
	}
  
	if (!res) val.addr = def_val;
	switch (type) {
	case T_string:
		*((char **)value) = malloc(strlen(val.addr)+1);
		if (!*((char**)value)) ERR_MALLOC;
		strcpy(*((char**)value), val.addr);
		break;
	case T_bool:
		for (s = val.addr; *s; s++) if (isupper(*s)) *s = tolower(*s);
		*((Bool *)value) = (!strncmp(val.addr, "true", 4) ||
				    !strncmp(val.addr, "yes",  3) ||
				    !strncmp(val.addr, "on",   2))? True : False;
		break;
	case T_int:
		*((int *)value) = atoi(val.addr);
	}
}

static
SearchList* MakeSearchList(char *str)
{
	int len = strlen(str), count = 0;
	char *i, *j;
	SearchList *ret;

	ret = malloc(sizeof(SearchList));
	if (!ret) ERR_MALLOC;
	ret->num = 0;
	ret->idx = NULL;
	ret->list = NULL;
	ret->next = NULL;
	if (!len) return ret;

	ret->list = malloc(len+1);
	if (!ret->list) ERR_MALLOC;

	i = str; j = ret->list;
	while (len) {
		count++;
		while ((*i != ' ') && (*i != '\t')) {
			*j++ = *i++;
			if (!(--len)) {
				*j = '\0';
				break;
			}
		}
		*j++ = '\0';
		while ((*i == ' ') || (*i == '\t')) {
			i++;
			if (!(--len)) break;
		}
	}
	ret->num = count;

	ret->idx = malloc(count * sizeof(char*));
	if (!ret->idx) ERR_MALLOC;

	for (count = 0, i = ret->list; count < ret->num; count++) {
		ret->idx[count] = i;
		while (*i++);
	}
	return ret;
}

void GetRes(db, name1, name2, type, def_val, value)
	XrmDatabase db;
	char *name1, *name2, *def_val;
	ResType type;
	void *value;
{
	char *name = malloc(strlen(name1) + strlen(name2) + 2);
	if (!name) ERR_MALLOC;
	sprintf(name, "%s.%s", name1, name2);
	ParseConfig(db, "", AppName, name, type, def_val, value);
	free(name);
}

void GetRes3(db, name1, name2, name3, type, def_val, value)
	XrmDatabase db;
	char *name1, *name2, *name3, *def_val;
	ResType type;
	void *value;
{
	char *name = malloc(strlen(name1) + strlen(name2) + strlen(name3) + 3);
	if (!name) ERR_MALLOC;
	sprintf(name, "%s.%s.%s", name1, name2, name3);
	ParseConfig(db, "", AppName, name, type, def_val, value);
	free(name);
}

void GetControlRes(db, name1, name2, def_val, controls, flag)
	XrmDatabase db;
	char *name1, *name2, *def_val;
	int *controls, flag;
{
	Bool value;
	GetRes(db, name1, name2, T_bool, def_val, &value);
	if (value)
		*controls |= flag;
	else
		*controls &= ~flag;
}

void GetPixmapRes(dpy, path, db, resname, defaults, pixmap)
	Display     *dpy;
	XrmDatabase db;
	char        *path, *resname;
	char        **defaults;
	Pixmap      *pixmap;
{
	int i;
	char buf[6], *name, *fullname;

	for(i = 0; i < 4; i++) {
		sprintf(buf, "xpm.%d", i+1);
		GetRes(db, resname, buf, T_string, defaults[i], &name);
		if (*name) {
			if (*name == '/') {
				load_image(dpy, name, &pixmap[i]);
			} else {
				fullname = malloc(strlen(path) + strlen(name) + 2);
				if (!fullname) ERR_MALLOC;
				sprintf(fullname, "%s/%s", path, name);
				load_image(dpy, fullname, &pixmap[i]);
				free(fullname);
			}
		} else {
			pixmap[i] = (Pixmap) 0;
		}
	}
}

void GetGeometryRes(db, resname, def_geometry, geometry)
	XrmDatabase db;
	char *resname;
	Geometry def_geometry, *geometry;
{
	char *str;
	int i;
	Geometry geom = def_geometry;

	GetRes(db, resname, "geometry", T_string, "", &str);
	geom.mask = XParseGeometry(str, &geom.x, &geom.y,
				   &geom.width, &geom.height);
	GetRes(db, resname, "gravity",  T_string, "", &str);
	for(i = 0; GravityTab[i].name; i++) {
		if (!strncmp(str, GravityTab[i].name, GravityTab[i].len)) {
			geom.gravity = GravityTab[i].res;
			break;
		}
	}
	*geometry = geom;
}

void GetConfig(dpy, conf)
	Display *dpy;
	XXkbConfig *conf;
{
	XrmDatabase db;
	char *xpmpath, *path, *filename, *resname, *str;
	int i, j;
	SearchList *list;
  
	XrmInitialize();

#ifdef XT_RESOURCE_SEARCH
	filename = XtResolvePathname(dpy, "app-defaults", NULL, NULL, NULL, NULL, 0, NULL);
#else
	filename = malloc(strlen(APPDEFDIR) + strlen(APPDEFFILE) + 2);
	if (!filename) ERR_MALLOC;
	sprintf(filename,"%s/%s", APPDEFDIR, APPDEFFILE);
#endif
	db = XrmGetFileDatabase(filename);
	free(filename);

	path = getenv("HOME");
#ifdef XT_RESOURCE_SEARCH
	filename = XtResolvePathname(dpy, path, USERDEFFILE, NULL,
				     "%T/%L/%N%C:%T/%l/%N%C:%T/%N%C:%T/%L/%N:%T/%l/%N:%T/%N",
				     NULL, 0, NULL);
#else
	filename = malloc(strlen(path) + strlen(USERDEFFILE) + 2);
	if (!filename) ERR_MALLOC;
	sprintf(filename,"%s/%s", path, USERDEFFILE);
#endif
	XrmCombineFileDatabase(filename, &db, True);
	conf->user_config = filename;

	if (!db) printf("Can't open resource file. Try to use defaults.\n");

	GetRes(db, "xpm", "path", T_string, PIXMAPDIR, &xpmpath);

	for (i = 0; i < FlagsNum; i++) {
		GetControlRes(db, "controls",
			      ControlsTable[i].name, ControlsTable[i].def,
			      &conf->controls, ControlsTable[i].flag);
	}

	resname = "group";
	GetRes(db, resname, "base", T_int, "1", &conf->Base_group);
	GetRes(db, resname, "alt",  T_int, "2", &conf->Alt_group);
	conf->Base_group--; conf->Alt_group--;

	resname = "bell";
	GetControlRes(db, resname, "enable",  No, &conf->controls, Bell_enable);
	GetRes       (db, resname, "percent", T_int, "-50", &conf->Bell_percent);

	resname = "mainwindow";
	GetControlRes(db, resname, "enable",  Yes, &conf->controls, Main_enable);
	GetControlRes(db, resname, "appicon", No,  &conf->controls, WMaker);
	GetPixmapRes(dpy, xpmpath, db, resname, MainXpmDflt, &conf->pictures);
	GetGeometryRes(db, resname, def_main_geom, &conf->main_geom);
	GetRes(db, resname, "in_tray", T_string, "none", &conf->tray_type);

	resname = "button";
	GetControlRes(db, resname, "enable",  Yes, &conf->controls, Button_enable);
	if (conf->controls&Button_enable) {
		GetPixmapRes(dpy, xpmpath, db, resname, ButXpmDflt, &conf->pictures[4]);
		GetGeometryRes(db, resname, def_but_geom, &conf->but_geom);
	}

	resname = "app_list";
	for (i = 0; i < match_num; i++) {
		for (j = 0; j < action_num; j++) {
			GetRes3(db, resname, MatchLookup[i].name, ActionLookup[j].name,
				T_string, "", &str);
			if (*str) {
				list = MakeSearchList(str);
				list->action = ActionLookup[j].action;
				list->action |= ActionLookup[j].group & GrpMask;
				list->type = MatchLookup[i].type;
				list->next = conf->lists[i];
				conf->lists[i] = list;
			}
		}
	}

	resname = "ignore";
	GetControlRes(db, resname , "reverse", No, &conf->controls, Ignore_reverse);

	/* keep temporary for compatibility */
	for (i = 0; i < 3; i++) {
		GetRes(db, resname, ignoreMatch[i], T_string, "", &str);
		if (*str) {
			list = MakeSearchList(str);
			list->action = Ignore;
			list->next = conf->lists[i];
			conf->lists[i] = list;
		}
	}

	resname = "mousebutton";
	GetControlRes(db, resname , "1.reverse", No, &conf->controls, But1_reverse);
	GetControlRes(db, resname , "3.reverse", No, &conf->controls, But3_reverse);
}

void err_malloc()
{
	printf("xxkb: ParseConfig: Memory allocation error\n");
	exit(0);
}

#include <X11/xpm.h>

int load_image(dpy, name, pixmap)
	Display *dpy;
	char * name;
	Pixmap *pixmap;
{
	int res;
	GC  gc;
	unsigned long valuemask = 0; /* No data in "values" */
	XImage    *picture;
	Pixmap    pixId;
	XGCValues values;

	*pixmap = (Pixmap) 0;

	res = XpmReadFileToImage(dpy, name, &picture, NULL, NULL);

	switch (res) {
	case XpmOpenFailed:
		printf("Xpm file open failed: %s\n", name);
		break;
	case XpmFileInvalid:
		printf("Xpm file is invalid: %s\n", name);
		break;
	case XpmNoMemory:
		printf("No memory for open xpm file: %s\n", name);
		break;
	default:
		pixId = XCreatePixmap(dpy, RootWindow(dpy, DefaultScreen(dpy)),
				      picture->width, picture->height,
				      picture->depth);
		gc = XCreateGC(dpy, pixId, valuemask, &values);
		XPutImage(dpy, pixId, gc, picture, 0, 0, 0, 0,
			  picture->width, picture->height);
		XFreeGC(dpy, gc);
		*pixmap = pixId;
	}
}

static
void FreeSearchList(SearchList *list)
{
	if (list == NULL)
		return;
	if (list->list != NULL) {
		free(list->list);
	}
	if (list->idx != NULL) {
		free(list->idx);
	}
	free(list);
}

static
void RemakeSearchList(XXkbConfig *conf,
                      MatchType type, ListAction act, char* string)
{
	SearchList *cur, *prev, *newlist;
	int i;

	newlist = MakeSearchList(string);
	newlist->action = act;
	newlist->type = type;

	for (i = 0; i < 3; i++) {
		if (MatchLookup[i].type == type)
			break;
	}
	newlist->next = conf->lists[i];
	conf->lists[i] = newlist;

	for (prev = newlist, cur = newlist->next;
	     cur != NULL && cur->action != act;
	     prev = cur, cur = cur->next);

	if (cur != NULL) {
		prev->next = cur->next;
		FreeSearchList(cur);
	}
}

void SaveAppInConfig(XXkbConfig *conf, char* name, MatchType type)
{
	XrmDatabase db;
	XrmValue val;
	char *res_patt = "XXkb.app_list.%s.ignore";
	char *type_ret, *full_res, *newlist, *ptr;
	int len = 0, i;

	for (i = 0; i < 3; i++) {
		if (MatchLookup[i].type == type)
			break;
	}
	if ( i > 2) return;

	full_res = (char*) malloc (strlen(res_patt) + strlen(MatchLookup[i].name));
	if (full_res == NULL)
		return;
	sprintf(full_res, res_patt, MatchLookup[i].name);

	XrmInitialize();
	db = XrmGetFileDatabase(conf->user_config);

	XrmGetResource(db, full_res, "", &type_ret, &val);

	if (val.addr != NULL) {
		len = strlen(val.addr) + 1;
	}
	len += strlen(name) + 1;

	newlist = (char*) malloc(len);
	if (newlist == NULL) {
		XrmDestroyDatabase(db);
		return; 
	}

	ptr = newlist;
	if (val.addr) {
		strcpy(ptr, val.addr);
		ptr += strlen(val.addr);
		*ptr++ = ' ';
	}
	strcpy(ptr, name);

	RemakeSearchList(conf, type, Ignore, newlist);
	XrmPutStringResource(&db, full_res, newlist);
	free(newlist);

	XrmPutFileDatabase(db, conf->user_config);
	XrmDestroyDatabase(db);
}
