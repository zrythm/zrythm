/* meter.lv2
 *
 * Copyright (C) 2013,2014 Robin Gareus <robin@gareus.org>
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

#ifndef MAX
#define MAX(A,B) ( (A) > (B) ? (A) : (B) )
#endif
#ifndef MIN
#define MIN(A,B) ( (A) < (B) ? (A) : (B) )
#endif

typedef enum {
	DR_CONTROL = 0,
	DR_HOST_TRANSPORT,
	DR_RESET,
	DR_BLKCNT,
	DR_INPUT0,
	DR_OUTPUT0,
	DR_V_PEAK0, DR_M_PEAK0,
	DR_V_RMS0, DR_M_RMS0,
	DR_DR0,
	DR_INPUT1,
	DR_OUTPUT1,
	DR_V_PEAK1, DR_M_PEAK1,
	DR_V_RMS1, DR_M_RMS1,
	DR_DR1,
	DR_TOTAL, // > 1 channel
} DRPortIndex;

#define DR_CHANNELS (2)
#define DR_HISTBINS (8000) // -80dB .. 0dB in .01dB steps

typedef struct {
	/* ports */
  const LV2_Atom_Sequence* control;
	float* p_follow_host_transport;
	float* p_reset_button;
	float* p_block_count;

	float* p_input[DR_CHANNELS];
	float* p_output[DR_CHANNELS];

	float *p_v_rms[DR_CHANNELS];
	float *p_v_peak[DR_CHANNELS];
	float *p_m_rms[DR_CHANNELS];
	float *p_m_peak[DR_CHANNELS];
	float *p_dr[DR_CHANNELS];
	float *p_dr_total;

	/* URI map for time+position */
  EBULV2URIs uris;

	/* settings */
	uint32_t n_channels;
	double rate;
	uint64_t n_sample_cnt;

	/* parameters */
	bool follow_host_transport; // reset on re-start.

	/* state */
	float  m_dbtp[DR_CHANNELS];
	float  m_peak[DR_CHANNELS];
	float  m_rms[DR_CHANNELS];
	bool tranport_rolling;
	uint64_t sample_count;

	Kmeterdsp *km[DR_CHANNELS];
	TruePeakdsp *tp[DR_CHANNELS];

	float rms_sum[DR_CHANNELS];
	float peak_cur[DR_CHANNELS];
	float peak_hist[DR_CHANNELS][2];
	uint64_t num_fragments;
	uint32_t *hist[DR_CHANNELS];
	bool reinit_gui;
	bool dr_operation_mode; // true for DR14 mode, false: dBTP+RMS only

} LV2dr14;

/******************************************************************************
 * LV2 callbacks
 */

static LV2_Handle
dr14_instantiate(
		const LV2_Descriptor*     descriptor,
		double                    rate,
		const char*               bundle_path,
		const LV2_Feature* const* features)
{
	uint32_t n_channels;
	bool dr_operation_mode;
	if (!strcmp(descriptor->URI, MTR_URI "dr14stereo")) {
		n_channels = 2;
		dr_operation_mode = true;
	}
	else if (!strcmp(descriptor->URI, MTR_URI "dr14mono")) {
		n_channels = 1;
		dr_operation_mode = true;
	}
	else if (!strcmp(descriptor->URI, MTR_URI "TPnRMSstereo")) {
		n_channels = 2;
		dr_operation_mode = false;
	}
	else if (!strcmp(descriptor->URI, MTR_URI "TPnRMSmono")) {
		n_channels = 1;
		dr_operation_mode = false;
	}
	else { return NULL; }

  LV2_URID_Map* map = NULL;
	for (int i=0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID__map)) {
			map = (LV2_URID_Map*)features[i]->data;
		}
	}

	if (!map) {
		fprintf(stderr, "DR14LV2 error: Host does not support urid:map\n");
		return NULL;
	}

	LV2dr14* self = (LV2dr14*)calloc(1, sizeof(LV2dr14));
	if (!self) return NULL;

	self->n_channels = n_channels;
	self->dr_operation_mode = dr_operation_mode;
	self->rate = rate;
	self->reinit_gui = false;

	map_eburlv2_uris(map, &self->uris);

	self->follow_host_transport = true;
	self->tranport_rolling = false;
	self->n_sample_cnt = rintf(rate * 3.0);
	self->sample_count = 0;

	for (uint32_t c = 0; c < self->n_channels; ++c) {
		self->km[c] = new Kmeterdsp();
		self->tp[c] = new TruePeakdsp();
		self->km[c]->init(rate);
		self->tp[c]->init(rate);
		self->m_rms[c] = -81;
		self->m_peak[c] = -81;
		if (dr_operation_mode) {
			self->hist[c] = (uint32_t*) calloc(DR_HISTBINS, sizeof(uint32_t));
		}
	}

	return (LV2_Handle)self;
}

