/* meter.lv2 -- GUI process (phase-wheel, spectrum,..)
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

static bool printed_capacity_warning = false;

typedef struct {
	/* I/O ports */
	float* input[MAX_CHANNELS];
	float* output[MAX_CHANNELS];
	const LV2_Atom_Sequence* control;
	LV2_Atom_Sequence* notify;

	/* atom-forge and URI mapping */
	LV2_URID_Map* map;
	XferLV2URIs uris;
	LV2_Atom_Forge forge;
	LV2_Atom_Forge_Frame frame;

	uint32_t n_channels;
	double rate;

	/* the state of the UI is stored here, so that
	 * the GUI can be displayed & closed
	 * without loosing current settings.
	 */
	bool ui_active;
	bool send_settings_to_ui;

	Stcorrdsp *stcor;
	float* p_phase;

} Xfer;

typedef enum {
	SPR_CONTROL  = 0,
	SPR_NOTIFY   = 1,
	SPR_INPUT0   = 2,
	SPR_OUTPUT0  = 3,
	SPR_INPUT1   = 4,
	SPR_OUTPUT1  = 5,
	SPR_PHASE    = 6,
	SPR_GAIN     = 7,
	SPR_RANGE    = 8,
} XFPortIndex;


static LV2_Handle
xfer_instantiate(const LV2_Descriptor*    descriptor,
                double                    rate,
                const char*               bundle_path,
                const LV2_Feature* const* features)
{
	(void) descriptor; /* unused variable */
	(void) bundle_path; /* unused variable */

	Xfer* self = (Xfer*)calloc(1, sizeof(Xfer));
	if(!self) {
		return NULL;
	}

	int i;
	for (i=0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID__map)) {
			self->map = (LV2_URID_Map*)features[i]->data;
		}
	}

	if (!self->map) {
		fprintf(stderr, "meters.lv2 error: Host does not support urid:map\n");
		free(self);
		return NULL;
	}

	if (!strcmp(descriptor->URI, MTR_URI "phasewheel")) {
		self->n_channels = 2;
		self->stcor = new Stcorrdsp();
		self->stcor->init(rate, 2e3f, 0.3f);
	} else
		if (!strcmp(descriptor->URI, MTR_URI "stereoscope")) {
		self->n_channels = 2;
	} else {
		free(self);
		return NULL;
	}

	assert(self->n_channels <= MAX_CHANNELS);

	self->ui_active = false;
	self->send_settings_to_ui = false;
	self->rate = rate;

	lv2_atom_forge_init(&self->forge, self->map);
	map_xfer_uris(self->map, &self->uris);
	return (LV2_Handle)self;
}

static void
xfer_connect_port(LV2_Handle handle,
             uint32_t   port,
             void*      data)
{
	Xfer* self = (Xfer*)handle;

	switch ((XFPortIndex)port) {
		case SPR_CONTROL:
			self->control = (const LV2_Atom_Sequence*)data;
			break;
		case SPR_NOTIFY:
			self->notify = (LV2_Atom_Sequence*)data;
			break;
		case SPR_PHASE:
			self->p_phase = (float*) data;
			break;
		default:
			if (port >= SPR_INPUT0 && port <= SPR_OUTPUT1) {
				if (port%2) {
					self->output[(port/2)-1] = (float*) data;
				} else {
					self->input[(port/2)-1] = (float*) data;
				}
			}
			break;
	}
}

/** forge atom-vector of raw data */
static void tx_rawaudio(LV2_Atom_Forge *forge, XferLV2URIs *uris,
    const int32_t channel, const size_t n_samples, void *data)
{
	LV2_Atom_Forge_Frame frame;
	/* forge container object of type 'rawaudio' */
	lv2_atom_forge_frame_time(forge, 0);
	x_forge_object(forge, &frame, 1, uris->rawaudio);

	/* add integer attribute 'channelid' */
	lv2_atom_forge_property_head(forge, uris->channelid, 0);
	lv2_atom_forge_int(forge, channel);

	/* add vector of floats raw 'audiodata' */
	lv2_atom_forge_property_head(forge, uris->audiodata, 0);
	lv2_atom_forge_vector(forge, sizeof(float), uris->atom_Float, n_samples, data);

	/* close off atom-object */
	lv2_atom_forge_pop(forge, &frame);
}

