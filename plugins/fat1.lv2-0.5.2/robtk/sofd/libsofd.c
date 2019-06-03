/* libSOFD - Simple Open File Dialog [for X11 without toolkit]
 *
 * Copyright (C) 2014 Robin Gareus <robin@gareus.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* Test and example:
 *   gcc -Wall -D SOFD_TEST -g -o sofd libsofd.c -lX11
 *
 * public API documentation and example code at the bottom of this file
 *
 * This small lib may one day include openGL rendering and
 * wayland window support, but not today. Today we celebrate
 * 30 years of X11.
 */

#ifdef SOFD_TEST
#define HAVE_X11
#include "libsofd.h"
#endif

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>

// shared 'recently used' implementation
// sadly, xbel does not qualify as simple.
// hence we use a simple format alike the
// gtk-bookmark list (one file per line)

#define MAX_RECENT_ENTRIES 24
#define MAX_RECENT_AGE (15552000) // 180 days (in sec)

typedef struct {
	char path[1024];
	time_t atime;
} FibRecentFile;

static FibRecentFile *_recentlist = NULL;
static unsigned int   _recentcnt = 0;
static uint8_t        _recentlock = 0;

static int fib_isxdigit (const char x) {
	if (
			(x >= '0' && x <= '9')
			||
			(x >= 'a' && x <= 'f')
			||
			(x >= 'A' && x <= 'F')
		 ) return 1;
	return 0;
}

static void decode_3986 (char *str) {
	int len = strlen (str);
	int idx = 0;
	while (idx + 2 < len) {
		char *in = &str[idx];
		if (('%' == *in) && fib_isxdigit (in[1]) && fib_isxdigit (in[2])) {
			char hexstr[3];
			hexstr[0] = in[1];
			hexstr[1] = in[2];
			hexstr[2] = 0;
			long hex = strtol (hexstr, NULL, 16);
			*in = hex;
			memmove (&str[idx+1], &str[idx + 3], len - idx - 2);
			len -= 2;
		}
		++idx;
	}
}

static char *encode_3986 (const char *str) {
	size_t alloc, newlen;
	char *ns = NULL;
	unsigned char in;
	size_t i = 0;
	size_t length;

	if (!str) return strdup ("");

	alloc = strlen (str) + 1;
	newlen = alloc;

	ns = (char*) malloc (alloc);

	length = alloc;
	while (--length) {
		in = *str;

		switch (in) {
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
			case 'a': case 'b': case 'c': case 'd': case 'e':
			case 'f': case 'g': case 'h': case 'i': case 'j':
			case 'k': case 'l': case 'm': case 'n': case 'o':
			case 'p': case 'q': case 'r': case 's': case 't':
			case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
			case 'A': case 'B': case 'C': case 'D': case 'E':
			case 'F': case 'G': case 'H': case 'I': case 'J':
			case 'K': case 'L': case 'M': case 'N': case 'O':
			case 'P': case 'Q': case 'R': case 'S': case 'T':
			case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
			case '_': case '~': case '.': case '-':
			case '/': case ',': // XXX not in RFC3986
				ns[i++] = in;
				break;
			default:
				newlen += 2; /* this'll become a %XX */
				if (newlen > alloc) {
					alloc *= 2;
					ns = (char*) realloc (ns, alloc);
				}
				snprintf (&ns[i], 4, "%%%02X", in);
				i += 3;
				break;
		}
		++str;
	}
	ns[i] = 0;
	return ns;
}

void x_fib_free_recent () {
	free (_recentlist);
	_recentlist = NULL;
	_recentcnt = 0;
}

static int cmp_recent (const void *p1, const void *p2) {
	FibRecentFile *a = (FibRecentFile*) p1;
	FibRecentFile *b = (FibRecentFile*) p2;
	if (a->atime == b->atime) return 0;
	return a->atime < b->atime;
}

int x_fib_add_recent (const char *path, time_t atime) {
	unsigned int i;
	struct stat fs;
	if (_recentlock) { return -1; }
	if (access (path, R_OK)) {
		return -1;
	}
	if (stat (path, &fs)) {
		return -1;
	}
	if (!S_ISREG (fs.st_mode)) {
		return -1;
	}
	if (atime == 0) atime = time (NULL);
	if (MAX_RECENT_AGE > 0 && atime + MAX_RECENT_AGE < time (NULL)) {
		return -1;
	}

	for (i = 0; i < _recentcnt; ++i) {
		if (!strcmp (_recentlist[i].path, path)) {
			if (_recentlist[i].atime < atime) {
				_recentlist[i].atime = atime;
			}
			qsort (_recentlist, _recentcnt, sizeof(FibRecentFile), cmp_recent);
			return _recentcnt;
		}
	}
	_recentlist = (FibRecentFile*)realloc (_recentlist, (_recentcnt + 1) * sizeof(FibRecentFile));
	_recentlist[_recentcnt].atime = atime;
	strcpy (_recentlist[_recentcnt].path, path);
	qsort (_recentlist, _recentcnt + 1, sizeof(FibRecentFile), cmp_recent);

	if (_recentcnt >= MAX_RECENT_ENTRIES) {
		return (_recentcnt);
	}
	return (++_recentcnt);
}

#ifdef PATHSEP
#undef PATHSEP
#endif

#ifdef PLATFORM_WINDOWS
#define DIRSEP '\\'
#else
#define DIRSEP '/'
#endif

static void mkpath(const char *dir) {
	char tmp[1024];
	char *p;
	size_t len;

	snprintf (tmp, sizeof(tmp), "%s", dir);
	len = strlen(tmp);
	if (tmp[len - 1] == '/')
		tmp[len - 1] = 0;
	for (p = tmp + 1; *p; ++p)
		if(*p == DIRSEP) {
			*p = 0;
#ifdef PLATFORM_WINDOWS
			mkdir(tmp);
#else
			mkdir(tmp, 0755);
#endif
			*p = DIRSEP;
		}
#ifdef PLATFORM_WINDOWS
	mkdir(tmp);
#else
	mkdir(tmp, 0755);
#endif
}

int x_fib_save_recent (const char *fn) {
	if (_recentlock) { return -1; }
	if (!fn) { return -1; }
	if (_recentcnt < 1 || !_recentlist) { return -1; }
	unsigned int i;
	char *dn = strdup (fn);
	mkpath (dirname (dn));
	free (dn);

	FILE *rf = fopen (fn, "w");
	if (!rf) return -1;

	qsort (_recentlist, _recentcnt, sizeof(FibRecentFile), cmp_recent);
	for (i = 0; i < _recentcnt; ++i) {
		char *n = encode_3986 (_recentlist[i].path);
		fprintf (rf, "%s %lu\n", n, _recentlist[i].atime);
		free (n);
	}
	fclose (rf);
	return 0;
}

int x_fib_load_recent (const char *fn) {
	char tmp[1024];
	if (_recentlock) { return -1; }
	if (!fn) { return -1; }
	x_fib_free_recent ();
	if (access (fn, R_OK)) {
		return -1;
	}
	FILE *rf = fopen (fn, "r");
	if (!rf) return -1;
	while (fgets (tmp, sizeof(tmp), rf)
			&& strlen (tmp) > 1
			&& strlen (tmp) < sizeof(tmp))
	{
		char *s;
		tmp[strlen (tmp) - 1] = '\0'; // strip newline
		if (!(s = strchr (tmp, ' '))) { // find name <> atime sep
			continue;
		}
		*s = '\0';
		time_t t = atol (++s);
		decode_3986 (tmp);
		x_fib_add_recent (tmp, t);
	}
	fclose (rf);
	return 0;
}

unsigned int x_fib_recent_count () {
	return _recentcnt;
}

const char *x_fib_recent_at (unsigned int i) {
	if (i >= _recentcnt)
		return NULL;
	return _recentlist[i].path;
}

#ifdef PLATFORM_WINDOWS
#define PATHSEP "\\"
#else
#define PATHSEP "/"
#endif

const char *x_fib_recent_file(const char *appname) {
	static char recent_file[1024];
	assert(!strchr(appname, '/'));
	const char *xdg = getenv("XDG_DATA_HOME");
	if (xdg && (strlen(xdg) + strlen(appname) + 10) < sizeof(recent_file)) {
		sprintf(recent_file, "%s" PATHSEP "%s" PATHSEP "recent", xdg, appname);
		return recent_file;
	}
#ifdef PLATFORM_WINDOWS
	const char * homedrive = getenv("HOMEDRIVE");
	const char * homepath = getenv("HOMEPATH");
	if (homedrive && homepath && (strlen(homedrive) + strlen(homepath) + strlen(appname) + 29) < PATH_MAX) {
		sprintf(recent_file, "%s%s" PATHSEP "Application Data" PATHSEP "%s" PATHSEP "recent.txt", homedrive, homepath, appname);
		return recent_file;
	}
#elif defined PLATFORM_OSX
	const char *home = getenv("HOME");
	if (home && (strlen(home) + strlen(appname) + 29) < sizeof(recent_file)) {
		sprintf(recent_file, "%s/Library/Preferences/%s/recent", home, appname);
		return recent_file;
	}
#else
	const char *home = getenv("HOME");
	if (home && (strlen(home) + strlen(appname) + 22) < sizeof(recent_file)) {
		sprintf(recent_file, "%s/.local/share/%s/recent", home, appname);
		return recent_file;
	}
#endif
	return NULL;
}

#ifdef HAVE_X11
#include <mntent.h>
#include <dirent.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xos.h>

#ifndef MIN
#define MIN(A,B) ( (A) < (B) ? (A) : (B) )
#endif

#ifndef MAX
#define MAX(A,B) ( (A) < (B) ? (B) : (A) )
#endif

static Window   _fib_win = 0;
static GC       _fib_gc = 0;
static XColor   _c_gray0, _c_gray1, _c_gray2, _c_gray3, _c_gray4, _c_gray5, _c_gray6;
static Font     _fibfont = 0;
static Pixmap   _pixbuffer = None;

static int      _fib_width  = 100;
static int      _fib_height = 100;
static int      _btn_w = 0;
static int      _btn_span = 0;

static int      _fib_font_height = 0;
static int      _fib_dir_indent  = 0;
static int      _fib_spc_norm = 0;
static int      _fib_font_ascent = 0;
static int      _fib_font_vsep = 0;
static int      _fib_font_size_width = 0;
static int      _fib_font_time_width = 0;
static int      _fib_place_width  = 0;

static int      _scrl_f = 0;
static int      _scrl_y0 = -1;
static int      _scrl_y1 = -1;
static int      _scrl_my = -1;
static int      _scrl_mf = -1;
static int      _view_p = -1;

static int      _fsel = -1;
static int      _hov_b = -1;
static int      _hov_f = -1;
static int      _hov_p = -1;
static int      _hov_h = -1;
static int      _hov_l = -1;
static int      _hov_s = -1;
static int      _sort = 0;
static int      _columns = 0;
static int      _fib_filter_fn = 1;
static int      _fib_hidden_fn = 0;
static int      _fib_show_places = 0;

static uint8_t  _fib_mapped = 0;
static uint8_t  _fib_resized = 0;
static unsigned long _dblclk = 0;

