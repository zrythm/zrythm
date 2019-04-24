/* robwidget - widget layout packing
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

/*****************************************************************************/
/* common container functions */

//#define DEBUG_HBOX
//#define DEBUG_VBOX
//#define DEBUG_TABLE

#define RTK_SHRINK 0
#define RTK_EXPAND 1
#define RTK_FILL   2
#define RTK_EXANDF 3

struct rob_container {
	bool homogeneous;
	bool expand;
	int padding;
};

struct rob_table_child {
	RobWidget *rw;
	int left;
	int right;
	int top;
	int bottom;
	int xpadding;
	int ypadding;
	int expand_x;
	int expand_y;
};

struct rob_table_field {
	int req_w;
	int req_h;
	bool is_expandable_x;
	bool is_expandable_y;
	int acq_w;
	int acq_h;
	int expand;
};

struct rob_table {
	bool homogeneous;
	bool expand;
	unsigned int nrows;
	unsigned int ncols;
	unsigned int nchilds;
	struct rob_table_child *chld;
	struct rob_table_field *rows;
	struct rob_table_field *cols;
};

static void rob_table_resize(struct rob_table *rt, unsigned int nrows, unsigned int ncols) {
	if (rt->ncols >= ncols && rt->nrows >= nrows) return;
#ifdef DEBUG_TABLE
	printf("rob_table_resize %d %d\n", nrows, ncols);
#endif

	if (rt->nrows != nrows) {
		rt->rows = (struct rob_table_field*) realloc(rt->rows, sizeof(struct rob_table_field) * nrows);
		rt->nrows = nrows;
	}
	if (rt->ncols != ncols) {
		rt->cols = (struct rob_table_field*) realloc(rt->cols, sizeof(struct rob_table_field) * ncols);
		rt->ncols = ncols;
	}
}

static void robwidget_position_cache(RobWidget *rw) {
	RobTkBtnEvent e;
	e.x = 0; e.y = 0;
	offset_traverse_from_child(rw, &e);

	rw->trel.x = e.x; rw->trel.y = e.y;
	rw->trel.width  = rw->area.width;
	rw->trel.height = rw->area.height;
	rw->resized = TRUE;
}

static void robwidget_position_set(RobWidget *rw,
		const int pw, const int ph) {
	assert (pw >= rw->area.width && ph >= rw->area.height);
	rw->area.x = rint((pw - rw->area.width) * rw->xalign);
	rw->area.y = rint((ph - rw->area.height) * rw->yalign);
}

static void rtable_size_allocate(RobWidget* rw, int w, int h);
static void rhbox_size_allocate(RobWidget* rw, int w, int h);
static void rvbox_size_allocate(RobWidget* rw, int w, int h);

static bool roblayout_can_expand(RobWidget *rw) {
	bool can_expand = FALSE;
	if (   rw->size_allocate == rhbox_size_allocate
	    || rw->size_allocate == rvbox_size_allocate) {
		can_expand = ((struct rob_container*)rw->self)->expand;
	} else if (rw->size_allocate == rtable_size_allocate) {
		can_expand = ((struct rob_table*)rw->self)->expand;
	} else if (rw->size_allocate) {
		can_expand = (rw->packing_opts & 1) ? TRUE : FALSE;
	}
	return can_expand;
}

static bool roblayout_can_fill(RobWidget *rw) {
	return (rw->packing_opts & 2);
}

static void rcontainer_child_pack(RobWidget *rw, RobWidget *chld, bool expand, bool fill) {
#ifndef NDEBUG
	if (chld->parent) {
		fprintf(stderr, "re-parent child\n");
	}
#endif
	if (   chld->size_allocate == rhbox_size_allocate
	    || chld->size_allocate == rvbox_size_allocate
			) {
		((struct rob_container*)chld->self)->expand = expand;
	}
	if (chld->size_allocate == rtable_size_allocate) {
		((struct rob_table*)chld->self)->expand = expand;
	}
	chld->packing_opts = (expand ? 1 : 0) | (fill ? 2 : 0);
	rw->children = (RobWidget**) realloc(rw->children, (rw->childcount + 1) * sizeof(RobWidget *));
	rw->children[rw->childcount] = chld;
	rw->childcount++;
	chld->parent = rw;
}
static void rcontainer_clear_bg(RobWidget* rw, cairo_t* cr, cairo_rectangle_t *ev) {
  float c[4];
#ifdef VISIBLE_EXPOSE
	c[0] = rand() / (float)RAND_MAX;
	c[1] = rand() / (float)RAND_MAX;
	c[2] = rand() / (float)RAND_MAX;
	c[3] = 1.0;
#else
  get_color_from_theme(1, c);
#endif
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgb (cr, c[0], c[1], c[2]);
  cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
#if 0
  cairo_fill_preserve(cr);
  cairo_set_source_rgb (cr, 1.0, 0.1, .1);
  cairo_stroke(cr);
#else
  cairo_fill(cr);
#endif
}

/*****************************************************************************/
/* common container functions, for event propagation */

#define MOUSEEVENT \
	RobTkBtnEvent event; \
	event.x = ev->x - c->area.x; \
	event.y = ev->y - c->area.y; \
	event.state = ev->state; \
	event.direction = ev->direction; \
	event.button = ev->button;

