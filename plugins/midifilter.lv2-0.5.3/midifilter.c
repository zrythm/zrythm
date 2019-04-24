/* midifilter.lv2
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifdef _WIN32
#define random() rand()
#define srandom(X) srand(X)
#endif

#include "midifilter.h"

/******************************************************************************
 * common 'helper' functions
 */
static int midi_limit_val(const int d) {
	if (d < 0) return 0;
	if (d > 127) return 127;
	return d;
}

static int midi_limit_chn(const int c) {
	if (c < 0) return 0;
	if (c > 15) return 15;
	return c;
}

static int midi_valid(const int d) {
	if (d >=0 && d < 128) return 1;
	return 0;
}

static int midi_14bit(const uint8_t * const b) {
	return ((b[1]) | (b[2]<<7));
}

static int midi_is_panic(const uint8_t * const b, const int s) {
	if (s == 3
			&& (b[0] & 0xf0) == MIDI_CONTROLCHANGE
			&& ( (b[1]&0x7f) == 123 || (b[1]&0x7f) == 120 )
			&& (b[2]&0x7f) == 0)
		return 1;
	return 0;
}

static float normrand(const float dev) {
	static char initialized = 0;
	static float randmem;

	if (!initialized) {
		randmem =  2.0 * random() / (float)RAND_MAX - 1;
		initialized = 1;
	}

	float U = 2.0 * random() / (float)RAND_MAX - 1; //rand E(-1,1)
	float S = SQUARE(U) + SQUARE(randmem); //map 2 random vars to unit circle

	if(S >= 1.0f) {
		//repull RV if outside unit circle
		U = 2.0* random() / (float)RAND_MAX - 1;
		S = SQUARE(U) + SQUARE(randmem);
		if(S >= 1.0f) {
			U = 2.0* random() / (float)RAND_MAX - 1;
			S = SQUARE(U) + SQUARE(randmem);
			if(S >= 1.0f) {
				U=0;
			}
		}
	}
	randmem = U; //store RV for next round
	return U ? (dev * U * sqrt(-2.0 * log(S) / S)) : 0;
}

/**
 * add a midi message to the output port
 */
void
forge_midimessage(MidiFilter* self,
		uint32_t tme,
		const uint8_t* const buffer,
		uint32_t size)
{
	LV2_Atom midiatom;
	midiatom.type = self->uris.midi_MidiEvent;
	midiatom.size = size;

	if (0 == lv2_atom_forge_frame_time(&self->forge, tme)) return;
	if (0 == lv2_atom_forge_raw(&self->forge, &midiatom, sizeof(LV2_Atom))) return;
	if (0 == lv2_atom_forge_raw(&self->forge, buffer, size)) return;
	lv2_atom_forge_pad(&self->forge, sizeof(LV2_Atom) + size);
}

/******************************************************************************
 * include vairant code
 */

#define MX_CODE
#include "filters.c"
#undef MX_CODE

/**
 * Update the current position based on a host message.  This is called by
 * run() when a time:Position is received.
 */
static void
update_position(MidiFilter* self, const LV2_Atom_Object* obj)
{
	const MidiFilterURIs* uris = &self->uris;

	// Received new transport position/speed
	LV2_Atom *beat = NULL, *bpm = NULL, *speed = NULL;
	LV2_Atom *fps = NULL, *frame = NULL;
	lv2_atom_object_get(obj,
	                    uris->time_barBeat, &beat,
	                    uris->time_beatsPerMinute, &bpm,
	                    uris->time_speed, &speed,
	                    uris->time_frame, &frame,
	                    uris->time_fps, &fps,
	                    NULL);
	if (bpm && bpm->type == uris->atom_Float) {
		// Tempo changed, update BPM
		self->bpm = ((LV2_Atom_Float*)bpm)->body;
		self->available_info |= NFO_BPM;
	}
	if (speed && speed->type == uris->atom_Float) {
		// Speed changed, e.g. 0 (stop) to 1 (play)
		self->speed = ((LV2_Atom_Float*)speed)->body;
		self->available_info |= NFO_SPEED;
	}
	if (beat && beat->type == uris->atom_Float) {
		const double samples_per_beat = 60.0 / self->bpm * self->samplerate;
		self->bar_beats    = ((LV2_Atom_Float*)beat)->body;
		self->beat_beats   = self->bar_beats - floor(self->bar_beats);
		self->pos_bbt      = self->beat_beats * samples_per_beat;
		self->available_info |= NFO_BEAT;
	}
	if (fps && fps->type == uris->atom_Float) {
		self->frames_per_second = ((LV2_Atom_Float*)fps)->body;
		self->available_info |= NFO_FPS;
	}
	if (frame && frame->type == uris->atom_Long) {
		self->pos_frame = ((LV2_Atom_Long*)frame)->body;
		self->available_info |= NFO_FRAME;
	}
}
/******************************************************************************
 * LV2
 */

