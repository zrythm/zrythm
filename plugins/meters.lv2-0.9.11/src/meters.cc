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

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#include "../jmeters/jmeterdsp.h"
#include "../jmeters/vumeterdsp.h"
#include "../jmeters/iec1ppmdsp.h"
#include "../jmeters/iec2ppmdsp.h"
#include "../jmeters/msppmdsp.h"
#include "../jmeters/stcorrdsp.h"
#include "../jmeters/truepeakdsp.h"
#include "../jmeters/kmeterdsp.h"
#include "../ebumeter/ebu_r128_proc.h"

#include "uris.h"
#include "uri2.h"

#define FREE_VARPORTS \
	free (self->mval); \
	free (self->mprev); \
	free (self->level); \
	free (self->input); \
	free (self->output); \
	free (self->peak);

#ifdef DISPLAY_INTERFACE
#include <cairo/cairo.h>
#include <pango/pangocairo.h>
#include "lv2_rgext.h"
#endif

using namespace LV2M;

typedef enum {
	MTR_REFLEVEL = 0,
	MTR_INPUT0   = 1,
	MTR_OUTPUT0  = 2,
	MTR_LEVEL0   = 3,
	MTR_INPUT1   = 4,
	MTR_OUTPUT1  = 5,
	MTR_LEVEL1   = 6,
	MTR_PEAK0    = 7,
	MTR_PEAK1    = 8,
	MTR_HOLD     = 9
} PortIndex;

enum MtrType {
	MT_NONE = 0,
	MT_BBC,
	MT_BM6,
	MT_EBU,
	MT_DIN,
	MT_NOR,
	MT_VU,
	MT_COR
};

typedef struct {
	float  rlgain;
	float  p_refl;
	float* reflvl;

	enum MtrType type;

	JmeterDSP **mtr;
	Stcorrdsp *cor;
	Msppmdsp  *bms[2];
	Ebu_r128_proc *ebu;

	Stcorrdsp *cor4[4];
	float* surc_a[4];
	float* surc_b[4];
	float* surc_c[4];

	float** level;
	float** input;
	float** output;
	float** peak;
	float* hold;

	float* mval;
	float* mprev;

	uint32_t chn;
	int      kstandard;
	float peak_max[2];
	float peak_hold;

	/* ebur specific */
  LV2_URID_Map* map;
  EBULV2URIs uris;

  LV2_Atom_Forge forge;
  LV2_Atom_Forge_Frame frame;
  const LV2_Atom_Sequence* control;
  LV2_Atom_Sequence* notify;

	double rate;
	bool ui_active;
	int follow_transport_mode; // bit1: follow start/stop, bit2: reset on re-start.

	bool tranport_rolling;
	bool ebu_integrating;
	bool dbtp_enable;
	bool bim_average;

	float *radarS, radarSC;
	float *radarM, radarMC;
	int radar_pos_cur, radar_pos_max;
	uint32_t radar_spd_cur, radar_spd_max;
	int radar_resync;
	uint64_t integration_time;
	bool send_state_to_ui;
	uint32_t ui_settings;
	float tp_max;

	int histM[HIST_LEN];
	int32_t histS[HIST_LEN];
	int hist_maxM;
	int hist_maxS;

	// signal distribution - use 'S' for 1st/left channel
	int hist_peakS;
	double hist_avgS;
	double hist_tmpS; // helper var for variance
	double hist_varS; // running variance

	// bitmeter

	float bim_min, bim_max;
	int bim_zero, bim_pos, bim_nan, bim_inf, bim_den;

	bool need_expose;
#ifdef DISPLAY_INTERFACE
	LV2_Inline_Display_Image_Surface surf;
	cairo_surface_t*         display;
	cairo_surface_t*         face;
	cairo_pattern_t*         mpat;
	LV2_Inline_Display*      queue_draw;
	uint32_t                 w, h;
#endif

} LV2meter;