static RobWidget* rcontainer_mousedown(RobWidget* handle, RobTkBtnEvent *ev) {
	RobWidget * rw = (RobWidget*)handle;
	if (rw->block_events ) return NULL;
	RobWidget * c = robwidget_child_at(rw, ev->x, ev->y);
	if (!c || !c->mousedown) return NULL;
	if (c->hidden) return NULL;
	MOUSEEVENT
	return c->mousedown(c, &event);
}

static RobWidget* rcontainer_mouseup(RobWidget* handle, RobTkBtnEvent *ev) {
	RobWidget * rw = (RobWidget*)handle;
	if (rw->block_events ) return NULL;
	RobWidget * c = robwidget_child_at(rw, ev->x, ev->y);
	if (!c || !c->mouseup) return NULL;
	if (c->hidden) return NULL;
	MOUSEEVENT
	return c->mouseup(c, &event);
}

static RobWidget* rcontainer_mousemove(RobWidget* handle, RobTkBtnEvent *ev) {
	RobWidget * rw = (RobWidget*)handle;
	if (rw->block_events ) return NULL;
	RobWidget * c = robwidget_child_at(rw, ev->x, ev->y);
	if (!c || !c->mousemove) return NULL;
	if (c->hidden) return NULL;
	MOUSEEVENT
	return c->mousemove(c, &event);
}

static RobWidget* rcontainer_mousescroll(RobWidget* handle, RobTkBtnEvent *ev) {
	RobWidget * rw = (RobWidget*)handle;
	if (rw->block_events ) return NULL;
	RobWidget * c = robwidget_child_at(rw, ev->x, ev->y);
	if (!c || !c->mousescroll) return NULL;
	if (c->hidden) return NULL;
	MOUSEEVENT
	return c->mousescroll(c, &event);
}

static bool rcontainer_expose_event_no_clear(RobWidget* rw, cairo_t* cr, cairo_rectangle_t *ev) {

	for (unsigned int i=0; i < rw->childcount; ++i) {
		RobWidget * c = (RobWidget *) rw->children[i];
		cairo_rectangle_t event;
		if (c->hidden) continue;
		if (!rect_intersect(&c->area, ev)) continue;

		if (rw->resized) {
			memcpy(&event, ev, sizeof(cairo_rectangle_t));
		} else {
			event.x = MAX(0, ev->x - c->area.x);
			event.y = MAX(0, ev->y - c->area.y);
			event.width  = MIN(c->area.x + c->area.width, ev->x + ev->width) - MAX(ev->x,  c->area.x);
			event.height = MIN(c->area.y + c->area.height, ev->y + ev->height) - MAX(ev->y, c->area.y);
		}

#ifdef DEBUG_EXPOSURE_UI
		printf("rce %.1f+%.1f , %.1fx%.1f  ||  cld %.1f+%.1f ,  %.1fx%.1f || ISC %.1f+%.1f , %.1fx%.1f\n",
				ev->x, ev->y, ev->width, ev->height,
				c->area.x, c->area.y,  c->area.width, c->area.height,
				event.x, event.y, event.width, event.height);
#endif
		cairo_save(cr);
		cairo_translate(cr, c->area.x, c->area.y);
		c->expose_event(c, cr, &event);
#if 0 // VISUAL LAYOUT DEBUG -- NB. expose_event may or may not alter event
		cairo_rectangle(cr, event.x, event.y, event.width, event.height);
		cairo_set_source_rgba(cr, rand()/(float)RAND_MAX, rand()/(float)RAND_MAX, rand()/(float)RAND_MAX, 0.5);
		cairo_fill(cr);
#endif
		cairo_restore(cr);
	}
	if (rw->resized) {
		rw->resized = FALSE;
	}
	return TRUE;
}

static bool rcontainer_expose_event(RobWidget* rw, cairo_t* cr, cairo_rectangle_t *ev) {
	if (rw->resized) {
		cairo_rectangle_t event;
		event.x = MAX(0, ev->x - rw->area.x);
		event.y = MAX(0, ev->y - rw->area.y);
		event.width  = MIN(rw->area.x + rw->area.width , ev->x + ev->width)   - MAX(ev->x, rw->area.x);
		event.height = MIN(rw->area.y + rw->area.height, ev->y + ev->height) - MAX(ev->y, rw->area.y);
		cairo_save(cr);
		rcontainer_clear_bg(rw, cr, &event);
		cairo_restore(cr);
	}
	return rcontainer_expose_event_no_clear(rw, cr, ev);
}



/*****************************************************************************/
/* specific containers */

