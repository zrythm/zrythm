/* dial with numeric display / spinbox
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

#ifndef _ROB_TK_SPIN_H_
#define _ROB_TK_SPIN_H_

#include "robtk_dial.h"
#include "robtk_label.h"

#define GSP_WIDTH 25
#define GSP_HEIGHT 30
#define GSP_RADIUS 10
#define GSP_CX 12.5
#define GSP_CY 12.5

typedef struct {
	RobTkDial *dial;
	RobWidget* rw;
	RobTkLbl* lbl_r;
	RobTkLbl* lbl_l;

	bool sensitive;
	char prec_fmt[8];

	bool (*cb) (RobWidget* w, void* handle);
	void* handle;
	int lbl;
	pthread_mutex_t _mutex;
} RobTkSpin;

static bool robtk_spin_render(RobTkSpin *d){
	pthread_mutex_lock (&d->_mutex);
	char buf[32];
#ifdef _WIN32
	sprintf(buf, d->prec_fmt, robtk_dial_get_value(d->dial));
#else
	snprintf(buf, 32, d->prec_fmt, robtk_dial_get_value(d->dial));
#endif
	buf[31] = '\0';
	if (d->lbl & 1) robtk_lbl_set_text(d->lbl_l, buf);
	if (d->lbl & 2) robtk_lbl_set_text(d->lbl_r, buf);
	pthread_mutex_unlock (&d->_mutex);
	return TRUE;
}

/******************************************************************************
 * child callbacks
 */

static bool robtk_spin_callback(RobWidget *w, void* handle) {
	RobTkSpin *d = (RobTkSpin *) handle;
	robtk_spin_render(d);
	if (d->cb) d->cb(robtk_dial_widget(d->dial), d->handle);
	return TRUE;
}

static void robtk_spin_position_set (RobWidget *rw, const int pw, const int ph) {
	//override default extra-space table packing
	rw->area.x = rint((pw - rw->area.width) * rw->xalign);
	rw->area.y = rint((ph - rw->area.height) * rw->yalign);
}

/******************************************************************************
 * public functions
 */

static void robtk_spin_set_digits(RobTkSpin *d, int prec) {
	if (prec > 4) prec = 4;
	if (prec <= 0) {
		sprintf(d->prec_fmt,"%%.0f");
	} else {
		sprintf(d->prec_fmt,"%%.%df", prec);
	}
	robtk_spin_render(d);
}

static RobTkSpin * robtk_spin_new(float min, float max, float step) {
	RobTkSpin *d = (RobTkSpin *) malloc(sizeof(RobTkSpin));

	d->sensitive = TRUE;
	d->cb = NULL;
	d->handle = NULL;
	d->lbl = 2;
	pthread_mutex_init (&d->_mutex, 0);

	d->dial = robtk_dial_new_with_size(min, max, step,
			GSP_WIDTH, GSP_HEIGHT, GSP_CX, GSP_CY, GSP_RADIUS);

	robtk_dial_set_callback(d->dial, robtk_spin_callback, d);

	d->lbl_r = robtk_lbl_new("");
	d->lbl_l = robtk_lbl_new("");

	d->rw = rob_hbox_new(FALSE, 2);
	rob_hbox_child_pack(d->rw, robtk_lbl_widget(d->lbl_l), FALSE, FALSE);
	rob_hbox_child_pack(d->rw, robtk_dial_widget(d->dial), FALSE, FALSE);
	rob_hbox_child_pack(d->rw, robtk_lbl_widget(d->lbl_r), FALSE, FALSE);

	d->rw->position_set = robtk_spin_position_set;

	robtk_spin_set_digits(d, - floorf(log10f(step)));
	robtk_spin_callback(0, d); // set value
	return d;
}

static void robtk_spin_destroy(RobTkSpin *d) {
	robtk_dial_destroy(d->dial);
	robtk_lbl_destroy(d->lbl_r);
	robtk_lbl_destroy(d->lbl_l);
	rob_box_destroy(d->rw);
	pthread_mutex_destroy(&d->_mutex);

	free(d);
}

static void robtk_spin_set_alignment(RobTkSpin *d, float x, float y) {
#ifndef GTK_BACKEND
	robwidget_set_alignment(d->rw, x, y);
#else
	if (x > .5) {
		gtk_box_set_child_packing(GTK_BOX(d->rw->c), d->lbl_l->rw->c, TRUE, FALSE, 0, GTK_PACK_START);
	} else {
		gtk_box_set_child_packing(GTK_BOX(d->rw->c), d->lbl_l->rw->c, FALSE, FALSE, 0, GTK_PACK_START);
	}
#endif
}

static void robtk_spin_label_width(RobTkSpin *d, float left, float right) {
	if (left < 0) {
		robwidget_hide(robtk_lbl_widget(d->lbl_l), false);
	} else {
		robtk_lbl_set_min_geometry(d->lbl_l, (float) left, 0);
		robwidget_show(robtk_lbl_widget(d->lbl_l), false);
	}
	if (right < 0) {
		robwidget_hide(robtk_lbl_widget(d->lbl_r), false);
	} else {
		robtk_lbl_set_min_geometry(d->lbl_r, (float) right, 0);
		robwidget_show(robtk_lbl_widget(d->lbl_r), false);
	}
	robtk_spin_render(d);
}

static void robtk_spin_set_label_pos(RobTkSpin *d, int p) {
	d->lbl = p&3;
	if (!(d->lbl & 1)) robtk_lbl_set_text(d->lbl_l, "");
	if (!(d->lbl & 2)) robtk_lbl_set_text(d->lbl_r, "");
	robtk_spin_render(d);
}

static RobWidget * robtk_spin_widget(RobTkSpin *d) {
	return d->rw;
}

static void robtk_spin_set_callback(RobTkSpin *d, bool (*cb) (RobWidget* w, void* handle), void* handle) {
	d->cb = cb;
	d->handle = handle;
}

static void robtk_spin_set_default(RobTkSpin *d, float v) {
	robtk_dial_set_default(d->dial, v);
}

static void robtk_spin_set_value(RobTkSpin *d, float v) {
	robtk_dial_set_value(d->dial, v);
}

static void robtk_spin_set_sensitive(RobTkSpin *d, bool s) {
	if (d->sensitive != s) {
		d->sensitive = s;
		robtk_lbl_set_sensitive(d->lbl_r, s);
		robtk_lbl_set_sensitive(d->lbl_l, s);
	}
	robtk_dial_set_sensitive(d->dial, s);
}

static float robtk_spin_get_value(RobTkSpin *d) {
	return robtk_dial_get_value(d->dial);
}

static bool robtk_spin_update_range (RobTkSpin *d, float min, float max, float step) {
	return robtk_dial_update_range(d->dial, min, max, step);
}

#endif
