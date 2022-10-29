// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Tracklist backend.
 */

#ifndef __AUDIO_TRACKLIST_H__
#define __AUDIO_TRACKLIST_H__

#include "audio/engine.h"
#include "audio/track.h"
#include "gui/widgets/track.h"

typedef struct Track                  Track;
typedef struct _TracklistWidget       TracklistWidget;
typedef struct _PinnedTracklistWidget PinnedTracklistWidget;
typedef struct Track                  ChordTrack;
typedef struct SupportedFile          SupportedFile;

/**
 * @addtogroup audio
 *
 * @{
 */

#define TRACKLIST_SCHEMA_VERSION 1

#define TRACKLIST (PROJECT->tracklist)
#define MAX_TRACKS 3000

#define tracklist_is_in_active_project(self) \
  (self->project == PROJECT \
   || \
   (self->sample_processor \
    && \
    sample_processor_is_in_active_project ( \
      self->sample_processor)))

#define tracklist_is_auditioner(self) \
  (self->sample_processor \
   && tracklist_is_in_active_project (self))

/**
 * Used in track search functions.
 */
typedef enum TracklistPinOption
{
  TRACKLIST_PIN_OPTION_PINNED_ONLY,
  TRACKLIST_PIN_OPTION_UNPINNED_ONLY,
  TRACKLIST_PIN_OPTION_BOTH,
} TracklistPinOption;

/**
 * The Tracklist contains all the tracks in the
 * Project.
 *
 * There should be a clear separation between the
 * Tracklist and the Mixer. The Tracklist should be
 * concerned with Tracks in the arranger, and the
 * Mixer should be concerned with Channels, routing
 * and Port connections.
 */
typedef struct Tracklist
{
  int schema_version;

  /**
   * All tracks that exist.
   *
   * These should always be sorted in the same way
   * they should appear in the GUI and include
   * hidden tracks.
   *
   * Pinned tracks should have lower indices. Ie,
   * the sequence must be:
   * {
   *   pinned track,
   *   pinned track,
   *   pinned track,
   *   track,
   *   track,
   *   track,
   *   ...
   * }
   */
  Track * tracks[MAX_TRACKS];

  int num_tracks;

  /** The chord track, for convenience. */
  Track * chord_track;

  /** The marker track, for convenience. */
  Track * marker_track;

  /** The tempo track, for convenience. */
  Track * tempo_track;

  /** The modulator track, for convenience. */
  Track * modulator_track;

  /** The master track, for convenience. */
  Track * master_track;

  /** Non-pinned TracklistWidget. */
  TracklistWidget * widget;

  /** PinnedTracklistWidget. */
  PinnedTracklistWidget * pinned_widget;

  /**
   * Index starting from which tracks are
   * unpinned.
   *
   * Tracks before this position will be considered
   * as pinned.
   */
  int pinned_tracks_cutoff;

  /** When this is true, some tracks may temporarily
   * be moved beyond num_tracks. */
  bool swapping_tracks;

  /** Pointer to owner sample processor, if any. */
  SampleProcessor * sample_processor;

  /** Pointer to owner project, if any. */
  Project * project;
} Tracklist;

static const cyaml_schema_field_t tracklist_fields_schema[] = {
  YAML_FIELD_INT (Tracklist, schema_version),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
    Tracklist,
    tracks,
    track_schema),
  YAML_FIELD_INT (Tracklist, pinned_tracks_cutoff),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t tracklist_schema = {
  YAML_VALUE_PTR (Tracklist, tracklist_fields_schema),
};

/**
 * Initializes the tracklist when loading a project.
 */
COLD NONNULL_ARGS (1) void tracklist_init_loaded (
  Tracklist *       self,
  Project *         project,
  SampleProcessor * sample_processor);

COLD Tracklist *
tracklist_new (
  Project *         project,
  SampleProcessor * sample_processor);

/**
 * Selects or deselects all tracks.
 *
 * @note When deselecting the last track will become
 *   selected (there must always be >= 1 tracks
 *   selected).
 */
NONNULL
void
tracklist_select_all (
  Tracklist * self,
  bool        select,
  bool        fire_events);

/**
 * Finds visible tracks and puts them in given array.
 */
void
tracklist_get_visible_tracks (
  Tracklist * self,
  Track **    visible_tracks,
  int *       num_visible);

/**
 * Returns the Track matching the given name, if
 * any.
 */