#define MTRDEF(NAME, CLASS, TYPE, KM) \
	else if (!strcmp(descriptor->URI, MTR_URI NAME "mono")) { \
		self->chn = 1; \
		self->kstandard = KM; \
		self->type = TYPE; \
		self->mtr = (JmeterDSP **)malloc (self->chn * sizeof (JmeterDSP *)); \
		self->mtr[0] = new CLASS(); \
		static_cast<CLASS *>(self->mtr[0])->init(rate); \
	} \
	else if (!strcmp(descriptor->URI, MTR_URI NAME "stereo")) { \
		self->chn = 2; \
		self->kstandard = KM; \
		self->type = TYPE; \
		self->mtr = (JmeterDSP **)malloc (self->chn * sizeof (JmeterDSP *)); \
		self->mtr[0] = new CLASS(); \
		self->mtr[1] = new CLASS(); \
		static_cast<CLASS *>(self->mtr[0])->init(rate); \
		static_cast<CLASS *>(self->mtr[1])->init(rate); \
	}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	LV2meter* self = (LV2meter*)calloc(1, sizeof(LV2meter));

	if (!self) return NULL;

	if (!strcmp(descriptor->URI, MTR_URI "COR")) {
		self->type = MT_COR; \
		self->cor = new Stcorrdsp();
		self->cor->init(rate, 2e3f, 0.3f);
		self->chn = 2;
	}
	else if (!strcmp(descriptor->URI, MTR_URI "BBCM6")) {
		self->chn = 2;
		self->type = MT_BM6; \
		self->bms[0] = new Msppmdsp(-6);
		self->bms[1] = new Msppmdsp(-6);
		self->bms[0]->init(rate);
	}
	MTRDEF("VU",   Vumeterdsp,  MT_VU,   0)
	MTRDEF("BBC",  Iec2ppmdsp,  MT_BBC,  0)
	MTRDEF("EBU",  Iec2ppmdsp,  MT_EBU,  0)
	MTRDEF("DIN",  Iec1ppmdsp,  MT_DIN,  0)
	MTRDEF("NOR",  Iec1ppmdsp,  MT_NOR,  0)
	MTRDEF("dBTP", TruePeakdsp, MT_NONE, 0)
	MTRDEF("K12",  Kmeterdsp,   MT_NONE, 12)
	MTRDEF("K14",  Kmeterdsp,   MT_NONE, 14)
	MTRDEF("K20",  Kmeterdsp,   MT_NONE, 20)
	else {
		free(self);
		return NULL;
	}

#ifdef DISPLAY_INTERFACE
	for (int i=0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_INLINEDISPLAY__queue_draw)) {
			self->queue_draw = (LV2_Inline_Display*) features[i]->data;
		}
	}
#endif

	// some ports are re-used, e.g #5 aka output[1] for mono K-meters
	uint32_t pca = self->chn > 2 ? self->chn : 2;

	self->mval   = (float*) calloc (pca, sizeof (float));
	self->mprev  = (float*) calloc (pca, sizeof (float));
	self->level  = (float**) calloc (pca, sizeof (float*));
	self->input  = (float**) calloc (pca, sizeof (float*));
	self->output = (float**) calloc (pca, sizeof (float*));
	self->peak   = (float**) calloc (pca, sizeof (float*));

	self->rlgain = 1.0;
	self->p_refl = -9999;

	self->peak_max[0] = 0;
	self->peak_max[1] = 0;
	self->peak_hold   = 0;

	return (LV2_Handle)self;
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	LV2meter* self = (LV2meter*)instance;

	switch ((PortIndex)port) {
	case MTR_REFLEVEL:
		self->reflvl = (float*) data;
		break;
	case MTR_INPUT0:
		self->input[0] = (float*) data;
		break;
	case MTR_OUTPUT0:
		self->output[0] = (float*) data;
		break;
	case MTR_LEVEL0:
		self->level[0] = (float*) data;
		break;
	case MTR_INPUT1:
		self->input[1] = (float*) data;
		break;
	case MTR_OUTPUT1:
		self->output[1] = (float*) data;
		break;
	case MTR_LEVEL1:
		self->level[1] = (float*) data;
		break;
	case MTR_PEAK0:
		self->peak[0] = (float*) data;
		break;
	case MTR_PEAK1:
		self->peak[1] = (float*) data;
		break;
	case MTR_HOLD:
		self->hold = (float*) data;
		break;
	}
}

