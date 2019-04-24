/* radio button widget
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

#ifndef _ROB_TK_RBTN_H_
#define _ROB_TK_RBTN_H_

#include "robtk_checkbutton.h"

typedef struct {
	RobTkCBtn *cbtn;
	void * cbtngrp;
	bool own_radiogrp;

	bool (*cb) (RobWidget* w, void* handle);
	void* handle;
} RobTkRBtn;

typedef struct {
	RobTkRBtn **btn;
	unsigned int cnt;
	pthread_mutex_t _mutex;
} RobTkRadioGrp;


static void btn_group_add_btn (RobTkRadioGrp *g, RobTkRBtn *btn) {
	pthread_mutex_lock (&g->_mutex);
	g->btn = (RobTkRBtn **) realloc(g->btn, (g->cnt + 1) * sizeof(RobTkRBtn*));
	g->btn[g->cnt] = btn;
	g->cnt++;
	pthread_mutex_unlock (&g->_mutex);
}

static void btn_group_remove_btn (RobTkRadioGrp *g, RobTkRBtn *btn) {
	pthread_mutex_lock (&g->_mutex);
	bool found = FALSE;
	for (unsigned int i = 0; i < g->cnt; ++i) {
		if (g->btn[i] == btn) {
			found = TRUE;
		}
		if (found && i+1 < g->cnt) {
			g->btn[i] = g->btn[i+1];
		}
	}
	g->cnt++;
	pthread_mutex_unlock (&g->_mutex);
}

static RobTkRadioGrp * btn_group_new() {
	RobTkRadioGrp *g = (RobTkRadioGrp*) malloc(sizeof(RobTkRadioGrp));
	g->btn=NULL;
	g->cnt=0;
	pthread_mutex_init (&g->_mutex, 0);
	return g;
}

static void btn_group_destroy(RobTkRadioGrp *g) {
	pthread_mutex_destroy(&g->_mutex);
	free(g->btn);
	free(g);
}


static void btn_group_propagate_change (RobTkRadioGrp *g, RobTkRBtn *btn) {
	pthread_mutex_lock (&g->_mutex);
	for (unsigned int i = 0; i < g->cnt; ++i) {
		if (g->btn[i] == btn) continue;
		robtk_cbtn_set_active(g->btn[i]->cbtn, FALSE);
	}
	pthread_mutex_unlock (&g->_mutex);
}

static bool btn_group_cbtn_callback(RobWidget *w, void* handle) {
	RobTkRBtn *d = (RobTkRBtn *) handle;
	if (robtk_cbtn_get_active(d->cbtn)) {
		btn_group_propagate_change((RobTkRadioGrp*) d->cbtngrp, d);
	}
	if (d->cb) d->cb(robtk_cbtn_widget(d->cbtn), d->handle);
	return TRUE;
}

/******************************************************************************
 * public functions
 */

static RobTkRBtn * robtk_rbtn_new(const char * txt, RobTkRadioGrp *group) {
	RobTkRBtn *d = (RobTkRBtn *) malloc(sizeof(RobTkRBtn));
	d->cbtn = robtk_cbtn_new(txt, GBT_LED_RADIO, TRUE);
	d->cb = NULL;
	d->handle = NULL;

	if (!group) {
		d->own_radiogrp = TRUE;
		d->cbtngrp = (void*) btn_group_new();
	} else {
		d->own_radiogrp = FALSE;
		d->cbtngrp = group;
	}
	btn_group_add_btn((RobTkRadioGrp*) d->cbtngrp, d);
	robtk_cbtn_set_callback(d->cbtn, btn_group_cbtn_callback, d);
	return d;
}

static void robtk_rbtn_destroy(RobTkRBtn *d) {
	if (d->own_radiogrp) {
		btn_group_destroy((RobTkRadioGrp*) d->cbtngrp);
	}
	robtk_cbtn_destroy(d->cbtn);
	free(d);
}

static void robtk_rbtn_set_alignment(RobTkRBtn *d, float x, float y) {
	robtk_cbtn_set_alignment(d->cbtn, x, y);
}

static RobWidget * robtk_rbtn_widget(RobTkRBtn *d) {
	return robtk_cbtn_widget(d->cbtn);
}

static RobTkRadioGrp * robtk_rbtn_group(RobTkRBtn *d) {
	assert(d->cbtngrp);
	return (RobTkRadioGrp*) d->cbtngrp;
}

static void robtk_rbtn_set_callback(RobTkRBtn *d, bool (*cb) (RobWidget* w, void* handle), void* handle) {
	d->cb = cb;
	d->handle = handle;
}

static void robtk_rbtn_set_active(RobTkRBtn *d, bool v) {
	robtk_cbtn_set_active(d->cbtn, v);
}

static void robtk_rbtn_set_sensitive(RobTkRBtn *d, bool s) {
	robtk_cbtn_set_sensitive(d->cbtn, s);
}

static bool robtk_rbtn_get_active(RobTkRBtn *d) {
	return robtk_cbtn_get_active(d->cbtn);
}
#endif