static int      _status = -2;
static char     _rv_open[1024] = "";

static char     _fib_cfg_custom_places[1024] = "";
static char     _fib_cfg_custom_font[256] = "";
static char     _fib_cfg_title[128] = "xjadeo - Open Video File";

typedef struct {
	char name[256];
	int x0;
	int xw;
} FibPathButton;

typedef struct {
	char name[256];
	char strtime[32];
	char strsize[32];
	int ssizew;
	off_t size;
	time_t mtime;
	uint8_t flags; // 2: selected, 4: isdir 8: recent-entry
	FibRecentFile *rfp;
} FibFileEntry;

typedef struct {
	char text[24];
	uint8_t flags; // 2: selected, 4: toggle, 8 disable
	int x0;
	int tw;
	int xw;
	void (*callback)(Display*);
} FibButton;

typedef struct {
	char name[256];
	char path[1024];
	uint8_t flags; // 1: hover, 2: selected, 4:add sep
} FibPlace;

static char           _cur_path[1024] = "";
static FibFileEntry  *_dirlist = NULL;
static FibPathButton *_pathbtn = NULL;
static FibPlace      *_placelist = NULL;
static int            _dircount = 0;
static int            _pathparts = 0;
static int            _placecnt = 0;

static FibButton     _btn_ok;
static FibButton     _btn_cancel;
static FibButton     _btn_filter;
static FibButton     _btn_places;
static FibButton     _btn_hidden;
static FibButton    *_btns[] = {&_btn_places, &_btn_filter, &_btn_hidden, &_btn_cancel, &_btn_ok};

static int (*_fib_filter_function)(const char *filename);

/* hardcoded layout */
#define DSEP 6 // px; horiz space beween elements, also l+r margin for file-list
#define PSEP 4 // px; horiz space beween paths
#define FILECOLUMN (17 * _fib_dir_indent) //px;  min width of file-column
#define LISTTOP 2.7 //em;  top of the file-browser list
#define LISTBOT 4.75 //em;  bottom of the file-browers list
#define BTNBTMMARGIN 0.75 //em;  height/margin of the button row
#define BTNPADDING 2 // px - only used for open/cancel buttons
#define SCROLLBARW (3 + (_fib_spc_norm&~1)) //px; - should be SCROLLBARW = (N * 2 + 3)
#define SCROLLBOXH 10 //px; arrow box top+bottom
#define PLACESW _fib_place_width //px;
#define PLACESWMAX (15 *_fib_spc_norm) //px;
#define PATHBTNTOP _fib_font_vsep //px; offset by (_fib_font_ascent);
#define FAREAMRGB 3 //px; base L+R margin
#define FAREAMRGR (FAREAMRGB + 1) //px; right margin of file-area + 1 (line width)
#define FAREAMRGL (_fib_show_places ? PLACESW + FAREAMRGB : FAREAMRGB) //px; left margin of file-area
#define TEXTSEP 4 //px;
#define FAREATEXTL (FAREAMRGL + TEXTSEP) //px; filename text-left FAREAMRGL + TEXTSEP
#define SORTBTNOFF -10 //px;

#ifndef DBLCLKTME
#define DBLCLKTME 200 //msec; double click time
#endif

#define DRAW_OUTLINE
#define DOUBLE_BUFFER

static int query_font_geometry (Display *dpy, GC gc, const char *txt, int *w, int *h, int *a, int *d) {
	XCharStruct text_structure;
	int font_direction, font_ascent, font_descent;
	XFontStruct *fontinfo = XQueryFont (dpy, XGContextFromGC (gc));

	if (!fontinfo) { return -1; }
	XTextExtents (fontinfo, txt, strlen (txt), &font_direction, &font_ascent, &font_descent, &text_structure);
	if (w) *w = XTextWidth (fontinfo, txt, strlen (txt));
	if (h) *h = text_structure.ascent + text_structure.descent;
	if (a) *a = text_structure.ascent;
	if (d) *d = text_structure.descent;
	XFreeFontInfo (NULL, fontinfo, 1);
	return 0;
}

static void VDrawRectangle (Display *dpy, Drawable d, GC gc, int x, int y, unsigned int w, unsigned int h) {
	const unsigned long blackColor = BlackPixel (dpy, DefaultScreen (dpy));
#ifdef DRAW_OUTLINE
	XSetForeground (dpy, gc, _c_gray5.pixel);
	XDrawLine (dpy, d, gc, x + 1, y + h, x + w, y + h);
	XDrawLine (dpy, d, gc, x + w, y + 1, x + w, y + h);

	XSetForeground (dpy, gc, blackColor);
	XDrawLine (dpy, d, gc, x + 1, y, x + w, y);
	XDrawLine (dpy, d, gc, x, y + 1, x, y + h);
#else
	XSetForeground (dpy, _fib_gc, blackColor);
	XDrawRectangle (dpy, d, gc, x, y, w, h);
#endif
}

