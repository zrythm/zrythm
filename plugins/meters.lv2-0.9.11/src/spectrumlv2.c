/* meter.lv2
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

/* static functions to be included in meters.cc
 *
 * broken out spectrum analyzer related LV2 functions
 */

/******************************************************************************
 * LV2 spec
 */

#if defined __cplusplus && !defined isfinite
#define isfinite std::isfinite
#endif

#define FILTER_COUNT (30)

typedef enum {
	SA_SPEED    = 60,
	SA_RESET    = 61,
	SA_AMP      = 62,
	SA_STATE    = 63,
	SA_INPUT0   = 64,
	SA_OUTPUT0  = 65,
	SA_INPUT1   = 66,
	SA_OUTPUT1  = 67,
} SAPortIndex;

typedef struct {
	float* input[2];
	float* output[2];

	float* spec[FILTER_COUNT];
	float* maxf[FILTER_COUNT];
	float* rst_p;
	float* spd_p;
	float* amp_p;

	float  rst_h;
	float  spd_h;

	uint32_t nchannels;
	double rate;

	float  omega;
	float  val_f[FILTER_COUNT];
	float  max_f[FILTER_COUNT];
	struct FilterBank flt[FILTER_COUNT];

} LV2spec;

/******************************************************************************
 * LV2 callbacks
 */

static LV2_Handle
spectrum_instantiate(
		const LV2_Descriptor*     descriptor,
		double                    rate,
		const char*               bundle_path,
		const LV2_Feature* const* features)
{
	uint32_t nchannels;
	if (!strcmp(descriptor->URI, MTR_URI "spectr30stereo")) {
		nchannels = 2;
	}
	else if (!strcmp(descriptor->URI, MTR_URI "spectr30mono")) {
		nchannels = 1;
	}
	else { return NULL; }

	LV2spec* self = (LV2spec*)calloc(1, sizeof(LV2spec));
	if (!self) return NULL;

	self->nchannels = nchannels;
	self->rate = rate;

	self->rst_h = -4;
	self->spd_h = 1.0;
	// 1.0 - e^(-2.0 * π * v / 48000)
	self->omega = 1.0f - expf(-2.0 * M_PI * self->spd_h / rate);

	/* filter-frequencies */
	const double f_r = 1000;
	const double b = 3;
	const double f1f = pow(2, -1. / (2. * b));
	const double f2f = pow(2,  1. / (2. * b));

	for (uint32_t i=0; i < FILTER_COUNT; ++i) {
		const int x = i - 16;
		const double f_m = pow(2, x / b) * f_r;
		const double f_1 = f_m * f1f;
		const double f_2 = f_m * f2f;
		const double bw  = f_2 - f_1;
#ifdef DEBUG_SPECTR
		printf("--F %2d (%3d): f:%9.2fHz b:%9.2fHz (%9.2fHz -> %9.2fHz)\n",i, x, f_m, bw, f_1, f_2);
#endif
		self->val_f[i] = 0;
		self->max_f[i] = 0;
		bandpass_setup(&self->flt[i], self->rate, f_m, bw, 6);
	}

	return (LV2_Handle)self;
}

static void
spectrum_connect_port(LV2_Handle instance, uint32_t port, void* data)
{
	LV2spec* self = (LV2spec*)instance;
	switch (port) {
	case SA_INPUT0:
		self->input[0] = (float*) data;
		break;
	case SA_OUTPUT0:
		self->output[0] = (float*) data;
		break;
	case SA_INPUT1:
		self->input[1] = (float*) data;
		break;
	case SA_OUTPUT1:
		self->output[1] = (float*) data;
		break;
	case SA_RESET:
		self->rst_p = (float*) data;
		break;
	case SA_SPEED:
		self->spd_p = (float*) data;
		break;
	case SA_AMP:
		break;
	default:
		if (port < 30) {
			self->spec[port] = (float*) data;
		}
		if (port >= 30 && port < 60) {
			self->maxf[port-30] = (float*) data;
		}
		break;
	}
}

