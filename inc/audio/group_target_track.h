// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