static void fib_expose (Display *dpy, Window realwin) {
	int i;
	XID win;
	const unsigned long whiteColor = WhitePixel (dpy, DefaultScreen (dpy));
	const unsigned long blackColor = BlackPixel (dpy, DefaultScreen (dpy));
	if (!_fib_mapped) return;

	if (_fib_resized
#ifdef DOUBLE_BUFFER
			|| !_pixbuffer
#endif
			)
	{
#ifdef DOUBLE_BUFFER
		unsigned int w = 0, h = 0;
		if (_pixbuffer != None) {
			Window ignored_w;
			int ignored_i;
			unsigned int ignored_u;
			XGetGeometry(dpy, _pixbuffer, &ignored_w, &ignored_i, &ignored_i, &w, &h, &ignored_u, &ignored_u);
			if (_fib_width != (int)w || _fib_height != (int)h) {
				XFreePixmap (dpy, _pixbuffer);
				_pixbuffer = None;
			}
		}
		if (_pixbuffer == None) {
			XWindowAttributes wa;
			XGetWindowAttributes (dpy, realwin, &wa);
			_pixbuffer = XCreatePixmap (dpy, realwin, _fib_width, _fib_height, wa.depth);
		}
#endif
		if (_pixbuffer != None) {
			XSetForeground (dpy, _fib_gc, _c_gray1.pixel);
			XFillRectangle (dpy, _pixbuffer, _fib_gc, 0, 0, _fib_width, _fib_height);
		} else {
			XSetForeground (dpy, _fib_gc, _c_gray1.pixel);
			XFillRectangle (dpy, realwin, _fib_gc, 0, 0, _fib_width, _fib_height);
		}
		_fib_resized = 0;
	}

	if (_pixbuffer == None) {
		win = realwin;
	} else {
		win = _pixbuffer;
	}

	// Top Row: dirs and up navigation

	int ppw = 0;
	int ppx = FAREAMRGB;

	for (i = _pathparts - 1; i >= 0; --i) {
		ppw += _pathbtn[i].xw + PSEP;
		if (ppw >= _fib_width - PSEP - _pathbtn[0].xw - FAREAMRGB) break; // XXX, first change is from "/" to "<", NOOP
	}
	++i;
	// border-less "<" parent/up, IFF space is limited
	if (i > 0) {
		if (0 == _hov_p || (_hov_p > 0 && _hov_p < _pathparts - 1)) {
			XSetForeground (dpy, _fib_gc, _c_gray4.pixel);
		} else {
			XSetForeground (dpy, _fib_gc, blackColor);
		}
		XDrawString (dpy, win, _fib_gc, ppx, PATHBTNTOP, "<", 1);
		ppx += _pathbtn[0].xw + PSEP;
		if (i == _pathparts) --i;
	}

	_view_p = i;

	while (i < _pathparts) {
		if (i == _hov_p) {
			XSetForeground (dpy, _fib_gc, _c_gray0.pixel);
		} else {
			XSetForeground (dpy, _fib_gc, _c_gray2.pixel);
		}
		XFillRectangle (dpy, win, _fib_gc,
				ppx + 1, PATHBTNTOP - _fib_font_ascent,
				_pathbtn[i].xw - 1, _fib_font_height);
		VDrawRectangle (dpy, win, _fib_gc,
				ppx, PATHBTNTOP - _fib_font_ascent,
				_pathbtn[i].xw, _fib_font_height);
		XDrawString (dpy, win, _fib_gc, ppx + 1 + BTNPADDING, PATHBTNTOP,
				_pathbtn[i].name, strlen (_pathbtn[i].name));
		_pathbtn[i].x0 = ppx; // current position
		ppx += _pathbtn[i].xw + PSEP;
		++i;
	}

	// middle, scroll list of file names
	const int ltop = LISTTOP * _fib_font_vsep;
	const int llen = (_fib_height - LISTBOT * _fib_font_vsep) / _fib_font_vsep;
	const int fsel_height = 4 + llen * _fib_font_vsep;
	const int fsel_width = _fib_width - FAREAMRGL - FAREAMRGR - (llen < _dircount ? SCROLLBARW : 0);
	const int t_x = FAREATEXTL;
	int t_s = FAREATEXTL + fsel_width;
	int t_t = FAREATEXTL + fsel_width;

	// check which colums can be visible
	// depending on available width of window.
	_columns = 0;
	if (fsel_width > FILECOLUMN + _fib_font_size_width + _fib_font_time_width) {
		_columns |= 2;
		t_s = FAREAMRGL + fsel_width - _fib_font_time_width - TEXTSEP;
	}
	if (fsel_width > FILECOLUMN + _fib_font_size_width) {
		_columns |= 1;
		t_t = t_s - _fib_font_size_width - TEXTSEP;
	}

	int fstop = _scrl_f; // first entry in scroll position
	const int ttop = ltop - _fib_font_height + _fib_font_ascent;

	if (fstop > 0 && fstop + llen > _dircount) {
		fstop = MAX (0, _dircount - llen);
		_scrl_f = fstop;
	}

	// list header
	XSetForeground (dpy, _fib_gc, _c_gray3.pixel);
	XFillRectangle (dpy, win, _fib_gc, FAREAMRGL, ltop - _fib_font_vsep, fsel_width, _fib_font_vsep);

	// draw background of file list
	XSetForeground (dpy, _fib_gc, _c_gray2.pixel);
	XFillRectangle (dpy, win, _fib_gc, FAREAMRGL, ltop, fsel_width, fsel_height);

#ifdef DRAW_OUTLINE
	VDrawRectangle (dpy, win, _fib_gc, FAREAMRGL, ltop - _fib_font_vsep -1, _fib_width - FAREAMRGL - FAREAMRGR, fsel_height + _fib_font_vsep + 1);
#endif

	switch (_hov_h) {
		case 1:
			XSetForeground (dpy, _fib_gc, _c_gray0.pixel);
			XFillRectangle (dpy, win, _fib_gc, t_x + _fib_dir_indent - TEXTSEP + 1, ltop - _fib_font_vsep, t_t - t_x - _fib_dir_indent - 1, _fib_font_vsep);
			break;
		case 2:
			XSetForeground (dpy, _fib_gc, _c_gray0.pixel);
			XFillRectangle (dpy, win, _fib_gc, t_t - TEXTSEP + 1, ltop - _fib_font_vsep, _fib_font_size_width + TEXTSEP - 1, _fib_font_vsep);
			break;
		case 3:
			XSetForeground (dpy, _fib_gc, _c_gray0.pixel);
			XFillRectangle (dpy, win, _fib_gc, t_s - TEXTSEP + 1, ltop - _fib_font_vsep, TEXTSEP + TEXTSEP + _fib_font_time_width - 1, _fib_font_vsep);
			break;
		default:
			break;
	}

	// column headings and sort order
	int arp = MAX (2, _fib_font_height / 5); // arrow scale
	const int trioff = _fib_font_height - _fib_font_ascent - arp + 1;
	XPoint ptri[4] = { {0, ttop - trioff }, {arp, -arp - arp - 1}, {-arp - arp, 0}, {arp, arp + arp + 1}};
	if (_sort & 1) {
		ptri[0].y = ttop -arp - arp - 1;
		ptri[1].y *= -1;
		ptri[3].y *= -1;
	}
	switch (_sort) {
		case 0:
		case 1:
			ptri[0].x = t_t + SORTBTNOFF + 2 - arp;
			XSetForeground (dpy, _fib_gc, _c_gray6.pixel);
			XFillPolygon (dpy, win, _fib_gc, ptri, 3, Convex, CoordModePrevious);
			XDrawLines (dpy, win, _fib_gc, ptri, 4, CoordModePrevious);
			break;
		case 2:
		case 3:
			if (_columns & 1) {
				ptri[0].x = t_s + SORTBTNOFF + 2 - arp;
				XSetForeground (dpy, _fib_gc, _c_gray6.pixel);
				XFillPolygon (dpy, win, _fib_gc, ptri, 3, Convex, CoordModePrevious);
				XDrawLines (dpy, win, _fib_gc, ptri, 4, CoordModePrevious);
			}
			break;
		case 4:
		case 5:
			if (_columns & 2) {
				ptri[0].x = FAREATEXTL + fsel_width + SORTBTNOFF + 2 - arp;
				XSetForeground (dpy, _fib_gc, _c_gray6.pixel);
				XFillPolygon (dpy, win, _fib_gc, ptri, 3, Convex, CoordModePrevious);
				XDrawLines (dpy, win, _fib_gc, ptri, 4, CoordModePrevious);
			}
			break;
	}

#if 0 // bottom header bottom border
	XSetForeground (dpy, _fib_gc, _c_gray5.pixel);
	XSetLineAttributes (dpy, _fib_gc, 1, LineOnOffDash, CapButt, JoinMiter);
	XDrawLine (dpy, win, _fib_gc,
			FAREAMRGL + 1, ltop,
			FAREAMRGL + fsel_width, ltop);
	XSetLineAttributes (dpy, _fib_gc, 1, LineSolid, CapButt, JoinMiter);
#endif

	XSetForeground (dpy, _fib_gc, _c_gray4.pixel);
	XDrawLine (dpy, win, _fib_gc,
			t_x + _fib_dir_indent - TEXTSEP, ltop - _fib_font_vsep + 3,
			t_x + _fib_dir_indent - TEXTSEP, ltop - 3);

	XSetForeground (dpy, _fib_gc, blackColor);
	XDrawString (dpy, win, _fib_gc, t_x + _fib_dir_indent, ttop, "Name", 4);

	if (_columns & 1) {
		XSetForeground (dpy, _fib_gc, _c_gray4.pixel);
		XDrawLine (dpy, win, _fib_gc,
				t_t - TEXTSEP, ltop - _fib_font_vsep + 3,
				t_t - TEXTSEP, ltop - 3);
		XSetForeground (dpy, _fib_gc, blackColor);
		XDrawString (dpy, win, _fib_gc, t_t, ttop, "Size", 4);
	}

	if (_columns & 2) {
		XSetForeground (dpy, _fib_gc, _c_gray4.pixel);
		XDrawLine (dpy, win, _fib_gc,
				t_s - TEXTSEP, ltop - _fib_font_vsep + 3,
				t_s - TEXTSEP, ltop - 3);
		XSetForeground (dpy, _fib_gc, blackColor);
		if (_pathparts > 0)
			XDrawString (dpy, win, _fib_gc, t_s, ttop, "Last Modified", 13);
		else
			XDrawString (dpy, win, _fib_gc, t_s, ttop, "Last Used", 9);
	}

	// scrollbar sep
	if (llen < _dircount) {
		const int sx0 = _fib_width - SCROLLBARW - FAREAMRGR;
		XSetForeground (dpy, _fib_gc, _c_gray4.pixel);
		XDrawLine (dpy, win, _fib_gc,
				sx0 - 1, ltop - _fib_font_vsep,
#ifdef DRAW_OUTLINE
				sx0 - 1, ltop + fsel_height
#else
				sx0 - 1, ltop - 1
#endif
				);
	}

	// clip area for file-name
	XRectangle clp = {FAREAMRGL + 1, ltop, t_t - FAREAMRGL - TEXTSEP - TEXTSEP - 1, fsel_height};

	// list files in view
	for (i = 0; i < llen; ++i) {
		const int j = i + fstop;
		if (j >= _dircount) break;

		const int t_y = ltop + (i+1) * _fib_font_vsep - 4;

		XSetForeground (dpy, _fib_gc, blackColor);
		if (_dirlist[j].flags & 2) {
			XSetForeground (dpy, _fib_gc, blackColor);
			XFillRectangle (dpy, win, _fib_gc,
					FAREAMRGL, t_y - _fib_font_ascent, fsel_width, _fib_font_height);
			XSetForeground (dpy, _fib_gc, whiteColor);
		}
		if (_hov_f == j && !(_dirlist[j].flags & 2)) {
			XSetForeground (dpy, _fib_gc, _c_gray4.pixel);
		}
		if (_dirlist[j].flags & 4) {
			XDrawString (dpy, win, _fib_gc, t_x, t_y, "D", 1);
		}
		XSetClipRectangles (dpy, _fib_gc, 0, 0, &clp, 1, Unsorted);
		XDrawString (dpy, win, _fib_gc,
				t_x + _fib_dir_indent, t_y,
				_dirlist[j].name, strlen (_dirlist[j].name));
		XSetClipMask (dpy, _fib_gc, None);

		if (_columns & 1) // right-aligned 'size'
			XDrawString (dpy, win, _fib_gc,
					t_s - TEXTSEP - 2 - _dirlist[j].ssizew, t_y,
					_dirlist[j].strsize, strlen (_dirlist[j].strsize));
		if (_columns & 2)
			XDrawString (dpy, win, _fib_gc,
					t_s, t_y,
					_dirlist[j].strtime, strlen (_dirlist[j].strtime));
	}

	// scrollbar
	if (llen < _dircount) {
		float sl = (fsel_height + _fib_font_vsep - (SCROLLBOXH + SCROLLBOXH)) / (float) _dircount;
		sl = MAX ((8. / llen), sl); // 8px min height of scroller
		const int sy1 = llen * sl;
		const float mx = (fsel_height + _fib_font_vsep - (SCROLLBOXH + SCROLLBOXH) - sy1) / (float)(_dircount - llen);
		const int sy0 = fstop * mx;
		const int sx0 = _fib_width - SCROLLBARW - FAREAMRGR;
		const int stop = ltop - _fib_font_vsep;

		_scrl_y0 = stop + SCROLLBOXH + sy0;
		_scrl_y1 = _scrl_y0 + sy1;

		assert (fstop + llen <= _dircount);
		// scroll-bar background
		XSetForeground (dpy, _fib_gc, _c_gray3.pixel);
		XFillRectangle (dpy, win, _fib_gc, sx0, stop, SCROLLBARW, fsel_height + _fib_font_vsep);

		// scroller
		if (_hov_s == 0) {
			XSetForeground (dpy, _fib_gc, _c_gray0.pixel);
		} else {
			XSetForeground (dpy, _fib_gc, _c_gray1.pixel);
		}
		XFillRectangle (dpy, win, _fib_gc, sx0 + 1, stop + SCROLLBOXH + sy0, SCROLLBARW - 2, sy1);

		int scrw = (SCROLLBARW -3) / 2;
		// arrows top and bottom
		if (_hov_s == 1) {
			XSetForeground (dpy, _fib_gc, _c_gray0.pixel);
		} else {
			XSetForeground (dpy, _fib_gc, _c_gray1.pixel);
		}
		XPoint ptst[4] = { {sx0 + 1, stop + 8}, {scrw, -7}, {scrw, 7}, {-2 * scrw, 0}};
		XFillPolygon (dpy, win, _fib_gc, ptst, 3, Convex, CoordModePrevious);
		XDrawLines (dpy, win, _fib_gc, ptst, 4, CoordModePrevious);

		if (_hov_s == 2) {
			XSetForeground (dpy, _fib_gc, _c_gray0.pixel);
		} else {
			XSetForeground (dpy, _fib_gc, _c_gray1.pixel);
		}
		XPoint ptsb[4] = { {sx0 + 1, ltop + fsel_height - 9}, {2*scrw, 0}, {-scrw, 7}, {-scrw, -7}};
		XFillPolygon (dpy, win, _fib_gc, ptsb, 3, Convex, CoordModePrevious);
		XDrawLines (dpy, win, _fib_gc, ptsb, 4, CoordModePrevious);
	} else {
		_scrl_y0 = _scrl_y1 = -1;
	}

	if (_fib_show_places) {
		assert (_placecnt > 0);

		// heading
		XSetForeground (dpy, _fib_gc, _c_gray3.pixel);
		XFillRectangle (dpy, win, _fib_gc, FAREAMRGB, ltop - _fib_font_vsep, PLACESW - TEXTSEP, _fib_font_vsep);

		// body
		XSetForeground (dpy, _fib_gc, _c_gray2.pixel);
		XFillRectangle (dpy, win, _fib_gc, FAREAMRGB, ltop, PLACESW - TEXTSEP, fsel_height);

#ifdef DRAW_OUTLINE
	VDrawRectangle (dpy, win, _fib_gc, FAREAMRGB, ltop - _fib_font_vsep -1, PLACESW - TEXTSEP, fsel_height + _fib_font_vsep + 1);
#endif

		XSetForeground (dpy, _fib_gc, blackColor);
		XDrawString (dpy, win, _fib_gc, FAREAMRGB + TEXTSEP, ttop, "Places", 6);

		XRectangle pclip = {FAREAMRGB + 1, ltop, PLACESW - TEXTSEP -1, fsel_height};
		XSetClipRectangles (dpy, _fib_gc, 0, 0, &pclip, 1, Unsorted);
		const int plx = FAREAMRGB + TEXTSEP;
		for (i = 0; i < llen && i < _placecnt; ++i) {
			const int ply = ltop + (i+1) * _fib_font_vsep - 4;
			if (i == _hov_l) {
				XSetForeground (dpy, _fib_gc, _c_gray4.pixel);
			} else {
				XSetForeground (dpy, _fib_gc, blackColor);
			}
			XDrawString (dpy, win, _fib_gc,
					plx, ply,
					_placelist[i].name, strlen (_placelist[i].name));
			if (_placelist[i].flags & 4) {
				XSetForeground (dpy, _fib_gc, _c_gray3.pixel);
				const int plly = ply - _fib_font_ascent + _fib_font_height;
				const int pllx0 = FAREAMRGB;
				const int pllx1 = FAREAMRGB + (PLACESW - TEXTSEP);
				XDrawLine (dpy, win, _fib_gc, pllx0, plly, pllx1, plly);
			}
		}
		XSetClipMask (dpy, _fib_gc, None);

		if (_placecnt > llen) {
			const int plly =  ltop + fsel_height - _fib_font_height + _fib_font_ascent;
			const int pllx0 = FAREAMRGB + (PLACESW - TEXTSEP) * .75;
			const int pllx1 = FAREAMRGB + (PLACESW - TEXTSEP - TEXTSEP);

			XSetForeground (dpy, _fib_gc, blackColor);
			XSetLineAttributes (dpy, _fib_gc, 1, LineOnOffDash, CapButt, JoinMiter);
			XDrawLine (dpy, win, _fib_gc, pllx0, plly, pllx1, plly);
			XSetLineAttributes (dpy, _fib_gc, 1, LineSolid, CapButt, JoinMiter);
		}
	}

	// Bottom Buttons
	const int numb = sizeof(_btns) / sizeof(FibButton*);
	int xtra = _fib_width - _btn_span;
	const int cbox = _fib_font_ascent - 2;
	const int bbase = _fib_height - BTNBTMMARGIN * _fib_font_vsep - BTNPADDING;
	const int cblw = cbox > 20 ? 5 : ( cbox > 9 ? 3 : 1);

	int bx = FAREAMRGB;
	for (i = 0; i < numb; ++i) {
		if (_btns[i]->flags & 8) { continue; }
		if (_btns[i]->flags & 4) {
			// checkbutton
			const int cby0 = bbase - cbox + 1 + BTNPADDING;
			if (i == _hov_b) {
				XSetForeground (dpy, _fib_gc, _c_gray4.pixel);
			} else {
				XSetForeground (dpy, _fib_gc, blackColor);
			}
			XDrawRectangle (dpy, win, _fib_gc,
					bx, cby0 - 1, cbox + 1, cbox + 1);

			if (i == _hov_b) {
				XSetForeground (dpy, _fib_gc, _c_gray5.pixel);
			} else {
				XSetForeground (dpy, _fib_gc, blackColor);
			}
			XDrawString (dpy, win, _fib_gc, BTNPADDING + bx + _fib_font_ascent, 1 + bbase + BTNPADDING,
					_btns[i]->text, strlen (_btns[i]->text));

			if (i == _hov_b) {
				XSetForeground (dpy, _fib_gc, _c_gray0.pixel);
			} else {
				if (_btns[i]->flags & 2) {
					XSetForeground (dpy, _fib_gc, _c_gray1.pixel);
				} else {
					XSetForeground (dpy, _fib_gc, _c_gray2.pixel);
				}
			}
			XFillRectangle (dpy, win, _fib_gc,
					bx+1, cby0, cbox, cbox);

			if (_btns[i]->flags & 2) {
				XSetLineAttributes (dpy, _fib_gc, cblw, LineSolid, CapRound, JoinMiter);
				XSetForeground (dpy, _fib_gc, _c_gray6.pixel);
				XDrawLine (dpy, win, _fib_gc,
						bx + 2, cby0 + 1,
						bx + cbox - 1, cby0 + cbox - 2);
				XDrawLine (dpy, win, _fib_gc,
						bx + cbox - 1, cby0 + 1,
						bx + 2, cby0 + cbox - 2);
				XSetLineAttributes (dpy, _fib_gc, 1, LineSolid, CapButt, JoinMiter);
			}
		} else {
			if (xtra > 0) {
				bx += xtra;
				xtra = 0;
			}
			// pushbutton

			uint8_t can_hover = 1; // special case
			if (_btns[i] == &_btn_ok) {
				if (_fsel < 0 || _fsel >= _dircount) {
					can_hover = 0;
				}
			}

			if (can_hover && i == _hov_b) {
				XSetForeground (dpy, _fib_gc, _c_gray0.pixel);
			} else {
				XSetForeground (dpy, _fib_gc, _c_gray2.pixel);
			}
			XFillRectangle (dpy, win, _fib_gc,
					bx + 1, bbase - _fib_font_ascent,
					_btn_w - 1, _fib_font_height + BTNPADDING + BTNPADDING);
			VDrawRectangle (dpy, win, _fib_gc,
					bx, bbase - _fib_font_ascent,
					_btn_w, _fib_font_height + BTNPADDING + BTNPADDING);
			XDrawString (dpy, win, _fib_gc, bx + (_btn_w - _btns[i]->tw) * .5, 1 + bbase + BTNPADDING,
					_btns[i]->text, strlen (_btns[i]->text));
		}
		_btns[i]->x0 = bx;
		bx += _btns[i]->xw + DSEP;
	}

	if (_pixbuffer != None) {
		XCopyArea(dpy, _pixbuffer, realwin, _fib_gc, 0, 0, _fib_width, _fib_height, 0, 0);
	}
	XFlush (dpy);
}

