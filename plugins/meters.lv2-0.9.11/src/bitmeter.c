/* meter.lv2 -- bitscope
 *
 * Copyright (C) 2015 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2005 Nicholas Lamb <njl195@zepler.org.uk>
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
 * -- reuses part of EBU API and com protocol
 */

typedef enum {
	BIM_CONTROL  = 0,
	BIM_NOTIFY   = 1,
	BIM_INPUT0   = 2,
	BIM_OUTPUT0  = 3,
} BIMPortIndex;


/******************************************************************************
 * helper functions
 */

static void bim_clear(LV2meter* self) {
	for (int i=0; i < HIST_LEN; ++i) {
		self->histS[i] = 0;
	}
	self->bim_min = INFINITY;
	self->bim_max = 0;
	self->bim_zero = self->bim_pos = 0;
	self->integration_time = 0;
}

static void bim_reset(LV2meter* self) {
	bim_clear(self);
	self->bim_nan = self->bim_inf = self->bim_den = 0;
}

// adopted from bitmeter http://devel.tlrmx.org/audio/source/
static void float_stats (LV2meter* self, float const * const sample) {
	unsigned int value = *((unsigned int *) sample);
	unsigned int exp = (value & 0x7f800000) >> 23;
	int sign = (value & 0x80000000) ? -1 : 1;
	value &= 0x7fffff;

	if (exp == 255) {
		if (value == 0) {
			++self->bim_inf;
		} else {
			++self->bim_nan;
		}
		return;
	} else if (exp == 0 && value == 0) {
		++self->bim_zero;
		return;
	} else if (exp == 0) {
		++self->bim_den;
	}

	if (sign > 0) {
		++self->bim_pos;
	}
	if (exp > 0) {
		assert (exp < 257);
		const float v = fabsf(*sample);
		if (v > self->bim_max) { self->bim_max = v;}
		if (v < self->bim_min) { self->bim_min = v;}
		++self->histS[BIM_NHIT + exp];
		++self->histS[BIM_NONE + exp];
	} else {
		exp = 1; /* E-126 not E-127 for denormals */
	}

	for (int k= 0; k < 23; ++k) {
		const int bit = 1 << k;
		++self->histS[BIM_DHIT + exp + k];
		if (value & bit) {
			++self->histS[BIM_DONE + exp + k];
			++self->histS[BIM_DSET + k];
		}
	}
}


/******************************************************************************
 * LV2
 */

static LV2_Handle
bim_instantiate(
		const LV2_Descriptor*     descriptor,
		double                    rate,
		const char*               bundle_path,
		const LV2_Feature* const* features)
{
	LV2meter* self = (LV2meter*)calloc (1, sizeof (LV2meter));
	if (!self) return NULL;

	if (strcmp (descriptor->URI, MTR_URI "bitmeter")) {
		free(self);
		return NULL;
	}

	for (int i = 0; features[i]; ++i) {
		if (!strcmp (features[i]->URI, LV2_URID__map)) {
			self->map = (LV2_URID_Map*)features[i]->data;
		}
	}

	if (!self->map) {
		fprintf (stderr, "Bitmeter error: Host does not support urid:map\n");
		free (self);
		return NULL;
	}

	map_eburlv2_uris(self->map, &self->uris);
	lv2_atom_forge_init(&self->forge, self->map);

	self->rate = rate;
	self->ui_active = false;
	self->send_state_to_ui = false;
	self->ebu_integrating = true;
	self->bim_average = false;

	self->chn = 1;
	self->input  = (float**) calloc (self->chn, sizeof (float*));
	self->output = (float**) calloc (self->chn, sizeof (float*));

	bim_reset (self);
	return (LV2_Handle)self;
}

static void
bim_connect_port (LV2_Handle instance, uint32_t port, void* data)
{
	LV2meter* self = (LV2meter*)instance;
	switch ((BIMPortIndex)port) {
		case BIM_INPUT0:
			self->input[0] = (float*) data;
			break;
		case BIM_OUTPUT0:
			self->output[0] = (float*) data;
			break;
		case BIM_NOTIFY:
			self->notify = (LV2_Atom_Sequence*)data;
			break;
		case BIM_CONTROL:
			self->control = (const LV2_Atom_Sequence*)data;
			break;
	}
}

