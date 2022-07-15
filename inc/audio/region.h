/*
 * Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>
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
 * A region in the timeline.
 */
#ifndef __AUDIO_REGION_H__
#define __AUDIO_REGION_H__

#include "audio/automation_point.h"
#include "audio/chord_object.h"
#include "audio/midi_note.h"
#include "audio/midi_region.h"
#include "audio/position.h"
#include "audio/region_identifier.h"
#include "gui/backend/arranger_object.h"
#include "utils/yaml.h"

#include <gtk/gtk.h>

typedef struct _RegionWidget    RegionWidget;
typedef struct Channel          Channel;
typedef struct Track            Track;
typedef struct MidiNote         MidiNote;
typedef struct TrackLane        TrackLane;
typedef struct _AudioClipWidget AudioClipWidget;
typedef struct RegionLinkGroup  RegionLinkGroup;
typedef struct Stretcher        Stretcher;
typedef struct AudioClip        AudioClip;

/**
 * @addtogroup audio
 *
 * @{
 */

#define REGION_SCHEMA_VERSION 1

#define REGION_MAGIC 93075327
#define IS_REGION(x) (((ZRegion *) x)->magic == REGION_MAGIC)
#define IS_REGION_AND_NONNULL(x) (x && IS_REGION (x))

#define REGION_PRINTF_FILENAME "%s_%s.mid"

#define region_is_selected(r) \
  arranger_object_is_selected ((ArrangerObject *) r)

/**
 * Musical mode setting for audio regions.
 */
typedef enum RegionMusicalMode
{
  /** Inherit from global musical mode setting. */
  REGION_MUSICAL_MODE_INHERIT,
  /** Musical mode off - don't auto-stretch when
   * BPM changes. */
  REGION_MUSICAL_MODE_OFF,
  /** Musical mode on - auto-stretch when BPM
   * changes. */
  REGION_MUSICAL_MODE_ON,
} RegionMusicalMode;

static const cyaml_strval_t region_musical_mode_strings[] = {
  {__ ("Inherit"), REGION_MUSICAL_MODE_INHERIT},
  { __ ("Off"),    REGION_MUSICAL_MODE_OFF    },
  { __ ("On"),     REGION_MUSICAL_MODE_ON     },
};

/**
 * A region (clip) is an object on the timeline that
 * contains either MidiNote's or AudioClip's.
 *
 * It is uniquely identified using its name, so it
 * must be unique throughout the Project.
 */