static void fib_reset () {
	_hov_p = _hov_f = _hov_h = _hov_l = -1;
	_scrl_f = 0;
	_fib_resized = 1;
}

static int cmp_n_up (const void *p1, const void *p2) {
	FibFileEntry *a = (FibFileEntry*) p1;
	FibFileEntry *b = (FibFileEntry*) p2;
	if ((a->flags & 4) && !(b->flags & 4)) return -1;
	if (!(a->flags & 4) && (b->flags & 4)) return 1;
	return strcmp (a->name, b->name);
}

static int cmp_n_down (const void *p1, const void *p2) {
	FibFileEntry *a = (FibFileEntry*) p1;
	FibFileEntry *b = (FibFileEntry*) p2;
	if ((a->flags & 4) && !(b->flags & 4)) return -1;
	if (!(a->flags & 4) && (b->flags & 4)) return 1;
	return strcmp (b->name, a->name);
}

static int cmp_t_up (const void *p1, const void *p2) {
	FibFileEntry *a = (FibFileEntry*) p1;
	FibFileEntry *b = (FibFileEntry*) p2;
	if ((a->flags & 4) && !(b->flags & 4)) return -1;
	if (!(a->flags & 4) && (b->flags & 4)) return 1;
	if (a->mtime == b->mtime) return 0;
	return a->mtime > b->mtime ? -1 : 1;
}

static int cmp_t_down (const void *p1, const void *p2) {
	FibFileEntry *a = (FibFileEntry*) p1;
	FibFileEntry *b = (FibFileEntry*) p2;
	if ((a->flags & 4) && !(b->flags & 4)) return -1;
	if (!(a->flags & 4) && (b->flags & 4)) return 1;
	if (a->mtime == b->mtime) return 0;
	return a->mtime > b->mtime ? 1 : -1;
}

static int cmp_s_up (const void *p1, const void *p2) {
	FibFileEntry *a = (FibFileEntry*) p1;
	FibFileEntry *b = (FibFileEntry*) p2;
	if ((a->flags & 4) && (b->flags & 4)) return 0; // dir, no size, retain order
	if ((a->flags & 4) && !(b->flags & 4)) return -1;
	if (!(a->flags & 4) && (b->flags & 4)) return 1;
	if (a->size == b->size) return 0;
	return a->size > b->size ? -1 : 1;
}

static int cmp_s_down (const void *p1, const void *p2) {
	FibFileEntry *a = (FibFileEntry*) p1;
	FibFileEntry *b = (FibFileEntry*) p2;
	if ((a->flags & 4) && (b->flags & 4)) return 0; // dir, no size, retain order
	if ((a->flags & 4) && !(b->flags & 4)) return -1;
	if (!(a->flags & 4) && (b->flags & 4)) return 1;
	if (a->size == b->size) return 0;
	return a->size > b->size ? 1 : -1;
}

static void fmt_size (Display *dpy, FibFileEntry *f) {
	if (f->size > 10995116277760) {
		sprintf (f->strsize, "%.0f TB", f->size / 1099511627776.f);
	}
	if (f->size > 1099511627776) {
		sprintf (f->strsize, "%.1f TB", f->size / 1099511627776.f);
	}
	else if (f->size > 10737418240) {
		sprintf (f->strsize, "%.0f GB", f->size / 1073741824.f);
	}
	else if (f->size > 1073741824) {
		sprintf (f->strsize, "%.1f GB", f->size / 1073741824.f);
	}
	else if (f->size > 10485760) {
		sprintf (f->strsize, "%.0f MB", f->size / 1048576.f);
	}
	else if (f->size > 1048576) {
		sprintf (f->strsize, "%.1f MB", f->size / 1048576.f);
	}
	else if (f->size > 10240) {
		sprintf (f->strsize, "%.0f KB", f->size / 1024.f);
	}
	else if (f->size >= 1000) {
		sprintf (f->strsize, "%.1f KB", f->size / 1024.f);
	}
	else {
		sprintf (f->strsize, "%.0f  B", f->size / 1.f);
	}
	int sw = 0;
	query_font_geometry (dpy, _fib_gc, f->strsize, &sw, NULL, NULL, NULL);
	if (sw > _fib_font_size_width) {
		_fib_font_size_width = sw;
	}
	f->ssizew = sw;
}

static void fmt_time (Display *dpy, FibFileEntry *f) {
	struct tm *tmp;
	tmp = localtime (&f->mtime);
	if (!tmp) {
		return;
	}
	strftime (f->strtime, sizeof(f->strtime), "%F %H:%M", tmp);

	int tw = 0;
	query_font_geometry (dpy, _fib_gc, f->strtime, &tw, NULL, NULL, NULL);
	if (tw > _fib_font_time_width) {
		_fib_font_time_width = tw;
	}
}

static void fib_resort (const char * sel) {
	if (_dircount < 1) { return; }
	int (*sortfn)(const void *p1, const void *p2);
	switch (_sort) {
		case 1: sortfn = &cmp_n_down; break;
		case 2: sortfn = &cmp_s_down; break;
		case 3: sortfn = &cmp_s_up; break;
		case 4: sortfn = &cmp_t_down; break;
		case 5: sortfn = &cmp_t_up; break;
		default:
						sortfn = &cmp_n_up;
						break;
	}
	qsort (_dirlist, _dircount, sizeof(_dirlist[0]), sortfn);
	int i;
	for (i = 0; i < _dircount && sel; ++i) {
		if (!strcmp (_dirlist[i].name, sel)) {
			_fsel = i;
			break;
		}
	}
}

static void fib_select (Display *dpy, int item) {
	if (_fsel >= 0) {
		_dirlist[_fsel].flags &= ~2;
	}
	_fsel = item;
	if (_fsel >= 0 && _fsel < _dircount) {
		_dirlist[_fsel].flags |= 2;
		const int llen = (_fib_height - LISTBOT * _fib_font_vsep) / _fib_font_vsep;
		if (_fsel < _scrl_f) {
			_scrl_f = _fsel;
		}
		else if (_fsel >= _scrl_f + llen) {
			_scrl_f = 1 + _fsel - llen;
		}
	} else {
		_fsel = -1;
	}

	fib_expose (dpy, _fib_win);
}