static void
dr14_connect_port(LV2_Handle instance, uint32_t port, void* data)
{
	LV2dr14* self = (LV2dr14*)instance;
	switch (port) {
	case DR_CONTROL:
		self->control = (const LV2_Atom_Sequence*)data;
		break;
	case DR_HOST_TRANSPORT:
		self->p_follow_host_transport = (float*) data;
		break;
	case DR_RESET:
		self->p_reset_button = (float*) data;
		break;
	case DR_BLKCNT:
		self->p_block_count = (float*) data;
		break;
	case DR_INPUT0:
		self->p_input[0] = (float*) data;
		break;
	case DR_OUTPUT0:
		self->p_output[0] = (float*) data;
		break;
	case DR_V_RMS0:
		self->p_v_rms[0] = (float*) data;
		break;
	case DR_M_RMS0:
		self->p_m_rms[0] = (float*) data;
		break;
	case DR_V_PEAK0:
		self->p_v_peak[0] = (float*) data;
		break;
	case DR_M_PEAK0:
		self->p_m_peak[0] = (float*) data;
		break;
	case DR_DR0:
		self->p_dr[0] = (float*) data;
		break;
	case DR_TOTAL:
		self->p_dr_total = (float*) data;
		break;
	case DR_INPUT1:
		self->p_input[1] = (float*) data;
		break;
	case DR_OUTPUT1:
		self->p_output[1] = (float*) data;
		break;
	case DR_V_RMS1:
		self->p_v_rms[1] = (float*) data;
		break;
	case DR_M_RMS1:
		self->p_m_rms[1] = (float*) data;
		break;
	case DR_V_PEAK1:
		self->p_v_peak[1] = (float*) data;
		break;
	case DR_M_PEAK1:
		self->p_m_peak[1] = (float*) data;
		break;
	case DR_DR1:
		self->p_dr[1] = (float*) data;
		break;
	default:
		break;
	}
}

static inline float coeff_to_db(const float coeff) {
	if (coeff < .0001) return -80;
	return 20 * log10f(coeff);
}

static inline float db_to_coeff(const float db) {
	if (db <= -80) return 0;
	return powf(10, 0.05 * db);
}

static void
reset_peaks(LV2dr14* self) {
	for (uint32_t c = 0; c < self->n_channels; ++c) {
		self->m_peak[c] = -81;
		self->m_rms[c] = -81;
		self->m_dbtp[c] = 0;
		self->rms_sum[c] = 0;
		self->peak_cur[c] = 0;
		self->peak_hist[c][0] = self->peak_hist[c][1] = 0;
		self->km[c]->reset();
		if (self->dr_operation_mode) {
			memset(self->hist[c], 0, DR_HISTBINS * sizeof(int32_t));
		}
	}
	self->sample_count = 0;
	self->num_fragments = 0;
}

static void
parse_time_position(LV2dr14* self, const LV2_Atom_Object* obj)
{
	const EBULV2URIs* uris = &self->uris;

	// Received new transport position/speed
	LV2_Atom *speed = NULL;
	lv2_atom_object_get(obj,
	                    uris->time_speed, &speed,
	                    NULL);
	if (speed && speed->type == uris->atom_Float) {
		float ts = ((LV2_Atom_Float*)speed)->body;
		if (ts != 0 && !self->tranport_rolling) {
			if (self->follow_host_transport) {
				reset_peaks(self);
			}
		}
		self->tranport_rolling = (ts != 0);
	}
}