static void
run(LV2_Handle instance, uint32_t n_samples)
{
	LV2meter* self = (LV2meter*)instance;

	if (self->p_refl != *self->reflvl) {
		self->p_refl = *self->reflvl;
		self->rlgain = powf (10.0f, 0.05f * (self->p_refl + 18.0));
	}

	for (uint32_t c = 0; c < self->chn; ++c) {

		float* const input  = self->input[c];
		float* const output = self->output[c];

		self->mtr[c]->process(input, n_samples);

		self->mval[c] = *self->level[c] = self->rlgain * self->mtr[c]->read();
		if (self->mval[c] != self->mprev[c]) {
			self->need_expose = true;
			self->mprev[c] = self->mval[c];
		}

		if (input != output) {
			memcpy(output, input, sizeof(float) * n_samples);
		}
	}
#ifdef DISPLAY_INTERFACE
	if (self->need_expose && self->queue_draw) {
		self->need_expose = false;
		self->queue_draw->queue_draw (self->queue_draw->handle);
	}
#endif
}

static void
kmeter_run(LV2_Handle instance, uint32_t n_samples)
{
	LV2meter* self = (LV2meter*)instance;
	bool reinit_gui = false;

	/* re-use port 0 to request/notify UI about
	 * peak values - force change to ports */
	if (self->p_refl != *self->reflvl) {

		/* reset peak-hold */
		if (fabsf(*self->reflvl) < 3) {
			self->peak_hold = 0;
			reinit_gui = true;
			for (uint32_t c = 0; c < self->chn; ++c) {
				self->mtr[c]->reset();
			}
		}
		/* re-notify UI, until UI acknowledges */
		if (fabsf(*self->reflvl) == 3) {
			reinit_gui = true;
		} else {
			self->p_refl = *self->reflvl;
		}
	}

	for (uint32_t c = 0; c < self->chn; ++c) {

		float* const input  = self->input[c];
		float* const output = self->output[c];

		self->mtr[c]->process(input, n_samples);

		if (input != output) {
			memcpy(output, input, sizeof(float) * n_samples);
		}
	}

	if (reinit_gui) {
		/* force parameter change */
		if (self->chn == 1) {
			*self->output[1] = -1 - (rand() & 0xffff); // portindex 5
		} else if (self->chn == 2) {
			*self->hold = -1 - (rand() & 0xffff);
		}
		return;
	}

	if (self->chn == 1) {
		float m, p;
		static_cast<Kmeterdsp*>(self->mtr[0])->read(m, p);
		*self->level[0] = self->rlgain * m;
		*self->input[1] = self->rlgain * p; // portindex 4
		if (*self->input[1] > self->peak_hold) self->peak_hold = *self->input[1];
		*self->output[1] = self->peak_hold; // portindex 5
	} else if (self->chn == 2) {
		float m, p;
		static_cast<Kmeterdsp*>(self->mtr[0])->read(m, p);
		*self->level[0] = self->rlgain * m;
		*self->peak[0] = self->rlgain * p;
		if (*self->peak[0] > self->peak_hold) self->peak_hold = *self->peak[0];

		static_cast<Kmeterdsp*>(self->mtr[1])->read(m, p);
		*self->level[1] = self->rlgain * m;
		*self->peak[1] = self->rlgain * p;
		if (*self->peak[1] > self->peak_hold) self->peak_hold = *self->peak[1];

		*self->hold = self->peak_hold;
	}

#ifdef DISPLAY_INTERFACE
	for (uint32_t c = 0; c < self->chn; ++c) {
		self->mval[c] = *self->level[c];
		// TODO: IFF difference >= 1/2 px at given self->w
		if (self->mval[c] != self->mprev[c]) {
			self->need_expose = true;
			self->mprev[c] = self->mval[c];
		}
	}

	if (self->need_expose && self->queue_draw) {
		self->need_expose = false;
		self->queue_draw->queue_draw (self->queue_draw->handle);
	}
#endif
}