typedef struct ZRegion
{
  /** Base struct. */
  ArrangerObject base;

  int schema_version;

  /** Unique ID. */
  RegionIdentifier id;

  /** Name to be shown on the widget. */
  char * name;

  /** Escaped name for drawing. */
  char * escaped_name;

  /**
   * TODO region color independent of track.
   *
   * If null, the track color is used.
   */
  GdkRGBA color;

  /* ==== MIDI REGION ==== */

  /** MIDI notes. */
  MidiNote ** midi_notes;
  int         num_midi_notes;
  size_t      midi_notes_size;

  /**
   * Unended notes started in recording with
   * MIDI NOTE ON
   * signal but haven't received a NOTE OFF yet.
   *
   * This is also used temporarily when reading
   * from MIDI files.
   *
   * FIXME allocate.
   *
   * @note These are present in
   *   \ref ZRegion.midi_notes and must not be
   *   free'd separately.
   */
  MidiNote * unended_notes[12000];
  int        num_unended_notes;

  /* ==== MIDI REGION END ==== */

  /* ==== AUDIO REGION ==== */

  /** Audio pool ID of the associated audio file,
   * mostly used during serialization. */
  int pool_id;

  /**
   * Whether currently running the stretching
   * algorithm.
   *
   * If this is true, region drawing will be
   * deferred.
   */
  bool stretching;

  /**
   * The length before stretching, in ticks.
   */
  double before_length;

  /** Used during arranger UI overlay actions. */
  double stretch_ratio;

  /**
   * Whether to read the clip from the pool (used
   * in most cases).
   */
  bool read_from_pool;

  /** Gain to apply to the audio (amplitude
   * 0.0-2.0). */
  float gain;

  /**
   * Clip to read frames from, if not from the pool.
   */
  AudioClip * clip;

#if 0
  /**
   * Frames to actually use, interleaved.
   *
   * Properties such as \ref AudioClip.channels can
   * be fetched from the AudioClip.
   */
  sample_t *        frames;
  size_t            num_frames;

  /**
   * Per-channel frames for convenience.
   */
  sample_t *        ch_frames[16];
#endif

  /** Musical mode setting. */
  RegionMusicalMode musical_mode;

  /** Array of split points. */
  Position * split_points;
  int        num_split_points;
  size_t     split_points_size;

  /* ==== AUDIO REGION END ==== */

  /* ==== AUTOMATION REGION ==== */

  /**
   * The automation points this region contains.
   *
   * Could also be used in audio regions for volume
   * automation.
   *
   * Must always stay sorted by position.
   */
  AutomationPoint ** aps;
  int                num_aps;
  size_t             aps_size;

  /** Last recorded automation point. */
  AutomationPoint * last_recorded_ap;

  /* ==== AUTOMATION REGION END ==== */

  /* ==== CHORD REGION ==== */

  /** ChordObject's in this Region. */
  ChordObject ** chord_objects;
  int            num_chord_objects;
  size_t         chord_objects_size;

  /* ==== CHORD REGION END ==== */

  /**
   * Set to ON during bouncing if this
   * region should be included.
   *
   * Only relevant for audio and midi regions.
   */
  int bounce;

  /* --- drawing caches --- */

  /* New region drawing needs to be cached in the
   * following situations:
   *
   * 1. when hidden part of the region is revealed
   *   (on x axis).
   *   TODO max 140% of the region should be
   *   cached (20% before and 20% after if before/
   *   after is not fully visible)
   * 2. when full rect (x/width) changes
   * 3. when a region marker is moved
   * 4. when the clip actually changes (use
   *   last-change timestamp on the clip or region)
   * 5. when fades change
   * 6. when region height changes (track/lane)
   */

  /** Cache layout for drawing the name. */
  PangoLayout * layout;

  /** Cache layout for drawing the chord names
   * inside the region. */
  PangoLayout * chords_layout;

  /* these are used for caching */
  GdkRectangle last_main_full_rect;

  /** Last main draw rect. */
  GdkRectangle last_main_draw_rect;

  /* these are used for caching */
  GdkRectangle last_lane_full_rect;

  /** Last lane draw rect. */
  GdkRectangle last_lane_draw_rect;

  /** Last timestamp the audio clip or its contents
   * changed. */
  gint64 last_clip_change;

  /** Last timestamp the region was cached. */
  gint64 last_cache_time;

  /** Last known marker positions (only positions
   * are used). */
  ArrangerObject last_positions_obj;

  /* --- drawing caches end --- */

  int magic;
} ZRegion;

