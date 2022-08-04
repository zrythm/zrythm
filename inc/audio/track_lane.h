// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Track lanes for each track.
 */

#ifndef __AUDIO_TRACK_LANE_H__
#define __AUDIO_TRACK_LANE_H__

#include "audio/region.h"
#include "utils/yaml.h"

typedef struct _TrackLaneWidget   TrackLaneWidget;
typedef struct Tracklist          Tracklist;
typedef struct CustomButtonWidget CustomButtonWidget;
typedef void                      MIDI_FILE;

/**
 * @addtogroup audio
 *
 * @{
 */

#define TRACK_LANE_SCHEMA_VERSION 1

#define track_lane_is_auditioner(self) \
  (self->track && track_is_auditioner (self->track))

#define track_lane_is_in_active_project(self) \
  (self->track && track_is_in_active_project (self->track))

/**
 * A TrackLane belongs to a Track (can have many
 * TrackLanes in a Track) and contains Regions.
 *
 * Only Tracks that have Regions can have TrackLanes,
 * such as InstrumentTrack and AudioTrack.
 */
typedef struct TrackLane
{
  int schema_version;

  /** Position in the Track. */
  int pos;

  /** Name of lane, e.g. "Lane 1". */
  char * name;

  /** TrackLaneWidget for this lane. */
  //TrackLaneWidget *   widget;

  /** Y local to track. */
  int y;

  /** Position of handle. */
  double height;

  /** Muted or not. */
  int mute;

  /** Soloed or not. */
  int solo;

  /** Regions in this track. */
  ZRegion ** regions;
  int        num_regions;
  size_t     regions_size;

  /**
   * MIDI channel, if MIDI lane, starting at 1.
   *
   * If this is set to 0, the value will be
   * inherited from the Track.
   */
  uint8_t midi_ch;

  /** Buttons used by the track widget. */
  CustomButtonWidget * buttons[8];
  int                  num_buttons;

  /** Owner track. */
  Track * track;

} TrackLane;

static const cyaml_schema_field_t track_lane_fields_schema[] = {
  YAML_FIELD_INT (TrackLane, schema_version),
  YAML_FIELD_INT (TrackLane, pos),
  YAML_FIELD_STRING_PTR (TrackLane, name),
  YAML_FIELD_FLOAT (TrackLane, height),
  YAML_FIELD_INT (TrackLane, mute),
  YAML_FIELD_INT (TrackLane, solo),
  YAML_FIELD_DYN_ARRAY_VAR_COUNT (
    TrackLane,
    regions,
    region_schema),
  YAML_FIELD_UINT (TrackLane, midi_ch),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t track_lane_schema = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    TrackLane,
    track_lane_fields_schema),
};

void
track_lane_init_loaded (TrackLane * self, Track * track);

/**
 * Creates a new TrackLane at the given pos in the
 * given Track.
 *
 * @param track The Track to create the TrackLane
 *   for.
 * @param pos The position (index) in the Track that
 *   this lane will be placed in.
 */
TrackLane *
track_lane_new (Track * track, int pos);

/**
 * Inserts a ZRegion to the given TrackLane at the
 * given index.
 */
void
track_lane_insert_region (
  TrackLane * self,
  ZRegion *   region,
  int         idx);

/**
 * Adds a ZRegion to the given TrackLane.
 */
void
track_lane_add_region (TrackLane * self, ZRegion * region);

/**
 * Removes but does not free the region.
 */
void
track_lane_remove_region (TrackLane * self, ZRegion * region);

/**
 * Unselects all arranger objects.
 */
void
track_lane_unselect_all (TrackLane * self);

/**
 * Removes all objects recursively from the track
 * lane.
 */
void
track_lane_clear (TrackLane * self);

/**
 * Rename the lane.
 *
 * @param with_action Whether to make this an
 *   undoable action.
 */
void
track_lane_rename (
  TrackLane *  self,
  const char * new_name,
  bool         with_action);

/**
 * Wrapper over track_lane_rename().
 */
void
track_lane_rename_with_action (
  TrackLane *  self,
  const char * new_name);

/**
 * Sets track lane soloed, updates UI and optionally
 * adds the action to the undo stack.
 *
 * @param trigger_undo Create and perform an
 *   undoable action.
 * @param fire_events Fire UI events.
 */
NONNULL
void
track_lane_set_soloed (
  TrackLane * self,
  bool        solo,
  bool        trigger_undo,
  bool        fire_events);

NONNULL
bool
track_lane_get_soloed (const TrackLane * const self);

/**
 * Sets track lane muted, updates UI and optionally
 * adds the action to the undo stack.
 *
 * @param trigger_undo Create and perform an
 *   undoable action.
 * @param fire_events Fire UI events.
 */
NONNULL
void
track_lane_set_muted (
  TrackLane * self,
  bool        mute,
  bool        trigger_undo,
  bool        fire_events);

NONNULL
bool
track_lane_get_muted (const TrackLane * const self);

const char *
track_lane_get_name (TrackLane * self);

/**
 * Updates the positions in each child recursively.
 *
 * @param from_ticks Whether to update the
 *   positions based on ticks (true) or frames
 *   (false).
 */
void
track_lane_update_positions (
  TrackLane * self,
  bool        from_ticks,
  bool        bpm_change);

/**
 * Sets the new track name hash to all the lane's
 * objects recursively.
 */
void
track_lane_update_track_name_hash (TrackLane * self);

/**
 * Clones the TrackLane.
 *
 * @param track New owner track, if any.
 */
TrackLane *
track_lane_clone (const TrackLane * src, Track * track);

/**
 * Writes the lane to the given MIDI file.
 *
 * @param lanes_as_tracks Export lanes as separate
 *   MIDI tracks.
 * @param use_track_or_lane_pos Whether to use the
 *   track position (or lane position if @ref
 *   lanes_as_tracks is true) in the MIDI data.
 *   The MIDI track will be set to 1 if false.
 * @param events Track events, if not using lanes
 *   as tracks.
 */
NONNULL_ARGS (1, 2)
void
track_lane_write_to_midi_file (
  TrackLane *  self,
  MIDI_FILE *  mf,
  MidiEvents * events,
  bool         lanes_as_tracks,
  bool         use_track_or_lane_pos);

NONNULL
Tracklist *
track_lane_get_tracklist (const TrackLane * self);

NONNULL
Track *
track_lane_get_track (const TrackLane * self);

/**
 * Calculates a unique index for this lane.
 */
NONNULL
int
track_lane_calculate_lane_idx (const TrackLane * self);

/**
 * Frees the TrackLane.
 */
NONNULL
void
track_lane_free (TrackLane * lane);

/**
 * @}
 */

#endif // __AUDIO_TRACK_LANE_H__