static void
cleanup(LV2_Handle instance)
{
	LV2meter* self = (LV2meter*)instance;
	for (uint32_t c = 0; c < self->chn; ++c) {
		delete self->mtr[c];
	}
	FREE_VARPORTS;
#ifdef DISPLAY_INTERFACE
	if (self->display) cairo_surface_destroy(self->display);
	if (self->face) cairo_surface_destroy(self->face);
	if (self->mpat) cairo_pattern_destroy(self->mpat);
#endif
	free (self->mtr);
	free(instance);
}

static void
dbtp_run(LV2_Handle instance, uint32_t n_samples)
{
	LV2meter* self = (LV2meter*)instance;
	bool reinit_gui = false;

	/* re-use port 0 to request/notify UI about
	 * peak values - force change to ports */
	if (self->p_refl != *self->reflvl) {
		/* reset peak-hold */
		if (fabsf(*self->reflvl) < 3) {
			reinit_gui = true;
			self->peak_max[0] = 0;
			self->peak_max[1] = 0;
			for (uint32_t c = 0; c < self->chn; ++c) {
				self->mtr[c]->reset();
			}
		}
		/* re-notify UI, until UI acknowledges */
		if (fabsf(*self->reflvl) != 3) {
			self->p_refl = *self->reflvl;
		}
	}
	if (fabsf(*self->reflvl) == 3) {
		reinit_gui = true;
	}

	for (uint32_t c = 0; c < self->chn; ++c) {

		float* const input  = self->input[c];
		float* const output = self->output[c];

		self->mtr[c]->process(input, n_samples);

		if (input != output) {
			memcpy(output, input, sizeof(float) * n_samples);
		}
	}

	if (reinit_gui) {
		/* force parameter change */
		if (self->chn == 1) {
			*self->level[0] = -500 - (rand() & 0xffff);
			*self->input[1] = -500 - (rand() & 0xffff); // portindex 4
		} else if (self->chn == 2) {
			*self->level[0] = -500 - (rand() & 0xffff);
			*self->level[1] = -500 - (rand() & 0xffff);
			*self->peak[0] = -500 - (rand() & 0xffff);
			*self->peak[1] = -500 - (rand() & 0xffff);
		}
		return;
	}

	if (self->chn == 1) {
		float m, p;
		static_cast<TruePeakdsp*>(self->mtr[0])->read(m, p);
		if (self->peak_max[0] < self->rlgain * p) { self->peak_max[0] = self->rlgain * p; }
		*self->level[0] = self->rlgain * m;
		*self->input[1] = self->peak_max[0]; // portindex 4
	} else if (self->chn == 2) {
		float m, p;
		static_cast<TruePeakdsp*>(self->mtr[0])->read(m, p);
		if (self->peak_max[0] < self->rlgain * p) { self->peak_max[0] = self->rlgain * p; }
		*self->level[0] = self->rlgain * m;
		*self->peak[0] = self->peak_max[0];
		static_cast<TruePeakdsp*>(self->mtr[1])->read(m, p);
		if (self->peak_max[1] < self->rlgain * p) { self->peak_max[1] = self->rlgain * p; }
		*self->level[1] = self->rlgain * m;
		*self->peak[1] = self->peak_max[1];
	}
}


static void
cor_run(LV2_Handle instance, uint32_t n_samples)
{
	LV2meter* self = (LV2meter*)instance;

	self->cor->process(self->input[0], self->input[1] , n_samples);
	self->mval[0] = *self->level[0] = self->cor->read();

	if (self->mval[0] != self->mprev[0]) {
		self->need_expose = true;
		self->mprev[0] = self->mval[0];
	}

	if (self->input[0] != self->output[0]) {
		memcpy(self->output[0], self->input[0], sizeof(float) * n_samples);
	}
	if (self->input[1] != self->output[1]) {
		memcpy(self->output[1], self->input[1], sizeof(float) * n_samples);
	}
#ifdef DISPLAY_INTERFACE
	if (self->need_expose && self->queue_draw) {
		self->need_expose = false;
		self->queue_draw->queue_draw (self->queue_draw->handle);
	}
#endif
}