static const cyaml_schema_field_t region_fields_schema[] = {
  YAML_FIELD_INT (ZRegion, schema_version),
  YAML_FIELD_MAPPING_EMBEDDED (
    ZRegion,
    base,
    arranger_object_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    ZRegion,
    id,
    region_identifier_fields_schema),
  YAML_FIELD_STRING_PTR (ZRegion, name),
  YAML_FIELD_INT (ZRegion, pool_id),
  YAML_FIELD_FLOAT (ZRegion, gain),
  CYAML_FIELD_SEQUENCE_COUNT (
    "midi_notes",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    ZRegion,
    midi_notes,
    num_midi_notes,
    &midi_note_schema,
    0,
    CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "aps",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    ZRegion,
    aps,
    num_aps,
    &automation_point_schema,
    0,
    CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "chord_objects",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    ZRegion,
    chord_objects,
    num_chord_objects,
    &chord_object_schema,
    0,
    CYAML_UNLIMITED),
  YAML_FIELD_ENUM (
    ZRegion,
    musical_mode,
    region_musical_mode_strings),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t region_schema = {
  YAML_VALUE_PTR_NULLABLE (ZRegion, region_fields_schema),
};

/**
 * Returns if the given ZRegion type can have fades.
 */
#define region_type_can_fade(rtype) \
  (rtype == REGION_TYPE_AUDIO)

/**
 * Only to be used by implementing structs.
 */
void
region_init (
  ZRegion *        region,
  const Position * start_pos,
  const Position * end_pos,
  unsigned int     track_name_hash,
  int              lane_pos,
  int              idx_inside_lane);

/**
 * Looks for the ZRegion matching the identifier.
 */
HOT NONNULL ZRegion *
region_find (const RegionIdentifier * const id);

#if 0
static inline void
region_set_track_name_hash (
  ZRegion *    self,
  unsigned int name_hash)
{
  self->id.track_name_hash = name_hash;
}
#endif

NONNULL
void
region_print_to_str (
  const ZRegion * self,
  char *          buf,
  const size_t    buf_size);

/**
 * Print region info for debugging.
 */
NONNULL
void
region_print (const ZRegion * region);

TrackLane *
region_get_lane (const ZRegion * region);

/**
 * Returns the region's link group.
 */
RegionLinkGroup *
region_get_link_group (ZRegion * self);

/**
 * Sets the link group to the region.
 *
 * @param group_idx If -1, the region will be
 *   removed from its current link group, if any.
 */
void
region_set_link_group (
  ZRegion * region,
  int       group_idx,
  bool      update_identifier);

NONNULL
bool
region_has_link_group (ZRegion * region);

/**
 * Returns the MidiNote matching the properties of
 * the given MidiNote.
 *
 * Used to find the actual MidiNote in the region
 * from a cloned MidiNote (e.g. when doing/undoing).
 */
MidiNote *
region_find_midi_note (ZRegion * r, MidiNote * _mn);

/**
 * Converts frames on the timeline (global)
 * to local frames (in the clip).
 *
 * If normalize is 1 it will only return a position
 * from 0 to loop_end (it will traverse the
 * loops to find the appropriate position),
 * otherwise it may exceed loop_end.
 *
 * @param timeline_frames Timeline position in
 *   frames.
 *
 * @return The local frames.
 */
NONNULL
HOT signed_frame_t
region_timeline_frames_to_local (
  const ZRegion * const self,
  const signed_frame_t  timeline_frames,
  const bool            normalize);

/**
 * Returns the number of frames until the next
 * loop end point or the end of the region.
 *
 * @param[in] timeline_frames Global frames at
 *   start.
 * @param[out] ret_frames Return frames.
 * @param[out] is_loop Whether the return frames
 *   are for a loop (if false, the return frames
 *   are for the region's end).
 */
NONNULL
void
region_get_frames_till_next_loop_or_end (
  const ZRegion * const self,
  const signed_frame_t  timeline_frames,
  signed_frame_t *      ret_frames,
  bool *                is_loop);

/**
 * Sets the track lane.
 */
NONNULL
void
region_set_lane (ZRegion * self, const TrackLane * const lane);

/**
 * Generates a name for the ZRegion, either using
 * the given AutomationTrack or Track, or appending
 * to the given base name.
 */
void
region_gen_name (
  ZRegion *         region,
  const char *      base_name,
  AutomationTrack * at,
  Track *           track);

/**
 * Stretch the region's contents.
 *
 * This should be called right after changing the
 * region's size.
 *
 * @param ratio The ratio to stretch by.
 */
NONNULL
void
region_stretch (ZRegion * self, double ratio);

/**
 * To be called every time the identifier changes
 * to update the region's children.
 */
NONNULL
void
region_update_identifier (ZRegion * self);

/**
 * Updates all other regions in the region link
 * group, if any.
 */
NONNULL
void
region_update_link_group (ZRegion * self);

/**
 * Moves the given ZRegion to the given TrackLane.
 *
 * Works with TrackLane's of other Track's as well.
 *
 * Maintains the selection status of the
 * Region.
 *
 * Assumes that the ZRegion is already in a
 * TrackLane.
 *
 * @param index_in_lane Index in lane in the
 *   new track to insert the region to, or -1 to
 *   append.
 */
void
region_move_to_lane (
  ZRegion *   region,
  TrackLane * lane,
  int         index_in_lane);

/**
 * Moves the ZRegion to the given Track, maintaining
 * the selection status of the ZRegion and the
 * TrackLane position.
 *
 * Assumes that the ZRegion is already in a
 * TrackLane.
 *
 * @param index_in_lane Index in lane in the
 *   new track to insert the region to, or -1 to
 *   append.
 */
void
region_move_to_track (
  ZRegion * region,
  Track *   track,
  int       index_in_lane);

/**
 * Returns if the given ZRegion type can exist
 * in TrackLane's.
 */
CONST
static inline int
region_type_has_lane (const RegionType type)
{
  return type == REGION_TYPE_MIDI || type == REGION_TYPE_AUDIO;
}

/**
 * Sets the automation track.
 */
void
region_set_automation_track (
  ZRegion *         region,
  AutomationTrack * at);

/**
 * Gets the AutomationTrack using the saved index.
 */
AutomationTrack *
region_get_automation_track (const ZRegion * const region);

/**
 * Copies the data from src to dest.
 *
 * Used when doing/undoing changes in name,
 * clip start point, loop start point, etc.
 */
void
region_copy (ZRegion * src, ZRegion * dest);

/**
 * Returns if the position is inside the region
 * or not.
 *
 * FIXME move to arranger object
 *
 * @param gframes Global position in frames.
 * @param inclusive Whether the last frame should
 *   be counted as part of the region.
 */
NONNULL
PURE int
region_is_hit (
  const ZRegion *      region,
  const signed_frame_t gframes,
  const bool           inclusive);

/**
 * Returns if any part of the ZRegion is inside the
 * given range, inclusive.
 *
 * FIXME move to arranger object
 */
int
region_is_hit_by_range (
  const ZRegion *      region,
  const signed_frame_t gframes_start,
  const signed_frame_t gframes_end,
  const bool           end_inclusive);

/**
 * Returns the region at the given position in the
 * given Track.
 *
 * @param at The automation track to look in.
 * @param track The track to look in, if at is
 *   NULL.
 * @param pos The position.
 */
NONNULL_ARGS (3)
PURE ZRegion *
region_at_position (
  const Track *           track,
  const AutomationTrack * at,
  const Position *        pos);

/**
 * Generates the filename for this region.
 *
 * MUST be free'd.
 */
char *
region_generate_filename (ZRegion * region);

void
region_get_type_as_string (RegionType type, char * buf);

/**
 * Returns if this region is currently being
 * recorded onto.
 */
bool
region_is_recording (ZRegion * self);

/**
 * Returns whether the region is effectively in
 * musical mode.
 *
 * @note Only applicable to audio regions.
 */
bool
region_get_musical_mode (ZRegion * self);

/**
 * Wrapper for adding an arranger object.
 */
void
region_add_arranger_object (
  ZRegion *        self,
  ArrangerObject * obj,
  bool             fire_events);

void
region_create_link_group_if_none (ZRegion * region);

/**
 * Removes the link group from the region, if any.
 */
void
region_unlink (ZRegion * region);

/**
 * Removes all children objects from the region.
 */
void
region_remove_all_children (ZRegion * region);

/**
 * Clones and copies all children from \ref src to
 * \ref dest.
 */
void
region_copy_children (ZRegion * dest, ZRegion * src);

NONNULL
bool
region_is_looped (const ZRegion * const self);

/**
 * Returns the ArrangerSelections based on the
 * given region type.
 */
ArrangerSelections *
region_get_arranger_selections (ZRegion * self);

/**
 * Sanity checking.
 */
bool
region_validate (ZRegion * self, bool is_project);

/**
 * Disconnects the region and anything using it.
 *
 * Does not free the ZRegion or its children's
 * resources.
 */
void
region_disconnect (ZRegion * self);

/**
 * @}
 */

#endif // __AUDIO_REGION_H__
