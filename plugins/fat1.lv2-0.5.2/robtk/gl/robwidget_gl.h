/* robwidget - GL wrapper
 *
 * Copyright (C) 2013 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#define GET_HANDLE(HDL) (((RobWidget*)HDL)->self)

#define robwidget_set_expose_event(RW, EVT)    { (RW)->expose_event = EVT; }
#define robwidget_set_size_request(RW, EVT)    { (RW)->size_request = EVT; }
#define robwidget_set_size_allocate(RW, EVT)   { (RW)->size_allocate = EVT; }
#define robwidget_set_size_limit(RW, EVT)      { (RW)->size_limit = EVT; }
#define robwidget_set_size_default(RW, EVT)    { (RW)->size_default = EVT; }
#define robwidget_set_mouseup(RW, EVT)         { (RW)->mouseup = EVT; }
#define robwidget_set_mousedown(RW, EVT)       { (RW)->mousedown = EVT; }
#define robwidget_set_mousemove(RW, EVT)       { (RW)->mousemove = EVT; }
#define robwidget_set_mousescroll(RW, EVT)     { (RW)->mousescroll = EVT; }
#define robwidget_set_enter_notify(RW, EVT)    { (RW)->enter_notify = EVT; }
#define robwidget_set_leave_notify(RW, EVT)    { (RW)->leave_notify = EVT; }


/* widget-tree & packing */
static void offset_traverse_parents(RobWidget *rw, RobTkBtnEvent *ev) {
	assert(rw);
	do {
		ev->x -= rw->area.x;
		ev->y -= rw->area.y;
		if (rw == rw->parent) {
			break;
		}
		rw = rw->parent;
	} while (rw);
}

static void offset_traverse_from_child(RobWidget *rw, RobTkBtnEvent *ev) {
	assert(rw);
	do {
		ev->x += rw->area.x;
		ev->y += rw->area.y;
		if (rw == rw->parent) {
			break;
		}
		rw = rw->parent;
	} while (rw);
}

static RobWidget * robwidget_child_at(RobWidget *rw, int x, int y) {
	for (unsigned int i=0; i < rw->childcount; ++i) {
		RobWidget * c = (RobWidget *) rw->children[i];
		if (c->hidden) continue;
		if (x >= c->area.x && y >= c->area.y
				&& x <= c->area.x + c->area.width
				&& y <= c->area.y + c->area.height
			 ) {
			return c;
		}
	}
	return NULL;
}

static RobWidget* decend_into_widget_tree(RobWidget *rw, int x, int y) {
	if (rw->childcount == 0) return rw;
	x-=rw->area.x; y-=rw->area.y;
	for (unsigned int i=0; i < rw->childcount; ++i) {
		RobWidget * c = (RobWidget *) rw->children[i];
		if (c->hidden) continue;
		if (c->block_events) continue;
		if (x >= c->area.x && y >= c->area.y
				&& x <= c->area.x + c->area.width
				&& y <= c->area.y + c->area.height
			 ) {
			return decend_into_widget_tree(c, x, y);
		}
	}
	return NULL;
}

/*****************************************************************************/
/* RobWidget implementation */

/*declared in packer.h */
static void rtoplevel_size_request(RobWidget* rw, int *w, int *h);
static bool rcontainer_expose_event(RobWidget* rw, cairo_t* cr, cairo_rectangle_t *ev);

static RobWidget * robwidget_new(void *handle) {
	RobWidget * rw = (RobWidget *) calloc(1, sizeof(RobWidget));
	rw->self = handle;
	rw->xalign = .5;
	rw->yalign = .5;
	rw->hidden = FALSE;
	rw->block_events = FALSE;
	rw->widget_scale = 1.0;
	return rw;
}

static void robwidget_make_toplevel(RobWidget *rw, void * const handle) {
	rw->top = handle;
	rw->parent = rw;
}

static void robwidget_destroy(RobWidget *rw) {
	if (!rw) return;
#ifndef NDEBUG
	if (rw->children && rw->childcount == 0)
		fprintf(stderr, "robwidget_destroy: '%s' children <> childcount = 0\n",
				ROBWIDGET_NAME(rw));
	if (!rw->children && rw->childcount != 0)
		fprintf(stderr, "robwidget_destroy: '%s' childcount <> children = NULL\n",
				ROBWIDGET_NAME(rw));
#endif

#if 0 // recursive
	for (unsigned int i=0; i < rw->childcount; ++i) {
		RobWidget * c = (RobWidget *) rw->children[i];
		robwidget_destroy(c);
	}
#endif

	free(rw->children);
#if 0
	rw->children = NULL;
	rw->childcount = 0;
#endif
	free(rw);
}

static void robwidget_set_size(RobWidget *rw, int w, int h) {
	rw->area.width  = w;
	rw->area.height = h;
}

static void robwidget_set_area(RobWidget *rw, int x, int y, int w, int h) {
	rw->area.x = x;
	rw->area.y = y;
	rw->area.width  = w;
	rw->area.height = h;
}

static void robwidget_set_alignment(RobWidget *rw, float xalign, float yalign) {
	rw->xalign = xalign;
	rw->yalign = yalign;
}

static void robwidget_resize_toplevel(RobWidget *rw, int w, int h) {
	resize_toplevel(rw, w, h);
}

static void robwidget_show(RobWidget *rw, bool resize_window) {
	// XXX never call from expose_event
	if (rw->hidden) {
		rw->hidden = FALSE;
		if (resize_window) resize_self(rw);
		else relayout_toplevel(rw);
	}
}

static void robwidget_hide(RobWidget *rw, bool resize_window) {
	// XXX never call from expose_event
	if (!rw->hidden) {
		rw->hidden = TRUE;
		if (resize_window) resize_self(rw);
		else relayout_toplevel(rw);
	}
}

/*****************************************************************************/
/* host helper */

static void const * robwidget_get_toplevel_handle(RobWidget *rw) {
	void const *h = NULL;
	while (rw) {
		if (rw == rw->parent) {
			h = rw->top;
			break;
		}
		rw = rw->parent;
	}
	return h;
}