/** forge atom-vector of raw data */
static void tx_rawstereo(LV2_Atom_Forge *forge, XferLV2URIs *uris,
    const size_t n_samples, void *left, void *right)
{
	LV2_Atom_Forge_Frame frame;
	/* forge container object of type 'rawaudio' */
	lv2_atom_forge_frame_time(forge, 0);
	x_forge_object(forge, &frame, 1, uris->rawstereo);

	lv2_atom_forge_property_head(forge, uris->audioleft, 0);
	lv2_atom_forge_vector(forge, sizeof(float), uris->atom_Float, n_samples, left);

	lv2_atom_forge_property_head(forge, uris->audioright, 0);
	lv2_atom_forge_vector(forge, sizeof(float), uris->atom_Float, n_samples, right);

	/* close off atom-object */
	lv2_atom_forge_pop(forge, &frame);
}

static void
xfer_run(LV2_Handle handle, uint32_t n_samples)
{
	Xfer* self = (Xfer*)handle;
	const size_t size = (sizeof(float) * n_samples + 64) * self->n_channels;
	const uint32_t capacity = self->notify->atom.size;
	bool capacity_ok = true;

	/* check if atom-port buffer is large enough to hold
	 * all audio-samples and configuration settings */
	if (capacity < size + 128) {
		capacity_ok = false;
		if (!printed_capacity_warning) {
#ifdef _WIN32
			fprintf(stderr, "meters.lv2 error: LV2 comm-buffersize is insufficient %d/%d bytes.\n",
					capacity, (int)size + 160);
#else
			fprintf(stderr, "meters.lv2 error: LV2 comm-buffersize is insufficient %d/%zu bytes.\n",
					capacity, size + 160);
#endif
			printed_capacity_warning = true;
		}
		return;
	}

	/* prepare forge buffer and initialize atom-sequence */
	lv2_atom_forge_set_buffer(&self->forge, (uint8_t*)self->notify, capacity);
	lv2_atom_forge_sequence_head(&self->forge, &self->frame, 0);

	/* Send settings to UI */
	if (self->send_settings_to_ui && self->ui_active) {
		self->send_settings_to_ui = false;
		/* forge container object of type 'ui_state' */
		LV2_Atom_Forge_Frame frame;
		lv2_atom_forge_frame_time(&self->forge, 0);
		x_forge_object(&self->forge, &frame, 1, self->uris.ui_state);
		/* forge attributes for 'ui_state' */
		lv2_atom_forge_property_head(&self->forge, self->uris.samplerate, 0);
		lv2_atom_forge_float(&self->forge, self->rate);

		/* close-off frame */
		lv2_atom_forge_pop(&self->forge, &frame);
	}

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
					self->ui_active = true;
					self->send_settings_to_ui = true;
				} else if (obj->body.otype == self->uris.ui_off) {
					/* UI was closed */
					self->ui_active = false;
				}
			}
			ev = lv2_atom_sequence_next(ev);
		}
	}

	if (self->stcor) {
		self->stcor->process(self->input[0], self->input[1] , n_samples);
		*self->p_phase = self->stcor->read();
	}

	/* if UI is active, send raw audio data to GUI */
	if (self->ui_active && capacity_ok) {
		if (self->n_channels == 2) {
			tx_rawstereo(&self->forge, &self->uris, n_samples,
					self->input[0], self->input[1]);
		} else {
			for (uint32_t c = 0; c < self->n_channels; ++c) {
				tx_rawaudio(&self->forge, &self->uris, c, n_samples, self->input[c]);
			}
		}
	}

	/* close off atom-sequence */
	lv2_atom_forge_pop(&self->forge, &self->frame);

	/* if not processing in-place, forward audio */
	for (uint32_t c = 0; c < self->n_channels; ++c) {
		if (self->input[c] != self->output[c]) {
			memcpy(self->output[c], self->input[c], sizeof(float) * n_samples);
		}
	}

}

static void
xfer_cleanup(LV2_Handle handle)
{
	Xfer* self = (Xfer*)handle;
	delete self->stcor;
	free(handle);
}


#define MXFERDESC(ID, NAME) \
static const LV2_Descriptor descriptor ## ID = { \
  MTR_URI NAME,      \
  xfer_instantiate,  \
  xfer_connect_port, \
  NULL,              \
  xfer_run,          \
  NULL,              \
  xfer_cleanup,      \
  extension_data     \
};

MXFERDESC(MultiPhase2, "phasewheel");
MXFERDESC(StereoScope, "stereoscope");
