/* meter.lv2 -- signal distribution histogram
 *
 * Copyright (C) 2014 Robin Gareus <robin@gareus.org>
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

#ifndef MAX
#define MAX(A,B) ( (A) > (B) ? (A) : (B) )
#endif

/* static functions to be included in meters.cc
 * for signal distribution histogram display.
 *
 * -- reuses part of EBU API and com. protocol
 */

typedef enum {
	SDH_CONTROL  = 0,
	SDH_NOTIFY   = 1,
	SDH_INPUT0   = 2,
	SDH_OUTPUT0  = 3,
	SDH_INPUT1   = 4,
	SDH_OUTPUT1  = 5,
} SDHPortIndex;


/******************************************************************************
 * helper functions
 */

static void sdh_reset(LV2meter* self) {
	forge_kvcontrolmessage(&self->forge, &self->uris, self->uris.mtr_control, CTL_LV2_RESETRADAR, 0);
	for (int i=0; i < HIST_LEN; ++i) {
		self->histS[i] = 0;
	}
	self->hist_peakS = -1;
	self->hist_avgS = 0;
	self->hist_tmpS = 0;
	self->hist_varS = 0;
	self->hist_maxS = 0;
	self->integration_time = 0;
	self->radar_resync = 0;
}

static void sdh_integrate(LV2meter* self, bool on) {
	if (self->ebu_integrating == on) return;
	if (on) {
		if (self->follow_transport_mode & 2) {
			sdh_reset(self);
		}
		self->ebu_integrating=true;
	} else {
		self->ebu_integrating=false;
	}
}

/**
 * Update transport state.
 * This is called by run() when a time:Position is received.
 */
static void
sdh_update_position(LV2meter* self, const LV2_Atom_Object* obj)
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
			if (self->follow_transport_mode & 1) { sdh_integrate(self, true); }
		}
		if (ts == 0 && self->tranport_rolling) {
			if (self->follow_transport_mode & 1) { sdh_integrate(self, false); }
		}
		self->tranport_rolling = (ts != 0);
	}
}


/******************************************************************************
 * LV2 callbacks
 */

static LV2_Handle
sdh_instantiate(
		const LV2_Descriptor*     descriptor,
		double                    rate,
		const char*               bundle_path,
		const LV2_Feature* const* features)
{
	LV2meter* self = (LV2meter*)calloc(1, sizeof(LV2meter));
	if (!self) return NULL;

	if (strcmp(descriptor->URI, MTR_URI "SigDistHist")) {
		free(self);
		return NULL;
	}

	self->chn = 1;
	self->input  = (float**) calloc (self->chn, sizeof (float*));
	self->output = (float**) calloc (self->chn, sizeof (float*));

	for (int i=0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID__map)) {
			self->map = (LV2_URID_Map*)features[i]->data;
		}
	}

	if (!self->map) {
		fprintf(stderr, "SigDistHist error: Host does not support urid:map\n");
		free(self);
		return NULL;
	}

	map_eburlv2_uris(self->map, &self->uris);
	lv2_atom_forge_init(&self->forge, self->map);

	self->rate = rate;
	self->ui_active = false;
	self->follow_transport_mode = 0; // 3
	self->tranport_rolling = false;
	self->ebu_integrating = false;

	self->ui_settings = 0;
	self->send_state_to_ui = false;

	for (int i=0; i < HIST_LEN; ++i) {
		self->histS[i] = 0;
	}

	self->hist_peakS = -1;
	self->hist_avgS = 0;
	self->hist_tmpS = 0;
	self->hist_varS = 0;
	self->hist_maxS = 0;
	self->integration_time = 0;
	self->radar_resync = 0;

	return (LV2_Handle)self;
}

static void
sdh_connect_port(LV2_Handle instance, uint32_t port, void* data)
{
	LV2meter* self = (LV2meter*)instance;
	switch ((SDHPortIndex)port) {
	case SDH_INPUT0:
		self->input[0] = (float*) data;
		break;
	case SDH_OUTPUT0:
		self->output[0] = (float*) data;
		break;
	case SDH_INPUT1:
		self->input[1] = (float*) data;
		break;
	case SDH_OUTPUT1:
		self->output[1] = (float*) data;
		break;
	case SDH_NOTIFY:
		self->notify = (LV2_Atom_Sequence*)data;
		break;
	case SDH_CONTROL:
		self->control = (const LV2_Atom_Sequence*)data;
		break;
	}
}

