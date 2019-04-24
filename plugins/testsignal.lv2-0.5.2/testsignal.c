/* testsignal -- LV2 test-tone generator
 *
 * Copyright (C) 2015,2018 Robin Gareus <robin@gareus.org>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE // needed for M_PI
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <limits.h>

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#define TST_URI "http://gareus.org/oss/lv2/testsignal"

#if INTPTR_MAX == INT64_MAX
#warning PCG-random
#define PCGRANDOM
#else
#warning LCG-random
#endif

typedef enum {
	TST_MODE   = 0,
	TST_REFLEV = 1,
	TST_OUTPUT = 2
} PortIndex;

typedef struct {
	// plugin ports
	float* mode;
	float* reflevel;
	float* output;

	// signal level
	float  lvl_db; // corresponds to 'reflevel'
	float  lvl_coeff_target;
	float  lvl_coeff;

	// sine/square wave generator state
	float  phase;
	float  phase_inc;

	// impulse period counters
	uint32_t k_cnt;
	uint32_t k_period100;
	uint32_t k_period1;
	uint32_t k_period5s;

	// sweep settings
	double swp_log_a, swp_log_b;
	// sweep counter
	uint32_t swp_period;
	uint32_t swp_cnt;

	// pseudo-random number state
#ifdef PCGRANDOM
	uint64_t rseed;
#else
	uint32_t rseed;
#endif

	bool  g_pass;
	float g_rn1;
	float b0, b1, b2, b3, b4, b5, b6; // pink noise

} TestSignal;


/* pseudo-random number generators */

static inline uint32_t
rand_int (TestSignal *self)
{
#ifdef PCGRANDOM
	uint64_t oldstate = self->rseed;
	self->rseed = oldstate * 6364136223846793005ULL + 1;
	uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
	uint32_t rot = oldstate >> 59u;
	return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
#else
	// 31bit Park-Miller-Carta Pseudo-Random Number Generator
	// http://www.firstpr.com.au/dsp/rand31/
	uint32_t hi, lo;
	lo = 16807 * (self->rseed & 0xffff);
	hi = 16807 * (self->rseed >> 16);

	lo += (hi & 0x7fff) << 16;
	lo += hi >> 15;
	lo = (lo & 0x7fffffff) + (lo >> 31);
	return (self->rseed = lo);
#endif
}

static inline float
rand_float (TestSignal *self)
{
#ifdef PCGRANDOM
	return (rand_int (self) / 2147483648.f) - 1.f;
#else
	return (rand_int (self) / 1073741824.f) - 1.f;
#endif
}

static float
rand_gauss (TestSignal *self)
{
	// Gaussian White Noise
	// http://www.musicdsp.org/archive.php?classid=0#109
	float x1, x2, r;

	if (self->g_pass) {
		self->g_pass = false;
		return self->g_rn1;
	}

	do {
		x1 = rand_float (self);
		x2 = rand_float (self);
		r = x1 * x1 + x2 * x2;
	} while ((r >= 1.0f) || (r < 1e-22f));

	r = sqrtf (-2.f * logf (r) / r);

	self->g_pass = true;
	self->g_rn1 = r * x2;
	return r * x1;
}


/* signal generators */

static void
gen_sine (TestSignal *self, uint32_t n_samples)
{
	float *out = self->output;
	float phase = self->phase;
	const float phase_inc = self->phase_inc;
	const float level = self->lvl_coeff;

	for (uint32_t i = 0 ; i < n_samples; ++i) {
		out[i] = level * sinf (phase);
		phase += phase_inc;
	}
	self->phase = fmodf (phase, 2.0 * M_PI);
}

static void
gen_square (TestSignal *self, uint32_t n_samples)
{
	float *out = self->output;
	double phase = self->phase;
	const float phase_inc = self->phase_inc;
	const float level = self->lvl_coeff;

	for (uint32_t i = 0 ; i < n_samples; ++i) {
		out[i] = sinf (phase) >= 0 ? level : - level;
		phase += phase_inc;
	}
	self->phase = fmod (phase, 2.0 * M_PI);
}

