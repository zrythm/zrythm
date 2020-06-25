/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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

typedef struct _RegionWidget RegionWidget;
typedef struct Channel Channel;
typedef struct Track Track;
typedef struct MidiNote MidiNote;
typedef struct TrackLane TrackLane;
typedef struct _AudioClipWidget AudioClipWidget;
typedef struct RegionLinkGroup RegionLinkGroup;
typedef struct Stretcher Stretcher;

/**
 * @addtogroup audio
 *
 * @{
 */

#define REGION_MAGIC 93075327
#define IS_REGION(tr) \
  ((ZRegion *) tr && \
   ((ZRegion *) tr)->magic == REGION_MAGIC)

#define REGION_PRINTF_FILENAME "%s_%s.mid"

#define region_is_selected(r) \
  arranger_object_is_selected ( \
    (ArrangerObject *) r)

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

static const cyaml_strval_t
region_musical_mode_strings[] =
{
  { __("Inherit"),
    REGION_MUSICAL_MODE_INHERIT },
  { __("Off"),
    REGION_MUSICAL_MODE_OFF },
  { __("On"),
    REGION_MUSICAL_MODE_ON },
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

  /** Unique ID. */
  RegionIdentifier id;

  /** Name to be shown on the widget. */
  char *          name;

  /**
   * TODO region color independent of track.
   *
   * If null, the track color is used.
   */
  GdkRGBA         color;

  /* ==== MIDI REGION ==== */

  /** MIDI notes. */
  MidiNote **     midi_notes;
  int             num_midi_notes;
  size_t          midi_notes_size;

  /**
   * Unended notes started in recording with
   * MIDI NOTE ON
   * signal but haven't received a NOTE OFF yet.
   *
   * This is also used temporarily when reading
   * from MIDI files.
   */
  MidiNote *      unended_notes[1200];
  int             num_unended_notes;

  /* ==== MIDI REGION END ==== */

  /* ==== AUDIO REGION ==== */

  /** Audio pool ID of the associated audio file,
   * mostly use during serialization. */
  int               pool_id;

  /**
   * Whether currently running the stretching
   * algorithm.
   *
   * If this is true, region drawing will be
   * deferred.
   */
  bool              stretching;

  /**
   * The length before stretching, in ticks.
   */
  double            before_length;

  /** Used during arranger UI overlay actions. */
  double            stretch_ratio;

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

  /** Musical mode setting. */
  RegionMusicalMode musical_mode;

  /* ==== AUDIO REGION END ==== */

  /* ==== AUTOMATION REGION ==== */

  /**
   * The automation points.
   *
   * Must always stay sorted by position.
   */
  AutomationPoint ** aps;
  int                num_aps;
  size_t             aps_size;

  /** Last recorded automation point. */
  AutomationPoint *  last_recorded_ap;

  /* ==== AUTOMATION REGION END ==== */

  /* ==== CHORD REGION ==== */

  /** ChordObject's in this Region. */
  ChordObject **     chord_objects;
  int                num_chord_objects;
  size_t             chord_objects_size;

  /* ==== CHORD REGION END ==== */

  /** Cache layout for drawing the name. */
  PangoLayout *      layout;

  /** Cache layout for drawing the chord names
   * inside the region. */
  PangoLayout *      chords_layout;

  /**
   * Set to ON during bouncing if this
   * region should be included.
   *
   * Only relevant for audio and midi regions.
   */
  int                bounce;

  /* these are used for caching */
  GdkRectangle       last_main_full_rect;

  /** Last main draw rect. */
  GdkRectangle       last_main_draw_rect;

  int                magic;
} ZRegion;

static const cyaml_schema_field_t
  region_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "base", CYAML_FLAG_DEFAULT,
    ZRegion, base, arranger_object_fields_schema),
  CYAML_FIELD_MAPPING (
    "id", CYAML_FLAG_DEFAULT,
    ZRegion, id,
    region_identifier_fields_schema),
  YAML_FIELD_STRING_PTR (
    ZRegion, name),
  YAML_FIELD_INT (
    ZRegion, pool_id),
  CYAML_FIELD_SEQUENCE_COUNT (
    "midi_notes",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    ZRegion, midi_notes, num_midi_notes,
    &midi_note_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "aps", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    ZRegion, aps, num_aps,
    &automation_point_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "chord_objects",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    ZRegion, chord_objects, num_chord_objects,
    &chord_object_schema, 0, CYAML_UNLIMITED),
  YAML_FIELD_ENUM (
    ZRegion, musical_mode,
    region_musical_mode_strings),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  region_schema =
{
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER_NULL_STR,
    ZRegion, region_fields_schema),
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
  ZRegion *   region,
  const Position * start_pos,
  const Position * end_pos,
  int              track_pos,
  int              lane_pos,
  int              idx_inside_lane);

/**
 * Looks for the ZRegion matching the identifier.
 */
ZRegion *
region_find (
  RegionIdentifier * id);

#define region_set_track_pos(_r,_pos) \
  _r->id.track_pos = _pos

/**
 * Print region info for debugging.
 */
void
region_print (
  const ZRegion * region);

TrackLane *
region_get_lane (
  const ZRegion * region);