static inline int fib_filter (const char *name) {
	if (_fib_filter_function) {
		return _fib_filter_function (name);
	} else {
		return 1;
	}
}

static void fib_pre_opendir (Display *dpy) {
	if (_dirlist) free (_dirlist);
	if (_pathbtn) free (_pathbtn);
	_dirlist = NULL;
	_pathbtn = NULL;
	_dircount = 0;
	_pathparts = 0;
	query_font_geometry (dpy, _fib_gc, "Size  ", &_fib_font_size_width, NULL, NULL, NULL);
	fib_reset ();
	_fsel = -1;
}

static void fib_post_opendir (Display *dpy, const char *sel) {
	if (_dircount > 0)
		_fsel = 0; // select first
	else
		_fsel = -1;
	fib_resort (sel);

	if (_dircount > 0 && _fsel >= 0) {
		fib_select (dpy, _fsel);
	} else {
		fib_expose (dpy, _fib_win);
	}
}

static int fib_dirlistadd (Display *dpy, const int i, const char* path, const char *name, time_t mtime) {
	char tp[1024];
	struct stat fs;
	if (!_fib_hidden_fn && name[0] == '.') return -1;
	if (!strcmp (name, ".")) return -1;
	if (!strcmp (name, "..")) return -1;
	strcpy (tp, path);
	strcat (tp, name);
	if (access (tp, R_OK)) {
		return -1;
	}
	if (stat (tp, &fs)) {
		return -1;
	}
	assert (i < _dircount); // could happen if dir changes while we're reading.
	if (i >= _dircount) return -1;
	if (S_ISDIR (fs.st_mode)) {
		_dirlist[i].flags |= 4;
	}
	else if (S_ISREG (fs.st_mode)) {
		if (!fib_filter (name)) return -1;
	}
#if 0 // only needed with lstat()
	else if (S_ISLNK (fs.st_mode)) {
		if (!fib_filter (name)) return -1;
	}
#endif
	else {
		return -1;
	}
	strcpy (_dirlist[i].name, name);
	_dirlist[i].mtime = mtime > 0 ? mtime : fs.st_mtime;
	_dirlist[i].size = fs.st_size;
	if (!(_dirlist[i].flags & 4))
		fmt_size (dpy, &_dirlist[i]);
	fmt_time (dpy, &_dirlist[i]);
	return 0;
}

static int fib_openrecent (Display *dpy, const char *sel) {
	int i;
	unsigned int j;
	assert (_recentcnt > 0);
	fib_pre_opendir (dpy);
	query_font_geometry (dpy, _fib_gc, "Last Used", &_fib_font_time_width, NULL, NULL, NULL);
	_dirlist = (FibFileEntry*) calloc (_recentcnt, sizeof(FibFileEntry));
	_dircount = _recentcnt;
	for (j = 0, i = 0; j < _recentcnt; ++j) {
		char base[1024];
		char *s = strrchr (_recentlist[j].path, '/');
		if (!s || !*++s) continue;
		size_t len = (s - _recentlist[j].path);
		strncpy (base, _recentlist[j].path, len);
		base[len] = '\0';
		if (!fib_dirlistadd (dpy, i, base, s, _recentlist[j].atime)) {
			_dirlist[i].rfp = &_recentlist[j];
			_dirlist[i].flags |= 8;
			++i;
		}
	}
	_dircount = i;
	fib_post_opendir (dpy, sel);
	return _dircount;
}

static int fib_opendir (Display *dpy, const char* path, const char *sel) {
	char *t0, *t1;
	int i;

	assert (path);

	if (strlen (path) == 0 && _recentcnt > 0) { // XXX we should use a better indication for this
		strcpy (_cur_path, "");
		return fib_openrecent (dpy, sel);
	}

	assert (strlen (path) < sizeof(_cur_path) -1);
	assert (strlen (path) > 0);
	assert (strstr (path, "//") == NULL);
	assert (path[0] == '/');

	fib_pre_opendir (dpy);

	query_font_geometry (dpy, _fib_gc, "Last Modified", &_fib_font_time_width, NULL, NULL, NULL);
	DIR *dir = opendir (path);
	if (!dir) {
		strcpy (_cur_path, "/");
	} else {
		int i;
		struct dirent *de;
		strcpy (_cur_path, path);

		if (_cur_path[strlen (_cur_path) -1] != '/')
			strcat (_cur_path, "/");

		while ((de = readdir (dir))) {
			if (!_fib_hidden_fn && de->d_name[0] == '.') continue;
			++_dircount;
		}

		if (_dircount > 0)
			_dirlist = (FibFileEntry*) calloc (_dircount, sizeof(FibFileEntry));

		rewinddir (dir);

		i = 0;
		while ((de = readdir (dir))) {
			if (!fib_dirlistadd (dpy, i, _cur_path, de->d_name, 0))
				++i;
		}
		_dircount = i;
		closedir (dir);
	}

	t0 = _cur_path;
	while (*t0 && (t0 = strchr (t0, '/'))) {
		++_pathparts;
		++t0;
	}
	assert (_pathparts > 0);
	_pathbtn = (FibPathButton*) calloc (_pathparts + 1, sizeof(FibPathButton));

	t1 = _cur_path;
	i = 0;
	while (*t1 && (t0 = strchr (t1, '/'))) {
		if (i == 0) {
			strcpy (_pathbtn[i].name, "/");
		} else {
			*t0 = 0;
			strcpy (_pathbtn[i].name, t1);
		}
		query_font_geometry (dpy, _fib_gc, _pathbtn[i].name, &_pathbtn[i].xw, NULL, NULL, NULL);
		_pathbtn[i].xw += BTNPADDING + BTNPADDING;
		*t0 = '/';
		t1 = t0 + 1;
		++i;
	}
	fib_post_opendir (dpy, sel);
	return _dircount;
}

static int fib_open (Display *dpy, int item) {
	char tp[1024];
	if (_dirlist[item].flags & 8) {
		assert (_dirlist[item].rfp);
		strcpy (_rv_open, _dirlist[item].rfp->path);
		_status = 1;
		return 0;
	}
	strcpy (tp, _cur_path);
	strcat (tp, _dirlist[item].name);
	if (_dirlist[item].flags & 4) {
		fib_opendir (dpy, tp, NULL);
		return 0;
	} else {
		_status = 1;
		strcpy (_rv_open, tp);
	}
	return 0;
}

static void cb_cancel (Display *dpy) {
	_status = -1;
}

static void cb_open (Display *dpy) {
	if (_fsel >= 0 && _fsel < _dircount) {
		fib_open (dpy, _fsel);
	}
}

static void sync_button_states () {
	if (_fib_show_places)
		_btn_places.flags |= 2;
	else
		_btn_places.flags &= ~2;
	if (_fib_filter_fn) // inverse -> show all
		_btn_filter.flags &= ~2;
	else
		_btn_filter.flags |= 2;
	if (_fib_hidden_fn)
		_btn_hidden.flags |= 2;
	else
		_btn_hidden.flags &= ~2;
}

static void cb_places (Display *dpy) {
	_fib_show_places = ! _fib_show_places;
	if (_placecnt < 1)
		_fib_show_places = 0;
	sync_button_states ();
	_fib_resized = 1;
	fib_expose (dpy, _fib_win);
}

static void cb_filter (Display *dpy) {
	_fib_filter_fn = ! _fib_filter_fn;
	sync_button_states ();
	char *sel = _fsel >= 0 ? strdup (_dirlist[_fsel].name) : NULL;
	fib_opendir (dpy, _cur_path, sel);
	free (sel);
}

static void cb_hidden (Display *dpy) {
	_fib_hidden_fn = ! _fib_hidden_fn;
	sync_button_states ();
	char *sel = _fsel >= 0 ? strdup (_dirlist[_fsel].name) : NULL;
	fib_opendir (dpy, _cur_path, sel);
	free (sel);
}

static int fib_widget_at_pos (Display *dpy, int x, int y, int *it) {
	const int btop = _fib_height - BTNBTMMARGIN * _fib_font_vsep - _fib_font_ascent - BTNPADDING;
	const int bbot = btop + _fib_font_height + BTNPADDING + BTNPADDING;
	const int llen = (_fib_height - LISTBOT * _fib_font_vsep) / _fib_font_vsep;
	const int ltop = LISTTOP * _fib_font_vsep;
	const int fbot = ltop + 4 + llen * _fib_font_vsep;
	const int ptop = PATHBTNTOP - _fib_font_ascent;
	assert (it);

	// paths at top
	if (y > ptop && y < ptop + _fib_font_height && _view_p >= 0 && _pathparts > 0) {
		int i = _view_p;
		*it = -1;
		if (i > 0) { // special case '<'
			if (x > FAREAMRGB && x <= FAREAMRGB + _pathbtn[0].xw) {
				*it = _view_p - 1;
				i = _pathparts;
			}
		}
		while (i < _pathparts) {
			if (x >= _pathbtn[i].x0 && x <= _pathbtn[i].x0 + _pathbtn[i].xw) {
				*it = i;
				break;
			}
			++i;
		}
		assert (*it < _pathparts);
		if (*it >= 0) return 1;
		else return 0;
	}

	// buttons at bottom
	if (y > btop && y < bbot) {
		size_t i;
		*it = -1;
		for (i = 0; i < sizeof(_btns) / sizeof(FibButton*); ++i) {
			const int bx = _btns[i]->x0;
			if (_btns[i]->flags & 8) { continue; }
			if (x > bx && x < bx + _btns[i]->xw) {
				*it = i;
			}
		}
		if (*it >= 0) return 3;
		else return 0;
	}

	// main file area
	if (y >= ltop - _fib_font_vsep && y < fbot && x > FAREAMRGL && x < _fib_width - FAREAMRGR) {
		// scrollbar
		if (_scrl_y0 > 0 && x >= _fib_width - (FAREAMRGR + SCROLLBARW) && x <= _fib_width - FAREAMRGR) {
			if (y >= _scrl_y0 && y < _scrl_y1) {
				*it = 0;
			} else if (y >= _scrl_y1) {
				*it = 2;
			} else {
				*it = 1;
			}
			return 4;
		}
		// file-list
		else if (y >= ltop) {
			const int item = (y - ltop) / _fib_font_vsep + _scrl_f;
			*it = -1;
			if (item >= 0 && item < _dircount) {
				*it = item;
			}
			if (*it >= 0) return 2;
			else return 0;
		}
		else {
			*it = -1;
			const int fsel_width = _fib_width - FAREAMRGL - FAREAMRGR - (llen < _dircount ? SCROLLBARW : 0);
			const int t_s = FAREAMRGL + fsel_width - _fib_font_time_width - TEXTSEP - TEXTSEP;
			const int t_t = FAREAMRGL + fsel_width - TEXTSEP - _fib_font_size_width - ((_columns & 2) ? ( _fib_font_time_width + TEXTSEP + TEXTSEP) : 0);
			if (x >= fsel_width + FAREAMRGL) ;
			else if ((_columns & 2) && x >= t_s) *it = 3;
			else if ((_columns & 1) && x >= t_t) *it = 2;
			else if (x >= FAREATEXTL + _fib_dir_indent - TEXTSEP) *it = 1;
			if (*it >= 0) return 5;
			else return 0;
		}
	}

	// places list
	if (_fib_show_places && y >= ltop && y < fbot && x > FAREAMRGB && x < FAREAMRGL - FAREAMRGB) {
			const int item = (y - ltop) / _fib_font_vsep;
			*it = -1;
			if (item >= 0 && item < _placecnt) {
				*it = item;
			}
			if (*it >= 0) return 6;
			else return 0;
	}

	return 0;
}