static void
dr14_calc_rms_score(LV2dr14* self)
{
	bool silent = true;
	/* ignore silence, don't add to histogram */
	for (uint32_t c = 0; c < self->n_channels; ++c) {
		if (self->rms_sum[c] > 1e-9 * (float) self->n_sample_cnt) silent = false;
	}
	if (silent) {
		for (uint32_t c = 0; c < self->n_channels; ++c) {
			self->rms_sum[c] = 0;
			//self->peak_cur[c] = 0;
		}
		return;
	}

	/* top 20% -> 1/5 */
	self->num_fragments++;
	uint32_t m_cut = MAX(1, floorf(self->num_fragments / 5.0));

	for (uint32_t c = 0; c < self->n_channels; ++c) {
		float rms = sqrt(2.f * self->rms_sum[c] / (float) self->n_sample_cnt);
		self->rms_sum[c] = 0;

		/* add to histogram bin -80dB .. 0dB */
		int bin = rintf(100.f * (80.f + coeff_to_db(rms))) - 1;
		if (bin >= DR_HISTBINS) bin = DR_HISTBINS -1;
		if (bin > 0) self->hist[c][bin]++;

		uint32_t n_cut = 0;
		float rms_score = 0;

		/* find top 20% - calc RMS average via coeffiencnts (not dB) */
		if (self->num_fragments > 2) {
			for (int32_t b = DR_HISTBINS -1; b > 0 && n_cut < m_cut; --b) {
				const uint32_t bc = self->hist[c][b];
				if (bc == 0)  continue;
				const float cd = db_to_coeff((b - DR_HISTBINS + 1)/100.0);
				rms_score += cd * cd * (float) bc;
				n_cut += bc;
			}
		}
		if (n_cut > 0) {
			rms_score = coeff_to_db(sqrtf(rms_score / n_cut));
		} else {
			rms_score = -81;
		}
		self->m_rms[c] = rms_score;

		/* various web-forum and implementations hint that the DR meter
		 * actually uses the 2nd highest peak (raw data not dbTP) of all
		 * 3sec windows. --  Whatever.
		 */
		if (self->peak_cur[c] >= self->peak_hist[c][0]) {
			self->peak_hist[c][1] = self->peak_hist[c][0];
			self->peak_hist[c][0] = self->peak_cur[c];
		} else if (self->peak_cur[c] > self->peak_hist[c][1]) {
			self->peak_hist[c][1] = self->peak_cur[c];
		}
		self->peak_cur[c] = 0;

		if (self->num_fragments > 2) {
			self->m_peak[c] = coeff_to_db(self->peak_hist[c][1]);
		} else {
			self->m_peak[c] = -81;
		}
	}
}