RegionLinkGroup *
region_get_link_group (
  ZRegion * self);

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

bool
region_has_link_group (
  ZRegion * region);

/**
 * Returns the MidiNote matching the properties of
 * the given MidiNote.
 *
 * Used to find the actual MidiNote in the region
 * from a cloned MidiNote (e.g. when doing/undoing).
 */
MidiNote *
region_find_midi_note (
  ZRegion * r,
  MidiNote * _mn);

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
long
region_timeline_frames_to_local (
  ZRegion * region,
  const long     timeline_frames,
  const int      normalize);

/**
 * Sets the track lane.
 */
void
region_set_lane (
  ZRegion * region,
  TrackLane * lane);

/**
 * Generates a name for the ZRegion, either using
 * the given AutomationTrack or Track, or appending
 * to the given base name.
 */
void
region_gen_name (
  ZRegion *          region,
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
void
region_stretch (
  ZRegion * self,
  double    ratio);

/**
 * To be called every time the identifier changes
 * to update the region's children.
 */
void
region_update_identifier (
  ZRegion * self);

/**
 * Updates all other regions in the region link
 * group, if any.
 */
void
region_update_link_group (
  ZRegion * self);

/**
 * Moves the given ZRegion to the given TrackLane.
 *
 * Works with TrackLane's of other Track's as well.
 *
 * Maintains the selection status of the
 * ZRegion.
 *
 * Assumes that the ZRegion is already in a
 * TrackLane.
 */
void
region_move_to_lane (
  ZRegion *    region,
  TrackLane * lane);

/**
 * Moves the ZRegion to the given Track, maintaining
 * the selection status of the ZRegion and the
 * TrackLane position.
 *
 * Assumes that the ZRegion is already in a
 * TrackLane.
 */
void
region_move_to_track (
  ZRegion *  region,
  Track *   track);

/**
 * Returns if the given ZRegion type can exist
 * in TrackLane's.
 */
int
region_type_has_lane (
  const RegionType type);

/**
 * Sets the automation track.
 */
void
region_set_automation_track (
  ZRegion * region,
  AutomationTrack * at);

/**
 * Gets the AutomationTrack using the saved index.
 */
AutomationTrack *
region_get_automation_track (
  ZRegion * region);

/**
 * Copies the data from src to dest.
 *
 * Used when doing/undoing changes in name,
 * clip start point, loop start point, etc.
 */
void
region_copy (
  ZRegion * src,
  ZRegion * dest);

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
int
region_is_hit (
  const ZRegion * region,
  const long     gframes,
  const int      inclusive);

/**
 * Returns if any part of the ZRegion is inside the
 * given range, inclusive.
 *
 * FIXME move to arranger object
 */
int
region_is_hit_by_range (
  const ZRegion * region,
  const long     gframes_start,
  const long     gframes_end,
  const int      end_inclusive);

/**
 * Returns the region at the given position in the
 * given Track.
 *
 * @param at The automation track to look in.
 * @param track The track to look in, if at is
 *   NULL.
 * @param pos The position.
 */
ZRegion *
region_at_position (
  Track    *        track,
  AutomationTrack * at,
  Position *        pos);

/**
 * Generates the filename for this region.
 *
 * MUST be free'd.
 */
char *
region_generate_filename (ZRegion * region);

/**
 * Sets ZRegion name (without appending anything to
 * it) to all associated regions.
 */
void
region_set_name (
  ZRegion * region,
  char *    name,
  int       fire_events);

void
region_get_type_as_string (
  RegionType type,
  char *     buf);

/**
 * Returns if this region is currently being
 * recorded onto.
 */
bool
region_is_recording (
  ZRegion * self);

/**
 * Returns whether the region is effectively in
 * musical mode.
 *
 * @note Only applicable to audio regions.
 */
bool
region_get_musical_mode (
  ZRegion * self);

/**
 * Removes the MIDI note and its components
 * completely.
 */
void
region_remove_midi_note (
  ZRegion *   region,
  MidiNote * midi_note);

void
region_create_link_group_if_none (
  ZRegion * region);

/**
 * Removes the link group from the region, if any.
 */
void
region_unlink (
  ZRegion * region);

/**
 * Removes all children objects from the region.
 */
void
region_remove_all_children (
  ZRegion * region);

/**
 * Clones and copies all children from \ref src to
 * \ref dest.
 */
void
region_copy_children (
  ZRegion * dest,
  ZRegion * src);

/**
 * Returns the ArrangerSelections based on the
 * given region type.
 */
ArrangerSelections *
region_get_arranger_selections (
  ZRegion * self);

/**
 * Sanity checking.
 *
 * @param is_project Whether this region ispart
 *   of the project (as opposed to a clone in
 *   the undo stack, etc.).
 */
bool
region_sanity_check (
  ZRegion * self,
  bool      is_project);

/**
 * Disconnects the region and anything using it.
 *
 * Does not free the ZRegion or its children's
 * resources.
 */
void
region_disconnect (
  ZRegion * self);

SERIALIZE_INC (ZRegion, region)
DESERIALIZE_INC (ZRegion, region)
PRINT_YAML_INC (ZRegion, region)

/**
 * @}
 */

#endif // __AUDIO_REGION_H__