/* horizontal box */
static void
rhbox_size_request(RobWidget* rw, int *w, int *h) {
	assert(w && h);
	int ww = 0;
	int hh = 0;
	bool homogeneous = ((struct rob_container*)rw->self)->homogeneous;
	int padding = ((struct rob_container*)rw->self)->padding;

	int cnt = 0;
	for (unsigned int i=0; i < rw->childcount; ++i) {
		int cw, ch;
		RobWidget * c = (RobWidget *) rw->children[i];
		if (c->hidden) continue;
		c->size_request(c, &cw, &ch);
		if (homogeneous) {
			ww = MAX(cw, ww);
		} else {
			ww += cw;
		}
		hh = MAX(ch, hh);
		c->area.width = cw;
		c->area.height = ch;
		cnt++;
#ifdef DEBUG_HBOX
		printf("HBOXCHILD %d wants %dx%d (new total: %dx%d)\n", i, cw, ch, ww, hh);
#endif
	}

	if (homogeneous) {
		for (unsigned int i=0; i < rw->childcount; ++i) {
			RobWidget * c = (RobWidget *) rw->children[i];
			if (c->hidden) continue;
			c->area.width = ww;
		}
		ww *= cnt;
#ifdef DEBUG_HBOX
		printf("HBOXCHILD set homogenious x %d (total: %dx%d)\n", cnt, ww, hh);
#endif
	}

	if (cnt > 0) {
		ww += (cnt-1) * padding;
	}

	ww = ceil(ww);
	hh = ceil(hh);
	*w = ww;
	*h = hh;
	robwidget_set_area(rw, 0, 0, ww, hh);
}

static void rhbox_size_allocate(RobWidget* rw, int w, int h) {
#ifdef DEBUG_HBOX
	printf("Hbox size_allocate %d, %d\n", w, h);
#endif
	int padding = ((struct rob_container*)rw->self)->padding;
	bool expand = ((struct rob_container*)rw->self)->expand;
	if (w < rw->area.width) {
		printf(" !!! hbox packing error alloc:%d, widget:%.1f\n", w, rw->area.width);
		w = rw->area.width;
	}
	float xtra_space = 0;
	bool grow = FALSE;

	if (w > rw->area.width) {
		// check what widgets can expand..
		int cnt = 0;
		int exp = 0;
		for (unsigned int i=0; i < rw->childcount; ++i) {
			RobWidget * c = (RobWidget *) rw->children[i];
			if (c->hidden) continue;
			++cnt;
			if (!roblayout_can_expand(c)) continue;
			if (c->size_allocate) { ++exp;}
		}
		if (exp > 0) {
			/* divide extra space.. */
			xtra_space = (w - rw->area.width /* - (cnt-1) * padding */) / (float)exp;
#ifdef DEBUG_HBOX
			printf("expand %d widgets by %.1f width\n", exp, xtra_space);
#endif
		} else if (!rw->position_set) {
			xtra_space = (w - rw->area.width) / 2.0;
			grow = TRUE;
#ifdef DEBUG_HBOX
			printf("grow self by %.1f width\n", xtra_space);
#endif
		} else {
#ifdef DEBUG_HBOX
			printf("don't grow\n");
#endif
		}
	}

	const int hh = rw->area.height;
	/* allocate kids */
	for (unsigned int i=0; i < rw->childcount; ++i) {
		RobWidget * c = (RobWidget *) rw->children[i];
		if (c->hidden) continue;
		if (c->size_allocate) {
			c->size_allocate(c, 
					c->area.width + ((grow || !roblayout_can_expand(c)) ? 0 : floorf(xtra_space)),
					roblayout_can_fill(c) ? h : hh);
		}
#ifdef DEBUG_HBOX
		printf("HBOXCHILD %d use %.1fx%.1f\n", i, c->area.width, c->area.height);
#endif
	}

	/* set position after allocation */
	float ww = grow ? xtra_space : 0;
	int ccnt = 0;
	for (unsigned int i=0; i < rw->childcount; ++i) {
		RobWidget * c = (RobWidget *) rw->children[i];
		if (c->hidden) continue;
		if (++ccnt != 1) { ww += padding; }
		if (c->position_set) {
			c->position_set(c, c->area.width, h);
		} else {
			robwidget_position_set(c, c->area.width, h);
		}
		c->area.x += floorf(ww);
		c->area.y += 0;
		if (!roblayout_can_fill(c)) {
			c->area.y += roblayout_can_expand(c) ? 0 : floor((hh - h) / 2.0);
		}
		ww += c->area.width;

#ifdef DEBUG_HBOX
		printf("HBOXCHILD %d '%s' packed to %.1f+%.1f  %.1fx%.1f\n", i,
				ROBWIDGET_NAME(c),
				c->area.x, c->area.y, c->area.width, c->area.height);
#endif
		if (c->redraw_pending) {
			queue_draw(c);
		}
	}
#ifdef DEBUG_HBOX
	if (grow) ww += xtra_space;
	if (ww != w) {
		printf("MISSED STH :) width: %.1f <> parent-w: %d\n", ww, w);
	}
#endif
	if (expand) {
		ww = w;
	} else {
		ww = rint(ww);
	}
	robwidget_set_area(rw, 0, 0, ww, h);
}

static void rob_hbox_child_pack(RobWidget *rw, RobWidget *chld, bool expand, bool fill) {
	rcontainer_child_pack(rw, chld, expand, fill);
}

