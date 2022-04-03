/*
 * Copyright (C) 2020-2022 Alexandros Theodotou <alex at zrythm dot org>
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
 * Common logic for tracks that can be group targets.
 */

#ifndef __AUDIO_GROUP_TARGET_TRACK_H__
#define __AUDIO_GROUP_TARGET_TRACK_H__

#include <stdbool.h>

typedef struct Track Track;

#define TRACK_CAN_BE_GROUP_TARGET(tr) \
  (IS_TRACK (tr) \
   && (tr->type == TRACK_TYPE_AUDIO_GROUP || tr->type == TRACK_TYPE_MIDI_GROUP || tr->type == TRACK_TYPE_INSTRUMENT || tr->type == TRACK_TYPE_MASTER))

void
group_target_track_init_loaded (Track * self);

void
group_target_track_init (Track * track);

/**
 * Removes a child track from the list of children.
 */
void
group_target_track_remove_child (
  Track *      self,
  unsigned int child_name_hash,
  bool         disconnect,
  bool         recalc_graph,
  bool         pub_events);

/**
 * Remove all known children.
 *
 * @param disconnect Also route the children to "None".
 */
void
group_target_track_remove_all_children (
  Track * self,
  bool    disconnect,
  bool    recalc_graph,
  bool    pub_events);

/**
 * Adds a child track to the list of children.
 *
 * @param connect Connect the child to the group track.
 */
void
group_target_track_add_child (
  Track *      self,
  unsigned int child_name_hash,
  bool         connect,
  bool         recalc_graph,
  bool         pub_events);

bool
group_target_track_validate (Track * self);

void
group_target_track_add_children (
  Track *        self,
  unsigned int * children,
  int            num_children,
  bool           connect,
  bool           recalc_graph,
  bool           pub_events);

/**
 * Returns the index of the child matching the
 * given hash.
 */
NONNULL
PURE int
group_target_track_find_child (
  Track *      self,
  unsigned int track_name_hash);

#endif /* __AUDIO_GROUP_TARGET_TRACK_H__ */
