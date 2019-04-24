/* Tuner.lv2
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <stdbool.h>

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#ifdef DISPLAY_INTERFACE
#include <cairo/cairo.h>
#include <pango/pangocairo.h>
#include "lv2_rgext.h"
#endif

#ifndef MIN
#define MIN(A,B) ( (A) < (B) ? (A) : (B) )
#endif
#ifndef MAX
#define MAX(A,B) ( (A) > (B) ? (A) : (B) )
#endif


/* use FFT signal if tracked freq & FFT-freq differ by more than */
#define FFT_FREQ_THESHOLD_FAC (.10f)
/* but at least .. [Hz] */
#define FFT_FREQ_THESHOLD_MIN (5.f)

/* for testing only -- output filtered signal */
//#define OUTPUT_POSTFILTER

/* use both rising and falling signal edge to track phase */
#define TWO_EDGES


/* debug */
#if 0
#define info_printf printf
#else
void info_printf (const char *fmt,...) {}
#endif

#if 0 // lots of output
#define debug_printf printf
#else
void debug_printf (const char *fmt,...) {}
#endif


/*****************************************************************************/

#include "tuna.h"
#include "spectr.c"
#include "fft.c"

#ifdef __ARMEL__
// TODO also use for "#one" - to spread out load
// [maybe] run fft in GUI thread for "#two"
#define BACKGROUND_FFT
#endif

#ifdef BACKGROUND_FFT
#include <pthread.h>
#include "ringbuf.h"
#endif

/* recursively scan octave-overtones up to 4 octaves */
static uint32_t fftx_scan_overtones(struct FFTAnalysis *ft,
		const float threshold, uint32_t bin, uint32_t octave,
		const float v_oct2)
{
	const float scan  = MAX(2, (float) bin * .1f);
	float peak_dat = 0;
	uint32_t peak_pos = 0;
	for (uint32_t i = MAX(1, floorf(bin-scan)); i < ceilf(bin+scan); ++i) {
		if (
				   ft->power[i] > threshold
				&& ft->power[i] > peak_dat
				&& ft->power[i] > ft->power[i-1]
				&& ft->power[i] > ft->power[i+1]
			 ) {
			peak_pos = i;
			peak_dat = ft->power[i];
			debug_printf("ovt: bin %d oct %d th-fact: %f\n", i, octave, 10.0 * fast_log10(ft->power[i]/ threshold));
			break;
		}
	}
	if (peak_pos > 0) {
		octave *= 2;
		if (octave <= 16) {
			octave = fftx_scan_overtones(ft, threshold * v_oct2, peak_pos * 2, octave, v_oct2);
		}
	}
	return octave;
}

/** find lowest peak frequency above a given threshold */
static float fftx_find_note(struct FFTAnalysis *ft,
		const float abs_threshold,
		const float v_ovr, const float v_fun, const float v_oct, const float v_ovt)
{
	uint32_t fundamental = 0;
	uint32_t octave = 0;
	float peak_dat = 0;
	const uint32_t brkpos = ft->data_size * 8000 / ft->rate;
	float threshold = abs_threshold;

	for (uint32_t i = 1; i < brkpos; ++i) {
		if (
				ft->power[i] > threshold
				&& ft->power[i] > ft->power[i-1]
				&& ft->power[i] > ft->power[i+1]
			 ) {

			uint32_t o = fftx_scan_overtones(ft, ft->power[i] * v_oct, i * 2, 2, v_ovt);
			debug_printf("Candidate (%d) %f Hz -> %d overtones\n", i, fftx_freq_at_bin(ft, i) , o);

			if (o > octave
					|| (ft->power[i] > threshold * v_ovr)
					) {
				if (ft->power[i] > peak_dat) {
					peak_dat = ft->power[i];
					fundamental = i;
					octave = o;
					/* only prefer higher 'fundamental' if it's louder than a /usual/ 1st overtone.  */
					if (o > 2) threshold = peak_dat * v_fun;
					//if (o > 16) break;
				}
			}
		}
	}

	debug_printf("fun: bin: %d octave: %d freq: %.1fHz th-fact: %fdB\n",
			fundamental, octave, fftx_freq_at_bin(ft, fundamental), 10 * fast_log10(threshold / abs_threshold));
	if (octave == 0) { return 0; }
	return fftx_freq_at_bin(ft, fundamental);
}

/******************************************************************************
 * LV2 routines
 */

