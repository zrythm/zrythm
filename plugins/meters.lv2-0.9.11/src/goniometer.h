/* goniometer LV2 GUI
 *
 * Copyright 2013 Robin Gareus <robin@gareus.org>
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <string.h>
#include <stdlib.h>
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"

/* simple lockless ringbuffer
 * for goniometer stereo signal
 */

typedef struct {
	float *c0;
	float *c1;

	size_t rp;
	size_t wp;
	size_t len;
} gmringbuf;

static gmringbuf * gmrb_alloc(size_t siz) {
	gmringbuf *rb  = (gmringbuf*) malloc(sizeof(gmringbuf));
	rb->c0 = (float*) malloc(siz * sizeof(float));
	rb->c1 = (float*) malloc(siz * sizeof(float));
	rb->len = siz;
	rb->rp = 0;
	rb->wp = 0;
	return rb;
}

static void gmrb_free(gmringbuf *rb) {
	free(rb->c0);
	free(rb->c1);
	free(rb);
}

static size_t gmrb_write_space(gmringbuf *rb) {
	if (rb->rp == rb->wp) return (rb->len -1);
	return ((rb->len + rb->rp - rb->wp) % rb->len) -1;
}

static size_t gmrb_read_space(gmringbuf *rb) {
	return ((rb->len + rb->wp - rb->rp) % rb->len);
}

static int gmrb_read_one(gmringbuf *rb, float *c0, float *c1) {
	if (gmrb_read_space(rb) < 1) return -1;
	*c0 = rb->c0[rb->rp];
	*c1 = rb->c1[rb->rp];
	rb->rp = (rb->rp + 1) % rb->len;
	return 0;
}

static int gmrb_read(gmringbuf *rb, float *c0, float *c1, size_t len) {
	if (gmrb_read_space(rb) < len) return -1;
	if (rb->rp + len <= rb->len) {
		memcpy((void*) c0, (void*) &rb->c0[rb->rp], len * sizeof (float));
		memcpy((void*) c1, (void*) &rb->c1[rb->rp], len * sizeof (float));
	} else {
		int part = rb->len - rb->rp;
		int remn = len - part;
		memcpy((void*) c0, (void*) &rb->c0[rb->rp], part * sizeof (float));
		memcpy((void*) c1, (void*) &rb->c1[rb->rp], part * sizeof (float));
		memcpy((void*) &c0[part], (void*) rb->c0, remn * sizeof (float));
		memcpy((void*) &c1[part], (void*) rb->c1, remn * sizeof (float));
	}
	rb->rp = (rb->rp + len) % rb->len;
	return 0;
}


static int gmrb_write(gmringbuf *rb, float *c0, float *c1, size_t len) {
	if (gmrb_write_space(rb) < len) return -1;
	if (rb->wp + len <= rb->len) {
		memcpy((void*) &rb->c0[rb->wp], (void*) c0, len * sizeof(float));
		memcpy((void*) &rb->c1[rb->wp], (void*) c1, len * sizeof(float));
	} else {
		int part = rb->len - rb->wp;
		int remn = len - part;

		memcpy((void*) &rb->c0[rb->wp], (void*) c0, part * sizeof(float));
		memcpy((void*) &rb->c1[rb->wp], (void*) c1, part * sizeof(float));

		memcpy((void*) rb->c0, (void*) &c0[part], remn * sizeof(float));
		memcpy((void*) rb->c1, (void*) &c1[part], remn * sizeof(float));
	}
	rb->wp = (rb->wp + len) % rb->len;
	return 0;
}

static void gmrb_read_clear(gmringbuf *rb) {
	rb->rp = rb->wp;
}


/* goniometer shared instance struct */

typedef struct {
	/* shared with ui */
	gmringbuf *rb;
	bool ui_active;
	bool rb_overrun;

	/* ui state/settings */
	volatile bool  s_autogain;
	volatile bool  s_oversample;
	volatile bool  s_line;
	volatile bool  s_persist;
	volatile bool  s_preferences;
	volatile int   s_sfact;
	volatile float s_linewidth;
	volatile float s_pointwidth;
	volatile float s_persistency;
	volatile float s_max_freq;
	volatile float s_compress;
	volatile float s_gattack;
	volatile float s_gdecay;
	volatile float s_gtarget;
	volatile float s_grms;

	/* private */
	float* input[2];
	float* output[2];

	float* gain;
	float* notify;
	float* correlation;

	double rate;

	uint32_t ntfy;
	uint32_t apv;
	uint32_t sample_cnt;

	Stcorrdsp *cor;

	/* explicit thread/redraw sync */
	pthread_mutex_t *msg_thread_lock;
	pthread_cond_t *data_ready;
	void (* queue_display) (void *ptr);
	void *ui;

	/* URI */
	LV2_URID_Map* map;

	LV2_URID atom_Vector;
	LV2_URID atom_Int;
	LV2_URID atom_Float;
	LV2_URID gon_State_F;
	LV2_URID gon_State_I;

} LV2gm;
