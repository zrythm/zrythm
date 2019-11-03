/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of ZPlugins
 *
 * ZPlugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * ZPlugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU General Affero Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Common code for both the DSP and the UI.
 */

#ifndef __Z_LFO_COMMON_H__
#define __Z_LFO_COMMON_H__

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"

typedef struct ZLfoUris
{
  LV2_URID atom_eventTransfer;
  LV2_URID atom_Blank;
	LV2_URID atom_Object;
	LV2_URID atom_Float;
  LV2_URID atom_Int;
} ZLfoUris;

typedef enum PortIndex
{
  /** GUI to plugin communication. */
  LFO_CONTROL,
  /** Plugin to UI communication. */
  LFO_NOTIFY,
  LFO_FREQ,
  LFO_PHASE,
  LFO_FOLLOW_TIME,
  LFO_SINE,
  LFO_SAW_UP,
  LFO_SAW_DOWN,
  LFO_TRIANGLE,
  LFO_SQUARE,
  LFO_RANDOM,
} PortIndex;

static inline void
map_uris (
  LV2_URID_Map* map,
  ZLfoUris* uris)
{
	uris->atom_Blank =
    map->map (map->handle, LV2_ATOM__Blank);
	uris->atom_Object =
    map->map (map->handle, LV2_ATOM__Object);
	uris->atom_Float =
    map->map (map->handle, LV2_ATOM__Float);
	uris->atom_Int =
    map->map (map->handle, LV2_ATOM__Int);
	uris->atom_eventTransfer =
    map->map (map->handle, LV2_ATOM__eventTransfer);
}

#endif