typedef struct {
	/* LV2 ports */
	float* a_in;
	float* a_out;

	float* p_mode;
	float* p_rms;
	float* p_tuning;
	float* p_freq_out;
	float* p_octave;
	float* p_note;
	float* p_cent;
	float* p_error;
	float* p_strobe;

	float* p_t_rms;
	float* p_t_flt;
	float* p_t_fft;
	float* p_t_ovr;
	float* p_t_fun;
	float* p_t_oct;
	float* p_t_ovt;

	LV2_Atom_Sequence* notify;
	const LV2_Atom_Sequence* control;

	/* internal state */
	double rate;
	struct FilterBank fb;
	float tuna_fc; // center freq of expected note
	uint32_t filter_init;
	bool initialize;

#ifdef __ARMEL__
	float freq_last;
	float cent_last;
#endif

	/* discriminator */
	float prev_smpl;

	/* RMS / threshold */
	float rms_omega;
	float rms_signal;
	float rms_postfilter;
	float rms_last;

	/* port thresholds */
	float t_rms, v_rms;
	float t_flt, v_flt;
	float t_fft, v_fft;
	float t_ovr, v_ovr;
	float t_fun, v_fun;
	float t_oct, v_oct;
	float t_ovt, v_ovt;

#ifdef BACKGROUND_FFT
	pthread_mutex_t lock;
	pthread_cond_t  signal;
	pthread_t       thread;
	bool            keep_running;

	ringbuf*        to_fft;
	ringbuf*        result;
#endif

	/* DLL */
	bool dll_initialized;
	uint32_t monotonic_cnt;
	double dll_e2, dll_e0;
	double dll_t0, dll_t1;
	double dll_b, dll_c;

	/* FFT */
	struct FFTAnalysis *fftx;
	bool fft_initialized;
	float fft_scale_freq;
	int fft_note_count;
	int fft_timeout;

	/* GUI communication */
	LV2_URID_Map* map;
	LV2_Atom_Forge forge;
	LV2_Atom_Forge_Frame frame;
	TunaLV2URIs uris;

	/* Spectrum */
	bool spectr_active;

#ifdef DISPLAY_INTERFACE
	LV2_Inline_Display_Image_Surface surf;
	PangoFontDescription*  font;
	cairo_surface_t*       display;
	LV2_Inline_Display*    queue_draw;
	uint32_t               w, h;
	uint32_t               fps_cnt;
	uint32_t               aspvf;
	float                  ui_strobe_dpy;
	float                  ui_strobe_phase;
	int                    guard1, guard2;
	float ui_note, ui_octave, ui_cent, ui_strobe_tme;
#endif
} Tuna;

#ifdef BACKGROUND_FFT
static void* worker (void* arg) {
	Tuna* self = (Tuna*)arg;
	const float rms_omega  = self->rms_omega;
	float rms_signal = 0;

  pthread_mutex_lock (&self->lock);
	while (self->keep_running) {
		// wait for signal
    pthread_cond_wait (&self->signal, &self->lock);

		if (!self->keep_running) {
			break;
		}

		size_t n_samples = rb_read_space (self->to_fft);
		do {
			if (n_samples < 1) {
				break;
			}
			if (n_samples > 8192) {
				n_samples = 8192;
			}

			float a_in[8192];
			rb_read (self->to_fft, a_in, n_samples);

			for (uint32_t n = 0; n < n_samples; ++n) {
				rms_signal += rms_omega * ((a_in[n] * a_in[n]) - rms_signal) + 1e-20;
			}

			if (rms_signal > .00000001f) {
				if (0 == fftx_run (self->fftx, n_samples, a_in)) {
					// TODO optimize: split RB here, call _fftx_run ()
					const float fft_peakfreq = fftx_find_note (self->fftx,
							rms_signal * self->v_fft,
							self->v_ovr, self->v_fun, self->v_oct, self->v_ovt);

					if (fft_peakfreq > 0) {
						rb_write (self->result, &fft_peakfreq, 1);
					}
				}
			}

			n_samples = rb_read_space (self->to_fft);
		} while (n_samples > 0);
	}
  pthread_mutex_unlock (&self->lock);
	return NULL;
}

static void feed_fft (Tuna* self, const float* data, size_t n_samples) {
	rb_write (self->to_fft, data, n_samples);

  if (pthread_mutex_trylock (&self->lock) == 0) {
    pthread_cond_signal (&self->signal);
    pthread_mutex_unlock (&self->lock);
  }
}
#endif


static LV2_Handle
instantiate(
		const LV2_Descriptor*     descriptor,
		double                    rate,
		const char*               bundle_path,
		const LV2_Feature* const* features)
{
	Tuna* self = (Tuna*)calloc(1, sizeof(Tuna));
	if(!self) {
		return NULL;
	}

	for (int i=0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID__map)) {
			self->map = (LV2_URID_Map*)features[i]->data;
		}
#ifdef DISPLAY_INTERFACE
		else if (!strcmp(features[i]->URI, LV2_INLINEDISPLAY__queue_draw)) {
			self->queue_draw = (LV2_Inline_Display*) features[i]->data;
		}