Track *
tracklist_find_track_by_name (
  Tracklist *  self,
  const char * name);

/**
 * Returns the Track matching the given name, if
 * any.
 */
NONNULL
OPTIMIZE_O3
Track *
tracklist_find_track_by_name_hash (
  Tracklist *  self,
  unsigned int hash);

NONNULL
int
tracklist_contains_master_track (Tracklist * self);

NONNULL
int
tracklist_contains_chord_track (Tracklist * self);

/**
 * Prints the tracks (for debugging).
 */
NONNULL
void
tracklist_print_tracks (Tracklist * self);

/**
 * Adds given track to given spot in tracklist.
 *
 * @param publish_events Publish UI events.
 * @param recalc_graph Recalculate routing graph.
 */
NONNULL
void
tracklist_insert_track (
  Tracklist * self,
  Track *     track,
  int         pos,
  int         publish_events,
  int         recalc_graph);

/**
 * Removes a track from the Tracklist and the
 * TracklistSelections.
 *
 * Also disconnects the channel.
 *
 * @param rm_pl Remove plugins or not.
 * @param free Free the track or not (free later).
 * @param publish_events Push a track deleted event
 *   to the UI.
 * @param recalc_graph Recalculate the mixer graph.
 */
NONNULL
void
tracklist_remove_track (
  Tracklist * self,
  Track *     track,
  bool        rm_pl,
  bool        free_track,
  bool        publish_events,
  bool        recalc_graph);

/**
 * Moves a track from its current position to the
 * position given by \p pos.
 *
 * @param pos Position to insert at, or -1 to
 *   insert at the end.
 * @param always_before_pos Whether the track
 *   should always be put before the track currently
 *   at @ref pos. If this is true, when moving
 *   down, the resulting track position will be
 *   @ref pos - 1.
 * @param publish_events Push UI update events or
 *   not.
 * @param recalc_graph Recalculate routing graph.
 */
void
tracklist_move_track (
  Tracklist * self,
  Track *     track,
  int         pos,
  bool        always_before_pos,
  bool        publish_events,
  bool        recalc_graph);

/**
 * Calls tracklist_insert_track with the given
 * options.
 */
void
tracklist_append_track (
  Tracklist * self,
  Track *     track,
  int         publish_events,
  int         recalc_graph);

/**
 * Pins or unpins the Track.
 */
void
tracklist_set_track_pinned (
  Tracklist * self,
  Track *     track,
  const int   pinned,
  int         publish_events,
  int         recalc_graph);

NONNULL
bool
tracklist_validate (Tracklist * self);

/**
 * Returns the track at the given index or NULL if
 * the index is invalid.
 *
 * Not to be used in real-time code.
 */
NONNULL
HOT Track *
tracklist_get_track (Tracklist * self, int idx);

/**
 * Returns the index of the given Track.
 */
int
tracklist_get_track_pos (Tracklist * self, Track * track);

/**
 * Returns the first track found with the given
 * type.
 */
Track *
tracklist_get_track_by_type (Tracklist * self, TrackType type);

ChordTrack *
tracklist_get_chord_track (const Tracklist * self);

/**
 * Returns the first visible Track.
 *
 * @param pinned 1 to check the pinned tracklist,
 *   0 to check the non-pinned tracklist.
 */
Track *
tracklist_get_first_visible_track (
  Tracklist * self,
  const int   pinned);

/**
 * Returns the previous visible Track in the same
 * Tracklist as the given one (ie, pinned or not).
 */
Track *
tracklist_get_prev_visible_track (
  Tracklist * self,
  Track *     track);

/**
 * Returns the index of the last Track.
 *
 * @param pin_opt Pin option.
 * @param visible_only Only consider visible
 *   Track's.
 */
int
tracklist_get_last_pos (
  Tracklist *              self,
  const TracklistPinOption pin_opt,
  const bool               visible_only);

/**
 * Returns the last Track.
 *
 * @param pin_opt Pin option.
 * @param visible_only Only consider visible
 *   Track's.
 */
Track *
tracklist_get_last_track (
  Tracklist *              self,
  const TracklistPinOption pin_opt,
  const int                visible_only);

/**
 * Returns the next visible Track in the same
 * Tracklist as the given one (ie, pinned or not).
 */
Track *
tracklist_get_next_visible_track (
  Tracklist * self,
  Track *     track);