static RobWidget * rob_hbox_new(bool homogeneous, int padding) {
	RobWidget * rw = robwidget_new(NULL);
	ROBWIDGET_SETNAME(rw, "hbox");
	rw->self = (struct rob_container*) calloc(1, sizeof(struct rob_container));
	((struct rob_container*)rw->self)->homogeneous = homogeneous;
	((struct rob_container*)rw->self)->padding = padding;
	((struct rob_container*)rw->self)->expand = TRUE;

	rw->size_request  = rhbox_size_request;
	rw->size_allocate = rhbox_size_allocate;

	rw->expose_event = rcontainer_expose_event;
	rw->mouseup      = rcontainer_mouseup;
	rw->mousedown    = rcontainer_mousedown;
	rw->mousemove    = rcontainer_mousemove;
	rw->mousemove    = rcontainer_mousemove;
	rw->mousescroll  = rcontainer_mousescroll;

	rw->area.x=0;
	rw->area.y=0;
	rw->area.width = 0;
	rw->area.height = 0;

	return rw;
}

/* vertical box */
static void rvbox_size_request(RobWidget* rw, int *w, int *h) {
	assert(w && h);
	int ww = 0;
	int hh = 0;
	bool homogeneous = ((struct rob_container*)rw->self)->homogeneous;
	int padding = ((struct rob_container*)rw->self)->padding;

	int cnt = 0;
	for (unsigned int i=0; i < rw->childcount; ++i) {
		int cw, ch;
		RobWidget * c = (RobWidget *) rw->children[i];
		if (c->hidden) continue;
		c->size_request(c, &cw, &ch);
		ww = MAX(cw, ww);
		if (homogeneous) {
			hh = MAX(ch, hh);
		} else {
			hh += ch;
		}
		c->area.width = cw;
		c->area.height = ch;
		cnt++;
#ifdef DEBUG_VBOX
		printf("VBOXCHILD %d ('%s') wants %dx%d (new total: %dx%d)\n", i,
				ROBWIDGET_NAME(rw->children[i]), cw, ch, ww, hh);
#endif
	}

	if (homogeneous) {
		for (unsigned int i=0; i < rw->childcount; ++i) {
			RobWidget * c = (RobWidget *) rw->children[i];
			if (c->hidden) continue;
			c->area.height = hh;
		}
		hh *= cnt;
	}

	if (cnt > 0) {
		hh += (cnt-1) * padding;
	}

	ww = ceil(ww);
	hh = ceil(hh);
	*w = ww;
	*h = hh;
	robwidget_set_area(rw, 0, 0, ww, hh);
#ifdef DEBUG_VBOX
	printf("VBOX request %d, %d (%.1f, %.1f)\n", ww, hh, rw->area.width, rw->area.height);
#endif
}

static void rvbox_size_allocate(RobWidget* rw, int w, int h) {
#ifdef DEBUG_VBOX
	printf("rvbox_size_allocate %s: %d, %d (%.1f, %.1f)\n",
			ROBWIDGET_NAME(rw), w, h, rw->area.width, rw->area.height);
#endif
	int padding = ((struct rob_container*)rw->self)->padding;
	bool expand = ((struct rob_container*)rw->self)->expand;
	if (h < rw->area.height) {
		printf(" !!! vbox packing error alloc:%d, widget:%.1f\n", h, rw->area.height);
		h = rw->area.height;
	}
	float xtra_space = 0;
	bool grow = FALSE;

	if (h > rw->area.height) {
		// check what widgets can expand..
		int cnt = 0;
		int exp = 0;
		for (unsigned int i=0; i < rw->childcount; ++i) {
			RobWidget * c = (RobWidget *) rw->children[i];
			if (c->hidden) continue;
			++cnt;
			if (!roblayout_can_expand(c)) continue;
			if (c->size_allocate) { ++exp;}
		}
		if (exp > 0) {
			/* divide extra space.. */
			xtra_space = (h - rw->area.height /*- (cnt-1) * padding*/) / (float) exp;
#ifdef DEBUG_VBOX
			printf("expand %d widgets by %.1f height (%d, %d)\n", exp, xtra_space, cnt, padding);
#endif
		} else if (!rw->position_set) {
			xtra_space = (h - rw->area.height) / 2.0;
			grow = TRUE;
#ifdef DEBUG_VBOX
			printf("grow self by %.1f height\n", xtra_space);
#endif
		} else {
#ifdef DEBUG_VBOX
			printf("don't grow\n");
#endif
		}
	}

	const int ww = rw->area.width;

	/* allocate kids */
	for (unsigned int i=0; i < rw->childcount; ++i) {
		RobWidget * c = (RobWidget *) rw->children[i];
		if (c->hidden) continue;
		if (c->size_allocate) {
			c->size_allocate(c, roblayout_can_expand(c) ? w : ww,
					c->area.height + ((grow || !roblayout_can_expand(c)) ? 0 : floorf(xtra_space)));
		}
#ifdef DEBUG_VBOX
		printf("VBOXCHILD %d ('%s') use %.1fx%.1f\n", i,
				ROBWIDGET_NAME(rw->children[i]), c->area.width, c->area.height);
#endif
	}

	/* set position after allocation */
	float hh = grow ? xtra_space : 0;
	int ccnt = 0;
	for (unsigned int i=0; i < rw->childcount; ++i) {
		RobWidget * c = (RobWidget *) rw->children[i];
		if (c->hidden) continue;
		if (++ccnt != 1) { hh += padding; }
		if (c->position_set) {
			c->position_set(c, w, c->area.height);
		} else {
			robwidget_position_set(c, w, c->area.height);
		}
		if (!roblayout_can_fill(c)) {
			c->area.x += roblayout_can_expand(c) ? 0 : floor((ww - w) / 2.0);
		}
		c->area.y += floorf(hh);
		hh += c->area.height;
#ifdef DEBUG_VBOX
		printf("VBOXCHILD %d packed to %.1f+%.1f  %.1fx%.1f\n", i, c->area.x, c->area.y, c->area.width, c->area.height);
#endif
		if (c->redraw_pending) {
			queue_draw(c);
		}
	}
#ifdef DEBUG_VBOX
	if (grow) hh += xtra_space;
	if (hh != h) {
		printf("MISSED STH :) height: %.1f <> parent-h: %d\n", hh, h);
	}
#endif
	if (expand) {
		hh = h;
	} else {
		hh = rint(hh);
	}
	robwidget_set_area(rw, 0, 0, w, hh);
}