#endif
	}
	if (!self->map) {
		fprintf(stderr, "tuna.lv2 error: Host does not support urid:map\n");
		free(self);
		return NULL;
	}

	if (strncmp(descriptor->URI, TUNA_URI, strlen (TUNA_URI))) {
		free(self);
		return NULL;
	}

	self->rate = rate;

	self->tuna_fc = 0;
	self->prev_smpl = 0;
	self->rms_signal = 0;
	self->rms_postfilter = 0;
	self->rms_last = -100;
#ifdef __ARMEL__
	self->freq_last = 0;
	self->cent_last = 0;
#endif
	self->initialize = true;
	self->spectr_active = false;

	self->rms_omega = 1.0f - expf(-2.0 * M_PI * 15.0 / rate);
	/* reset DLL */
	self->dll_initialized = false;
	self->dll_e0 = self->dll_e2 = 0;
	self->dll_t1 = self->dll_t0 = 0;

	/* initialize FFT */
	self->fft_scale_freq = 0;
	self->fft_note_count = 0;
	self->fft_initialized = false;

	self->fftx = (struct FFTAnalysis*) calloc(1, sizeof(struct FFTAnalysis));
	int fft_size;
	fft_size = MAX(6144, rate / 15);

	/* round up to next power of two */
	fft_size--;
	fft_size |= fft_size >> 1;
	fft_size |= fft_size >> 2;
	fft_size |= fft_size >> 4;
	fft_size |= fft_size >> 8;
	fft_size |= fft_size >> 16;
	fft_size++;
	fft_size = MIN(32768, fft_size);

#ifdef __ARMEL__
	// TODO investigate autocorrelation
	// https://en.wikipedia.org/wiki/Wiener%E2%80%93Khinchin_theorem
	// http://miracle.otago.ac.nz/tartini/papers/A_Smarter_Way_to_Find_Pitch.pdf
	// for "note finding"
	fft_size = MIN(16384, fft_size);
#endif

	fftx_init(self->fftx, fft_size, rate, 0);

	/* map LV2 Atom URIs */
	map_tuna_uris(self->map, &self->uris);
	lv2_atom_forge_init(&self->forge, self->map);

#ifdef BACKGROUND_FFT
	pthread_mutex_init (&self->lock, NULL);
	pthread_cond_init (&self->signal, NULL);

	self->to_fft = rb_alloc (fft_size * 8);
	self->result = rb_alloc (32);
	self->keep_running = true;
	if (pthread_create (&self->thread, NULL, worker, self)) {
		pthread_mutex_destroy (&self->lock);
		pthread_cond_destroy (&self->signal);
		rb_free (self->to_fft);
		rb_free (self->result);
		fftx_free(self->fftx);
		free (self);
		return NULL;
	}
#endif
#ifdef DISPLAY_INTERFACE
	self->aspvf = rate / 25;
#endif
	return (LV2_Handle)self;
}

static void
connect_port_tuna(
		LV2_Handle handle,
		uint32_t   port,
		void*      data)
{
	Tuna* self = (Tuna*)handle;

	switch ((PortIndexTuna)port) {
		case TUNA_CONTROL:
			self->control = (const LV2_Atom_Sequence*)data;
			break;
		case TUNA_NOTIFY:
			self->notify = (LV2_Atom_Sequence*)data;
			break;
		case TUNA_AIN:
			self->a_in  = (float*)data;
			break;
		case TUNA_AOUT:
			self->a_out = (float*)data;
			break;
		case TUNA_RMS:
			self->p_rms = (float*)data;
			break;
		case TUNA_MODE:
			self->p_mode = (float*)data;
			break;
		case TUNA_TUNING:
			self->p_tuning = (float*)data;
			break;
		case TUNA_FREQ_OUT:
			self->p_freq_out = (float*)data;
			break;
		case TUNA_OCTAVE:
			self->p_octave = (float*)data;
			break;
		case TUNA_NOTE:
			self->p_note = (float*)data;
			break;
		case TUNA_CENT:
			self->p_cent = (float*)data;
			break;
		case TUNA_ERROR:
			self->p_error = (float*)data;
			break;
		case TUNA_STROBE:
			self->p_strobe = (float*)data;
			break;
		case TUNA_T_RMS:
			self->p_t_rms = (float*)data;
			break;
		case TUNA_T_FLT:
			self->p_t_flt = (float*)data;
			break;
		case TUNA_T_FFT:
			self->p_t_fft = (float*)data;
			break;
		case TUNA_T_OVR:
			self->p_t_ovr = (float*)data;
			break;
		case TUNA_T_FUN:
			self->p_t_fun = (float*)data;
			break;
		case TUNA_T_OCT:
			self->p_t_oct = (float*)data;
			break;
		case TUNA_T_OVT:
			self->p_t_ovt = (float*)data;
			break;
	}
}

