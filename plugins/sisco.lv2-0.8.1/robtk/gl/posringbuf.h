/* robtk LV2 GUI
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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <string.h>
#include <stdlib.h>

/* simple lockless ringbuffer */

typedef struct {
	uint8_t *d;
	size_t rp;
	size_t wp;
	size_t len;
} posringbuf;

static posringbuf * posrb_alloc(size_t siz) {
	posringbuf *rb  = (posringbuf*) malloc(sizeof(posringbuf));
	rb->d = (uint8_t*) malloc(siz * sizeof(uint8_t));
	rb->len = siz;
	rb->rp = 0;
	rb->wp = 0;
	return rb;
}

static void posrb_free(posringbuf *rb) {
	free(rb->d);
	free(rb);
}

static size_t posrb_write_space(posringbuf *rb) {
	if (rb->rp == rb->wp) return (rb->len -1);
	return ((rb->len + rb->rp - rb->wp) % rb->len) -1;
}

static size_t posrb_read_space(posringbuf *rb) {
	return ((rb->len + rb->wp - rb->rp) % rb->len);
}

static int posrb_read(posringbuf *rb, uint8_t *d, size_t len) {
	if (posrb_read_space(rb) < len) return -1;
	if (rb->rp + len <= rb->len) {
		memcpy((void*) d, (void*) &rb->d[rb->rp], len * sizeof (uint8_t));
	} else {
		int part = rb->len - rb->rp;
		int remn = len - part;
		memcpy((void*) d, (void*) &(rb->d[rb->rp]), part * sizeof (uint8_t));
		memcpy((void*) &(d[part]), (void*) rb->d, remn * sizeof (uint8_t));
	}
	rb->rp = (rb->rp + len) % rb->len;
	return 0;
}


static int posrb_write(posringbuf *rb, uint8_t *d, size_t len) {
	if (posrb_write_space(rb) < len) return -1;
	if (rb->wp + len <= rb->len) {
		memcpy((void*) &rb->d[rb->wp], (void*) d, len * sizeof(uint8_t));
	} else {
		int part = rb->len - rb->wp;
		int remn = len - part;
		memcpy((void*) &rb->d[rb->wp], (void*) d, part * sizeof(uint8_t));
		memcpy((void*) rb->d, (void*) &d[part], remn * sizeof(uint8_t));
	}
	rb->wp = (rb->wp + len) % rb->len;
	return 0;
}

static void posrb_read_clear(posringbuf *rb) {
	rb->rp = rb->wp;
}
