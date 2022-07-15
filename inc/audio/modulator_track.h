/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Object to hold information for the Modulator track.
 */

#ifndef __AUDIO_MODULATOR_TRACK_H__
#define __AUDIO_MODULATOR_TRACK_H__

#include <stdint.h>

#include "audio/track.h"
#include "utils/types.h"

/**
 * @addtogroup audio
 *
 * @{
 */

#define P_MODULATOR_TRACK (TRACKLIST->modulator_track)

/**
 * Inserts and connects a Modulator to the Track.
 *
 * @param replace_mode Whether to perform the
 *   operation in replace mode (replace current
 *   modulator if true, not touching other
 *   modulators, or push other modulators forward
 *   if false).
 */
void
modulator_track_insert_modulator (
  Track *  self,
  int      slot,
  Plugin * modulator,
  bool     replace_mode,
  bool     confirm,
  bool     gen_automatables,
  bool     recalc_graph,
  bool     pub_events);

/**
 * Removes a plugin at pos from the track.
 *
 * @param replacing Whether replacing the modulator.
 *   If this is false, modulators after this slot
 *   will be pushed back.
 * @param deleting_modulator
 * @param deleting_track If true, the automation
 *   tracks associated with the plugin are not
 *   deleted at this time.
 * @param recalc_graph Recalculate mixer graph.
 */
void
modulator_track_remove_modulator (
  Track * self,
  int     slot,
  bool    replacing,
  bool    deleting_modulator,
  bool    deleting_track,
  bool    recalc_graph);

/**
 * Creates the default modulator track.
 */
Track *
modulator_track_default (int track_pos);

/**
 * Inits the modulator track.
 */
void
modulator_track_init (Track * track);

/**
 * @}
 */

#endif