static void tx_spectrum(Tuna *self, struct FFTAnalysis *ft)
{
	/* prepare data to transmit */
	float sp_x[512], sp_y[512];
	uint32_t p = 0;
	const uint32_t b = ft->data_size * 3000 / ft->rate;
	for (uint32_t i = 1; i < b && p < 512; i++) {
		if (ft->power[i] < .00000000063) { // (-92dB)^2
			continue;
		}
		sp_x[p] = fftx_freq_at_bin(ft, i);
		sp_y[p] = fftx_power_at_bin(ft, i);
		p++;
	}
	if (p == 0) return;

	LV2_Atom_Forge_Frame frame;
	lv2_atom_forge_frame_time(&self->forge, 0);
	x_forge_object(&self->forge, &frame, 1, self->uris.spectrum);

	lv2_atom_forge_property_head(&self->forge, self->uris.spec_data_x, 0);
	lv2_atom_forge_vector(&self->forge, sizeof(float), self->uris.atom_Float, p, sp_x);

	lv2_atom_forge_property_head(&self->forge, self->uris.spec_data_y, 0);
	lv2_atom_forge_vector(&self->forge, sizeof(float), self->uris.atom_Float, p, sp_y);

	lv2_atom_forge_pop(&self->forge, &frame);
}


/* round frequency to the closest note on the given scale.
 * use midi notation 0..127 for note-names
 */
static float freq_to_scale(Tuna *self, const float freq, int *midinote) {
	const float tuning = *self->p_tuning;
	/* calculate corresponding note - use midi notation 0..127 */
	const int note = rintf(12.f * log2f(freq / tuning) + 69.0);
	if (midinote) *midinote = note;
	/* ..and round it back to frequency */
	return tuning * powf(2.0, (note - 69.f) / 12.f);
}