static void rob_vbox_child_pack(RobWidget *rw, RobWidget *chld, bool expand, bool fill) {
	rcontainer_child_pack(rw, chld, expand, fill);
}

static RobWidget * rob_vbox_new(bool homogeneous, int padding) {
	RobWidget * rw = robwidget_new(NULL);
	ROBWIDGET_SETNAME(rw, "vbox");
	rw->self = (struct rob_container*) calloc(1, sizeof(struct rob_container));
	((struct rob_container*)rw->self)->homogeneous = homogeneous;
	((struct rob_container*)rw->self)->padding = padding;
	((struct rob_container*)rw->self)->expand = TRUE;

	rw->size_request  = rvbox_size_request;
	rw->size_allocate = rvbox_size_allocate;

	rw->expose_event = rcontainer_expose_event;
	rw->mouseup      = rcontainer_mouseup;
	rw->mousedown    = rcontainer_mousedown;
	rw->mousemove    = rcontainer_mousemove;
	rw->mousemove    = rcontainer_mousemove;
	rw->mousescroll  = rcontainer_mousescroll;

	rw->area.x=0;
	rw->area.y=0;
	rw->area.width = 0;
	rw->area.height = 0;

	return rw;
}

static void dump_tbl_req(struct rob_table *rt) {
	unsigned int x,y;
	printf("---REQ---\n");
	printf("COLS: | ");
	for (x=0; x < rt->ncols; ++x) {
		printf(" *%4d* x  %4d (%d,%d)|", rt->cols[x].req_w, rt->cols[x].req_h, rt->cols[x].is_expandable_x, rt->cols[x].is_expandable_y);
	}
	printf("\n---------------\n");
	for (y=0; y < rt->nrows; ++y) {
		printf("ROW %d ||  %4d  x *%4d* (%d,%d)\n", y, rt->rows[y].req_w, rt->rows[y].req_h, rt->rows[y].is_expandable_x, rt->rows[y].is_expandable_y);
	}
}

static void dump_tbl_acq(struct rob_table *rt) {
	unsigned int x,y;
	printf("---ALLOC---\n");
	printf("COLS: | ");
	for (x=0; x < rt->ncols; ++x) {
		printf(" *%4d* x  %4d  |", rt->cols[x].acq_w, rt->cols[x].acq_h);
	}
	printf("\n---------------\n");
	for (y=0; y < rt->nrows; ++y) {
		printf("ROW %d ||  %4d  x *%4d*\n", y, rt->rows[y].acq_w, rt->rows[y].acq_h);
	}
}

static void rob_box_destroy(RobWidget * rw) {
	free(rw->self);
	robwidget_destroy(rw);
}

/* table layout [jach] */