static void
gen_uniform_white (TestSignal *self, uint32_t n_samples)
{
	const float level = self->lvl_coeff;
	float *out = self->output;
	for (uint32_t i = 0 ; i < n_samples; ++i) {
		out[i] = level * rand_float (self);
	}
}

static void
gen_gaussian_white (TestSignal *self, uint32_t n_samples)
{
	const float level = self->lvl_coeff * 0.7079f;
	float *out = self->output;
	for (uint32_t i = 0 ; i < n_samples; ++i) {
		out[i] = level * rand_gauss (self);
	}
}

static void
gen_pink (TestSignal *self, uint32_t n_samples)
{
	float *out = self->output;
	const float level = self->lvl_coeff / 2.527f;

	// localize variables
	float _b0 = self->b0;
	float _b1 = self->b1;
	float _b2 = self->b2;
	float _b3 = self->b3;
	float _b4 = self->b4;
	float _b5 = self->b5;
	float _b6 = self->b6;

	while (n_samples-- > 0) {
		// Paul Kellet's refined method
		// http://www.musicdsp.org/files/pink.txt
		// NB. If 'white' consists of uniform random numbers,
		// the pink noise will have an almost gaussian distribution.
		const float white = level * rand_float (self);
		_b0 = .99886f * _b0 + white * .0555179f;
		_b1 = .99332f * _b1 + white * .0750759f;
		_b2 = .96900f * _b2 + white * .1538520f;
		_b3 = .86650f * _b3 + white * .3104856f;
		_b4 = .55000f * _b4 + white * .5329522f;
		_b5 = -.7616f * _b5 - white * .0168980f;
		*out++ = _b0 + _b1 + _b2 + _b3 + _b4 + _b5 + _b6 + white * 0.5362f;
		_b6 = white * 0.115926f;
	}

	// copy back variables
	self->b0 = _b0;
	self->b1 = _b1;
	self->b2 = _b2;
	self->b3 = _b3;
	self->b4 = _b4;
	self->b5 = _b5;
	self->b6 = _b6;
}

static void
gen_kroneker_delta (TestSignal *self, uint32_t n_samples, const uint32_t period)
{
	float *out = self->output;
	memset (out, 0, n_samples * sizeof (float));

	uint32_t k_cnt = self->k_cnt;

	while (n_samples > k_cnt) {
		out[k_cnt] = 1.0f;
		k_cnt += period;
	}

	self->k_cnt = k_cnt - n_samples;
}

static void
gen_sawtooth (TestSignal *self, uint32_t n_samples, const uint32_t period)
{
	float *out = self->output;
	uint32_t k_cnt = self->k_cnt % period;
	const float level = self->lvl_coeff;
	for (uint32_t i = 0 ; i < n_samples; ++i) {
		out[i] = -1.f + 2.f * k_cnt / (float) period;
		out[i] *= level;
		k_cnt = (k_cnt + 1) % period;
	}
	self->k_cnt = k_cnt;
}

static void
gen_triangle (TestSignal *self, uint32_t n_samples, const uint32_t period)
{
	float *out = self->output;
	uint32_t k_cnt = self->k_cnt % period;
	const float level = self->lvl_coeff;
	for (uint32_t i = 0 ; i < n_samples; ++i) {
		out[i] = -1.f + 2.f * fabsf (1 - 2.f * k_cnt / (float) period);
		out[i] *= level;
		k_cnt = (k_cnt + 1) % period;
	}
	self->k_cnt = k_cnt;
}

static void
gen_sine_log_sweep (TestSignal *self, uint32_t n_samples)
{

	float *out = self->output;
	uint32_t swp_cnt = self->swp_cnt;
	const uint32_t swp_period = self->swp_period;
	const double swp_log_a = self->swp_log_a;
	const double swp_log_b = self->swp_log_b;
	const float level = self->lvl_coeff;

	for (uint32_t i = 0 ; i < n_samples; ++i) {
		const double phase = swp_log_a * exp (swp_log_b * swp_cnt) - swp_log_a;
		out[i] = level * sin (2. * M_PI * (phase - floor (phase)));
		swp_cnt = (swp_cnt + 1) % swp_period;
	}
	self->swp_cnt = swp_cnt;
}