static void
run(LV2_Handle handle, uint32_t n_samples)
{
	Tuna* self = (Tuna*)handle;

	/* first time around.
	 *
	 * this plugin does not always set ports every run()
	 * so we better initialize them.
	 *
	 * (liblilv does it according to .ttl,too * but better safe than sorry)
	 * */
	if (self->initialize) {
		self->initialize  = false;
		*self->p_freq_out = 0;
		*self->p_octave   = 4;
		*self->p_note     = 9;
		*self->p_cent     = 0;
		*self->p_error    = -100;
	}

	const uint32_t capacity = self->notify->atom.size;
	lv2_atom_forge_set_buffer(&self->forge, (uint8_t*)self->notify, capacity);
	lv2_atom_forge_sequence_head(&self->forge, &self->frame, 0);

	/* input ports */
	float const * const a_in = self->a_in;
	const float  mode   = *self->p_mode;
#ifdef OUTPUT_POSTFILTER
	float * const a_out = self->a_out;
#endif

	/* get thesholds */
#define GET_THRESHOLD(VAR) \
	if (*self->p_t_ ## VAR != self->t_ ## VAR) { \
		self->t_ ## VAR = *self->p_t_ ## VAR; \
		self->v_ ## VAR = powf(10, .1 * self->t_ ## VAR); \
	}

	GET_THRESHOLD(rms)
	GET_THRESHOLD(flt)
	GET_THRESHOLD(fft)
	GET_THRESHOLD(ovr)
	GET_THRESHOLD(fun)
	GET_THRESHOLD(oct)
	GET_THRESHOLD(ovt)

	/* localize variables */
	float prev_smpl = self->prev_smpl;
	float rms_signal = self->rms_signal;
	float rms_postfilter = self->rms_postfilter;
	const float rms_omega  = self->rms_omega;
	float freq = self->tuna_fc;
	const float rms_threshold = self->v_rms;
	const float v_flt = self->v_flt;

	/* initialize local vars */
	float    detected_freq = 0;
	uint32_t detected_count = 0;
	bool fft_ran_this_cycle = false;
	bool fft_proc_this_cycle = false;
	bool fft_active = false;

	/* operation mode */
	if (mode > 0 && mode < 10000) {
		/* fixed user-specified frequency */
		freq = mode;
		fft_proc_this_cycle = true;
		fft_active = false;
	} else if (mode <= -1 && mode >= -128) {
		/* midi-note */
		freq = (*self->p_tuning) * powf(2.0, floorf(-70 - mode) / 12.f);
		fft_proc_this_cycle = true;
		fft_active = false;
	} else {
		/* auto-detect  - run FFT */
		fft_active = true;
	}

#ifdef BACKGROUND_FFT
	if (fft_active) {
		feed_fft (self, a_in, n_samples);
	} else {
		rb_read_clear (self->result);
	}
#else
	if (fft_active || self->spectr_active) {
		fft_ran_this_cycle = 0 == fftx_run(self->fftx, n_samples, a_in);
	}
#endif

	/* Process incoming events from GUI */
	if (self->control) {
		LV2_Atom_Event* ev = lv2_atom_sequence_begin(&(self->control)->body);
		/* for each message from UI... */
		while(!lv2_atom_sequence_is_end(&(self->control)->body, (self->control)->atom.size, ev)) {
			/* .. only look at atom-events.. */
			if (ev->body.type == self->uris.atom_Blank || ev->body.type == self->uris.atom_Object) {
				const LV2_Atom_Object* obj = (LV2_Atom_Object*)&ev->body;
				/* interpret atom-objects: */
				if (obj->body.otype == self->uris.ui_on) {
					/* UI was activated */
					self->spectr_active = true;
				} else if (obj->body.otype == self->uris.ui_off) {
					/* UI was closed */
					self->spectr_active = false;
				}
			}
			ev = lv2_atom_sequence_next(ev);
		}
	}

	if (fft_ran_this_cycle && self->spectr_active) {
		tx_spectrum(self, self->fftx);
	}

#ifdef BACKGROUND_FFT
	fft_ran_this_cycle = rb_read_space (self->result) > 0;
#endif

	/* process every sample */
	for (uint32_t n = 0; n < n_samples; ++n) {

		/* 1) calculate RMS */
		rms_signal += rms_omega * ((a_in[n] * a_in[n]) - rms_signal) + 1e-20;

		if (rms_signal < rms_threshold) {
			/* signal below threshold */
			self->dll_initialized = false;
			self->fft_initialized = false;
			self->fft_note_count = 0;
			prev_smpl = 0;
#ifdef OUTPUT_POSTFILTER
			a_out[n] = 0;
#endif
			continue;
		}

		/* 2) detect frequency to track
		 * use FFT to roughly detect the area
		 */

		/* FFT accumulates data and only returns us some
		 * valid data once in a while.. */
		if (fft_ran_this_cycle && !fft_proc_this_cycle) {
			fft_proc_this_cycle = true;
			/* get lowest peak frequency */
#ifdef BACKGROUND_FFT
			float fft_peakfreq = 0;
			while (0 == rb_read_one (self->result, &fft_peakfreq)) ;
#else
			const float fft_peakfreq = fftx_find_note(self->fftx, rms_signal * self->v_fft, self->v_ovr, self->v_fun, self->v_oct, self->v_ovt);
#endif
			if (fft_peakfreq < 20) {
				self->fft_note_count = 0;
			} else {
				const float note_freq = freq_to_scale(self, fft_peakfreq, NULL);

				/* keep track of fft stability */
				if (note_freq == self->fft_scale_freq) {
					self->fft_note_count+=n_samples;
				} else {
					self->fft_note_count = 0;
				}
				self->fft_scale_freq = note_freq;

				debug_printf("FFT found peak: %fHz -> freq: %fHz (%d)\n", fft_peakfreq, note_freq, self->fft_note_count);

				if (freq != note_freq &&
						(   (!self->dll_initialized && self->fft_note_count > 768)
						 || (self->fft_note_count > 1536 && fabsf(freq - note_freq) > MAX(FFT_FREQ_THESHOLD_MIN, freq * FFT_FREQ_THESHOLD_FAC))
						 || (self->fft_note_count > self->rate / 8)
						)
					 ) {
					info_printf("FFT adjust %fHz -> %fHz (fft:%fHz) cnt:%d\n", freq, note_freq, fft_peakfreq, self->fft_note_count);
					freq = note_freq;
				}
			}
		}

		/* refuse to track insanity */
		if (freq < 20 || freq > 10000 ) {
			self->dll_initialized = false;
			prev_smpl = 0;
#ifdef OUTPUT_POSTFILTER
			a_out[n] = 0;
#endif
			continue;
		}

		/* 2a) re-init detector coefficients with frequency to track */
		if (freq != self->tuna_fc) {
			self->tuna_fc = freq;
			info_printf("set filter: %.2fHz\n", freq);
			
			/* calculate DLL coefficients */
			const double omega = ((self->tuna_fc < 50) ? 6.0 : 4.0) * M_PI * self->tuna_fc / self->rate;
			self->dll_b = 1.4142135623730950488 * omega; // sqrt(2)
			self->dll_c = omega * omega;
			self->dll_initialized = false;

			/* re-initialize filter */
			bandpass_setup(&self->fb, self->rate, self->tuna_fc
					, MAX(10, self->tuna_fc * .10)
					, 4 /*th order butterworth */);
			self->filter_init = 16;
		}
		
		/* 3) band-pass filter the signal to clean up the
		 * waveform for counting zero-transitions.
		 */
		const float signal = bandpass_process(&self->fb, a_in[n]);

		if (self->filter_init > 0) {
			self->filter_init--;
			rms_postfilter = 0;
#ifdef OUTPUT_POSTFILTER
			a_out[n] = signal * (16.0 - self->filter_init) / 16.0;
#endif
			continue;
		}
#ifdef OUTPUT_POSTFILTER
		a_out[n] = signal;
#endif

		/* 4) reject signals outside in the band */
		rms_postfilter += rms_omega * ( (signal * signal) - rms_postfilter) + 1e-20;
		if (rms_postfilter < rms_signal * v_flt) {
			debug_printf("signal too low after filter: %f %f\n",
					10.*fast_log10(2.f *rms_signal),
					10.*fast_log10(2.f *rms_postfilter));
			self->dll_initialized = false;
			prev_smpl = 0;
			continue;
		}

		/* 5) track phase by counting
		 * rising-edge zero-transitions
		 * and a 2nd order phase-locked loop
		 */
		if (   (signal >= 0 && prev_smpl < 0)
#ifdef TWO_EDGES
				|| (signal <= 0 && prev_smpl > 0)
#endif
				) {

			if (!self->dll_initialized) {
				info_printf("reinit DLL\n");
				/* re-initialize DLL */
				self->dll_initialized = true;
				self->dll_e0 = self->dll_t0 = 0;
#ifdef TWO_EDGES
				self->dll_e2 = self->rate / self->tuna_fc / 2.f;
#else
				self->dll_e2 = self->rate / self->tuna_fc;
#endif
				self->dll_t1 = self->monotonic_cnt + n + self->dll_e2;
			} else {
				/* phase 'error' = detected_phase - expected_phase */
				self->dll_e0 = (self->monotonic_cnt + n) - self->dll_t1;

				/* update DLL, keep track of phase */
				self->dll_t0 = self->dll_t1;
				self->dll_t1 += self->dll_b * self->dll_e0 + self->dll_e2;
				self->dll_e2 += self->dll_c * self->dll_e0;

#ifdef TWO_EDGES
				const float dfreq0 = self->rate / (self->dll_t1 - self->dll_t0) / 2.f;
				const float dfreq2 = self->rate / (self->dll_e2) / 2.f;
#else
				const float dfreq0 = self->rate / (self->dll_t1 - self->dll_t0);
				const float dfreq2 = self->rate / (self->dll_e2);
#endif
				debug_printf("detected Freq: %.2f flt: %.2f (error: %.2f [samples]) diff:%f)\n",
						dfreq0, dfreq2, self->dll_e0, (self->dll_t1 - self->dll_t0) - self->dll_e2);

				float dfreq;
				if (fabsf (self->dll_e0 * freq / self->rate) > .02) {
					dfreq = dfreq0;
				} else {
					dfreq = dfreq2;
				}

#if 1
				/* calculate average of all detected values in this cycle.
				 * this is questionable, just use last value.
				 */
				detected_freq += dfreq;
				detected_count++;
#else
				detected_freq = dfreq;
				detected_count= 1;
#endif
			}
		}
		prev_smpl = signal;
	}

	/* copy back variables */
	self->prev_smpl = prev_smpl;
	self->rms_signal = rms_signal;
	self->rms_postfilter = rms_postfilter;

	if (!self->dll_initialized) {
		self->monotonic_cnt = 0;
	} else {
		self->monotonic_cnt += n_samples;
	}

	/* close off atom sequence */
	lv2_atom_forge_pop(&self->forge, &self->frame);

	/* post-processing and data-output */
	if (detected_count > 0) {
		/* calculate average of detected frequency */
		int note;
		const float freq_avg = detected_freq / (float)detected_count;
		/* ..and the corresponding note on the scale */
		const float note_freq = freq_to_scale(self, freq_avg, &note);

		debug_printf("detected Freq: %.2f (error: %.2f [samples])\n", freq_avg, self->dll_e0);

		/* calculate cent difference
		 * One cent is one hundredth part of the semitone in 12-tone equal temperament
		 */
		const float cent = 1200.0 * log2(freq_avg / note_freq);

		/* assign output port data */
#ifdef __ARMEL__
		if (fabsf (self->freq_last - freq_avg) > .05) {
			*self->p_freq_out = freq_avg;
			self->freq_last = freq_avg;
		} else {
			*self->p_freq_out = self->freq_last;
		}
		if (fabsf (self->cent_last - cent) > .02) {
			*self->p_cent     = cent;
			self->cent_last   = cent;
		} else {
			*self->p_cent     = self->cent_last;
		}
#else
		*self->p_freq_out = freq_avg;
		*self->p_cent     = cent;
#endif

	  *self->p_octave   = (note/12) -1;
	  *self->p_note     = note%12;
	  *self->p_error    = 100.0 * self->dll_e0 * note_freq / self->rate;

	}
	else if (!self->dll_initialized) {
		/* no signal detected; or below threshold */
		*self->p_freq_out = 0;
	  *self->p_error = 0;
	}
	/* else { no change, maybe short cycle } */

	/* report input level
	 * NB. 20 *log10f(sqrt(x)) == 10 * log10f(x) */
	const float rms = (rms_signal > .0000000001f) ? 10. * fast_log10(2 * rms_signal) : -100;
	//const float rms = (rms_postfilter > .0000000001f) ? 10. * fast_log10(2 * rms_postfilter) : -100;
#ifdef __ARMEL__
	if (fabsf (self->rms_last - rms) > 1) {
		*self->p_rms = rms;
		self->rms_last = rms;
	} else {
		*self->p_rms = self->rms_last;
	}
#else
	*self->p_rms = rms;
#endif

	*self->p_strobe = self->monotonic_cnt / self->rate; // kick UI

	/* forward audio */
	if (self->a_in != self->a_out) {
		memcpy(self->a_out, self->a_in, sizeof(float) * n_samples);
	}

#ifdef DISPLAY_INTERFACE
	if (self->queue_draw) {
		self->fps_cnt += n_samples;
		if (self->fps_cnt > self->aspvf) {
			self->fps_cnt = self->fps_cnt % self->aspvf;

			self->guard1++;
			self->ui_octave = *self->p_octave;
			self->ui_note = *self->p_note;
			self->ui_cent = *self->p_cent;
			self->ui_strobe_tme = *self->p_strobe;
			self->guard2++;

			self->queue_draw->queue_draw (self->queue_draw->handle);
		}
	}
#endif
}