static void
rtable_size_request(RobWidget* rw, int *w, int *h) {
	assert(w && h);
	struct rob_table *rt = (struct rob_table*)rw->self;

	// reset
	for (unsigned int r=0; r < rt->nrows; ++r) {
		memset(&rt->rows[r], 0, sizeof(struct rob_table_field));
		rt->rows[r].is_expandable_x = TRUE;
		rt->rows[r].is_expandable_y = TRUE;
	}
	for (unsigned int c=0; c < rt->ncols; ++c) {
		memset(&rt->cols[c], 0, sizeof(struct rob_table_field));
		rt->cols[c].is_expandable_x = TRUE;
		rt->cols[c].is_expandable_y = TRUE;
	}

	// fill in childs
	for (unsigned int i=0; i < rt->nchilds; ++i) {
		int cw, ch;
		struct rob_table_child *tc = &rt->chld[i];
		RobWidget * c = (RobWidget *) tc->rw;
		if (c->hidden) continue;
		c->size_request(c, &cw, &ch);

#ifdef DEBUG_TABLE
		printf("widget %d wants (%d x %d) x-span:%d y-span: %d\n", i, cw, ch, (tc->right - tc->left), (tc->bottom - tc->top));
#endif
		int curw = 0, curh = 0;
		for (int span_x = tc->left; span_x < tc->right; ++span_x) {
			curw += rt->cols[span_x].req_w;
		}
		for (int span_y = tc->top; span_y < tc->bottom; ++span_y) {
			curh += rt->rows[span_y].req_h;
		}

		float avg_w = MAX(0, tc->xpadding * 2 + cw - curw) / (float)(tc->right - tc->left);
		float avg_h = MAX(0, tc->ypadding * 2 + ch - curh) / (float)(tc->bottom - tc->top);

		for (int span_x = tc->left; span_x < tc->right; ++span_x) {
			int tcw = rint (avg_w * (1 + span_x - tc->left)) - rint (avg_w * (span_x - tc->left));
			rt->cols[span_x].req_w += tcw;
			rt->cols[span_x].req_h = MAX(rt->cols[span_x].req_h, ch); // unused -- homog
			if (!(tc->expand_x & RTK_EXPAND)) {
				rt->cols[span_x].is_expandable_x = FALSE;
			}
		}
		for (int span_y = tc->top; span_y < tc->bottom; ++span_y) {
			int tch = rint (avg_h * (1 + span_y -  tc->top)) - rint (avg_h * (span_y -  tc->top));
			rt->rows[span_y].req_w = MAX(rt->rows[span_y].req_w, cw); // unused -- homog
			rt->rows[span_y].req_h += tch;
			if (!(tc->expand_y & RTK_EXPAND)) {
				rt->rows[span_y].is_expandable_y = FALSE;
			}
		}
		// reset initial size
		c->area.width = cw;
		c->area.height = ch;
	}
	// calc size of table
	int ww = 0;
	int hh = 0;
	for (unsigned int r=0; r < rt->nrows; ++r) {
		hh += rt->rows[r].req_h;
	}
	for (unsigned int c=0; c < rt->ncols; ++c) {
		ww += rt->cols[c].req_w;
	}

#if 0 // homogeneous
	// set area of children to detected max
	for (unsigned int i=0; i < rt->nchilds; ++i) {
		int cw = 0;
		int ch = 0;
		struct rob_table_child *tc = &rt->chld[i];
		RobWidget * c = (RobWidget *) tc->rw;
		if (c->hidden) continue;

		for (int span_x = tc->left; span_x < tc->right; ++span_x) {
			cw += rt->cols[span_x].req_w;
			//ch += rt->cols[span_x].req_h;
		}
		for (int span_y = tc->top; span_y < tc->bottom; ++span_y) {
			//cw += rt->rows[span_y].req_w;
			ch += rt->rows[span_y].req_h;
		}
		c->area.width = cw;
		c->area.height = ch;
	}
#endif
#ifdef DEBUG_TABLE
	dump_tbl_req(rt);
#endif

	ww = ceil(ww);
	hh = ceil(hh);
	*w = ww;
	*h = hh;
#ifdef DEBUG_TABLE
	printf("REQUEST TABLE SIZE: %d %d\n", ww, hh);
#endif
	robwidget_set_area(rw, 0, 0, ww, hh);
}

