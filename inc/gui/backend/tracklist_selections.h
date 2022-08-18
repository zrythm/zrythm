// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Tracklist selections.
 */

#ifndef __ACTIONS_TRACKLIST_SELECTIONS_H__
#define __ACTIONS_TRACKLIST_SELECTIONS_H__

#include "audio/track.h"
#include "utils/yaml.h"

typedef enum TracklistSelectionsActionType
  TracklistSelectionsActionType;
typedef enum EditTracksActionType EditTracksActionType;

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define TRACKLIST_SELECTIONS_SCHEMA_VERSION 1

#define TRACKLIST_SELECTIONS (PROJECT->tracklist_selections)

/**
 * Selections to be used for the tracklist's current
 * selections, copying, undoing, etc.
 *
 * TracklistSelections are special in that they do
 * not allow 0 selections. There must always be a
 * Track selected.
 */
typedef struct TracklistSelections
{
  int schema_version;

  /** Selected Tracks. */
  Track * tracks[600];
  int     num_tracks;

  /** Whether these are the project selections. */
  bool is_project;

  /**
   * Flag to free tracks even if these are the
   * project selections (e.g. when temporarily
   * cloning the project to save).
   */
  bool free_tracks;
} TracklistSelections;

static const cyaml_schema_field_t
  tracklist_selections_fields_schema[] = {
    YAML_FIELD_INT (TracklistSelections, schema_version),
    YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
      TracklistSelections,
      tracks,
      track_schema),
    YAML_FIELD_INT (TracklistSelections, is_project),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t tracklist_selections_schema = {
  YAML_VALUE_PTR (
    TracklistSelections,
    tracklist_selections_fields_schema),
};

void
tracklist_selections_init_loaded (TracklistSelections * ts);

/**
 * @param is_project Whether these selections are
 *   the project selections (as opposed to clones).
 */
TracklistSelections *
tracklist_selections_new (bool is_project);

/**
 * Clone the struct for copying, undoing, etc.
 */
NONNULL_ARGS (1)
TracklistSelections *
tracklist_selections_clone (
  TracklistSelections * src,
  GError **             error);

/**
 * Gets highest track in the selections.
 *
 * If transient is 1, transient objects rae checked
 * instead.
 */
Track *
tracklist_selections_get_highest_track (
  TracklistSelections * self);

/**
 * Gets lowest track in the selections.
 *
 * If transient is 1, transient objects rae checked
 * instead.
 */
Track *
tracklist_selections_get_lowest_track (
  TracklistSelections * self);

void
tracklist_selections_add_track (
  TracklistSelections * self,
  Track *               track,
  bool                  fire_events);

void
tracklist_selections_add_tracks_in_range (
  TracklistSelections * self,
  int                   min_pos,
  int                   max_pos,
  bool                  fire_events);

/**
 * Clears the selections.
 */
void
tracklist_selections_clear (TracklistSelections * self);

/**
 * Make sure all children of foldable tracks in
 * the selection are also selected.
 */
void
tracklist_selections_select_foldable_children (
  TracklistSelections * self);

/**
 * Handle a click selection.
 */
void
tracklist_selections_handle_click (
  Track * track,
  bool    ctrl,
  bool    shift,
  bool    dragged);

bool
tracklist_selections_contains_uninstantiated_plugin (
  const TracklistSelections * self);

bool
tracklist_selections_contains_undeletable_track (
  const TracklistSelections * self);

bool
tracklist_selections_contains_uncopyable_track (
  const TracklistSelections * self);

/**
 * Returns whether the tracklist selections contains a track
 * that cannot have automation lanes.
 */
bool
tracklist_selections_contains_non_automatable_track (
  const TracklistSelections * self);

/**
 * Returns whether the selections contain a soloed
 * track if @ref soloed is true or an unsoloed track
 * if @ref soloed is false.
 *
 * @param soloed Whether to check for soloed or
 *   unsoloed tracks.
 */
bool
tracklist_selections_contains_soloed_track (
  TracklistSelections * self,
  bool                  soloed);

/**
 * Returns whether the selections contain a listened
 * track if @ref listened is true or an unlistened
 * track if @ref listened is false.
 *
 * @param listened Whether to check for listened or
 *   unlistened tracks.
 */
bool
tracklist_selections_contains_listened_track (
  TracklistSelections * self,
  bool                  listened);

/**
 * Returns whether the selections contain a muted
 * track if @ref muted is true or an unmuted track
 * if @ref muted is false.
 *
 * @param muted Whether to check for muted or
 *   unmuted tracks.
 */
bool
tracklist_selections_contains_muted_track (
  TracklistSelections * self,
  bool                  muted);

bool
tracklist_selections_contains_enabled_track (
  TracklistSelections * self,
  bool                  enabled);

/**
 * Returns if the Track is selected or not.
 */
bool
tracklist_selections_contains_track (
  TracklistSelections * self,
  Track *               track);

bool
tracklist_selections_contains_track_index (
  TracklistSelections * self,
  int                   track_idx);

void
tracklist_selections_remove_track (
  TracklistSelections * ts,
  Track *               track,
  int                   fire_events);

/**
 * Selects a single track after clearing the
 * selections.
 */
void
tracklist_selections_select_single (
  TracklistSelections * ts,
  Track *               track,
  bool                  fire_events);

/**
 * Selects all Track's.
 *
 * @param visible_only Only select visible tracks.
 */
void
tracklist_selections_select_all (
  TracklistSelections * ts,
  int                   visible_only);

/**
 * Selects the last visible track after clearing the
 * selections.
 */
void
tracklist_selections_select_last_visible (
  TracklistSelections * ts);

/**
 * Toggle visibility of the selected tracks.
 */
void
tracklist_selections_toggle_visibility (
  TracklistSelections * ts);

/**
 * Toggle pin/unpin of the selected tracks.
 */
void
tracklist_selections_toggle_pinned (TracklistSelections * ts);

/**
 * For debugging.
 */
void
tracklist_selections_print (TracklistSelections * self);

/**
 * To be called after receiving tracklist selections
 * from the clipboard.
 */
void
tracklist_selections_post_deserialize (
  TracklistSelections * self);

void
tracklist_selections_paste_to_pos (
  TracklistSelections * ts,
  int                   pos);

/**
 * Sorts the tracks by position.
 *
 * @param asc Ascending or not.
 */
void
tracklist_selections_sort (
  TracklistSelections * self,
  bool                  asc);

NONNULL
void
tracklist_selections_get_plugins (
  TracklistSelections * self,
  GPtrArray *           arr);

/**
 * Marks the tracks to be bounced.
 *
 * @param with_parents Also mark all the track's
 *   parents recursively.
 * @param mark_master Also mark the master track.
 *   Set to true when exporting the mixdown, false
 *   otherwise.
 */
void
tracklist_selections_mark_for_bounce (
  TracklistSelections * ts,
  bool                  with_parents,
  bool                  mark_master);

void
tracklist_selections_free (TracklistSelections * self);

/**
 * @}
 */

#endif
