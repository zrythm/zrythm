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
 * broken out goniometer related LV2 functions
 */

#define UPDATE_FPS (25.0)

typedef enum {
	JF_INPUT0   = 0,
	JF_OUTPUT0  = 1,
	JF_INPUT1   = 2,
	JF_OUTPUT1  = 3,
	JF_GAIN     = 4,
	JF_CORR     = 5,
	JF_NOTIFY   = 6,
} JFPortIndex;

#include "goniometer.h"

/******************************************************************************
 * LV2 callbacks
 */

static LV2_Handle
goniometer_instantiate(
		const LV2_Descriptor*     descriptor,
		double                    rate,
		const char*               bundle_path,
		const LV2_Feature* const* features)
{
	if (strcmp(descriptor->URI, MTR_URI "goniometer")) {
		return NULL;
	}
	LV2gm* self = (LV2gm*)calloc(1, sizeof(LV2gm));
	if (!self) return NULL;

	for (int i=0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID__map)) {
			self->map = (LV2_URID_Map*)features[i]->data;
		}
	}

	if (!self->map) {
		fprintf(stderr, "Goniometer error: Host does not support urid:map\n");
		free(self);
		return NULL;
	}

	self->atom_Vector = self->map->map(self->map->handle, LV2_ATOM__Vector);
	self->atom_Int    = self->map->map(self->map->handle, LV2_ATOM__Int);
	self->atom_Float  = self->map->map(self->map->handle, LV2_ATOM__Float);
	self->gon_State_F = self->map->map(self->map->handle, MTR_URI "gon_stateF");
	self->gon_State_I = self->map->map(self->map->handle, MTR_URI "gon_stateI");

	self->cor = new Stcorrdsp();
	self->cor->init(rate, 2e3f, 0.3f);

	self->rate = rate;
	self->ui_active = false;
	self->rb_overrun = false;

	self->apv = rint(rate / UPDATE_FPS);
	self->sample_cnt = 0;
	self->ntfy = 0;

	self->msg_thread_lock = NULL;
	self->data_ready = NULL;
	self->ui = NULL;
	self->queue_display = NULL;

	self->s_autogain = false;
	self->s_oversample = false;
	self->s_line = false;
	self->s_persist = false;
	self->s_preferences = false;
	self->s_sfact = 4;
	self->s_linewidth = .75;
	self->s_pointwidth = 1.75;
	self->s_persistency = 33;
	self->s_max_freq = 50;
	self->s_compress = 0.0;
	self->s_gattack = 54.0;
	self->s_gdecay = 58.0;
	self->s_gtarget = 40.0;
	self->s_grms = 50.0;

	uint32_t rbsize = self->rate / 5;
	if (rbsize < 8192u) rbsize = 8192u;
	if (rbsize < 2 * self->apv) rbsize = 2 * self->apv;

	self->rb = gmrb_alloc(rbsize);

	return (LV2_Handle)self;
}

static void
goniometer_connect_port(LV2_Handle instance, uint32_t port, void* data)
{
	LV2gm* self = (LV2gm*)instance;
	switch ((JFPortIndex)port) {
	case JF_INPUT0:
		self->input[0] = (float*) data;
		break;
	case JF_OUTPUT0:
		self->output[0] = (float*) data;
		break;
	case JF_INPUT1:
		self->input[1] = (float*) data;
		break;
	case JF_OUTPUT1:
		self->output[1] = (float*) data;
		break;
	case JF_GAIN:
		self->gain = (float*) data;
		break;
	case JF_CORR:
		self->correlation = (float*) data;
		break;
	case JF_NOTIFY:
		self->notify = (float*) data;
		break;
	}
}

static void
goniometer_run(LV2_Handle instance, uint32_t n_samples)
{
	LV2gm* self = (LV2gm*)instance;

	self->cor->process(self->input[0], self->input[1] , n_samples);

	if (self->ui_active) {
		if (gmrb_write(self->rb, self->input[0], self->input[1], n_samples) < 0) {
			self->rb_overrun = true; // reset by UI
		}

		/* notify UI about new data */
		self->sample_cnt += n_samples;
		if (self->sample_cnt >= self->apv) {
			/* directly wake up instance */
			if (self->msg_thread_lock) {
				self->queue_display(self->ui);
				if (pthread_mutex_trylock (self->msg_thread_lock) == 0) {
					pthread_cond_signal (self->data_ready);
					pthread_mutex_unlock (self->msg_thread_lock);
				}
			} else {
				/* notify UI by creating a port-event
				 * -- adds host comm-buffer latency
				 */
				self->ntfy = (self->ntfy + 1) % 10000;
			}
			self->sample_cnt = self->sample_cnt % self->apv;
		}
		*self->notify = self->ntfy;
		*self->correlation = self->cor->read();
	} else {
		self->rb_overrun = false;
	}

	if (self->input[0] != self->output[0]) {
		memcpy(self->output[0], self->input[0], sizeof(float) * n_samples);
	}
	if (self->input[1] != self->output[1]) {
		memcpy(self->output[1], self->input[1], sizeof(float) * n_samples);
	}
}

