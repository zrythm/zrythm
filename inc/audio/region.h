/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
#include "gui/backend/arranger_object.h"
#include "utils/yaml.h"

#include <gtk/gtk.h>

typedef struct _RegionWidget RegionWidget;
typedef struct Channel Channel;
typedef struct Track Track;
typedef struct MidiNote MidiNote;
typedef struct TrackLane TrackLane;
typedef struct _AudioClipWidget AudioClipWidget;

/**
 * @addtogroup audio
 *
 * @{
 */

#define REGION_PRINTF_FILENAME "%s_%s.mid"

#define region_is_transient(r) \
  arranger_object_is_transient ( \
    (ArrangerObject *) r)
#define region_is_lane(r) \
  arranger_object_is_lane ( \
    (ArrangerObject *) r)
#define region_is_main(r) \
  arranger_object_is_main ( \
    (ArrangerObject *) r)
#define region_get_lane(r) \
  ((Region *) \
   arranger_object_get_lane ( \
     (ArrangerObject *) r))
#define region_get_main(r) \
  ((Region *) \
   arranger_object_get_main ( \
     (ArrangerObject *) r))
#define region_get_lane_trans(r) \
  ((Region *) \
   arranger_object_get_lane_trans ( \
     (ArrangerObject *) r))
#define region_get_main_trans(r) \
  ((Region *) \
   arranger_object_get_main_trans ( \
     (ArrangerObject *) r))
#define region_is_selected(r) \
  arranger_object_is_selected ( \
    (ArrangerObject *) r)
#define region_get_visible_counterpart(r) \
  ((Region *) \
   arranger_object_get_visible_counterpart ( \
     (ArrangerObject *) r))

/**
 * Type of Region.
 *
 * Bitfield instead of plain enum so multiple
 * values can be passed to some functions (eg to
 * collect all Regions of the given types in a
 * Track).
 */
typedef enum RegionType
{
  REGION_TYPE_MIDI = 0x01,
  REGION_TYPE_AUDIO = 0x02,
  REGION_TYPE_AUTOMATION = 0x04,
  REGION_TYPE_CHORD = 0x08,
} RegionType;

static const cyaml_bitdef_t
region_type_bitvals[] =
{
  { .name = "midi", .offset =  0, .bits =  1 },
  { .name = "audio", .offset =  1, .bits =  1 },
  { .name = "automation", .offset = 2, .bits = 1 },
  { .name = "chord", .offset = 3, .bits = 1 },
};

/**
 * A region (clip) is an object on the timeline that
 * contains either MidiNote's or AudioClip's.
 *
 * It is uniquely identified using its name (and
 * ArrangerObjectInfo type), so name
 * must be unique throughout the Project.
 *
 * Each main Region must have its obj_info member
 * filled in with clones.
 */
typedef struct Region
{
  /** Base struct. */
  ArrangerObject base;

  /**
   * Unique Region name to be shown on the
   * RegionWidget.
   */
  char *         name;

  RegionType     type;

  /**
   * Owner Track position.
   *
   * Used in actions after cloning.
   */
  int            track_pos;

  /** Owner lane. */
  TrackLane *    lane;
  int            lane_pos;

  /**
   * Linked parent region.
   *
   * Either the midi notes from this region, or the midi
   * notes from the linked region are used
   */
  char *          linked_region_name;
  struct Region * linked_region; ///< cache

  /** Muted or not */
  int                muted;

  /**
   * TODO region color independent of track.
   *
   * If null, the track color is used.
   */
  GdkRGBA        color;

  /* ==== MIDI REGION ==== */

  /** MIDI notes. */
  MidiNote **     midi_notes;
  int             num_midi_notes;
  size_t          midi_notes_size;

  /**
   * Unended notes started in recording with MIDI NOTE ON
   * signal but haven't received a NOTE OFF yet
   */
  MidiNote *      unended_notes[40];
  int             num_unended_notes;

  /* ==== MIDI REGION END ==== */

  /* ==== AUDIO REGION ==== */

  /** Audio pool ID of the associated audio file. */
  int                 pool_id;

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

  /**
   * Pointer back to the AutomationTrack.
   *
   * This doesn't have to be serialized - during
   * loading, you can traverse the AutomationTrack's
   * automation Region's and set it.
   */
  AutomationTrack *  at;

  /**
   * Used when undo/redoing.
   */
  int                at_index;

  /* ==== AUTOMATION REGION END ==== */

  /* ==== CHORD REGION ==== */

  /** ChordObject's in this Region. */
  ChordObject **     chord_objects;
  int                num_chord_objects;
  size_t             chord_objects_size;

  /* ==== CHORD REGION END ==== */

  /**
   * Current lane for this Region.
   *
   * Used in live operations temporarily and in
   * the widgets for positioning if it's non-NULL.
   */
  TrackLane *        tmp_lane;
} Region;