static void rtable_size_allocate(RobWidget* rw, const int w, const int h) {
	struct rob_table *rt = (struct rob_table*)rw->self;
#ifdef DEBUG_TABLE
	printf("table '%s' size_allocate %d, %d\n", ROBWIDGET_NAME(rw), w, h);
#endif
	if (h < rw->area.height || w < rw->area.width) {
		printf(" !!! table size request error. want %.1fx%.1f got %dx%d\n", rw->area.width, rw->area.height, w, h);
	}

	if (h > rw->area.height) {
		int exp = 0;
#ifdef DEBUG_TABLE
		printf("---TABLE CAN EXPAND in height to %d\n", h);
#endif
		for (unsigned int r=0; r < rt->nrows; ++r) {
			if (rt->rows[r].req_h == 0) continue;
			if (rt->rows[r].is_expandable_y) { ++exp; }
		}
		if (exp > 0) {
			int cnt = 0;
			float xtra_height = (h - rw->area.height) / (float)exp;
#ifdef DEBUG_TABLE
			printf("table expand %d rows by %.1f height\n", exp, xtra_height);
#endif
			for (unsigned int r=0; r < rt->nrows; ++r) {
				if (rt->rows[r].req_h == 0) continue;
				if (!rt->rows[r].is_expandable_y) continue;
				rt->rows[r].expand = rint (xtra_height * (1 + cnt)) - rint (xtra_height * (cnt));
				++cnt;
			}
		} else {
#ifdef DEBUG_TABLE
			printf("table no grow height\n");
#endif
		}
	}

	if (w > rw->area.width) {
		int exp = 0;
#ifdef DEBUG_TABLE
		printf("TABLE CAN EXPAND in width to %d\n", w);
#endif
		for (unsigned int c=0; c < rt->ncols; ++c) {
			if (rt->cols[c].req_w == 0) continue;
			if (rt->cols[c].is_expandable_x) { ++exp; }
		}
		if (exp > 0) {
			int cnt = 0;
			float xtra_width = (w - rw->area.width) / (float)exp;
#ifdef DEBUG_TABLE
			printf("table expand %d columns by %.1f width\n", exp, xtra_width);
#endif

			for (unsigned int c=0; c < rt->ncols; ++c) {
				if (rt->cols[c].req_w == 0) continue;
				if (!rt->cols[c].is_expandable_x) continue;
				rt->cols[c].expand = rint (xtra_width * (1 + cnt)) - rint (xtra_width * (cnt));
				++cnt;
			}

		} else {
#ifdef DEBUG_TABLE
			printf("table no grow width\n");
#endif
		}
	}


	for (unsigned int c=0; c < rt->ncols; ++c) {
		rt->cols[c].acq_w = rt->cols[c].req_w + rt->cols[c].expand;
	}

	for (unsigned int r=0; r < rt->nrows; ++r) {
		rt->rows[r].acq_h = rt->rows[r].req_h + rt->rows[r].expand;
	}

	for (unsigned int i=0; i < rt->nchilds; ++i) {
		int cw = 0;
		int ch = 0;
		struct rob_table_child *tc = &rt->chld[i];
		RobWidget * c = (RobWidget *) tc->rw;
		if (c->hidden) continue;
		c->size_request(c, &cw, &ch);

#ifdef DEBUG_TABLE
		printf("widget %d wants (%d x %d) x-span:%d y-span: %d  %d, %d\n", i, cw, ch, (tc->right - tc->left), (tc->bottom - tc->top), tc->left, tc->top);
#endif

		int curw = 0, curh = 0;
		for (int span_x = tc->left; span_x < tc->right; ++span_x) {
			curw += rt->cols[span_x].acq_w;
		}
		for (int span_y = tc->top; span_y < tc->bottom; ++span_y) {
			curh += rt->rows[span_y].acq_h;
		}

		if (c->size_allocate) {
			int aw = curw - tc->xpadding * 2;
			int ah = curh - tc->ypadding * 2;
			if (tc->expand_x & RTK_FILL) cw = MAX(cw, aw);
			if (tc->expand_y & RTK_FILL) ch = MAX(ch, ah);
			c->size_allocate(c, cw, ch);
			cw = c->area.width;
			ch = c->area.height;
		} else {
			// shift the position of the item..
			// XXX use acq rather than add expand?
			for (int tci = tc->left; tci < tc->right; ++tci) {
				cw += rt->cols[tci].expand;
			}
			for (int tri = tc->top; tri < tc->bottom; ++tri) {
				ch += rt->rows[tri].expand;
			}
		}

#if 1 // verify layout
		if (cw + tc->xpadding * 2 > curw) {
			printf("TABLE child %d WIDTH %d > %d\n", i, cw, curw);
		}
		if (ch +tc->ypadding * 2 > curh) {
			printf("TABLE child %d HEIGHT %d > %d \n", i, ch, curh);
		}
#endif

#ifdef DEBUG_TABLE
		dump_tbl_acq(rt);
		printf("TABLECHILD %d '%s' use %.1fx%.1f (field: %dx%d)\n", i, ROBWIDGET_NAME(c), c->area.width, c->area.height, curw, curh);
#endif
	}

#ifdef DEBUG_TABLE
	dump_tbl_acq(rt);
#endif
	int max_w = 0;
	int max_h = 0;
	/* set position after allocation */
	for (unsigned int i=0; i < rt->nchilds; ++i) {
		int cw = 0;
		int ch = 0;
		int cx = 0;
		int cy = 0;
		struct rob_table_child *tc = &rt->chld[i];
		RobWidget * c = (RobWidget *) tc->rw;
		if (c->hidden) continue;
		for (int span_x = tc->left; span_x < tc->right; ++span_x) {
			cw += rt->cols[span_x].acq_w;
		}
		for (int span_y = tc->top; span_y < tc->bottom; ++span_y) {
			ch += rt->rows[span_y].acq_h;
		}
		for (int span_x = 0; span_x < tc->left; ++span_x) {
			cx += rt->cols[span_x].acq_w;
		}
		for (int span_y = 0; span_y < tc->top; ++span_y) {
			cy += rt->rows[span_y].acq_h;
		}
#ifdef DEBUG_TABLE
		printf("TABLECHILD %d avail %dx%d at %d+%d (wsize: %.1fx%.1f)\n", i, cw, ch, cx, cy, c->area.width, c->area.height);
#endif

		if (tc->xpadding > 0) {
			if (cw < c->area.width + 2 * tc->xpadding) {
				printf("!!!! Table Padding:%d + cell %.0f < widget-width %d\n", tc->xpadding, c->area.width, cw);
			}
		}
		if (tc->ypadding > 0) {
			if (ch < c->area.height + 2 * tc->ypadding) {
				printf("!!!! Table Padding:%d + cell %.0f < widget-height %d\n", tc->ypadding, c->area.height, ch);
			}
		}

		cw -= tc->xpadding * 2;
		ch -= tc->ypadding * 2;

		if (c->position_set) {
			c->position_set(c, cw, ch);
		} else {
			robwidget_position_set(c, cw, ch);
		}

		c->area.x += cx + tc->xpadding;
		c->area.y += cy + tc->ypadding;

		if (c->area.x + c->area.width + tc->xpadding > max_w) max_w = c->area.x + c->area.width + tc->xpadding;
		if (c->area.y + c->area.height + tc->ypadding > max_h) max_h = c->area.y + c->area.height + tc->ypadding;
#ifdef DEBUG_TABLE
		printf("TABLE %d packed to %.1f+%.1f  %.1fx%.1f\n", i, c->area.x, c->area.y, c->area.width, c->area.height);
#endif
		if (c->redraw_pending) {
			queue_draw(c);
		}
	}
#ifdef DEBUG_TABLE
	printf("TABLE PACKED %.1f+%.1f  %dx%d  (given: %dx%d)\n", (w - max_w) / 2.0, (h - max_h) / 2.0, max_w, max_h, w, h);
#endif
	if (max_w > w || max_h > h) {
		printf("TABLE OVERFLOW total %dx%d  (given: %dx%d)\n", max_w, max_h, w, h);
	} else if (max_w < w || max_h < h) {
		// re-center content in case table did not fully expand
		const int xoff = floor((w - max_w) / 2.0);
		const int yoff = floor((h - max_h) / 2.0);
#ifdef DEBUG_TABLE
		printf("RE-CENTER CHILDS by %dx%d %s\n", xoff, yoff, ROBWIDGET_NAME(rw));
#endif
		for (unsigned int i=0; i < rt->nchilds; ++i) {
			struct rob_table_child *tc = &rt->chld[i];
			RobWidget * c = (RobWidget *) tc->rw;
			if (c->hidden) continue;
			c->area.x += xoff;
			c->area.y += yoff;
		}
	}
	robwidget_set_area(rw, 0, 0, w, h);
}