static void
cleanup(LV2_Handle handle)
{
	Tuna* self = (Tuna*)handle;

#ifdef DISPLAY_INTERFACE
	if (self->display) {
		cairo_surface_destroy (self->display);
	}
	if (self->font) {
		pango_font_description_free (self->font);
	}
#endif
#ifdef BACKGROUND_FFT
  pthread_mutex_lock (&self->lock);
	self->keep_running = false;
	pthread_cond_signal (&self->signal);
	pthread_mutex_unlock (&self->lock);
	pthread_join (self->thread, NULL);

	pthread_mutex_destroy (&self->lock);
	pthread_cond_destroy (&self->signal);
	rb_free (self->to_fft);
	rb_free (self->result);
#endif

	fftx_free(self->fftx);
	free(handle);
}

#ifdef WITH_SIGNATURE
#define RTK_URI TUNA_URI
#include "gpg_init.c"
#include WITH_SIGNATURE
struct license_info license_infos = {
	"x42-Tuner",
	"http://x42-plugins.com/x42/x42-tuner"
};
#include "gpg_lv2ext.c"
#endif

/******************************************************************************
 * Inline Display
 */

#ifdef DISPLAY_INTERFACE
static LV2_Inline_Display_Image_Surface *
tuna_render (LV2_Handle handle, uint32_t w, uint32_t max_h)
{
#ifdef WITH_SIGNATURE
	if (!is_licensed (handle)) { return NULL; }
#endif
	uint32_t h = MAX (32, MIN (1 | (uint32_t)ceilf (w / 3.f), max_h));

	static const char notename[12][3] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

	Tuna* self = (Tuna*)handle;
	char txt[32];

	if (!self->display || self->w != w || self->h != h) {
		if (self->display) cairo_surface_destroy(self->display);
		self->display = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, w, h);
		self->w = w;
		self->h = h;
		if (self->font) {
			pango_font_description_free (self->font);
		}
		snprintf(txt, 32, "Mono %.0fpx", floor (h * .375));
		self->font = pango_font_description_from_string (txt);
	}
	cairo_t* cr = cairo_create (self->display);
	cairo_rectangle (cr, 0, 0, w, h);
	cairo_set_source_rgba (cr, .2, .2, .2, 1.0);
	cairo_fill (cr);

	float ui_octave;
	float ui_note;
	float ui_cent;
	float ui_strobe_tme;

	int tries = 0;
	do {
		if (tries == 10) {
			tries = 0;
			sched_yield ();
		}
		ui_octave = self->ui_octave;
		ui_note = self->ui_note;
		ui_cent = self->ui_cent;
		ui_strobe_tme = self->ui_strobe_tme;
		++tries;
	} while (self->guard1 != self->guard2);

	/* strobe setup */
	cairo_set_source_rgba (cr, .5, .5, .5, .8);
	if (self->ui_strobe_dpy != ui_strobe_tme) {
		if (ui_strobe_tme > self->ui_strobe_dpy) {
			float tdiff = ui_strobe_tme - self->ui_strobe_dpy;
			self->ui_strobe_phase += tdiff * ui_cent * 4;
			if (fabsf (ui_cent) < 5) {
				cairo_set_source_rgba (cr, .2, .9, .2, .7);
			} else if (fabsf (ui_cent) < 10) {
				cairo_set_source_rgba (cr, .8, .8, .0, .7);
			} else {
				cairo_set_source_rgba (cr, .9, .2, .2, .7);
			}
		}
		self->ui_strobe_dpy = ui_strobe_tme;
	}

	/* render strobe */
	cairo_save(cr);
	const double dash1[] = {8.0};
	const double dash2[] = {16.0};

	cairo_set_dash(cr, dash1, 1, self->ui_strobe_phase * -2.);
	cairo_set_line_width(cr, 8.0);
	cairo_move_to(cr, 0, h * .75);
	cairo_line_to(cr, w, h * .75);
	cairo_stroke (cr);

	cairo_set_dash(cr, dash2, 1, -self->ui_strobe_phase);
	cairo_set_line_width(cr, 16.0);
	cairo_move_to(cr, 0, h * .75);
	cairo_line_to(cr, w, h * .75);
	cairo_stroke (cr);
	cairo_restore(cr);

	/* render text */
	int tw, th;
	if (fabsf (ui_cent) < 100) {
		snprintf(txt, 32, "%-2s%.0f %+3.0f\u00A2", notename[(int)ui_note], ui_octave, ui_cent);
	} else {
		snprintf(txt, 32, "%-2s%.0f", notename[(int)ui_note], ui_octave);
	}
	PangoLayout * pl = pango_cairo_create_layout (cr);
	pango_layout_set_font_description (pl, self->font);
	pango_layout_set_text (pl, txt, -1);
	pango_layout_get_pixel_size (pl, &tw, &th);
	cairo_move_to (cr, 0.5 * (w - tw), 0.25 * h - .5 * th);