static void
cor_cleanup(LV2_Handle instance)
{
	LV2meter* self = (LV2meter*)instance;
	delete self->cor;
	FREE_VARPORTS;
#ifdef DISPLAY_INTERFACE
	if (self->display) cairo_surface_destroy(self->display);
	if (self->face) cairo_surface_destroy(self->face);
	if (self->mpat) cairo_pattern_destroy(self->mpat);
#endif
	free(instance);
}

static void
bbcm_run(LV2_Handle instance, uint32_t n_samples)
{
	LV2meter* self = (LV2meter*)instance;

	if (self->p_refl != *self->reflvl) {
		self->p_refl = *self->reflvl;
		self->rlgain = powf (10.0f, 0.05f * (self->p_refl + 18.0));
	}

	self->bms[0]->processM(self->input[0], self->input[1], n_samples);
	self->mval[0] = *self->level[0] = self->rlgain * self->bms[0]->read();

	self->bms[1]->processS(self->input[0], self->input[1], n_samples);
	self->mval[1] = *self->level[1] = self->rlgain * self->bms[1]->read();

	if (self->mval[0] != self->mprev[0] || self->mval[1] != self->mprev[1]) {
		self->need_expose = true;
		self->mprev[0] = self->mval[1];
		self->mprev[0] = self->mval[1];
	}

	if (self->input[0] != self->output[0]) {
		memcpy(self->output[0], self->input[0], sizeof(float) * n_samples);
	}
	if (self->input[1] != self->output[1]) {
		memcpy(self->output[1], self->input[1], sizeof(float) * n_samples);
	}
#ifdef DISPLAY_INTERFACE
	if (self->need_expose && self->queue_draw) {
		self->need_expose = false;
		self->queue_draw->queue_draw (self->queue_draw->handle);
	}
#endif
}

static void
bbcm_cleanup(LV2_Handle instance)
{
	LV2meter* self = (LV2meter*)instance;
	delete self->bms[0];
	delete self->bms[1];
	FREE_VARPORTS;
#ifdef DISPLAY_INTERFACE
	if (self->display) cairo_surface_destroy(self->display);
	if (self->face) cairo_surface_destroy(self->face);
	if (self->mpat) cairo_pattern_destroy(self->mpat);
#endif
	free(instance);
}


#ifdef WITH_SIGNATURE
#define RTK_URI MTR_URI
#include "gpg_init.c"
#include WITH_SIGNATURE
struct license_info license_infos = {
	"x42-Meters",
	"http://x42-plugins.com/x42/x42-meters"
};
#include "gpg_lv2ext.c"
#endif


const void*
extension_data(const char* uri)
{
#ifdef WITH_SIGNATURE
	LV2_LICENSE_EXT_C
#endif
	return NULL;
}

#ifdef DISPLAY_INTERFACE
#include "dpy_needle.c"
#include "dpy_bargraph.c"
#endif

const void*
extension_data_needle(const char* uri)
{
#ifdef DISPLAY_INTERFACE
	static const LV2_Inline_Display_Interface display  = { needle_render };
	if (!strcmp(uri, LV2_INLINEDISPLAY__interface)) {
#if (defined _WIN32 && defined RTK_STATIC_INIT)
		static int once = 0;
		if (!once) {once = 1; gobject_init_ctor();}
#endif
		return &display;
	}
#endif
	return extension_data (uri);
}

const void*
extension_data_kmeter(const char* uri)
{
#ifdef DISPLAY_INTERFACE
	static const LV2_Inline_Display_Interface display  = { bargraph_render };
	if (!strcmp(uri, LV2_INLINEDISPLAY__interface)) {
#if (defined _WIN32 && defined RTK_STATIC_INIT)
		static int once = 0;
		if (!once) {once = 1; gobject_init_ctor();}
#endif
		return &display;
	}
#endif
	return extension_data (uri);
}


//#ifdef DEBUG_SPECTR
#include "spectr.c"