static void
run(LV2_Handle instance, uint32_t n_samples)
{
	int i;
	MidiFilter* self = (MidiFilter*)instance;
	self->n_samples = n_samples;

	if (!self->midiout || !self->midiin) {
		/* eg. ARDOUR::LV2Plugin::latency_compute_run()
		 * -> midi ports are not yet connected
		 */
		goto out;
	}

	/* prepare midiout port */
	const uint32_t capacity = self->midiout->atom.size;
	lv2_atom_forge_set_buffer(&self->forge, (uint8_t*)self->midiout, capacity);
	lv2_atom_forge_sequence_head(&self->forge, &self->frame, 0);

	if (self->preproc_fn) {
		self->preproc_fn(self);
	}

	/* process events on the midiin port */
	LV2_Atom_Event* ev = lv2_atom_sequence_begin(&(self->midiin)->body);
	while(!lv2_atom_sequence_is_end(&(self->midiin)->body, (self->midiin)->atom.size, ev)) {
		if (ev->body.type == self->uris.midi_MidiEvent) {
			self->filter_fn(self, ev->time.frames, (uint8_t*)(ev+1), ev->body.size);
		}
		else if (ev->body.type == self->uris.atom_Blank || ev->body.type == self->uris.atom_Object) {
			const LV2_Atom_Object* obj = (LV2_Atom_Object*)&ev->body;
			if (obj->body.otype == self->uris.time_Position) {
				update_position(self, obj);
			}
		}
		ev = lv2_atom_sequence_next(ev);
	}

	if (self->postproc_fn) {
		self->postproc_fn(self);
	}

	/* increment position for next cycle */
	if (self->available_info & NFO_BEAT) {
		float bpm = self->bpm;
		if (self->available_info & NFO_SPEED) {
			bpm *= self->speed;
		}
		if (bpm != 0) {
			const double samples_per_beat = 60.0 * self->samplerate / bpm;
			self->bar_beats    += (double) n_samples / samples_per_beat;
			self->beat_beats   = self->bar_beats - floor(self->bar_beats);
			self->pos_bbt      = self->beat_beats * samples_per_beat;
		}
	}
	if (self->available_info & NFO_FRAME) {
		self->pos_frame += n_samples;
	}

out:
	if (self->latency_port) {
		*self->latency_port = self->latency;
	}

	for (i = 0 ; i < MAXCFG ; ++i) {
		if (!self->cfg[i]) continue;
		self->lcfg[i] = *self->cfg[i];
	}
}


static inline void
map_mf_uris(LV2_URID_Map* map, MidiFilterURIs* uris)
{
	uris->atom_Blank         = map->map(map->handle, LV2_ATOM__Blank);
	uris->atom_Object        = map->map(map->handle, LV2_ATOM__Object);
	uris->midi_MidiEvent     = map->map(map->handle, LV2_MIDI__MidiEvent);
	uris->atom_Sequence      = map->map(map->handle, LV2_ATOM__Sequence);

	uris->atom_Long           = map->map(map->handle, LV2_ATOM__Long);
	uris->atom_Float          = map->map(map->handle, LV2_ATOM__Float);
	uris->time_Position       = map->map(map->handle, LV2_TIME__Position);
	uris->time_barBeat        = map->map(map->handle, LV2_TIME__barBeat);
	uris->time_beatsPerMinute = map->map(map->handle, LV2_TIME__beatsPerMinute);
	uris->time_speed          = map->map(map->handle, LV2_TIME__speed);
	uris->time_frame          = map->map(map->handle, LV2_TIME__frame);
	uris->time_fps            = map->map(map->handle, LV2_TIME__framesPerSecond);
}

static LV2_Handle
instantiate(const LV2_Descriptor*         descriptor,
		double                    rate,
		const char*               bundle_path,
		const LV2_Feature* const* features)
{
	int i;
	MidiFilter* self = (MidiFilter*)calloc(1, sizeof(MidiFilter));
	if (!self) return NULL;

	for (i=0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID__map)) {
			self->map = (LV2_URID_Map*)features[i]->data;
		}
	}

	if (!self->map) {
		fprintf(stderr, "midifilter.lv2 error: Host does not support urid:map\n");
		free(self);
		return NULL;
	}

	map_mf_uris(self->map, &self->uris);
	lv2_atom_forge_init(&self->forge, self->map);
	self->samplerate = rate;
	self->bpm = 120;

	if (0) ;
#define MX_FILTER
#include "filters.c"
#undef MX_FILTER
	else {
		fprintf(stderr, "midifilter.lv2 error: unsupported plugin function.\n");
		free(self);
		return NULL;
	}

	for (i=0; i < MAXCFG; ++i) {
		self->lcfg[i] = 0;
	}

	return (LV2_Handle)self;
}

#define CFG_PORT(n) \
	case (n+3): \
		self->cfg[n] = (float*)data; \
		break;

static void
connect_port(LV2_Handle    instance,
		uint32_t   port,
		void*      data)
{
	MidiFilter* self = (MidiFilter*)instance;

	switch (port) {
		case 0:
			self->midiin = (const LV2_Atom_Sequence*)data;
			break;
		case 1:
			self->midiout = (LV2_Atom_Sequence*)data;
			break;
		case 2:
			self->latency_port = (float*)data;
			break;
		LOOP_CFG(CFG_PORT)
		default:
			break;
	}
}

static void
cleanup(LV2_Handle instance)
{
	MidiFilter* self = (MidiFilter*)instance;
	if (self->cleanup_fn) {
		self->cleanup_fn(self);
	}

	free(instance);
}

const void*
extension_data(const char* uri)
{
	return NULL;
}

#define MX_DESC
#include "filters.c"
#undef MX_DESC

#define LV2DESC(ID) \
	case ID: return &(descriptor ## ID);


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
	LOOP_DESC(LV2DESC)
	default: return NULL;
	}
}
/* vi:set ts=8 sts=8 sw=8: */