static void
bim_run(LV2_Handle instance, uint32_t n_samples)
{
	LV2meter* self = (LV2meter*)instance;

	const uint32_t capacity = self->notify->atom.size;
	assert(capacity > 920);
	lv2_atom_forge_set_buffer(&self->forge, (uint8_t*)self->notify, capacity);
	lv2_atom_forge_sequence_head(&self->forge, &self->frame, 0);

	if (self->send_state_to_ui && self->ui_active) {
		self->send_state_to_ui = false;
		forge_kvcontrolmessage(&self->forge, &self->uris, self->uris.mtr_control, CTL_SAMPLERATE, self->rate);
	}

	/* Process incoming events from GUI */
	if (self->control) {
		LV2_Atom_Event* ev = lv2_atom_sequence_begin(&(self->control)->body);
		while(!lv2_atom_sequence_is_end(&(self->control)->body, (self->control)->atom.size, ev)) {
			if (ev->body.type == self->uris.atom_Blank || ev->body.type == self->uris.atom_Object) {
				const LV2_Atom_Object* obj = (LV2_Atom_Object*)&ev->body;
				if (obj->body.otype == self->uris.mtr_meters_on) {
					self->ui_active = true;
					self->send_state_to_ui = true;
				}
				else if (obj->body.otype == self->uris.mtr_meters_off) {
					self->ui_active = false;
				}
				else if (obj->body.otype == self->uris.mtr_meters_cfg) {
					int k; float v;
					get_cc_key_value(&self->uris, obj, &k, &v);
					switch (k) {
						case CTL_START:
							self->ebu_integrating = true;
							break;
						case CTL_PAUSE:
							self->ebu_integrating = false;
							break;
						case CTL_RESET:
							bim_reset(self);
							self->send_state_to_ui = true;
							break;
						case CTL_AVERAGE:
							self->bim_average = true;
							break;
						case CTL_WINDOWED:
							self->bim_average = false;
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
			for (uint32_t s = 0; s < n_samples; ++s) {
				float_stats(self, self->input[0] + s);
			}
			self->integration_time += n_samples;
		}
	}

	const int fps_limit = n_samples * ceil(self->rate / (5.f * n_samples)); // ~ 5fps
	self->radar_resync += n_samples;

	if (self->radar_resync >= fps_limit || self->send_state_to_ui) {
		self->radar_resync = self->radar_resync % fps_limit;

		if (self->ui_active && (self->ebu_integrating || self->send_state_to_ui)) {
			LV2_Atom_Forge_Frame frame;
			lv2_atom_forge_frame_time(&self->forge, 0);
			x_forge_object(&self->forge, &frame, 1, self->uris.bim_stats);

			lv2_atom_forge_property_head(&self->forge, self->uris.ebu_integr_time, 0);
			lv2_atom_forge_long(&self->forge, self->integration_time);

			lv2_atom_forge_property_head(&self->forge, self->uris.bim_zero, 0);
			lv2_atom_forge_int(&self->forge, self->bim_zero);
			lv2_atom_forge_property_head(&self->forge, self->uris.bim_pos, 0);
			lv2_atom_forge_int(&self->forge, self->bim_pos);

			lv2_atom_forge_property_head(&self->forge, self->uris.bim_max, 0);
			lv2_atom_forge_double(&self->forge, self->bim_max);
			lv2_atom_forge_property_head(&self->forge, self->uris.bim_min, 0);
			lv2_atom_forge_double(&self->forge, self->bim_min);
			lv2_atom_forge_property_head(&self->forge, self->uris.bim_nan, 0);
			lv2_atom_forge_int(&self->forge, self->bim_nan);
			lv2_atom_forge_property_head(&self->forge, self->uris.bim_inf, 0);
			lv2_atom_forge_int(&self->forge, self->bim_inf);
			lv2_atom_forge_property_head(&self->forge, self->uris.bim_den, 0);
			lv2_atom_forge_int(&self->forge, self->bim_den);

			lv2_atom_forge_property_head(&self->forge, self->uris.bim_data, 0);
			lv2_atom_forge_vector(&self->forge, sizeof(int32_t), self->uris.atom_Int, BIM_LAST, self->histS);
			lv2_atom_forge_pop(&self->forge, &frame);
		}

		if (self->ui_active) {
			LV2_Atom_Forge_Frame frame;
			lv2_atom_forge_frame_time(&self->forge, 0);
			x_forge_object(&self->forge, &frame, 1, self->uris.bim_information);
			lv2_atom_forge_property_head(&self->forge, self->uris.ebu_integrating, 0);
			lv2_atom_forge_bool(&self->forge, self->ebu_integrating);
			lv2_atom_forge_property_head(&self->forge, self->uris.bim_averaging, 0);
			lv2_atom_forge_bool(&self->forge, self->bim_average);
			lv2_atom_forge_pop(&self->forge, &frame);
		}

		if (!self->bim_average) {
			bim_clear (self);
		}
	}

	/* foward audio-data */
	if (self->input[0] != self->output[0]) {
		memcpy(self->output[0], self->input[0], sizeof(float) * n_samples);
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
bim_cleanup(LV2_Handle instance)
{
	LV2meter* self = (LV2meter*)instance;
	FREE_VARPORTS;
	free(instance);
}

static LV2_State_Status
bim_save(LV2_Handle        instance,
     LV2_State_Store_Function  store,
     LV2_State_Handle          handle,
     uint32_t                  flags,
     const LV2_Feature* const* features)
{
	LV2meter* self = (LV2meter*)instance;
	uint32_t cfg = self->bim_average ? 1 : 0;
	store(handle, self->uris.bim_state,
			(void*) &cfg, sizeof(uint32_t),
			self->uris.atom_Int,
			LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
  return LV2_STATE_SUCCESS;
}

static LV2_State_Status
bim_restore(LV2_Handle              instance,
        LV2_State_Retrieve_Function retrieve,
        LV2_State_Handle            handle,
        uint32_t                    flags,
        const LV2_Feature* const*   features)
{
	LV2meter* self = (LV2meter*)instance;
  size_t   size;
  uint32_t type;
  uint32_t valflags;
  const void* value = retrieve(handle, self->uris.bim_state, &size, &type, &valflags);
  if (value && size == sizeof(uint32_t) && type == self->uris.atom_Int) {
		uint32_t cfg = *((const int*)value);
		self->bim_average = (cfg & 0x1) ? true : false;
		self->send_state_to_ui = true;
	}
  return LV2_STATE_SUCCESS;
}

static const void*
extension_data_bim(const char* uri)
{
  static const LV2_State_Interface  state  = { bim_save, bim_restore };
  if (!strcmp(uri, LV2_STATE__interface)) {
    return &state;
  }
#ifdef WITH_SIGNATURE
	LV2_LICENSE_EXT_C
#endif
  return NULL;
}

static const LV2_Descriptor descriptorBIM = {
	MTR_URI "bitmeter",
	bim_instantiate,
	bim_connect_port,
	NULL,
	bim_run,
	NULL,
	bim_cleanup,
	extension_data_bim
};
