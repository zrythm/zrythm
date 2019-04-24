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

#include "lv2/lv2plug.in/ns/ext/state/state.h"

/* static functions to be included in meters.cc
 *
 * broken out ebu-r128 related LV2 functions
 */

typedef enum {
	EBU_CONTROL  = 0,
	EBU_NOTIFY   = 1,
	EBU_INPUT0   = 2,
	EBU_OUTPUT0  = 3,
	EBU_INPUT1   = 4,
	EBU_OUTPUT1  = 5,
} EBUPortIndex;


/******************************************************************************
 * helper functions
 */

static void ebu_reset(LV2meter* self) {
	self->ebu->integr_reset();
	forge_kvcontrolmessage(&self->forge, &self->uris, self->uris.mtr_control, CTL_LV2_RESETRADAR, 0);
	for (int i=0; i < self->radar_pos_max; ++i) {
		self->radarS[i] = -INFINITY;
		self->radarM[i] = -INFINITY;
	}
	for (int i=0; i < HIST_LEN; ++i) {
		self->histM[i] = 0;
		self->histS[i] = 0;
	}
	self->radar_pos_cur = 0;
	self->integration_time = 0;
	self->hist_maxM = 0;
	self->hist_maxS = 0;
	self->tp_max = -INFINITY;
}

static void ebu_integrate(LV2meter* self, bool on) {
	if (self->ebu_integrating == on) return;
	if (on) {
		if (self->follow_transport_mode & 2) {
			ebu_reset(self);
		}
		self->ebu->integr_start(); self->ebu_integrating=true;
	} else {
		self->ebu->integr_pause(); self->ebu_integrating=false;
	}
}

static void ebu_set_radarspeed(LV2meter* self, float seconds) {
	self->radar_spd_max = rint(seconds * self->rate / self->radar_pos_max);
	if (self->radar_spd_max < 4096) self->radar_spd_max = 4096;
}

/**
 * Update transport state.
 * This is called by run() when a time:Position is received.
 */
static void
update_position(LV2meter* self, const LV2_Atom_Object* obj)
{
	const EBULV2URIs* uris = &self->uris;

	// Received new transport position/speed
	LV2_Atom *speed = NULL;
	LV2_Atom *frame = NULL;
	lv2_atom_object_get(obj,
	                    uris->time_speed, &speed,
	                    uris->time_frame, &frame,
	                    NULL);
	if (speed && speed->type == uris->atom_Float) {
		float ts = ((LV2_Atom_Float*)speed)->body;
		if (ts != 0 && !self->tranport_rolling) {
			if (self->follow_transport_mode & 1) { ebu_integrate(self, true); }
		}
		if (ts == 0 && self->tranport_rolling) {
			if (self->follow_transport_mode & 1) { ebu_integrate(self, false); }
		}
		self->tranport_rolling = (ts != 0);
	}
#if 0
	if (frame && frame->type == uris->atom_Long) {
		self->pos_frame = ((LV2_Atom_Long*)frame)->body;
	}
#endif
}



/******************************************************************************
 * LV2 callbacks
 */

