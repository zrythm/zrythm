/* Tuner.lv2
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

#ifndef TUNA_H
#define TUNA_H

#ifndef TUNA_URI
#define TUNA_URI "http://gareus.org/oss/lv2/tuna#"
#endif

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"

#ifdef HAVE_LV2_1_8
#define x_forge_object lv2_atom_forge_object
#else
#define x_forge_object lv2_atom_forge_blank
#endif

typedef enum {
	TUNA_CONTROL = 0,
	TUNA_NOTIFY,
	TUNA_AIN,
	TUNA_AOUT,
	TUNA_MODE,
	TUNA_TUNING,
	TUNA_RMS,
	TUNA_FREQ_OUT,
	TUNA_OCTAVE,
	TUNA_NOTE,
	TUNA_CENT,
	TUNA_ERROR,
	TUNA_STROBE,
	TUNA_T_RMS,
	TUNA_T_FLT,
	TUNA_T_FFT,
	TUNA_T_OVR,
	TUNA_T_FUN,
	TUNA_T_OCT,
	TUNA_T_OVT,
} PortIndexTuna;


typedef struct {
	LV2_URID atom_Blank;
	LV2_URID atom_Object;
	LV2_URID atom_Vector;
	LV2_URID atom_Float;
	LV2_URID atom_Int;
	LV2_URID atom_eventTransfer;

	LV2_URID spectrum;
	LV2_URID spec_data_x;
	LV2_URID spec_data_y;
	LV2_URID ui_on;
	LV2_URID ui_off;
} TunaLV2URIs;

static inline void
map_tuna_uris(LV2_URID_Map* map, TunaLV2URIs* uris) {
	uris->atom_Blank         = map->map(map->handle, LV2_ATOM__Blank);
	uris->atom_Object        = map->map(map->handle, LV2_ATOM__Object);
	uris->atom_Vector        = map->map(map->handle, LV2_ATOM__Vector);
	uris->atom_Float         = map->map(map->handle, LV2_ATOM__Float);
	uris->atom_Int           = map->map(map->handle, LV2_ATOM__Int);
	uris->atom_eventTransfer = map->map(map->handle, LV2_ATOM__eventTransfer);

	uris->spectrum           = map->map(map->handle, TUNA_URI "#_spectrum");
	uris->spec_data_x        = map->map(map->handle, TUNA_URI "#_data_x");
	uris->spec_data_y        = map->map(map->handle, TUNA_URI "#_data_y");
	uris->ui_on              = map->map(map->handle, TUNA_URI "#_ui_on");
	uris->ui_off             = map->map(map->handle, TUNA_URI "#_ui_off");
}

#endif