static void
dr14_run(LV2_Handle instance, uint32_t n_samples)
{
	LV2dr14* self = (LV2dr14*)instance;

	self->follow_host_transport = (*self->p_follow_host_transport != 0);

	/* Process events (reset from GUI, transport from host) */
	LV2_Atom_Event* ev = lv2_atom_sequence_begin(&(self->control)->body);
	while(!lv2_atom_sequence_is_end(&(self->control)->body, (self->control)->atom.size, ev)) {
		if (ev->body.type == self->uris.atom_Blank || ev->body.type == self->uris.atom_Object) {
			const LV2_Atom_Object* obj = (LV2_Atom_Object*)&ev->body;
			if (obj->body.otype == self->uris.time_Position) {
				parse_time_position(self, obj);
			}
			if (obj->body.otype == self->uris.mtr_dr14reset) {
				reset_peaks(self);
			}
			if (obj->body.otype == self->uris.mtr_meters_on) {
				self->reinit_gui = true;
			}
			if (obj->body.otype == self->uris.mtr_meters_off) {
				self->reinit_gui = false;
			}
		}
		ev = lv2_atom_sequence_next(ev);
	}

	if (*self->p_reset_button != 0 ) {
		reset_peaks(self);
	}

	/* calculate
	 * - RMS for bar-graph display
	 * - dBTP peak for DR calculation
	 * - dBTP filtered for bar-graph display
	 */
	for (uint32_t c = 0; c < self->n_channels; ++c) {
		self->km[c]->process(self->p_input[c], n_samples);
		self->tp[c]->process(self->p_input[c], n_samples);
	}

	/* DR specs says RMS is to be calculated over a 3 second
	 * non-overlapping window. Aaarg! well, this is not the place
	 * to question the stupidity of the spec. let's do it:
	 */
	uint64_t scnt = self->sample_count;
	const uint64_t slmt = self->n_sample_cnt;

	if (self->dr_operation_mode) {
		// TODO: optimize, unroll loop to remaining samples / block
		for (uint32_t s = 0; s < n_samples; ++s) {
			for (uint32_t c = 0; c < self->n_channels; ++c) {
				const float v = self->p_input[c][s];
				self->rms_sum[c] += v * v;
				self->peak_cur[c] = MAX(self->peak_cur[c], v);
			}
			if (++scnt > slmt) {
				dr14_calc_rms_score(self);
				scnt = 0;
			}
		}
		self->sample_count = scnt;
	}


	/* assing values to ports, clap to ranges,
	 * average DR value forall channels
	 */
	float dr_total = 0;
	int   dr_valid = 0;
	for (uint32_t c = 0; c < self->n_channels; ++c) {
		float rv, rp;
		float pv, pp;
		self->tp[c]->read(pv, pp);
		self->km[c]->read(rv, rp);
		self->m_dbtp[c] = MAX(self->m_dbtp[c], pp);

		/* assign output data to ports */
		*self->p_v_rms[c]  = coeff_to_db(rv);
		*self->p_v_peak[c] = coeff_to_db(pv);
		*self->p_m_peak[c] = coeff_to_db(self->m_dbtp[c]);

		if (self->dr_operation_mode) {
			const float rdb = self->m_rms[c];
			const float pdb = self->m_peak[c];
			const float dr = MIN(0,pdb) - rdb;
			if (rdb > -80 && pdb > -80) {
				dr_total += dr;
				dr_valid++;
			}
			*self->p_dr[c]     = (rdb > -80 && pdb > -80) ? MAX(1, MIN(20, dr)) : 21;
			*self->p_m_rms[c]  = rdb;
		} else {
			*self->p_m_rms[c]  = coeff_to_db(rp);
		}
	}

	if (self->n_channels > 1 && self->dr_operation_mode) {
		if (dr_valid > 0) {
			*self->p_dr_total = MAX(1, MIN(20, dr_total / (float) dr_valid));
		} else {
			*self->p_dr_total = 21;
		}
	}

	*self->p_block_count = 3.0 * self->num_fragments;

	if (self->reinit_gui) {
		if (self->n_channels > 1 && self->dr_operation_mode) {
			*self->p_dr_total = 21;
		}
		for (uint32_t c = 0; c < self->n_channels; ++c) {
			*self->p_m_peak[c] = -100;
			*self->p_m_rms[c]  = -100;
			if (self->dr_operation_mode) {
				*self->p_dr[c]     = 21;
			}
		}
		*self->p_block_count = -1 - (rand() & 0xffff);
	}

	if (self->p_input[0] != self->p_output[0]) {
		memcpy(self->p_output[0], self->p_input[0], sizeof(float) * n_samples);
	}
	if (self->p_input[1] != self->p_output[1]) {
		memcpy(self->p_output[1], self->p_input[1], sizeof(float) * n_samples);
	}
}

static void
dr14_cleanup(LV2_Handle instance)
{
	LV2dr14* self = (LV2dr14*)instance;

	for (uint32_t c = 0; c < self->n_channels; ++c) {
		delete self->km[c];
		delete self->tp[c];
		if (self->dr_operation_mode) {
			free(self->hist[c]);
		}
	}
	free(instance);
}

#define DR14DESC(ID, NAME) \
static const LV2_Descriptor descriptor ## ID = { \
	MTR_URI NAME, \
	dr14_instantiate, \
	dr14_connect_port, \
	NULL, \
	dr14_run, \
	NULL, \
	dr14_cleanup, \
	extension_data \
};

DR14DESC(DR14_1, "dr14mono");
DR14DESC(DR14_2, "dr14stereo");

DR14DESC(TPRMS_1, "TPnRMSmono");
DR14DESC(TPRMS_2, "TPnRMSstereo");