#include "ebulv2.cc"
#include "goniometerlv2.c"
#include "spectrumlv2.c"
#include "xfer.c"
#include "dr14.c"
#include "sigdistlv2.c"
#include "bitmeter.c"
#include "surmeter.c"

#define mkdesc(ID, NAME, RUN, EXT) \
static const LV2_Descriptor descriptor ## ID = { \
	MTR_URI NAME, \
	instantiate, \
	connect_port, \
	NULL, \
	RUN, \
	NULL, \
	cleanup, \
	EXT \
};

mkdesc(0, "VUmono",   run, extension_data_needle)
mkdesc(1, "VUstereo", run, extension_data_needle)
mkdesc(2, "BBCmono",  run, extension_data_needle)
mkdesc(3, "BBCstereo",run, extension_data_needle)
mkdesc(4, "EBUmono",  run, extension_data_needle)
mkdesc(5, "EBUstereo",run, extension_data_needle)
mkdesc(6, "DINmono",  run, extension_data_needle)
mkdesc(7, "DINstereo",run, extension_data_needle)
mkdesc(8, "NORmono",  run, extension_data_needle)
mkdesc(9, "NORstereo",run, extension_data_needle)

mkdesc(14,"dBTPmono",   dbtp_run, extension_data)
mkdesc(15,"dBTPstereo", dbtp_run, extension_data)

mkdesc(K12M,"K12mono", kmeter_run, extension_data_kmeter)
mkdesc(K14M,"K14mono", kmeter_run, extension_data_kmeter)
mkdesc(K20M,"K20mono", kmeter_run, extension_data_kmeter)
mkdesc(K12S,"K12stereo", kmeter_run, extension_data_kmeter)
mkdesc(K14S,"K14stereo", kmeter_run, extension_data_kmeter)
mkdesc(K20S,"K20stereo", kmeter_run, extension_data_kmeter)

static const LV2_Descriptor descriptorCor = {
	MTR_URI "COR",
	instantiate,
	connect_port,
	NULL,
	cor_run,
	NULL,
	cor_cleanup,
	extension_data_needle
};

static const LV2_Descriptor descriptorBBCMS = {
	MTR_URI "BBCM6",
	instantiate,
	connect_port,
	NULL,
	bbcm_run,
	NULL,
	bbcm_cleanup,
	extension_data_needle
};


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
	case  3: return &descriptor3;
	case  4: return &descriptor4;
	case  5: return &descriptor5;
	case  6: return &descriptor6;
	case  7: return &descriptor7;
	case  8: return &descriptor8;
	case  9: return &descriptor9;
	case 10: return &descriptorCor;
	case 11: return &descriptorEBUr128;
	case 12: return &descriptorGoniometer;
	case 13: return &descriptorSpectrum1;
	case 14: return &descriptor14;
	case 15: return &descriptor15;

	case 16: return &descriptorK12M;
	case 17: return &descriptorK14M;
	case 18: return &descriptorK20M;
	case 19: return &descriptorK12S;
	case 20: return &descriptorK14S;
	case 21: return &descriptorK20S;

	case 22: return &descriptorSpectrum2;
	case 23: return &descriptorMultiPhase2;
	case 24: return &descriptorStereoScope;
	case 25: return &descriptorDR14_1;
	case 26: return &descriptorDR14_2;
	case 27: return &descriptorTPRMS_1;
	case 28: return &descriptorTPRMS_2;
	case 29: return &descriptorSDH;
	case 30: return &descriptorBBCMS;
	case 31: return &descriptorBIM;
	case 32: return &descriptorSUR8;
	case 33: return &descriptorSUR7;
	case 34: return &descriptorSUR6;
	case 35: return &descriptorSUR5;
	case 36: return &descriptorSUR4;
	case 37: return &descriptorSUR3;
	default: return NULL;
	}
}

#ifdef _WIN32
static void __attribute__((constructor)) x42_init() {
	        pthread_win32_process_attach_np();
}

static void __attribute__((destructor)) x42_fini() {
	        pthread_win32_process_detach_np();
}
#endif
