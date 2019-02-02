/**
   Copyright (C) 2011-2013 Robin Gareus <robin@gareus.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser Public License as published by
   the Free Software Foundation; either version 2.1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "audio_decoder/ad_plugin.h"

int ad_debug_level = 0;

#define UNUSED(x) (void)(x)

int     ad_eval_null(const char *f) { UNUSED(f); return -1; }
void *  ad_open_null(const char *f, struct adinfo *n) { UNUSED(f); UNUSED(n); return NULL; }
int     ad_close_null(void *x) { UNUSED(x); return -1; }
int     ad_info_null(void *x, struct adinfo *n) { UNUSED(x); UNUSED(n); return -1; }
int64_t ad_seek_null(void *x, int64_t p) { UNUSED(x); UNUSED(p); return -1; }
ssize_t ad_read_null(void *x, float*d, size_t s) { UNUSED(x); UNUSED(d); UNUSED(s); return -1;}

typedef struct {
	ad_plugin const *b; ///< decoder back-end
	void *d; ///< backend data
} adecoder;

/* samplecat api */

void ad_init() { /* global init */ }

static ad_plugin const * choose_backend(const char *fn) {
	int max, val;
	ad_plugin const *b=NULL;
	max=0;

	val=adp_get_sndfile()->eval(fn);
	if (val>max) {max=val; b=adp_get_sndfile();}

	val=adp_get_ffmpeg()->eval(fn);
	if (val>max) {max=val; b=adp_get_ffmpeg();}

	return b;
}

void *ad_open(const char *fn, struct adinfo *nfo) {
	adecoder *d = (adecoder*) calloc(1, sizeof(adecoder));
	ad_clear_nfo(nfo);

	d->b = choose_backend(fn);
	if (!d->b) {
		dbg(0, "fatal: no decoder backend available");
		free(d);
		return NULL;
	}
	d->d = d->b->open(fn, nfo);
	if (!d->d) {
		free(d);
		return NULL;
	}
	return (void*)d;
}

int ad_info(void *sf, struct adinfo *nfo) {
	adecoder *d = (adecoder*) sf;
	if (!d) return -1;
	return d->b->info(d->d, nfo);
}

int ad_close(void *sf) {
	adecoder *d = (adecoder*) sf;
	if (!d) return -1;
	int rv = d->b->close(d->d);
	free(d);
	return rv;
}

int64_t ad_seek(void *sf, int64_t pos) {
	adecoder *d = (adecoder*) sf;
	if (!d) return -1;
	return d->b->seek(d->d, pos);
}

ssize_t ad_read(void *sf, float* out, size_t len){
	adecoder *d = (adecoder*) sf;
	if (!d) return -1;
	return d->b->read(d->d, out, len);
}

/*
 *  side-effects: allocates buffer
 */
ssize_t ad_read_mono_dbl(void *sf, struct adinfo *nfo, double* d, size_t len){
	unsigned int c,f;
	unsigned int chn = nfo->channels;
	if (len<1) return 0;

	static float *buf = NULL;
	static size_t bufsiz = 0;
	if (!buf || bufsiz != len*chn) {
		bufsiz=len*chn;
		buf = (float*) realloc((void*)buf, bufsiz * sizeof(float));
	}

	len = ad_read(sf, buf, bufsiz);

	for (f=0;f< (len/chn);f++) {
		double val=0.0;
		for (c=0;c<chn;c++) {
			val+=buf[f*chn + c];
		}
		d[f]= val/chn;
	}
	return len/chn;
}


int ad_finfo (const char *fn, struct adinfo *nfo) {
	ad_clear_nfo(nfo);
	void * sf = ad_open(fn, nfo);
	return ad_close(sf)?1:0;
}

void ad_clear_nfo(struct adinfo *nfo) {
	memset(nfo, 0, sizeof(struct adinfo));
}

void ad_free_nfo(struct adinfo *nfo) {
	if (nfo->meta_data) free(nfo->meta_data);
}

void ad_dump_nfo(int dbglvl, struct adinfo *nfo) {
	dbg(dbglvl, "sample_rate: %u", nfo->sample_rate);
	dbg(dbglvl, "channels:    %u", nfo->channels);
	dbg(dbglvl, "length:      %"PRIi64" ms", nfo->length);
	dbg(dbglvl, "frames:      %"PRIi64, nfo->frames);
	dbg(dbglvl, "bit_rate:    %d", nfo->bit_rate);
	dbg(dbglvl, "bit_depth:   %d", nfo->bit_depth);
	dbg(dbglvl, "channels:    %u", nfo->channels);
	dbg(dbglvl, "meta-data:   %s", nfo->meta_data?nfo->meta_data:"-");
}

void ad_debug_printf(const char* func, int level, const char* format, ...) {
    va_list args;

    va_start(args, format);
    if (level <= ad_debug_level) {
        fprintf(stderr, "%s(): ", func);
        vfprintf(stderr, format, args);
        fprintf(stderr, "\n");
    }
    va_end(args);
}

void ad_set_debuglevel(int lvl) {
	ad_debug_level = lvl;
	if (ad_debug_level<-1) ad_debug_level=-1;
	if (ad_debug_level>3) ad_debug_level=3;
}