/* LV2 */

static LV2_Handle
instantiate (const LV2_Descriptor*     descriptor,
             double                    rate,
             const char*               bundle_path,
             const LV2_Feature* const* features)
{
	if (rate < 8000) {
		return NULL;
	}

	TestSignal* self = (TestSignal*)calloc (1, sizeof (TestSignal));

	self->phase_inc = 2 * M_PI * 1000 / rate;
	self->k_period100 = rate / 100;
	self->k_period1   = rate;
	self->k_period5s  = rate * 5;

	// log frequency sweep
	const double f_min = 20.;
	const double f_max = (rate * .5) < 20000. ? (rate * .5) : 20000.;
	self->swp_period = ceil (10.0 * rate); // 10 seconds
	self->swp_log_b = log (f_max / f_min) / self->swp_period;
	self->swp_log_a = f_min / (self->swp_log_b * rate);

#ifdef PCGRANDOM
	self->rseed = time (NULL) ^ (intptr_t)self;
#else
	self->rseed = (time (NULL) + (intptr_t)self) % INT_MAX;
	if (self->rseed == 0) self->rseed = 1;
#endif

	return (LV2_Handle)self;
}

static void
connect_port (LV2_Handle instance,
              uint32_t   port,
              void*      data)
{
	TestSignal* self = (TestSignal*)instance;

	switch ((PortIndex)port) {
	case TST_MODE:
		self->mode = data;
		break;
	case TST_REFLEV:
		self->reflevel = data;
		break;
	case TST_OUTPUT:
		self->output = data;
		break;
	}
}

static void
run (LV2_Handle instance, uint32_t n_samples)
{
	TestSignal* self = (TestSignal*)instance;
	if (self->lvl_db != *self->reflevel) {
		self->lvl_db = *self->reflevel;
		float db = self->lvl_db;
		if (db < -24) db = -24;
		if (db > -9)  db = -9;
		self->lvl_coeff_target = powf (10, 0.05 * db);
	}

	self->lvl_coeff += .1 * (self->lvl_coeff_target - self->lvl_coeff) + 1e-12;

	int mode = rint (*self->mode);
	if (mode <= 0)      { gen_sine (self, n_samples); }
	else if (mode <= 1) { gen_square (self, n_samples); }
	else if (mode <= 2) { gen_uniform_white (self, n_samples); }
	else if (mode <= 3) { gen_gaussian_white (self, n_samples); }
	else if (mode <= 4) { gen_pink (self, n_samples); }
	else if (mode <= 5) { gen_kroneker_delta (self, n_samples, self->k_period100); }
	else if (mode <= 6) { gen_sine_log_sweep (self, n_samples); }
	else if (mode <= 7) { gen_kroneker_delta (self, n_samples, self->k_period1); }
	else if (mode <= 8) { gen_kroneker_delta (self, n_samples, self->k_period5s); }
	else if (mode <= 9) { gen_sawtooth  (self, n_samples, self->k_period100); }
	else                { gen_triangle  (self, n_samples, self->k_period100); }
}

static void
cleanup (LV2_Handle instance)
{
	free (instance);
}

const void*
extension_data (const char* uri)
{
	return NULL;
}

static const LV2_Descriptor descriptor = {
	TST_URI,
	instantiate,
	connect_port,
	NULL,
	run,
	NULL,
	cleanup,
	extension_data
};

#undef LV2_SYMBOL_EXPORT
#ifdef _WIN32
#    define LV2_SYMBOL_EXPORT __declspec(dllexport)
#else
#    define LV2_SYMBOL_EXPORT  __attribute__ ((visibility ("default")))
#endif
LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor (uint32_t index)
{
	switch (index) {
	case 0:
		return &descriptor;
	default:
		return NULL;
	}
}