static const cyaml_schema_field_t
  region_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "base", CYAML_FLAG_DEFAULT,
    Region, base, arranger_object_fields_schema),
  CYAML_FIELD_STRING_PTR (
    "name", CYAML_FLAG_POINTER,
    Region, name,
   	0, CYAML_UNLIMITED),
  CYAML_FIELD_BITFIELD (
    "type", CYAML_FLAG_DEFAULT,
    Region, type, region_type_bitvals,
    CYAML_ARRAY_LEN (region_type_bitvals)),
	CYAML_FIELD_INT (
    "pool_id", CYAML_FLAG_DEFAULT,
    Region, pool_id),
	CYAML_FIELD_STRING_PTR (
    "linked_region_name",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    Region, linked_region_name,
    0, CYAML_UNLIMITED),
	CYAML_FIELD_INT (
    "muted", CYAML_FLAG_DEFAULT,
    Region, muted),
  CYAML_FIELD_SEQUENCE_COUNT (
    "midi_notes", CYAML_FLAG_POINTER,
    Region, midi_notes, num_midi_notes,
    &midi_note_schema, 0, CYAML_UNLIMITED),
	CYAML_FIELD_INT (
    "track_pos", CYAML_FLAG_DEFAULT,
    Region, track_pos),
	CYAML_FIELD_INT (
    "lane_pos", CYAML_FLAG_DEFAULT,
    Region, lane_pos),
  CYAML_FIELD_SEQUENCE_COUNT (
    "aps", CYAML_FLAG_POINTER,
    Region, aps, num_aps,
    &automation_point_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "chord_objects", CYAML_FLAG_POINTER,
    Region, chord_objects, num_chord_objects,
    &chord_object_schema, 0, CYAML_UNLIMITED),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
region_schema = {
  CYAML_VALUE_MAPPING (CYAML_FLAG_POINTER,
    Region, region_fields_schema),
};

/**
 * Only to be used by implementing structs.
 *
 * @param is_main Is main Region. If this is 1 then
 *   arranger_object_info_init_main() is called to
 *   create 3 additional regions in obj_info.
 */
void
region_init (
  Region *   region,
  const Position * start_pos,
  const Position * end_pos,
  const int        is_main);

/**
 * Looks for the Region under the given name.
 *
 * Warning: very expensive function.
 */
Region *
region_find_by_name (
  const char * name);

#define region_set_track_pos(_r,_pos) \
  arranger_object_set_primitive ( \
    Region, _r, track_pos, _pos, AO_UPDATE_ALL)

/**
 * Print region info for debugging.
 */
void
region_print (
  const Region * region);

/**
 * Returns the MidiNote matching the properties of
 * the given MidiNote.
 *
 * Used to find the actual MidiNote in the region
 * from a cloned MidiNote (e.g. when doing/undoing).
 */
MidiNote *
region_find_midi_note (
  Region * r,
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
  Region * region,
  const long     timeline_frames,
  const int      normalize);

/**
 * Sets the track lane.
 */
void
region_set_lane (
  Region * region,
  TrackLane * lane);

/**
 * Generates a name for the Region, either using
 * the given AutomationTrack or Track, or appending
 * to the given base name.
 */
void
region_gen_name (
  Region *          region,
  const char *      base_name,
  AutomationTrack * at,
  Track *           track);

/**
 * Moves the given Region to the given TrackLane.
 *
 * Works with TrackLane's of other Track's as well.
 *
 * Maintains the selection status of the
 * Region.
 *
 * Assumes that the Region is already in a
 * TrackLane.
 *
 * @param tmp If the Region should be moved
 *   temporarily (the tmp_lane member will be used
 *   instead of actually moving).
 */
void
region_move_to_lane (
  Region *    region,
  TrackLane * lane,
  const int   tmp);

/**
 * Moves the Region to the given Track, maintaining
 * the selection status of the Region and the
 * TrackLane position.
 *
 * Assumes that the Region is already in a
 * TrackLane.
 *
 * @param tmp If the Region should be moved
 *   temporarily (the tmp_lane member will be used
 *   instead of actually moving).
 */
void
region_move_to_track (
  Region *  region,
  Track *   track,
  const int tmp);

/**
 * Returns if the given Region type can exist
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
  Region * region,
  AutomationTrack * at);

/**
 * Gets the AutomationTrack using the saved index.
 */
AutomationTrack *
region_get_automation_track (
  Region * region);

/**
 * Copies the data from src to dest.
 *
 * Used when doing/undoing changes in name,
 * clip start point, loop start point, etc.
 */
void
region_copy (
  Region * src,
  Region * dest);

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
  const Region * region,
  const long     gframes,
  const int      inclusive);

/**
 * Returns if any part of the Region is inside the
 * given range, inclusive.
 *
 * FIXME move to arranger object
 */
int
region_is_hit_by_range (
  const Region * region,
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
Region *
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
region_generate_filename (Region * region);

/**
 * Sets Region name (without appending anything to
 * it) to all associated regions.
 */
void
region_set_name (
  Region * region,
  char *   name);

void
region_get_type_as_string (
  RegionType type,
  char *     buf);

/**
 * Removes the MIDI note and its components
 * completely.
 */
void
region_remove_midi_note (
  Region *   region,
  MidiNote * midi_note);

/**
 * Disconnects the region and anything using it.
 *
 * Does not free the Region or its children's
 * resources.
 */
void
region_disconnect (
  Region * self);

SERIALIZE_INC (Region, region)
DESERIALIZE_INC (Region, region)
PRINT_YAML_INC (Region, region)

/**
 * @}
 */

#endif // __AUDIO_REGION_H__