#if 0
	cairo_set_source_rgba (cr, 1, 1, 1, 1);
	pango_cairo_show_layout (cr, pl);
#else
	pango_cairo_layout_path (cr, pl);
	cairo_set_line_width(cr, 2.5);
	cairo_set_source_rgba (cr, 0, 0, 0, .5);
	cairo_stroke_preserve (cr);
	cairo_set_source_rgba (cr, 1, 1, 1, 1);
	cairo_fill (cr);
#endif
	g_object_unref(pl);

	/* finish surface */
	cairo_destroy (cr);
	cairo_surface_flush (self->display);
	self->surf.width = cairo_image_surface_get_width (self->display);
	self->surf.height = cairo_image_surface_get_height (self->display);
	self->surf.stride = cairo_image_surface_get_stride (self->display);
	self->surf.data = cairo_image_surface_get_data  (self->display);

	return &self->surf;
}
#endif

/******************************************************************************
 * LV2 setup
 */
const void*
extension_data(const char* uri)
{
#ifdef DISPLAY_INTERFACE
	static const LV2_Inline_Display_Interface display  = { tuna_render };
	if (!strcmp(uri, LV2_INLINEDISPLAY__interface)) {
#if (defined _WIN32 && defined RTK_STATIC_INIT)
		static int once = 0;
		if (!once) {once = 1; gobject_init_ctor();}
#endif
		return &display;
	}
#endif
#ifdef WITH_SIGNATURE
	LV2_LICENSE_EXT_C
#endif
	return NULL;
}

#define mkdesc_tuna(ID, NAME) \
static const LV2_Descriptor descriptor ## ID = { \
	TUNA_URI NAME,     \
	instantiate,       \
	connect_port_tuna, \
	NULL,              \
	run,               \
	NULL,              \
	cleanup,           \
	extension_data     \
};

mkdesc_tuna(0, "one")
mkdesc_tuna(1, "two")
mkdesc_tuna(2, "mod")

#undef LV2_SYMBOL_EXPORT
#ifdef _WIN32
#    define LV2_SYMBOL_EXPORT __declspec(dllexport)
#else
#    define LV2_SYMBOL_EXPORT  __attribute__ ((visibility ("default")))
#endif
LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch (index) {
		case  0: return &descriptor0;
		case  1: return &descriptor1;
		case  2: return &descriptor2;
		default: return NULL;
	}
}

/* vi:set ts=2 sts=2 sw=2: */