static void fib_update_hover (Display *dpy, int need_expose, const int type, const int item) {
	int hov_p = -1;
	int hov_b = -1;
	int hov_h = -1;
	int hov_s = -1;
#ifdef LIST_ENTRY_HOVER
	int hov_f = -1;
	int hov_l = -1;
#endif

	switch (type) {
		case 1: hov_p = item; break;
		case 3: hov_b = item; break;
		case 4: hov_s = item; break;
		case 5: hov_h = item; break;
#ifdef LIST_ENTRY_HOVER
		case 6: hov_l = item; break;
		case 2: hov_f = item; break;
#endif
		default: break;
	}
#ifdef LIST_ENTRY_HOVER
	if (hov_f != _hov_f) { _hov_f = hov_f; need_expose = 1; }
	if (hov_l != _hov_l) { _hov_l = hov_l; need_expose = 1; }
#endif
	if (hov_b != _hov_b) { _hov_b = hov_b; need_expose = 1; }
	if (hov_p != _hov_p) { _hov_p = hov_p; need_expose = 1; }
	if (hov_h != _hov_h) { _hov_h = hov_h; need_expose = 1; }
	if (hov_s != _hov_s) { _hov_s = hov_s; need_expose = 1; }

	if (need_expose) {
		fib_expose (dpy, _fib_win);
	}
}

static void fib_motion (Display *dpy, int x, int y) {
	int it = -1;

	if (_scrl_my >= 0) {
		const int sdiff = y - _scrl_my;
		const int llen = (_fib_height - LISTBOT * _fib_font_vsep) / _fib_font_vsep;
		const int fsel_height = 4 + llen * _fib_font_vsep;
		const float sl = (fsel_height + _fib_font_vsep - (SCROLLBOXH + SCROLLBOXH)) / (float) _dircount;

		int news = _scrl_mf + sdiff / sl;
		if (news < 0) news = 0;
		if (news >= (_dircount - llen)) news = _dircount - llen;
		if (news != _scrl_f) {
			_scrl_f = news;
			fib_expose (dpy, _fib_win);
		}
		return;
	}

	const int type = fib_widget_at_pos (dpy, x, y, &it);
	fib_update_hover (dpy, 0, type, it);
}

static void fib_mousedown (Display *dpy, int x, int y, int btn, unsigned long time) {
	int it;
	switch (fib_widget_at_pos (dpy, x, y, &it)) {
		case 4: // scrollbar
			if (btn == 1) {
				_dblclk = 0;
				if (it == 0) {
					_scrl_my = y;
					_scrl_mf = _scrl_f;
				} else {
					int llen = (_fib_height - LISTBOT * _fib_font_vsep) / _fib_font_vsep;
					if (llen < 2) llen = 2;
					int news = _scrl_f;
					if (it == 1) {
						news -= llen - 1;
					} else {
						news += llen - 1;
					}
					if (news < 0) news = 0;
					if (news >= (_dircount - llen)) news = _dircount - llen;
					if (news != _scrl_f && _scrl_y0 >= 0) {
						assert (news >=0);
						_scrl_f = news;
						fib_update_hover (dpy, 1, 4, it);
					}
				}
			}
			break;
		case 2: // file-list
			if (btn == 4 || btn == 5) {
				const int llen = (_fib_height - LISTBOT * _fib_font_vsep) / _fib_font_vsep;
				int news = _scrl_f + ((btn == 4) ? - 1 : 1);
				if (news < 0) news = 0;
				if (news >= (_dircount - llen)) news = _dircount - llen;
				if (news != _scrl_f && _scrl_y0 >= 0) {
					assert (news >=0);
					_scrl_f = news;
					fib_update_hover (dpy, 1, 0, 0);
				}
				_dblclk = 0;
			}
			else if (btn == 1 && it >= 0 && it < _dircount) {
				if (_fsel == it) {
					if (time - _dblclk < DBLCLKTME) {
						fib_open (dpy, it);
						_dblclk = 0;
					}
					_dblclk = time;
				} else {
					fib_select (dpy, it);
					_dblclk = time;
				}
				if (_fsel >= 0) {
					if (!(_dirlist[_fsel].flags & 4));
				}
			}
			break;
		case 1: // paths
			assert (_fsel < _dircount);
			assert (it >= 0 && it < _pathparts);
			{
				int i = 0;
				char path[1024] = "/";
				while (++i <= it) {
					strcat (path, _pathbtn[i].name);
					strcat (path, "/");
				}
				char *sel = NULL;
				if (i < _pathparts)
					sel = strdup (_pathbtn[i].name);
				else if (i == _pathparts && _fsel >= 0)
					sel = strdup (_dirlist[_fsel].name);
				fib_opendir (dpy, path, sel);
				free (sel);
			}
			break;
		case 3: // btn
			if (btn == 1 && _btns[it]->callback) {
				_btns[it]->callback (dpy);
			}
			break;
		case 5: // sort
			if (btn == 1) {
				switch (it) {
					case 1: if (_sort == 0) _sort = 1; else _sort = 0; break;
					case 2: if (_sort == 2) _sort = 3; else _sort = 2; break;
					case 3: if (_sort == 4) _sort = 5; else _sort = 4; break;
				}
				if (_fsel >= 0) {
					assert (_dirlist && _dircount >= _fsel);
					_dirlist[_fsel].flags &= ~2;
					char *sel = strdup (_dirlist[_fsel].name);
					fib_resort (sel);
					free (sel);
				} else {
					fib_resort (NULL);
					_fsel = -1;
				}
				fib_reset ();
				_hov_h = it;
				fib_select (dpy, _fsel);
			}
			break;
		case 6:
			if (btn == 1 && it >= 0 && it < _placecnt) {
				fib_opendir (dpy, _placelist[it].path, NULL);
			}
			break;
		default:
			break;
	}
}

static void fib_mouseup (Display *dpy, int x, int y, int btn, unsigned long time) {
	_scrl_my = -1;
}

static void add_place_raw (Display *dpy, const char *name, const char *path) {
	_placelist = (FibPlace*) realloc (_placelist, (_placecnt + 1) * sizeof(FibPlace));
	strcpy (_placelist[_placecnt].path, path);
	strcpy (_placelist[_placecnt].name, name);
	_placelist[_placecnt].flags = 0;

	int sw;
	query_font_geometry (dpy, _fib_gc, name, &sw, NULL, NULL, NULL);
	if (sw > _fib_place_width) {
		_fib_place_width = sw;
	}
	++_placecnt;
}

static int add_place_places (Display *dpy, const char *name, const char *url) {
	char const * path;
	struct stat fs;
	int i;
	if (!url || strlen (url) < 1) return -1;
	if (!name || strlen (name) < 1) return -1;
	if (url[0] == '/') {
		path = url;
	}
	else if (!strncmp (url, "file:///", 8)) {
		path = &url[7];
	}
	else {
		return -1;
	}

	if (access (path, R_OK)) {
		return -1;
	}
	if (stat (path, &fs)) {
		return -1;
	}
	if (!S_ISDIR (fs.st_mode)) {
		return -1;
	}

	for (i = 0; i < _placecnt; ++i) {
		if (!strcmp (path, _placelist[i].path)) {
			return -1;
		}
	}
	add_place_raw (dpy, name, path);
	return 0;
}

static int parse_gtk_bookmarks (Display *dpy, const char *fn) {
	char tmp[1024];
	if (access (fn, R_OK)) {
		return -1;
	}
	FILE *bm = fopen (fn, "r");
	if (!bm) return -1;
	int found = 0;
	while (fgets (tmp, sizeof(tmp), bm)
			&& strlen (tmp) > 1
			&& strlen (tmp) < sizeof(tmp))
	{
		char *s, *n;
		tmp[strlen (tmp) - 1] = '\0'; // strip newline
		if ((s = strchr (tmp, ' '))) {
			*s = '\0';
			n = strdup (++s);
			decode_3986 (tmp);
			if (!add_place_places (dpy, n, tmp)) {
				++found;
			}
			free (n);
		} else if ((s = strrchr (tmp, '/'))) {
			n = strdup (++s);
			decode_3986 (tmp);
			if (!add_place_places (dpy, n, tmp)) {
				++found;
			}
			free (n);
		}
	}
	fclose (bm);
	return found;
}

static const char *ignore_mountpoints[] = {
	"/bin",  "/boot", "/dev",  "/etc",
	"/lib",  "/live", "/mnt",  "/opt",
	"/root", "/sbin", "/srv",  "/tmp",
	"/usr",  "/var",  "/proc", "/sbin",
	"/net",  "/sys"
};

static const char *ignore_fs[] = {
	"auto",      "autofs",
	"debugfs",   "devfs",
	"devpts",    "ecryptfs",
	"fusectl",   "kernfs",
	"linprocfs", "proc",
	"ptyfs",     "rootfs",
	"selinuxfs", "sysfs",
	"tmpfs",     "usbfs",
	"nfsd",      "rpc_pipefs",
};

static const char *ignore_devices[] = {
	"binfmt_",   "devpts",
	"gvfs",      "none",
	"nfsd",      "sunrpc",
	"/dev/loop", "/dev/vn"
};

static int check_mount (const char *mountpoint, const char *fs, const char *device) {
	size_t i;
	if (!mountpoint || !fs || !device) return -1;
	//printf("%s %s %s\n", mountpoint, fs, device);
	for (i = 0 ; i < sizeof(ignore_mountpoints) / sizeof(char*); ++i) {
		if (!strncmp (mountpoint, ignore_mountpoints[i], strlen (ignore_mountpoints[i]))) {
			return 1;
		}
	}
	if (!strncmp (mountpoint, "/home", 5)) {
		return 1;
	}
	for (i = 0 ; i < sizeof(ignore_fs) / sizeof(char*); ++i) {
		if (!strncmp (fs, ignore_fs[i], strlen (ignore_fs[i]))) {
			return 1;
		}
	}
	for (i = 0 ; i < sizeof(ignore_devices) / sizeof(char*); ++i) {
		if (!strncmp (device, ignore_devices[i], strlen (ignore_devices[i]))) {
			return 1;
		}
	}
	return 0;
}

static int read_mtab (Display *dpy, const char *mtab) {
	FILE *mt = fopen (mtab, "r");
	if (!mt) return -1;
	int found = 0;
	struct mntent *mntent;
	while ((mntent = getmntent (mt)) != NULL) {
		char *s;
		if (check_mount (mntent->mnt_dir, mntent->mnt_type, mntent->mnt_fsname))
			continue;

		if ((s = strrchr (mntent->mnt_dir, '/'))) {
			++s;
		} else {
			s = mntent->mnt_dir;
		}
		if (!add_place_places (dpy, s, mntent->mnt_dir)) {
			++found;
		}
	}
	fclose (mt);
	return found;
}