static void
spectrum_run(LV2_Handle instance, uint32_t n_samples)
{
	LV2spec* self = (LV2spec*)instance;
	float* inL = self->input[0];
	float* inR = self->input[1];
	bool reinit_gui = false;

	/* calculate time-constant when it is changed,
	 * (no-need to smoothen transition for the visual display)
	 */
	if (self->spd_h != *self->spd_p) {
		self->spd_h = *self->spd_p;
		float v = self->spd_h;
		if (v < 0.01) v = 0.01;
		if (v > 15.0) v = 15.0;
		self->omega = 1.0f - expf(-2.0 * M_PI * v / self->rate);
		self->rst_h = 0; // reset peak-hold on change
	}

	/* localize variables */
	float val_f[FILTER_COUNT];
	float max_f[FILTER_COUNT];
	const float omega  = self->omega;
	struct FilterBank *flt[FILTER_COUNT];

	for(int i=0; i < FILTER_COUNT; ++i) {
		val_f[i] = self->val_f[i];
		max_f[i] = self->max_f[i];
		flt[i]   = &self->flt[i];
	}

	if (self->rst_h != *self->rst_p) {
		/* reset peak-hold */
		if (fabsf(*self->rst_p) < 3 || self->rst_h == 0) {
			reinit_gui = true;
			for(int i = 0; i < FILTER_COUNT; ++i) {
				max_f[i] = 0;
			}
		}
		if (fabsf(*self->rst_p) != 3) {
			self->rst_h = *self->rst_p;
		}
	}
	if (fabsf(*self->rst_p) == 3) {
		reinit_gui = true;
	}

	const bool stereo = self->nchannels == 2;

	/* .. and go */
	for (uint32_t j = 0 ; j < n_samples; ++j) {
		float in;
		// TODO separate loop implementation for mono+stereo for efficiency
		if (stereo) {
			const float L = *(inL++);
			const float R = *(inR++);
			in = (L + R) / 2.0f;
		} else {
			in = *(inL++);
		}
				
		for(int i = 0; i < FILTER_COUNT; ++i) {
			const float v = bandpass_process(flt[i], in);
			const float s = v * v;
			val_f[i] += omega * (s - val_f[i]);
			if (val_f[i] > max_f[i]) max_f[i] = val_f[i];
		}
	}

	/* copy back variables and assign value */
	for(int i=0; i < FILTER_COUNT; ++i) {
		if (!isfinite(val_f[i])) val_f[i] = 0;
		if (!isfinite(max_f[i])) max_f[i] = 0;
		for (uint32_t j=0; j < flt[i]->filter_stages; ++j) {
			if (!isfinite(flt[i]->f[j].z[0])) flt[i]->f[j].z[0] = 0;
			if (!isfinite(flt[i]->f[j].z[1])) flt[i]->f[j].z[1] = 0;
		}
		self->val_f[i] = val_f[i] + 1e-20f;
		self->max_f[i] = max_f[i];

		const float vs = sqrtf(2. * val_f[i]);
		const float mx = sqrtf(2. * max_f[i]);
		*(self->spec[i]) = vs > .00001f ? 20.0 * log10f(vs) : -100.0;
		if (reinit_gui) {
			/* force parameter change */
			*(self->maxf[i]) = -500 - (rand() & 0xffff);
		} else {
			*(self->maxf[i]) = mx > .00001f ? 20.0 * log10f(mx) : -100.0;
		}
	}

	if (self->input[0] != self->output[0]) {
		memcpy(self->output[0], self->input[0], sizeof(float) * n_samples);
	}
	if (self->input[1] != self->output[1]) {
		memcpy(self->output[1], self->input[1], sizeof(float) * n_samples);
	}
}

static void
spectrum_cleanup(LV2_Handle instance)
{
	free(instance);
}

#define SPECTRDESC(ID, NAME) \
static const LV2_Descriptor descriptor ## ID = { \
	MTR_URI NAME, \
	spectrum_instantiate, \
	spectrum_connect_port, \
	NULL, \
	spectrum_run, \
	NULL, \
	spectrum_cleanup, \
	extension_data \
};

SPECTRDESC(Spectrum1, "spectr30mono");
SPECTRDESC(Spectrum2, "spectr30stereo");