#define DEBUG_FORGE(VAR,NAME) \
	static uint32_t max_cap##VAR = 0; \
	if (self->notify->atom.size > max_cap##VAR) { \
		max_cap##VAR = self->notify->atom.size; \
		printf("POX %s new max: %d\n", NAME , max_cap##VAR); \
	}

static void
sdh_run(LV2_Handle instance, uint32_t n_samples)
{
	LV2meter* self = (LV2meter*)instance;

	const uint32_t capacity = self->notify->atom.size;
	assert(capacity > 920);
	lv2_atom_forge_set_buffer(&self->forge, (uint8_t*)self->notify, capacity);
	lv2_atom_forge_sequence_head(&self->forge, &self->frame, 0);

	if (self->send_state_to_ui && self->ui_active) {
		self->send_state_to_ui = false;
		forge_kvcontrolmessage(&self->forge, &self->uris, self->uris.mtr_control, CTL_LV2_FTM, self->follow_transport_mode);
		forge_kvcontrolmessage(&self->forge, &self->uris, self->uris.mtr_control, CTL_SAMPLERATE, self->rate);
		forge_kvcontrolmessage(&self->forge, &self->uris, self->uris.mtr_control, CTL_UISETTINGS, self->ui_settings);
	}

	/* Process incoming events from GUI */
	if (self->control) {
		LV2_Atom_Event* ev = lv2_atom_sequence_begin(&(self->control)->body);
		while(!lv2_atom_sequence_is_end(&(self->control)->body, (self->control)->atom.size, ev)) {
			if (ev->body.type == self->uris.atom_Blank || ev->body.type == self->uris.atom_Object) {
				const LV2_Atom_Object* obj = (LV2_Atom_Object*)&ev->body;
				if (obj->body.otype == self->uris.time_Position) {
					sdh_update_position(self, obj);
				}
				else if (obj->body.otype == self->uris.mtr_meters_on) {
					self->ui_active = true;
					self->send_state_to_ui = true;
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
							sdh_integrate(self, true);
							break;
						case CTL_PAUSE:
							sdh_integrate(self, false);
							break;
						case CTL_RESET:
							sdh_reset(self);
							break;
						case CTL_TRANSPORTSYNC:
							if (v==1) {
								self->follow_transport_mode|=1;
								if (self->tranport_rolling != self->ebu_integrating) {
									sdh_integrate(self, self->tranport_rolling);
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
						case CTL_UISETTINGS:
							self->ui_settings = (uint32_t) v;
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


	/* process */

	if (self->ebu_integrating && self->integration_time < 2147483647) {
		/* currently 'self->histS' is int32,
		 * the max peak that can be recorded is 2^31,
		 * for now we simply limit data-acquisition to at
		 * most 2^31 points.
		 */
		if (self->integration_time > 2147483647 - n_samples) {
			self->integration_time = 2147483647;
		} else {
			// localize variables (for current channel)
			int *bins = self->histS;
			int peak_cnt = self->hist_maxS;
			int peak_bin = self->hist_peakS;
			double avg   = self->hist_avgS;
			double var_m = self->hist_tmpS;
			double var_s = self->hist_varS;

			for (uint32_t s = 0; s < n_samples; ++s) {
				const float val = self->input[0][s];
				int bin = rintf(DIST_ZERO + val * DIST_RANGE);
				if (bin < 0) continue;
				if (bin >= DIST_BIN) continue;
				if ((++bins[bin]) > peak_cnt) {
					peak_cnt = bins[bin];
					peak_bin = bin;
				}
				avg += val;
				// running variance
				const double var_m1 = var_m;
				const double cnt_a = self->integration_time + s + 1;
				var_m = var_m + ((double)val - var_m) / cnt_a;
				var_s = var_s + ((double)val - var_m) * ((double)val - var_m1);
			}
			self->integration_time += n_samples;
			// copy back variables
			self->hist_maxS = peak_cnt;
			self->hist_peakS = peak_bin;
			self->hist_avgS = avg;
			self->hist_tmpS = var_m;
			self->hist_varS = var_s;
		}
	}

	const int fps_limit = MAX(self->rate / 25.f, n_samples);
	self->radar_resync += n_samples;

	if (self->radar_resync >= fps_limit || self->send_state_to_ui) {
		self->radar_resync = self->radar_resync % fps_limit;

		if (self->ui_active && (self->ebu_integrating || self->send_state_to_ui)) {
			// TODO limit data-array to changed values only
			// this needs a 'smart' approach, depending on n_samples:
			// if less than half of the data was changed: use key+value pairs.
			// then remove 'radar_resync' fps limit.
			LV2_Atom_Forge_Frame frame;
			lv2_atom_forge_frame_time(&self->forge, 0);
			x_forge_object(&self->forge, &frame, 1, self->uris.sdh_histogram);

			lv2_atom_forge_property_head(&self->forge, self->uris.sdh_hist_max, 0);
			lv2_atom_forge_int(&self->forge, self->hist_maxS);
			lv2_atom_forge_property_head(&self->forge, self->uris.sdh_hist_avg, 0);
			lv2_atom_forge_double(&self->forge, self->hist_avgS);
			lv2_atom_forge_property_head(&self->forge, self->uris.sdh_hist_var, 0);
			lv2_atom_forge_double(&self->forge, self->hist_varS);
			lv2_atom_forge_property_head(&self->forge, self->uris.sdh_hist_peak, 0);
			lv2_atom_forge_int(&self->forge, self->hist_peakS);

			lv2_atom_forge_property_head(&self->forge, self->uris.sdh_hist_data, 0);
			lv2_atom_forge_vector(&self->forge, sizeof(int32_t), self->uris.atom_Int, DIST_BIN, self->histS);
			lv2_atom_forge_pop(&self->forge, &frame);
		}

		if (self->ui_active) {
			/* report values to UI - TODO only if changed*/
			LV2_Atom_Forge_Frame frame;
			lv2_atom_forge_frame_time(&self->forge, 0);
			x_forge_object(&self->forge, &frame, 1, self->uris.sdh_information);
			lv2_atom_forge_property_head(&self->forge, self->uris.ebu_integrating, 0);
			lv2_atom_forge_bool(&self->forge, self->ebu_integrating);
			lv2_atom_forge_property_head(&self->forge, self->uris.ebu_integr_time, 0);
			lv2_atom_forge_long(&self->forge, self->integration_time);
			lv2_atom_forge_pop(&self->forge, &frame);
		}
	}

	/* foward audio-data */
	if (self->input[0] != self->output[0]) {
		memcpy(self->output[0], self->input[0], sizeof(float) * n_samples);
	}
	if (self->chn > 1 && self->input[1] != self->output[1]) {
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
sdh_cleanup(LV2_Handle instance)
{
	LV2meter* self = (LV2meter*)instance;
	FREE_VARPORTS;
	free(instance);
}

static LV2_State_Status
sdh_save(LV2_Handle        instance,
     LV2_State_Store_Function  store,
     LV2_State_Handle          handle,
     uint32_t                  flags,
     const LV2_Feature* const* features)
{
	LV2meter* self = (LV2meter*)instance;
	uint32_t cfg = self->ui_settings;
	cfg |= self->follow_transport_mode << 8;
	store(handle, self->uris.sdh_state,
			(void*) &cfg, sizeof(uint32_t),
			self->uris.atom_Int,
			LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
  return LV2_STATE_SUCCESS;
}

static LV2_State_Status
sdh_restore(LV2_Handle              instance,
        LV2_State_Retrieve_Function retrieve,
        LV2_State_Handle            handle,
        uint32_t                    flags,
        const LV2_Feature* const*   features)
{
	LV2meter* self = (LV2meter*)instance;
  size_t   size;
  uint32_t type;
  uint32_t valflags;
  const void* value = retrieve(handle, self->uris.sdh_state, &size, &type, &valflags);
  if (value && size == sizeof(uint32_t) && type == self->uris.atom_Int) {
		uint32_t cfg = *((const int*)value);
		self->ui_settings = cfg & 0xff;
		self->follow_transport_mode = (cfg >> 8) & 0x3;
		self->send_state_to_ui = true;
	}
  return LV2_STATE_SUCCESS;
}

static const void*
extension_data_sdh(const char* uri)
{
  static const LV2_State_Interface  state  = { sdh_save, sdh_restore };
  if (!strcmp(uri, LV2_STATE__interface)) {
    return &state;
  }
#ifdef WITH_SIGNATURE
	LV2_LICENSE_EXT_C
#endif
  return NULL;
}

static const LV2_Descriptor descriptorSDH = {
	MTR_URI "SigDistHist",
	sdh_instantiate,
	sdh_connect_port,
	NULL,
	sdh_run,
	NULL,
	sdh_cleanup,
	extension_data_sdh
};