static LV2_Handle
ebur128_instantiate(
		const LV2_Descriptor*     descriptor,
		double                    rate,
		const char*               bundle_path,
		const LV2_Feature* const* features)
{
	LV2meter* self = (LV2meter*)calloc(1, sizeof(LV2meter));
	if (!self) return NULL;

	if (strcmp(descriptor->URI, MTR_URI "EBUr128")) {
		free(self);
		return NULL;
	}

	for (int i=0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID__map)) {
			self->map = (LV2_URID_Map*)features[i]->data;
		}
	}

	if (!self->map) {
		fprintf(stderr, "EBUrLV2 error: Host does not support urid:map\n");
		free(self);
		return NULL;
	}

	map_eburlv2_uris(self->map, &self->uris);
	lv2_atom_forge_init(&self->forge, self->map);

	self->chn = 2;
	self->input  = (float**) calloc (self->chn, sizeof (float*));
	self->output = (float**) calloc (self->chn, sizeof (float*));

	self->rate = rate;
	self->ui_active = false;
	self->follow_transport_mode = 0; // 3
	self->tranport_rolling = false;
	self->ebu_integrating = false;
	self->dbtp_enable = false;

	self->radar_pos_max = 360;
	self->radar_resync = -1;

	self->ui_settings = 8;
	self->send_state_to_ui = false;

	self->radarS = (float*) malloc(self->radar_pos_max * sizeof(float));
	self->radarM = (float*) malloc(self->radar_pos_max * sizeof(float));
	self->radarSC = self->radarMC = -INFINITY;
	self->radar_pos_cur = self->radar_spd_cur = 0;

	for (int i=0; i < self->radar_pos_max; ++i) {
		self->radarS[i] = -INFINITY;
		self->radarM[i] = -INFINITY;
	}

	ebu_set_radarspeed(self, 2.0 * 60.0);

	for (int i=0; i < HIST_LEN; ++i) {
		self->histM[i] = 0;
		self->histS[i] = 0;
	}

	self->radar_pos_cur = 0;
	self->integration_time = 0;
	self->hist_maxM = 0;
	self->hist_maxS = 0;
	self->tp_max = -INFINITY;

	self->ebu = new Ebu_r128_proc();
	self->ebu->init (2, rate);

	self->mtr = (JmeterDSP **)malloc (2 * sizeof (JmeterDSP *));
	self->mtr[0] = new TruePeakdsp();
	self->mtr[1] = new TruePeakdsp();
	static_cast<TruePeakdsp *>(self->mtr[0])->init(rate);
	static_cast<TruePeakdsp *>(self->mtr[1])->init(rate);

	return (LV2_Handle)self;
}

static void
ebur128_connect_port(LV2_Handle instance, uint32_t port, void* data)
{
	LV2meter* self = (LV2meter*)instance;
	switch ((EBUPortIndex)port) {
	case EBU_INPUT0:
		self->input[0] = (float*) data;
		break;
	case EBU_OUTPUT0:
		self->output[0] = (float*) data;
		break;
	case EBU_INPUT1:
		self->input[1] = (float*) data;
		break;
	case EBU_OUTPUT1:
		self->output[1] = (float*) data;
		break;
	case EBU_NOTIFY:
		self->notify = (LV2_Atom_Sequence*)data;
		break;
	case EBU_CONTROL:
		self->control = (const LV2_Atom_Sequence*)data;
		break;
	}
}

static inline float coef_to_db (const float val) {
	if (val == 0) return -INFINITY;
	return 20.0 * log10f(val);
}

