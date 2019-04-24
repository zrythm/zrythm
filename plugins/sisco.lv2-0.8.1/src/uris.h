/* simple scope -- example pipe raw audio data to UI
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

#ifndef SCO_URIS_H
#define SCO_URIS_H

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"

#define SCO_URI "http://gareus.org/oss/lv2/sisco"

#ifdef HAVE_LV2_1_8
#define x_forge_object lv2_atom_forge_object
#else
#define x_forge_object lv2_atom_forge_blank
#endif

typedef struct {
	LV2_URID atom_Blank;
	LV2_URID atom_Object;
	LV2_URID atom_Vector;
	LV2_URID atom_Float;
	LV2_URID atom_Int;
	LV2_URID atom_eventTransfer;
	LV2_URID rawaudio;
	LV2_URID channelid;
	LV2_URID audiodata;

	LV2_URID samplerate;
	LV2_URID ui_on;
	LV2_URID ui_off;
	LV2_URID ui_state;

	LV2_URID ui_state_chn;
	LV2_URID ui_state_grid;
	LV2_URID ui_state_trig;
	LV2_URID ui_state_curs;
	LV2_URID ui_state_misc; // bitwise bool, currently only amp-lock bit:1
} ScoLV2URIs;

static inline void
map_sco_uris(LV2_URID_Map* map, ScoLV2URIs* uris) {
	uris->atom_Blank         = map->map(map->handle, LV2_ATOM__Blank);
	uris->atom_Object        = map->map(map->handle, LV2_ATOM__Object);
	uris->atom_Vector        = map->map(map->handle, LV2_ATOM__Vector);
	uris->atom_Float         = map->map(map->handle, LV2_ATOM__Float);
	uris->atom_Int           = map->map(map->handle, LV2_ATOM__Int);
	uris->atom_eventTransfer = map->map(map->handle, LV2_ATOM__eventTransfer);
	uris->rawaudio           = map->map(map->handle, SCO_URI "#rawaudio");
	uris->audiodata          = map->map(map->handle, SCO_URI "#audiodata");
	uris->channelid          = map->map(map->handle, SCO_URI "#channelid");
	uris->samplerate         = map->map(map->handle, SCO_URI "#samplerate");
	uris->ui_on              = map->map(map->handle, SCO_URI "#ui_on");
	uris->ui_off             = map->map(map->handle, SCO_URI "#ui_off");
	uris->ui_state           = map->map(map->handle, SCO_URI "#ui_state");
	uris->ui_state_chn       = map->map(map->handle, SCO_URI "#ui_state_chn");
	uris->ui_state_grid      = map->map(map->handle, SCO_URI "#ui_state_grid");
	uris->ui_state_trig      = map->map(map->handle, SCO_URI "#ui_state_trig");
	uris->ui_state_curs      = map->map(map->handle, SCO_URI "#ui_state_curs");
	uris->ui_state_misc      = map->map(map->handle, SCO_URI "#ui_state_misc");
}

struct triggerstate {
	float mode;
	float type;
	float xpos;
	float hold;
	float level;
};

struct channelstate {
	float gain;
	float xoff;
	float yoff;
	float opts;
};

struct cursorstate {
	int32_t xpos[2];
	int32_t chn[2];
};

#define MAX_CHANNELS (4)

#endif