static void
goniometer_cleanup(LV2_Handle instance)
{
	LV2gm* self = (LV2gm*)instance;
	gmrb_free(self->rb);
	delete self->cor;
	free(instance);
}


struct VectorOfFloat {
	uint32_t child_size;
	uint32_t child_type;
	float    cfg[9];
};

struct VectorOfInt {
	uint32_t child_size;
	uint32_t child_type;
	int32_t  cfg[2];
};

static LV2_State_Status
goniometer_save(LV2_Handle     instance,
     LV2_State_Store_Function  store,
     LV2_State_Handle          handle,
     uint32_t                  flags,
     const LV2_Feature* const* features)
{
	LV2gm* self = (LV2gm*)instance;
	struct VectorOfFloat vof;
	struct VectorOfInt voi;

	vof.child_type = self->atom_Float;
	vof.child_size = sizeof(float);

	voi.child_type = self->atom_Int;
	voi.child_size = sizeof(int32_t);

	vof.cfg[0] = self->s_linewidth;
	vof.cfg[1] = self->s_pointwidth;
	vof.cfg[2] = self->s_persistency;
	vof.cfg[3] = self->s_max_freq;
	vof.cfg[4] = self->s_compress;
	vof.cfg[5] = self->s_gattack;
	vof.cfg[6] = self->s_gdecay;
	vof.cfg[7] = self->s_gtarget;
	vof.cfg[8] = self->s_grms;

	voi.cfg[1] = self->s_sfact;
	voi.cfg[0] = 0;
	voi.cfg[0] |= self->s_autogain    ? 1 : 0;
	voi.cfg[0] |= self->s_oversample  ? 2 : 0;
	voi.cfg[0] |= self->s_line        ? 4 : 0;
	voi.cfg[0] |= self->s_persist     ? 8 : 0;
	voi.cfg[0] |= self->s_preferences ?16 : 0;

	store(handle, self->gon_State_F,
			(void*) &vof, sizeof(struct VectorOfFloat),
			self->atom_Vector, LV2_STATE_IS_POD /*| LV2_STATE_IS_PORTABLE*/);

	store(handle, self->gon_State_I,
			(void*) &voi, sizeof(struct VectorOfInt),
			self->atom_Vector, LV2_STATE_IS_POD /*| LV2_STATE_IS_PORTABLE */);

  return LV2_STATE_SUCCESS;
}

static LV2_State_Status
goniometer_restore(LV2_Handle       instance,
        LV2_State_Retrieve_Function retrieve,
        LV2_State_Handle            handle,
        uint32_t                    flags,
        const LV2_Feature* const*   features)
{
	LV2gm* self = (LV2gm*)instance;
  size_t   size;
  uint32_t type;
  uint32_t valflags;
  const void* v1 = retrieve(handle, self->gon_State_F, &size, &type, &valflags);
  if (v1 && size == (sizeof(LV2_Atom_Vector_Body) + (9 * sizeof(float))) && type == self->atom_Vector)
	{
		float *cfg = (float*) LV2_ATOM_BODY(v1);
		self->s_linewidth   = cfg[0];
		self->s_pointwidth  = cfg[1];
		self->s_persistency = cfg[2];
		self->s_max_freq    = cfg[3];
		self->s_compress    = cfg[4];
		self->s_gattack     = cfg[5];
		self->s_gdecay      = cfg[6];
		self->s_gtarget     = cfg[7];
		self->s_grms        = cfg[8];
	}
  const void* v2 = retrieve(handle, self->gon_State_I, &size, &type, &valflags);
  if (v2 && size == (sizeof(LV2_Atom_Vector_Body) + (2 * sizeof(int32_t))) && type == self->atom_Vector)
	{
		int *cfg = (int32_t*) LV2_ATOM_BODY(v2);
		self->s_sfact = cfg[1];
		self->s_autogain   = (cfg[0] & 1) ? true: false;
		self->s_oversample = (cfg[0] & 2) ? true: false;
		self->s_line       = (cfg[0] & 4) ? true: false;
		self->s_persist    = (cfg[0] & 8) ? true: false;
		self->s_preferences= (cfg[0] &16) ? true: false;
	}
  return LV2_STATE_SUCCESS;
}

static const void*
goniometer_extension_data(const char* uri)
{
  static const LV2_State_Interface  state  = { goniometer_save, goniometer_restore };
  if (!strcmp(uri, LV2_STATE__interface)) {
    return &state;
  }
#ifdef WITH_SIGNATURE
	LV2_LICENSE_EXT_C
#endif
  return NULL;
}

static const LV2_Descriptor descriptorGoniometer = {
	MTR_URI "goniometer",
	goniometer_instantiate,
	goniometer_connect_port,
	NULL,
	goniometer_run,
	NULL,
	goniometer_cleanup,
	goniometer_extension_data
};
