/* midigen -- LV2 midi event generator
 *
 * Copyright (C) 2016 Robin Gareus <robin@gareus.org>
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
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <lv2/lv2plug.in/ns/ext/log/logger.h>

#define GEN_URI "http://gareus.org/oss/lv2/midigen#zrythm"

typedef struct {
	float   beat_time;
	uint8_t size;
	uint8_t event[3];
} MIDISequence;

#include "sequences.h"

#define N_MIDI_SEQ (sizeof (sequences) / sizeof(MIDISequence*))

typedef struct {
	/* ports */
	LV2_Atom_Sequence* midiout;
	float* p_seq;
	float* p_bpm;
	float* p_prog;

	/* atom-forge and URI mapping */
	LV2_Atom_Forge forge;
	LV2_Atom_Forge_Frame frame;
	LV2_URID midi_MidiEvent;

	/* LV2 Output */
	LV2_Log_Log* log;
	LV2_Log_Logger logger;

	/* Cached Port */
	float bpm;
	float seq;

	/* Settings */
	float sample_rate;
	float spb; // samples / beat

	uint32_t sid; // sequence ID
	uint32_t pos; // event number
	int32_t  tme;  // sample time

} MidiGen;

/* *****************************************************************************
 * helper functions
 */

/**
 * add a midi message to the output port
 */
static void
forge_midimessage (MidiGen* self,
                   uint32_t tme,
                   const uint8_t* const buffer,
                   uint32_t size)
{
	LV2_Atom midiatom;
	midiatom.type = self->midi_MidiEvent;
	midiatom.size = size;

	if (0 == lv2_atom_forge_frame_time (&self->forge, tme)) return;
	if (0 == lv2_atom_forge_raw (&self->forge, &midiatom, sizeof (LV2_Atom))) return;
	if (0 == lv2_atom_forge_raw (&self->forge, buffer, size)) return;
	lv2_atom_forge_pad (&self->forge, sizeof (LV2_Atom) + size);
}

static void
midi_panic (MidiGen* self)
{
	uint8_t event[3];
	event[2] = 0;

	for (uint32_t c = 0; c < 0xf; ++c) {
		event[0] = 0xb0 | c;
		event[1] = 0x40; // sustain pedal
		forge_midimessage (self, 0, event, 3);
		event[1] = 0x7b; // all notes off
		forge_midimessage (self, 0, event, 3);
#if 0
		event[1] = 0x78; // all sound off
		forge_midimessage (self, 0, event, 3);
#endif
	}
}

/* *****************************************************************************
 * LV2 Plugin
 */

static LV2_Handle
instantiate (const LV2_Descriptor*     descriptor,
             double                    rate,
             const char*               bundle_path,
             const LV2_Feature* const* features)
{
	MidiGen* self = (MidiGen*)calloc (1, sizeof (MidiGen));
	LV2_URID_Map* map = NULL;

	int i;
	for (i=0; features[i]; ++i) {
		if (!strcmp (features[i]->URI, LV2_URID__map)) {
			map = (LV2_URID_Map*)features[i]->data;
		} else if (!strcmp (features[i]->URI, LV2_LOG__log)) {
			self->log = (LV2_Log_Log*)features[i]->data;
		}
	}

	lv2_log_logger_init (&self->logger, map, self->log);

	if (!map) {
		lv2_log_error (&self->logger, "MidiGen.lv2 error: Host does not support urid:map\n");
		free (self);
		return NULL;
	}

	lv2_atom_forge_init (&self->forge, map);
	self->midi_MidiEvent = map->map (map->handle, LV2_MIDI__MidiEvent);

	self->sample_rate = rate;
	self->bpm = 120;
	self->spb = self->sample_rate * 60.f / self->bpm;

	return (LV2_Handle)self;
}

static void
connect_port (LV2_Handle instance,
              uint32_t   port,
              void*      data)
{
	MidiGen* self = (MidiGen*)instance;

	switch (port) {
		case 0:
			self->midiout = (LV2_Atom_Sequence*)data;
			break;
		case 1:
			self->p_seq = (float*)data;
			break;
		case 2:
			self->p_bpm = (float*)data;
			break;
		case 3:
			self->p_prog = (float*)data;
			break;
		default:
			break;
	}
}


static void
run (LV2_Handle instance, uint32_t n_samples)
{
	MidiGen* self = (MidiGen*)instance;
	if (!self->midiout) {
		return;
	}

	const uint32_t capacity = self->midiout->atom.size;
	lv2_atom_forge_set_buffer (&self->forge, (uint8_t*)self->midiout, capacity);
	lv2_atom_forge_sequence_head (&self->forge, &self->frame, 0);

	if (*self->p_seq != self->seq) {
		self->seq = *self->p_seq;
		self->sid = (int)rintf (self->seq) % N_MIDI_SEQ;
		self->pos = 0;
		self->tme = 0;
		midi_panic (self);
	}

	MIDISequence const* seq = sequences[self->sid];
	uint32_t pos = self->pos;

	if (*self->p_bpm != self->bpm) {
		const float old = self->spb;
		self->bpm = *self->p_bpm;
		self->spb = self->sample_rate * 60.f / self->bpm;
		if (self->spb < 20) { self->spb = 20; }
		if (self->spb > 3 * self->sample_rate) { self->spb = self->sample_rate; }

		const int32_t old_off = seq[pos].beat_time * old;
		const int32_t new_off = seq[pos].beat_time * self->spb;
		self->tme += (new_off - old_off);
	}

	int32_t tme = self->tme;
	const float spb = self->spb;

	while (1) {
		const int32_t ev_beat_time = seq[pos].beat_time * spb - tme;
		if (ev_beat_time < 0) {
			break;
		}
		if (ev_beat_time >= n_samples) {
			break;
		}

		forge_midimessage (self, ev_beat_time, seq[pos].event, seq[pos].size);
		++pos;

		if (seq[pos].event[0] == 0xff && seq[pos].event[1] == 0xff) {
			tme -= seq[pos].beat_time * spb;
			pos = 0;
		}
	}
	self->tme = tme + n_samples;
	self->pos = pos;

	if (self->p_prog) {
#if 0
		*self->p_prog = 100.f * tme / (float)(seq[seq_len[self->sid] - 1].beat_time * spb);
#else
		*self->p_prog = 100.f * pos / (seq_len[self->sid] - 1.f);
#endif
	}
}

static void
cleanup (LV2_Handle instance)
{
	free (instance);
}

static const void*
extension_data (const char* uri)
{
	return NULL;
}

static const LV2_Descriptor descriptor = {
	GEN_URI,
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