static void rob_table_attach(RobWidget *rw, RobWidget *chld,
		unsigned int left, unsigned int right, unsigned int top, unsigned int bottom,
		int xpadding, int ypadding,
		int xexpand, int yexpand
		) {
	assert(left < right);
	assert(top < bottom);

	rcontainer_child_pack(rw, chld, (yexpand | xexpand) & RTK_FILL, /*unused*/ TRUE);
	struct rob_table *rt = (struct rob_table*)rw->self;

	if (right >= rt->ncols) {
		rob_table_resize (rt, rt->nrows, right);
	}
	if (bottom >= rt->nrows) {
		rob_table_resize (rt, bottom, rt->ncols);
	}

	rt->chld = (struct rob_table_child*) realloc(rt->chld, (rt->nchilds + 1) * sizeof(struct rob_table_child));
	rt->chld[rt->nchilds].rw       = chld;
	rt->chld[rt->nchilds].left     = left;
	rt->chld[rt->nchilds].right    = right;
	rt->chld[rt->nchilds].top      = top;
	rt->chld[rt->nchilds].bottom   = bottom;
	rt->chld[rt->nchilds].xpadding = xpadding;
	rt->chld[rt->nchilds].ypadding = ypadding;
	rt->chld[rt->nchilds].expand_x  = xexpand;
	rt->chld[rt->nchilds].expand_y  = yexpand;
	rt->nchilds++;
}

static void rob_table_attach_defaults(RobWidget *rw, RobWidget *chld,
		unsigned int left, unsigned int right, unsigned int top, unsigned int bottom) {
	rob_table_attach(rw, chld, left, right, top, bottom, 0, 0, RTK_EXANDF, RTK_EXANDF);
}

static RobWidget * rob_table_new(int rows, int cols, bool homogeneous) {
	RobWidget * rw = robwidget_new(NULL);
	ROBWIDGET_SETNAME(rw, "tbl");
	rw->self = (struct rob_table*) calloc(1, sizeof(struct rob_table));
	struct rob_table *rt = (struct rob_table*)rw->self;
	rt->homogeneous = homogeneous;
	rt->expand = TRUE;
	rob_table_resize (rt, rows, cols);

	rw->size_request  = rtable_size_request;
	rw->size_allocate = rtable_size_allocate;

	rw->expose_event = rcontainer_expose_event;
	rw->mouseup      = rcontainer_mouseup;
	rw->mousedown    = rcontainer_mousedown;
	rw->mousemove    = rcontainer_mousemove;
	rw->mousemove    = rcontainer_mousemove;
	rw->mousescroll  = rcontainer_mousescroll;

	rw->area.x=0;
	rw->area.y=0;
	rw->area.width = 0;
	rw->area.height = 0;

	return rw;
}

static void rob_table_destroy(RobWidget * rw) {
	struct rob_table *rt = (struct rob_table*)rw->self;
	free(rt->chld);
	free(rt->rows);
	free(rt->cols);
	free(rw->self);
	robwidget_destroy(rw);
}

/* recursive childpos cache */
static void
rtoplevel_cache(RobWidget* rw, bool valid) {
	for (unsigned int i=0; i < rw->childcount; ++i) {
		RobWidget * c = (RobWidget *) rw->children[i];
		if (c->hidden) {
			valid= FALSE;
		}
		rtoplevel_cache(c, valid);
	}
	robwidget_position_cache(rw);
	rw->cached_position = valid;
}

/* recursive ui-scale update */
static void
rtoplevel_scale(RobWidget* rw, const float ws) {
	for (unsigned int i=0; i < rw->childcount; ++i) {
		RobWidget * c = (RobWidget *) rw->children[i];
		rtoplevel_scale (c, ws);
	}
	rw->widget_scale = ws;
}