static void populate_places (Display *dpy) {
	char tmp[1024];
	int spacer = -1;
	if (_placecnt > 0) return;
	_fib_place_width = 0;

	if (_recentcnt > 0) {
		add_place_raw (dpy, "Recently Used", "");
		_placelist[0].flags |= 4;
	}

	add_place_places (dpy, "Home", getenv ("HOME"));

	if (getenv ("HOME")) {
		strcpy (tmp, getenv ("HOME"));
		strcat (tmp, "/Desktop");
		add_place_places (dpy, "Desktop", tmp);
	}

	add_place_places (dpy, "Filesystem", "/");

	if (_placecnt > 0) spacer = _placecnt -1;

	if (strlen (_fib_cfg_custom_places) > 0) {
		parse_gtk_bookmarks (dpy, _fib_cfg_custom_places);
	}

	if (read_mtab (dpy, "/proc/mounts") < 1) {
		read_mtab (dpy, "/etc/mtab");
	}

	int parsed_bookmarks = 0;
	if (!parsed_bookmarks && getenv ("HOME")) {
		strcpy (tmp, getenv ("HOME"));
		strcat (tmp, "/.gtk-bookmarks");
		if (parse_gtk_bookmarks (dpy, tmp) > 0) {
			parsed_bookmarks = 1;
		}
	}
	if (!parsed_bookmarks && getenv ("XDG_CONFIG_HOME")) {
		strcpy (tmp, getenv ("XDG_CONFIG_HOME"));
		strcat (tmp, "/gtk-3.0/bookmarks");
		if (parse_gtk_bookmarks (dpy, tmp) > 0) {
			parsed_bookmarks = 1;
		}
	}
	if (!parsed_bookmarks && getenv ("HOME")) {
		strcpy (tmp, getenv ("HOME"));
		strcat (tmp, "/.config/gtk-3.0/bookmarks");
		if (parse_gtk_bookmarks (dpy, tmp) > 0) {
			parsed_bookmarks = 1;
		}
	}
	if (_fib_place_width > 0) {
		_fib_place_width = MIN (_fib_place_width + TEXTSEP + _fib_dir_indent /*extra*/ , PLACESWMAX);
	}
	if (spacer > 0 && spacer < _placecnt -1) {
		_placelist[ spacer ].flags |= 4;
	}
}

static uint8_t font_err = 0;
static int x_error_handler (Display *d, XErrorEvent *e) {
	font_err = 1;
	return 0;
}