/**
 * Returns the Track after delta visible Track's.
 *
 * Negative delta searches backwards.
 *
 * This function searches tracks only in the same
 * Tracklist as the given one (ie, pinned or not).
 */
Track *
tracklist_get_visible_track_after_delta (
  Tracklist * self,
  Track *     track,
  int         delta);

/**
 * Returns the number of visible Tracks between
 * src and dest (negative if dest is before src).
 *
 * The caller is responsible for checking that
 * both tracks are in the same tracklist (ie,
 * pinned or not).
 */
int
tracklist_get_visible_track_diff (
  Tracklist *   self,
  const Track * src,
  const Track * dest);

/**
 * Multiplies all tracks' heights and returns if
 * the operation was valid.
 *
 * @param visible_only Only apply to visible tracks.
 */
bool
tracklist_multiply_track_heights (
  Tracklist * self,
  double      multiplier,
  bool        visible_only,
  bool        check_only,
  bool        fire_events);

/**
 * Handles a file drop inside the timeline or in
 * empty space in the tracklist.
 *
 * @param uri_list URI list, if URI list was dropped.
 * @param file File, if SupportedFile was dropped.
 * @param track Track, if any.
 * @param lane TrackLane, if any.
 * @param pos Position the file was dropped at, if
 *   inside track.
 * @param with_progress Whether to show a progress dialog TODO.
 * @param perform_actions Whether to perform
 *   undoable actions in addition to creating the
 *   regions/tracks.
 */
void
tracklist_import_files (
  Tracklist *     self,
  char **         uri_list,
  SupportedFile * orig_file,
  Track *         track,
  TrackLane *     lane,
  Position *      pos,
  bool            with_progress,
  bool            perform_actions);

/**
 * Handles a move or copy action based on a drag.
 *
 * @param this_track The track at the cursor (where
 *   the selection was dropped to.
 * @param location Location relative to @ref
 *   this_track.
 */
void
tracklist_handle_move_or_copy (
  Tracklist *          self,
  Track *              this_track,
  TrackWidgetHighlight location,
  GdkDragAction        action);

/**
 * Returns 1 if the track name is not taken.
 *
 * @param track_to_skip Track to skip when searching.
 */
bool
tracklist_track_name_is_unique (
  Tracklist *  self,
  const char * name,
  Track *      track_to_skip);

/**
 * Returns if the tracklist has soloed tracks.
 */
NONNULL
bool
tracklist_has_soloed (const Tracklist * self);

/**
 * Returns if the tracklist has listened tracks.
 */
NONNULL
bool
tracklist_has_listened (const Tracklist * self);

NONNULL
int
tracklist_get_num_muted_tracks (const Tracklist * self);

NONNULL
int
tracklist_get_num_soloed_tracks (const Tracklist * self);

NONNULL
int
tracklist_get_num_listened_tracks (const Tracklist * self);

/**
 * @param visible 1 for visible, 0 for invisible.
 */
int
tracklist_get_num_visible_tracks (
  Tracklist * self,
  int         visible);

/**
 * Fills in the given array (if non-NULL) with all
 * plugins in the tracklist and returns the number
 * of plugins.
 */
int
tracklist_get_plugins (
  const Tracklist * const self,
  GPtrArray *             arr);

/**
 * Activate or deactivate all plugins.
 *
 * This is useful for exporting: deactivating and
 * reactivating a plugin will reset its state.
 */
void
tracklist_activate_all_plugins (
  Tracklist * self,
  bool        activate);

/**
 * Exposes each track's ports that should be
 * exposed to the backend.
 *
 * This should be called after setting up the
 * engine.
 */
void
tracklist_expose_ports_to_backend (Tracklist * self);

/**
 * Marks or unmarks all tracks for bounce.
 */
void
tracklist_mark_all_tracks_for_bounce (
  Tracklist * self,
  bool        bounce);

void
tracklist_get_total_bars (Tracklist * self, int * total_bars);

/**
 * Set various caches (snapshots, track name hashes, plugin
 * input/output ports, etc).
 */
NONNULL
void
tracklist_set_caches (Tracklist * self, CacheTypes types);

/**
 * Only clones what is needed for project save.
 *
 * @param src Source tracklist. Must be the
 *   tracklist of the project in use.
 */
Tracklist *
tracklist_clone (Tracklist * src);

void
tracklist_free (Tracklist * self);

/**
 * Define guile module.
 */
void
guile_tracklist_define_module (void);

/**
 * @}
 */

#endif