#define DEBUG_FORGE(VAR,NAME) \
	static uint32_t max_cap##VAR = 0; \
	if (self->notify->atom.size > max_cap##VAR) { \
		max_cap##VAR = self->notify->atom.size; \
		printf("POX %s new max: %d\n", NAME , max_cap##VAR); \
	}

static void
ebur128_run(LV2_Handle instance, uint32_t n_samples)
{
	LV2meter* self = (LV2meter*)instance;

	const uint32_t capacity = self->notify->atom.size;
	assert(capacity > 920);
	lv2_atom_forge_set_buffer(&self->forge, (uint8_t*)self->notify, capacity);
	lv2_atom_forge_sequence_head(&self->forge, &self->frame, 0);

	if (self->send_state_to_ui && self->ui_active) {
		self->send_state_to_ui = false;
		forge_kvcontrolmessage(&self->forge, &self->uris, self->uris.mtr_control, CTL_LV2_FTM, self->follow_transport_mode);
		forge_kvcontrolmessage(&self->forge, &self->uris, self->uris.mtr_control, CTL_LV2_RADARTIME,
				(self->radar_pos_max * self->radar_spd_max / self->rate));
		forge_kvcontrolmessage(&self->forge, &self->uris, self->uris.mtr_control, CTL_UISETTINGS, self->ui_settings);
	}

	/* Process incoming events from GUI */
	if (self->control) {
		LV2_Atom_Event* ev = lv2_atom_sequence_begin(&(self->control)->body);
		while(!lv2_atom_sequence_is_end(&(self->control)->body, (self->control)->atom.size, ev)) {
			if (ev->body.type == self->uris.atom_Blank || ev->body.type == self->uris.atom_Object) {
				const LV2_Atom_Object* obj = (LV2_Atom_Object*)&ev->body;
				if (obj->body.otype == self->uris.time_Position) {
					update_position(self, obj);
				}
				else if (obj->body.otype == self->uris.mtr_meters_on) {
					self->ui_active = true;
					self->send_state_to_ui = true;
					self->radar_resync = 0;
					/* resync histogram */
					for (int i=0; i < HIST_LEN; ++i) {
						self->histM[i] = 0;
						self->histS[i] = 0;
					}
					self->hist_maxM = 0;
					self->hist_maxS = 0;
				}
				else if (obj->body.otype == self->uris.mtr_meters_off) {
					self->ui_active = false;
				}
				else if (obj->body.otype == self->uris.mtr_meters_cfg) {
					int k; float v;
					get_cc_key_value(&self->uris, obj, &k, &v);
					// printf("MSG %d %f\n", k, v); // DEBUG
					switch (k) {
						case CTL_START:
							ebu_integrate(self, true);
							break;
						case CTL_PAUSE:
							ebu_integrate(self, false);
							break;
						case CTL_RESET:
							ebu_reset(self);
							break;
						case CTL_TRANSPORTSYNC:
							if (v==1) {
								self->follow_transport_mode|=1;
								if (self->tranport_rolling != self->ebu_integrating) {
									ebu_integrate(self, self->tranport_rolling);
								}
							} else {
								self->follow_transport_mode&=~1;
							}
							break;
						case CTL_AUTORESET:
							if (v==1) {
								self->follow_transport_mode|=2;
							} else {
								self->follow_transport_mode&=~2;
							}
							break;
						case CTL_RADARTIME:
							if (v >= 30 && v <= 600) {
								ebu_set_radarspeed(self, v);
								if (self->radar_spd_max < 2 * n_samples) self->radar_spd_max = 2 * n_samples;
							}
							forge_kvcontrolmessage(&self->forge, &self->uris, self->uris.mtr_control, CTL_LV2_RADARTIME,
									(self->radar_pos_max * self->radar_spd_max / self->rate));
							break;
						case CTL_UISETTINGS:
							self->ui_settings = (uint32_t) v;
							self->dbtp_enable = (self->ui_settings & 64) ? true : false;
							break;
						default:
							break;
					}
				}
			}
			ev = lv2_atom_sequence_next(ev);
		}
	}
#if 0
	static uint32_t max_post = 0;
	if (self->notify->atom.size > max_post) {
		max_post = self->notify->atom.size;
		printf("new post parse: %d\n", max_post);
	}
#endif

	/* process audio -- delayline + balance & gain */
	float *input [] = {self->input[0], self->input[1]};
	self->ebu->process(n_samples, input);

	if (self->dbtp_enable) {
		static_cast<TruePeakdsp*>(self->mtr[0])->process_max(self->input[0], n_samples);
		static_cast<TruePeakdsp*>(self->mtr[1])->process_max(self->input[1], n_samples);
	}

	/* get processed data */
	const float lm = self->ebu->loudness_M();
	const float mm = self->ebu->maxloudn_M();

	const float ls = self->ebu->loudness_S();
	float ms = self->ebu->maxloudn_S();

	const float il = self->ebu->integrated();
	const float rn = self->ebu->range_min();
	const float rx = self->ebu->range_max();

	if (self->dbtp_enable) {
		const float tp0 = self->mtr[0]->read();
		const float tp1 = self->mtr[1]->read();
		const float tp = coef_to_db(tp0 > tp1 ? tp0 : tp1);
		if (tp > self->tp_max) self->tp_max = tp;
	} else {
		self->tp_max = -INFINITY;
	}
	
	if (self->radar_resync >= 0) {
		int batch = (capacity - 512) / 192;
		if (batch > 16) batch = 16; // limit max data transfer per cycle
		for (int i=0; i < batch; i++, self->radar_resync++) {
			if (self->radar_resync >= self->radar_pos_max) {
				self->radar_resync = -1;
				forge_kvcontrolmessage(&self->forge, &self->uris, self->uris.mtr_control, CTL_LV2_RESYNCDONE, 0);
				break;
			}
			LV2_Atom_Forge_Frame frame;
			lv2_atom_forge_frame_time(&self->forge, 0);
			x_forge_object(&self->forge, &frame, 1, self->uris.rdr_radarpoint);
			lv2_atom_forge_property_head(&self->forge, self->uris.ebu_loudnessM, 0);  lv2_atom_forge_float(&self->forge, self->radarM[self->radar_resync]);
			lv2_atom_forge_property_head(&self->forge, self->uris.ebu_loudnessS, 0);  lv2_atom_forge_float(&self->forge, self->radarS[self->radar_resync]);
			lv2_atom_forge_property_head(&self->forge, self->uris.rdr_pointpos, 0); lv2_atom_forge_int(&self->forge, self->radar_resync);
			lv2_atom_forge_property_head(&self->forge, self->uris.rdr_pos_cur, 0); lv2_atom_forge_int(&self->forge, self->radar_pos_cur);
			lv2_atom_forge_property_head(&self->forge, self->uris.rdr_pos_max, 0); lv2_atom_forge_int(&self->forge, self->radar_pos_max);
			lv2_atom_forge_pop(&self->forge, &frame);
		}
	}

	/* radar history */
	if (lm > self->radarMC) self->radarMC = lm;
	if (lm > self->radarSC) self->radarSC = ls;

	if (self->ebu_integrating) {
		self->integration_time += n_samples;
	}

	self->radar_spd_cur += n_samples;
	if (self->radar_spd_cur > self->radar_spd_max) {
		if (self->ui_active) {
			LV2_Atom_Forge_Frame frame; // max 128 bytes
			lv2_atom_forge_frame_time(&self->forge, 0);
			x_forge_object(&self->forge, &frame, 1, self->uris.rdr_radarpoint);
			lv2_atom_forge_property_head(&self->forge, self->uris.ebu_loudnessM, 0);  lv2_atom_forge_float(&self->forge, self->radarMC);
			lv2_atom_forge_property_head(&self->forge, self->uris.ebu_loudnessS, 0);  lv2_atom_forge_float(&self->forge, self->radarSC);
			lv2_atom_forge_property_head(&self->forge, self->uris.rdr_pointpos, 0); lv2_atom_forge_int(&self->forge, self->radar_pos_cur);
			lv2_atom_forge_property_head(&self->forge, self->uris.rdr_pos_cur, 0); lv2_atom_forge_int(&self->forge, self->radar_pos_cur);
			lv2_atom_forge_property_head(&self->forge, self->uris.rdr_pos_max, 0); lv2_atom_forge_int(&self->forge, self->radar_pos_max);
			lv2_atom_forge_pop(&self->forge, &frame);
		}
#if 0
		printf("RADAR: @%d/%d  %f,%f\n",
				self->radar_pos_cur, self->radar_pos_max, self->radarMC, self->radarSC);
#endif

		self->radarM[self->radar_pos_cur] = self->radarMC;
		self->radarS[self->radar_pos_cur] = self->radarSC;
		self->radar_spd_cur = self->radar_spd_cur % self->radar_spd_max;
		self->radar_pos_cur = (self->radar_pos_cur + 1) % self->radar_pos_max;
		self->radarSC = self->radarMC = -INFINITY;
	}

	if (self->ui_active) {
		int msgtx = 0;
		int countM = self->ebu->hist_M_count();
		int countS = self->ebu->hist_S_count();
		if (countM > 10 && countS > 10) {
			const int *histM = self->ebu->histogram_M();
			const int *histS = self->ebu->histogram_S();
			bool max_changed = false;
			// TODO limit data-array from HIST_LEN to visible area only
			for (int i = 110; i < 650; i++) {
				const int vm = histM [i];
				const int vs = histS [i];
				if (capacity - self->notify->atom.size <= 512) {
					break;
				}
				if (self->histM[i] != vm || self->histS[i] != vs) {
					if (msgtx++ > 16) { break; } // limit max data-rate
					self->histM[i] = vm;
					self->histS[i] = vs;
					LV2_Atom_Forge_Frame frame; // max 128 bytes
					lv2_atom_forge_frame_time(&self->forge, 0);
					x_forge_object(&self->forge, &frame, 1, self->uris.rdr_histpoint);
					lv2_atom_forge_property_head(&self->forge, self->uris.ebu_loudnessM, 0);  lv2_atom_forge_int(&self->forge, vm);
					lv2_atom_forge_property_head(&self->forge, self->uris.ebu_loudnessS, 0);  lv2_atom_forge_int(&self->forge, vs);
					lv2_atom_forge_property_head(&self->forge, self->uris.rdr_pointpos, 0); lv2_atom_forge_int(&self->forge, i);
					lv2_atom_forge_pop(&self->forge, &frame);
				}
				if (vm > self->hist_maxM) { self->hist_maxM = vm; max_changed = true; }
				if (vs > self->hist_maxS) { self->hist_maxS = vs; max_changed = true; }
				//printf ("%5.1lf %8.6lf %8.6lf\n", (0.1f * (i - 700)), vm / countM, vs / countS);
			}
			if (max_changed) {
				LV2_Atom_Forge_Frame frame; // max 128 bytes
				lv2_atom_forge_frame_time(&self->forge, 0);
				x_forge_object(&self->forge, &frame, 1, self->uris.rdr_histogram);
				lv2_atom_forge_property_head(&self->forge, self->uris.ebu_loudnessM, 0); lv2_atom_forge_int(&self->forge, self->hist_maxM);
				lv2_atom_forge_property_head(&self->forge, self->uris.ebu_loudnessS, 0); lv2_atom_forge_int(&self->forge, self->hist_maxS);
				lv2_atom_forge_pop(&self->forge, &frame);
			}
		}
	}

	/* report values to UI - TODO only if changed*/
	if (self->ui_active) {
		LV2_Atom_Forge_Frame frame; // max 264 bytes
		lv2_atom_forge_frame_time(&self->forge, 0);
		x_forge_object(&self->forge, &frame, 1, self->uris.mtr_ebulevels);
		lv2_atom_forge_property_head(&self->forge, self->uris.ebu_loudnessM, 0);   lv2_atom_forge_float(&self->forge, lm);
		lv2_atom_forge_property_head(&self->forge, self->uris.ebu_maxloudnM, 0);   lv2_atom_forge_float(&self->forge, mm);
		lv2_atom_forge_property_head(&self->forge, self->uris.ebu_loudnessS, 0);   lv2_atom_forge_float(&self->forge, ls);
		lv2_atom_forge_property_head(&self->forge, self->uris.ebu_maxloudnS, 0);   lv2_atom_forge_float(&self->forge, ms);
		lv2_atom_forge_property_head(&self->forge, self->uris.ebu_integrated, 0);  lv2_atom_forge_float(&self->forge, il);
		lv2_atom_forge_property_head(&self->forge, self->uris.ebu_range_min, 0);   lv2_atom_forge_float(&self->forge, rn);
		lv2_atom_forge_property_head(&self->forge, self->uris.ebu_range_max, 0);   lv2_atom_forge_float(&self->forge, rx);
		lv2_atom_forge_property_head(&self->forge, self->uris.mtr_truepeak, 0);    lv2_atom_forge_float(&self->forge, self->tp_max);
		lv2_atom_forge_property_head(&self->forge, self->uris.ebu_integrating, 0); lv2_atom_forge_bool(&self->forge, self->ebu_integrating);
		lv2_atom_forge_property_head(&self->forge, self->uris.ebu_integr_time, 0); lv2_atom_forge_float(&self->forge, (self->integration_time/ self->rate));

		lv2_atom_forge_pop(&self->forge, &frame);
	}

	if (self->input[0] != self->output[0]) {
		memcpy(self->output[0], self->input[0], sizeof(float) * n_samples);
	}
	if (self->input[1] != self->output[1]) {
		memcpy(self->output[1], self->input[1], sizeof(float) * n_samples);
	}
#if 0
	//printf("forged %d bytes\n", self->notify->atom.size);
	static uint32_t max_cap = 0;
	if (self->notify->atom.size > max_cap) {
		max_cap = self->notify->atom.size;
		printf("new max: %d (of %d avail)\n", max_cap, capacity);
	}
#endif
}

static void
ebur128_cleanup(LV2_Handle instance)
{
	LV2meter* self = (LV2meter*)instance;
	free(self->radarS);
	free(self->radarM);
	delete self->ebu;
	delete self->mtr[0];
	delete self->mtr[1];
	free (self->mtr);
	FREE_VARPORTS;
	free(instance);
}

static LV2_State_Status
ebur128_save(LV2_Handle        instance,
     LV2_State_Store_Function  store,
     LV2_State_Handle          handle,
     uint32_t                  flags,
     const LV2_Feature* const* features)
{
	LV2meter* self = (LV2meter*)instance;
	uint32_t cfg = self->ui_settings;
	cfg |= self->follow_transport_mode << 8;
	cfg |= self->radar_spd_max << 16;
	store(handle, self->uris.ebu_state,
			(void*) &cfg, sizeof(uint32_t),
			self->uris.atom_Int,
			LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
  return LV2_STATE_SUCCESS;
}

static LV2_State_Status
ebur128_restore(LV2_Handle          instance,
        LV2_State_Retrieve_Function retrieve,
        LV2_State_Handle            handle,
        uint32_t                    flags,
        const LV2_Feature* const*   features)
{
	LV2meter* self = (LV2meter*)instance;
  size_t   size;
  uint32_t type;
  uint32_t valflags;
  const void* value = retrieve(handle, self->uris.ebu_state, &size, &type, &valflags);
  if (value && size == sizeof(uint32_t) && type == self->uris.atom_Int) {
		uint32_t cfg = *((const int*)value);
		self->ui_settings = cfg & 0xff;
		self->follow_transport_mode = (cfg >> 8) & 0x3;
		self->radar_spd_max = cfg >> 16;
		self->dbtp_enable = (self->ui_settings & 64) ? true : false;
		self->send_state_to_ui = true;
	}
  return LV2_STATE_SUCCESS;
}

static const void*
extension_data_ebur(const char* uri)
{
  static const LV2_State_Interface  state  = { ebur128_save, ebur128_restore };
  if (!strcmp(uri, LV2_STATE__interface)) {
    return &state;
  }
#ifdef WITH_SIGNATURE
	LV2_LICENSE_EXT_C
#endif
  return NULL;
}

static const LV2_Descriptor descriptorEBUr128 = {
	MTR_URI "EBUr128",
	ebur128_instantiate,
	ebur128_connect_port,
	NULL,
	ebur128_run,
	NULL,
	ebur128_cleanup,
	extension_data_ebur
};