int x_fib_show (Display *dpy, Window parent, int x, int y) {
	if (_fib_win) {
		XSetInputFocus (dpy, _fib_win, RevertToParent, CurrentTime);
		return -1;
	}

	_status = 0;
	_rv_open[0] = '\0';

	Colormap colormap = DefaultColormap (dpy, DefaultScreen (dpy));
	_c_gray1.flags= DoRed | DoGreen | DoBlue;
	_c_gray0.red = _c_gray0.green = _c_gray0.blue = 61710; // 95% hover prelight
	_c_gray1.red = _c_gray1.green = _c_gray1.blue = 60416; // 93% window bg, scrollbar-fg
	_c_gray2.red = _c_gray2.green = _c_gray2.blue = 54016; // 83% button & list bg
	_c_gray3.red = _c_gray3.green = _c_gray3.blue = 48640; // 75% heading + scrollbar-bg
	_c_gray4.red = _c_gray4.green = _c_gray4.blue = 26112; // 40% prelight text, sep lines
	_c_gray5.red = _c_gray5.green = _c_gray5.blue = 12800; // 20% 3D border
	_c_gray6.red = _c_gray6.green = _c_gray6.blue =  6400; // 10% checkbox cross, sort triangles

	if (!XAllocColor (dpy, colormap, &_c_gray0)) return -1;
	if (!XAllocColor (dpy, colormap, &_c_gray1)) return -1;
	if (!XAllocColor (dpy, colormap, &_c_gray2)) return -1;
	if (!XAllocColor (dpy, colormap, &_c_gray3)) return -1;
	if (!XAllocColor (dpy, colormap, &_c_gray4)) return -1;
	if (!XAllocColor (dpy, colormap, &_c_gray5)) return -1;
	if (!XAllocColor (dpy, colormap, &_c_gray6)) return -1;

	XSetWindowAttributes attr;
	memset (&attr, 0, sizeof(XSetWindowAttributes));
	attr.border_pixel = _c_gray2.pixel;

	attr.event_mask = ExposureMask | KeyPressMask
		| ButtonPressMask | ButtonReleaseMask
		| ConfigureNotify | StructureNotifyMask
		| PointerMotionMask | LeaveWindowMask;

	_fib_win = XCreateWindow (
			dpy, DefaultRootWindow (dpy),
			x, y, _fib_width, _fib_height,
			1, CopyFromParent, InputOutput, CopyFromParent,
			CWEventMask | CWBorderPixel, &attr);

	if (!_fib_win) { return 1; }

	if (parent)
		XSetTransientForHint (dpy, _fib_win, parent);

	XStoreName (dpy, _fib_win, "Select File");

	Atom wmDelete = XInternAtom (dpy, "WM_DELETE_WINDOW", True);
	XSetWMProtocols (dpy, _fib_win, &wmDelete, 1);

	_fib_gc = XCreateGC (dpy, _fib_win, 0, NULL);
	XSetLineAttributes (dpy, _fib_gc, 1, LineSolid, CapButt, JoinMiter);
	const char dl[1] = {1};
	XSetDashes (dpy, _fib_gc, 0, dl, 1);

	int (*handler)(Display *, XErrorEvent *) = XSetErrorHandler (&x_error_handler);

#define _XTESTFONT(FN) \
	{ \
		font_err = 0; \
		_fibfont = XLoadFont (dpy, FN); \
		XSetFont (dpy, _fib_gc, _fibfont); \
		XSync (dpy, False); \
	}

	font_err = 1;
	if (getenv ("XJFONT")) _XTESTFONT (getenv ("XJFONT"));
	if (font_err && strlen (_fib_cfg_custom_font) > 0) _XTESTFONT (_fib_cfg_custom_font);
	if (font_err) _XTESTFONT ("-*-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*");
	if (font_err) _XTESTFONT ("-*-verdana-medium-r-normal-*-12-*-*-*-*-*-*-*");
	if (font_err) _XTESTFONT ("-misc-fixed-medium-r-normal-*-13-*-*-*-*-*-*-*");
	if (font_err) _XTESTFONT ("-misc-fixed-medium-r-normal-*-12-*-*-*-*-*-*-*");
	if (font_err) _fibfont = None;
	XSync (dpy, False);
	XSetErrorHandler (handler);

	if (_fib_font_height == 0) { // 1st time only
		query_font_geometry (dpy, _fib_gc, "D ", &_fib_dir_indent, NULL, NULL, NULL);
		query_font_geometry (dpy, _fib_gc, "_", &_fib_spc_norm, NULL, NULL, NULL);
		if (query_font_geometry (dpy, _fib_gc, "|0Yy", NULL, &_fib_font_height, &_fib_font_ascent, NULL)) {
			XFreeGC (dpy, _fib_gc);
			XDestroyWindow (dpy, _fib_win);
			_fib_win = 0;
			return -1;
		}
		_fib_font_height += 3;
		_fib_font_ascent += 2;
		_fib_font_vsep = _fib_font_height + 2;
	}

	populate_places (dpy);

	strcpy (_btn_ok.text,     "Open");
	strcpy (_btn_cancel.text, "Cancel");
	strcpy (_btn_filter.text, "List All Files");
	strcpy (_btn_places.text, "Show Places");
	strcpy (_btn_hidden.text, "Show Hidden");

	_btn_ok.callback     = &cb_open;
	_btn_cancel.callback = &cb_cancel;
	_btn_filter.callback = &cb_filter;
	_btn_places.callback = &cb_places;
	_btn_hidden.callback = &cb_hidden;
	_btn_filter.flags |= 4;
	_btn_places.flags |= 4;
	_btn_hidden.flags |= 4;

	if (!_fib_filter_function) {
		_btn_filter.flags |= 8;
	}

	size_t i;
	int btncnt = 0;
	_btn_w = 0;
	_btn_span = 0;
	for (i = 0; i < sizeof(_btns) / sizeof(FibButton*); ++i) {
		if (_btns[i]->flags & 8) { continue; }
		query_font_geometry (dpy, _fib_gc, _btns[i]->text, &_btns[i]->tw, NULL, NULL, NULL);
		if (_btns[i]->flags & 4) {
			_btn_span += _btns[i]->tw + _fib_font_ascent + TEXTSEP;
		} else {
			++btncnt;
			if (_btns[i]->tw > _btn_w)
				_btn_w = _btns[i]->tw;
		}
	}

	_btn_w += BTNPADDING + BTNPADDING + TEXTSEP + TEXTSEP + TEXTSEP;
	_btn_span += _btn_w  * btncnt + DSEP * (i - 1) + FAREAMRGR + FAREAMRGB;

	for (i = 0; i < sizeof(_btns) / sizeof(FibButton*); ++i) {
		if (_btns[i]->flags & 8) { continue; }
		if (_btns[i]->flags & 4) {
			_btns[i]->xw = _btns[i]->tw + _fib_font_ascent + TEXTSEP;
		} else {
			_btns[i]->xw = _btn_w;
		}
	}

	sync_button_states () ;

	_fib_height = _fib_font_vsep * (15.8);
	_fib_width  = MAX (_btn_span, 440);

	XResizeWindow (dpy, _fib_win, _fib_width, _fib_height);

	XTextProperty x_wname, x_iname;
	XSizeHints hints;
	XWMHints wmhints;

	hints.flags = PSize | PMinSize;
	hints.min_width = _btn_span;
	hints.min_height = 8 * _fib_font_vsep;

	char *w_name = & _fib_cfg_title[0];

	wmhints.input = True;
	wmhints.flags = InputHint;
	if (XStringListToTextProperty (&w_name, 1, &x_wname) &&
			XStringListToTextProperty (&w_name, 1, &x_iname))
	{
		XSetWMProperties (dpy, _fib_win, &x_wname, &x_iname, NULL, 0, &hints, &wmhints, NULL);
		XFree (x_wname.value);
		XFree (x_iname.value);
	}

	XSetWindowBackground (dpy, _fib_win, _c_gray1.pixel);

	_fib_mapped = 0;
	XMapRaised (dpy, _fib_win);

	if (!strlen (_cur_path) || !fib_opendir (dpy, _cur_path, NULL)) {
		fib_opendir (dpy, getenv ("HOME") ? getenv ("HOME") : "/", NULL);
	}

#if 0
	XGrabPointer (dpy, _fib_win, True,
			ButtonReleaseMask | ButtonPressMask | EnterWindowMask | LeaveWindowMask | PointerMotionMask | StructureNotifyMask,
			GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
	XGrabKeyboard (dpy, _fib_win, True, GrabModeAsync, GrabModeAsync, CurrentTime);
	//XSetInputFocus (dpy, parent, RevertToNone, CurrentTime);
#endif
	_recentlock = 1;
	return 0;
}

void x_fib_close (Display *dpy) {
	if (!_fib_win) return;
	XFreeGC (dpy, _fib_gc);
	XDestroyWindow (dpy, _fib_win);
	_fib_win = 0;
	free (_dirlist);
	_dirlist = NULL;
	free (_pathbtn);
	_pathbtn = NULL;
	if (_fibfont != None) XUnloadFont (dpy, _fibfont);
	_fibfont = None;
	free (_placelist);
	_placelist = NULL;
	_dircount = 0;
	_pathparts = 0;
	_placecnt = 0;
	if (_pixbuffer != None) XFreePixmap (dpy, _pixbuffer);
	_pixbuffer = None;
	Colormap colormap = DefaultColormap (dpy, DefaultScreen (dpy));
	XFreeColors (dpy, colormap, &_c_gray0.pixel, 1, 0);
	XFreeColors (dpy, colormap, &_c_gray1.pixel, 1, 0);
	XFreeColors (dpy, colormap, &_c_gray2.pixel, 1, 0);
	XFreeColors (dpy, colormap, &_c_gray3.pixel, 1, 0);
	XFreeColors (dpy, colormap, &_c_gray4.pixel, 1, 0);
	XFreeColors (dpy, colormap, &_c_gray5.pixel, 1, 0);
	XFreeColors (dpy, colormap, &_c_gray6.pixel, 1, 0);
	_recentlock = 0;
}

int x_fib_handle_events (Display *dpy, XEvent *event) {
	if (!_fib_win) return 0;
	if (_status) return 0;
	if (event->xany.window != _fib_win) {
		return 0;
	}

	switch (event->type) {
		case MapNotify:
			_fib_mapped = 1;
			break;
		case UnmapNotify:
			_fib_mapped = 0;
			break;
		case LeaveNotify:
			fib_update_hover (dpy, 1, 0, 0);
			break;
		case ClientMessage:
			if (!strcmp (XGetAtomName (dpy, event->xclient.message_type), "WM_PROTOCOLS")) {
				_status = -1;
			}
		case ConfigureNotify:
			if (
					(event->xconfigure.width > 1 && event->xconfigure.height > 1)
					&&
					(event->xconfigure.width != _fib_width || event->xconfigure.height != _fib_height)
				 )
			{
				_fib_width = event->xconfigure.width;
				_fib_height = event->xconfigure.height;
				_fib_resized = 1;
			}
			break;
		case Expose:
			if (event->xexpose.count == 0) {
				fib_expose (dpy, event->xany.window);
			}
			break;
		case MotionNotify:
			fib_motion (dpy, event->xmotion.x, event->xmotion.y);
			if (event->xmotion.is_hint == NotifyHint) {
				XGetMotionEvents (dpy, event->xany.window, CurrentTime, CurrentTime, NULL);
			}
			break;
		case ButtonPress:
			fib_mousedown (dpy, event->xbutton.x, event->xbutton.y, event->xbutton.button, event->xbutton.time);
			break;
		case ButtonRelease:
			fib_mouseup (dpy, event->xbutton.x, event->xbutton.y, event->xbutton.button, event->xbutton.time);
			break;
		case KeyRelease:
			break;
		case KeyPress:
			{
				KeySym key;
				char buf[100];
				static XComposeStatus stat;
				XLookupString (&event->xkey, buf, sizeof(buf), &key, &stat);
				switch (key) {
					case XK_Escape:
						_status = -1;
						break;
					case XK_Up:
						if (_fsel > 0) {
							fib_select (dpy, _fsel - 1);
						}
						break;
					case XK_Down:
						if (_fsel < _dircount -1) {
							fib_select ( dpy, _fsel + 1);
						}
						break;
					case XK_Page_Up:
						if (_fsel > 0) {
							int llen = (_fib_height - LISTBOT * _fib_font_vsep) / _fib_font_vsep;
							if (llen < 1) llen = 1; else --llen;
							int fs = MAX (0, _fsel - llen);
							fib_select ( dpy, fs);
						}
						break;
					case XK_Page_Down:
						if (_fsel < _dircount) {
							int llen = (_fib_height - LISTBOT * _fib_font_vsep) / _fib_font_vsep;
							if (llen < 1) llen = 1; else --llen;
							int fs = MIN (_dircount - 1, _fsel + llen);
							fib_select ( dpy, fs);
						}
						break;
					case XK_Left:
						if (_pathparts > 1) {
							int i = 0;
							char path[1024] = "/";
							while (++i < _pathparts - 1) {
								strcat (path, _pathbtn[i].name);
								strcat (path, "/");
							}
							char *sel = strdup (_pathbtn[_pathparts-1].name);
							fib_opendir (dpy, path, sel);
							free (sel);
						}
						break;
					case XK_Right:
						if (_fsel >= 0 && _fsel < _dircount) {
							if (_dirlist[_fsel].flags & 4) {
								cb_open (dpy);
							}
						}
						break;
					case XK_Return:
						cb_open (dpy);
						break;
					default:
						if ((key >= XK_a && key <= XK_z) || (key >= XK_0 && key <= XK_9)) {
							int i;
							for (i = 0; i < _dircount; ++i) {
								int j = (_fsel + i + 1) % _dircount;
								char kcmp = _dirlist[j].name[0];
								if (kcmp > 0x40 && kcmp <= 0x5A) kcmp |= 0x20;
								if (kcmp == (char)key) {
									fib_select ( dpy, j);
									break;
								}
							}
						}
						break;
				}
			}
			break;
	}

	if (_status) {
		x_fib_close (dpy);
	}
	return _status;
}

int x_fib_status () {
	return _status;
}

int x_fib_configure (int k, const char *v) {
	if (_fib_win) { return -1; }
	switch (k) {
		case 0:
			if (strlen (v) >= sizeof(_cur_path) -1) return -2;
			if (strlen (v) < 1) return -2;
			if (v[0] != '/') return -2;
			if (strstr (v, "//")) return -2;
			strncpy (_cur_path, v, sizeof(_cur_path));
			break;
		case 1:
			if (strlen (v) >= sizeof(_fib_cfg_title) -1) return -2;
			strncpy (_fib_cfg_title, v, sizeof(_fib_cfg_title));
			break;
		case 2:
			if (strlen (v) >= sizeof(_fib_cfg_custom_font) -1) return -2;
			strncpy (_fib_cfg_custom_font, v, sizeof(_fib_cfg_custom_font));
			break;
		case 3:
			if (strlen (v) >= sizeof(_fib_cfg_custom_places) -1) return -2;
			strncpy (_fib_cfg_custom_places, v, sizeof(_fib_cfg_custom_places));
			break;
		default:
			return -2;
	}
	return 0;
}

int x_fib_cfg_buttons (int k, int v) {
	if (_fib_win) { return -1; }
	switch (k) {
		case 1:
			if (v < 0) {
				_btn_hidden.flags |= 8;
			} else {
				_btn_hidden.flags &= ~8;
			}
			if (v == 1) {
				_btn_hidden.flags |= 2;
				_fib_hidden_fn = 1;
			} else if (v == 0) {
				_btn_hidden.flags &= 2;
				_fib_hidden_fn = 0;
			}
			break;
		case 2:
			if (v < 0) {
				_btn_places.flags |= 8;
			} else {
				_btn_places.flags &= ~8;
			}
			if (v == 1) {
				_btn_places.flags |= 2;
				_fib_show_places = 1;
			} else if (v == 0) {
				_btn_places.flags &= ~2;
				_fib_show_places = 0;
			}
			break;
		case 3:
			// NB. filter button is automatically hidden
			// IFF the filter-function is NULL.
			if (v < 0) {
				_btn_filter.flags |= 8;
			} else {
				_btn_filter.flags &= ~8;
			}
			if (v == 1) {
				_btn_filter.flags &= ~2; // inverse - 'show all' = !filter
				_fib_filter_fn = 1;
			} else if (v == 0) {
				_btn_filter.flags |= 2;
				_fib_filter_fn = 0;
			}
		default:
			return -2;
	}
	return 0;
}

int x_fib_cfg_filter_callback (int (*cb)(const char*)) {
	if (_fib_win) { return -1; }
	_fib_filter_function = cb;
	return 0;
}

char *x_fib_filename () {
	if (_status > 0 && !_fib_win)
		return strdup (_rv_open);
	else
		return NULL;
}
#endif // HAVE_X11


/* example usage */
#ifdef SOFD_TEST

static int fib_filter_movie_filename (const char *name) {
	if (!_fib_filter_fn) return 1;
	const int l3 = strlen (name) - 3;
	const int l4 = l3 - 1;
	const int l5 = l4 - 1;
	const int l6 = l5 - 1;
	const int l9 = l6 - 3;
	if (
			(l4 > 0 && (
				   !strcasecmp (&name[l4], ".avi")
				|| !strcasecmp (&name[l4], ".mov")
				|| !strcasecmp (&name[l4], ".ogg")
				|| !strcasecmp (&name[l4], ".ogv")
				|| !strcasecmp (&name[l4], ".mpg")
				|| !strcasecmp (&name[l4], ".mov")
				|| !strcasecmp (&name[l4], ".mp4")
				|| !strcasecmp (&name[l4], ".mkv")
				|| !strcasecmp (&name[l4], ".vob")
				|| !strcasecmp (&name[l4], ".asf")
				|| !strcasecmp (&name[l4], ".avs")
				|| !strcasecmp (&name[l4], ".dts")
				|| !strcasecmp (&name[l4], ".flv")
				|| !strcasecmp (&name[l4], ".m4v")
				)) ||
			(l5 > 0 && (
				   !strcasecmp (&name[l5], ".h264")
				|| !strcasecmp (&name[l5], ".webm")
				)) ||
			(l6 > 0 && (
				   !strcasecmp (&name[l6], ".dirac")
				)) ||
			(l9 > 0 && (
				   !strcasecmp (&name[l9], ".matroska")
				)) ||
			(l3 > 0 && (
				   !strcasecmp (&name[l3], ".dv")
				|| !strcasecmp (&name[l3], ".ts")
				))
			)
			{
				return 1;
			}
	return 0;
}

int main (int argc, char **argv) {
	Display* dpy = XOpenDisplay (0);
	if (!dpy) return -1;

	x_fib_cfg_filter_callback (fib_filter_movie_filename);
	x_fib_configure (1, "Open Movie File");
	x_fib_load_recent ("/tmp/sofd.recent");
	x_fib_show (dpy, 0, 300, 300);

	while (1) {
		XEvent event;
		while (XPending (dpy) > 0) {
			XNextEvent (dpy, &event);
			if (x_fib_handle_events (dpy, &event)) {
				if (x_fib_status () > 0) {
					char *fn = x_fib_filename ();
					printf ("OPEN '%s'\n", fn);
					x_fib_add_recent (fn, time (NULL));
					free (fn);
				}
			}
		}
		if (x_fib_status ()) {
			break;
		}
		usleep (80000);
	}
	x_fib_close (dpy);

	x_fib_save_recent ("/tmp/sofd.recent");

	x_fib_free_recent ();
	XCloseDisplay (dpy);
	return 0;
}
#endif
