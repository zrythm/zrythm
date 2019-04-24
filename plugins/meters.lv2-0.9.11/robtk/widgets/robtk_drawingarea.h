/* Image widget
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

#ifndef _ROB_TK_DAREA_H_
#define _ROB_TK_DAREA_H_

typedef struct {
	RobWidget *rw;
	float w_width, w_height;
	void (*expose) (cairo_t* cr, void *d);
	void* handle;

} RobTkDarea;

static bool robtk_darea_expose_event(RobWidget* handle, cairo_t* cr, cairo_rectangle_t* ev) {
	RobTkDarea* d = (RobTkDarea *)GET_HANDLE(handle);
	cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
	cairo_clip (cr);
	if (d->expose) d->expose(cr, d->handle);
	return TRUE;
}

/******************************************************************************
 * RobWidget stuff
 */

static void
priv_darea_size_request(RobWidget* handle, int *w, int *h) {
	RobTkDarea* d = (RobTkDarea*)GET_HANDLE(handle);
	*w = d->w_width;
	*h = d->w_height;
}


/******************************************************************************
 * public functions
 */

static RobTkDarea * robtk_darea_new(const unsigned int w, const unsigned int h, void (*expose) (cairo_t* cr, void *d), void *handle) {
	RobTkDarea *d = (RobTkDarea *) malloc(sizeof(RobTkDarea));
	d->w_width = w;
	d->w_height = h;

	d->rw = robwidget_new(d);
	ROBWIDGET_SETNAME(d->rw, "DArea");
	robwidget_set_expose_event(d->rw, robtk_darea_expose_event);
	robwidget_set_size_request(d->rw, priv_darea_size_request);

	d->expose = expose;
	d->handle = handle;

	return d;
}

static void robtk_darea_destroy(RobTkDarea *d) {
	robwidget_destroy(d->rw);
	free(d);
}

static void robtk_darea_set_alignment(RobTkDarea *d, float x, float y) {
	robwidget_set_alignment(d->rw, x, y);
}

static RobWidget * robtk_darea_widget(RobTkDarea *d) {
	return d->rw;
}

static void robtk_darea_redraw(RobTkDarea *d) {
	queue_draw(d->rw);
}
#endif
