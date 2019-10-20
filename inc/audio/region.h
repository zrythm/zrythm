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
#include "audio/automation_curve.h"
#include "audio/chord_object.h"
#include "audio/midi_note.h"
#include "audio/midi_region.h"
#include "audio/position.h"
#include "gui/backend/arranger_object.h"
#include "gui/backend/arranger_object_info.h"
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
  arranger_object_info_is_transient ( \
    &r->obj_info)
#define region_is_lane(r) \
  arranger_object_info_is_lane ( \
    &r->obj_info)
#define region_is_main(r) \
  arranger_object_info_is_main ( \
    &r->obj_info)

/**
 * Gets the TrackLane counterpart of the Region.
 *
 * Only applies to Regions that have lanes.
 */
#define region_get_lane_region(r) \
  ((Region *) r->obj_info.lane)

/** Gets the non-TrackLane counterpart of the Region. */
#define region_get_main_region(r) \
  ((Region *) r->obj_info.main)

/**
 * Gets the TrackLane counterpart of the Region
 * (transient).
 *
 * Only applies to Regions that have lanes.
 */
#define region_get_lane_trans_region(r) \
  ((Region *) r->obj_info.lane_trans)

/** Gets the non-TrackLane counterpart of the Region
 * (transient). */
#define region_get_main_trans_region(r) \
  ((Region *) r->obj_info.main_trans)

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
 * Flag do indicate how to clone the Region.
 */
typedef enum RegionCloneFlag
{
  /** Create a new Region to be added to a
   * Track as a main Region. */
  REGION_CLONE_COPY_MAIN,

  /** Create a new Region that will not be used
   * as a main Region. */
  REGION_CLONE_COPY,

  /** TODO */
  REGION_CLONE_LINK
} RegionCloneFlag;

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
  /**
   * Unique Region name to be shown on the
   * RegionWidget.
   */
  char         * name;

  RegionType   type;

  /* GLOBAL POSITIONS ON THE TIMELINE */
  /* -------------------------------- */

  /** Start position in the timeline. */
  Position     start_pos;

  /** Cache, used in runtime operations. */
  Position     cache_start_pos;

  /** End position in the timeline. */
  Position     end_pos;

  /** Cache, used in runtime operations. */
  Position     cache_end_pos;

  /* LOCAL POSITIONS STARTING FROM 0.0.0.0 */
  /* ------------------------------------- */

  /**
   * Position that the original region ends in,
   * without any loops or modifications.
   *
   * FIXME Is this just the length?
   */
  Position     true_end_pos;

  /** Start position of the clip. */
  Position     clip_start_pos;

  /** Start position of the clip loop.
   *
   * The first time the region plays it will start
   * playing from the clip_start_pos and then loop
   * to this position.
   *
   */
  Position     loop_start_pos;

  /** Cache, used in runtime operations. */
  Position     cache_loop_start_pos;

  /** End position of the clip loop.
   *
   * Once this is reached, the clip will go back to
   * the clip  loop start position.
   *
   */
  Position     loop_end_pos;

  /** Cache, used in runtime operations. */
  Position     cache_loop_end_pos;

  /* ---------------------------------------- */

  /**
   * Region widget.
   */
  RegionWidget * widget;

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

  /** Position to fade in until. */
  Position            fade_in_pos;

  /** Position to fade out from. */
  Position            fade_out_pos;

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
   * The AutomationCurve's.
   *
   * Their size will always be aps_size - 1 (or 0 if
   * there are no AutomationPoint's).
   */
  AutomationCurve ** acs;
  int                num_acs;

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

  /**
   * Info on whether this Region is transient/lane
   * and pointers to transient/lane equivalents.
   */
  ArrangerObjectInfo  obj_info;
} Region;

static const cyaml_schema_field_t
  region_fields_schema[] =
{
  CYAML_FIELD_STRING_PTR (
    "name", CYAML_FLAG_POINTER,
    Region, name,
   	0, CYAML_UNLIMITED),
  CYAML_FIELD_BITFIELD (
    "type", CYAML_FLAG_DEFAULT,
    Region, type, region_type_bitvals,
    CYAML_ARRAY_LEN (region_type_bitvals)),
  CYAML_FIELD_MAPPING (
    "start_pos", CYAML_FLAG_DEFAULT,
    Region, start_pos, position_fields_schema),
  CYAML_FIELD_MAPPING (
    "end_pos", CYAML_FLAG_DEFAULT,
    Region, end_pos, position_fields_schema),
  CYAML_FIELD_MAPPING (
    "true_end_pos", CYAML_FLAG_DEFAULT,
    Region, true_end_pos, position_fields_schema),
  CYAML_FIELD_MAPPING (
    "clip_start_pos", CYAML_FLAG_DEFAULT,
    Region, clip_start_pos, position_fields_schema),
  CYAML_FIELD_MAPPING (
    "loop_start_pos", CYAML_FLAG_DEFAULT,
    Region, loop_start_pos, position_fields_schema),
  CYAML_FIELD_MAPPING (
    "loop_end_pos", CYAML_FLAG_DEFAULT,
    Region, loop_end_pos, position_fields_schema),
  CYAML_FIELD_MAPPING (
    "fade_in_pos", CYAML_FLAG_DEFAULT,
    Region, fade_in_pos, position_fields_schema),
  CYAML_FIELD_MAPPING (
    "fade_out_pos", CYAML_FLAG_DEFAULT,
    Region, fade_out_pos, position_fields_schema),
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
    "acs", CYAML_FLAG_POINTER,
    Region, acs, num_acs,
    &automation_curve_schema, 0, CYAML_UNLIMITED),
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

ARRANGER_OBJ_DECLARE_MOVABLE_W_LENGTH (
  Region, region);

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
 * Inits freshly loaded region.
 */
void
region_init_loaded (Region * region);

/**
 * Mainly used for copy-pasting.
 */
void
region_post_deserialize (
  Region * self);

/**
 * Finds the region corresponding to the given one.
 *
 * This should be called when we have a copy or a
 * clone, to get the actual region in the project.
 */
Region *
region_find (
  Region * r);

/**
 * Looks for the Region under the given name.
 *
 * Warning: very expensive function.
 */
Region *
region_find_by_name (
  const char * name);

/**
 * Splits the given Region at the given Position,
 * deletes the original Region and adds 2 new
 * Regions in the same parent (Track or
 * AutomationTrack).
 *
 * The given region must be the main region, as this
 * will create 2 new main regions.
 *
 * @param region The Region to split. This Region
 *   will be deleted.
 * @param pos The Position to split at.
 * @param pos_is_local If the position is local (1)
 *   or global (0).
 * @param r1 Address to hold the pointer to the
 *   newly created region 1.
 * @param r2 Address to hold the pointer to the
 *   newly created region 2.
 */
void
region_split (
  Region *         region,
  const Position * pos,
  const int        pos_is_local,
  Region **        r1,
  Region **        r2);

/**
 * Undoes what region_split() did.
 */
void
region_unsplit (
  Region *         r1,
  Region *         r2,
  Region **        region);

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
 * Returns the full length as it appears on the
 * timeline in ticks.
 */
long
region_get_full_length_in_ticks (
  Region * region);

/**
 * Returns the full length as it appears on the
 * timeline in frames.
 */
long
region_get_full_length_in_frames (
  const Region * region);

/**
 * Returns the true length as it appears on the
 * piano roll (not taking into account any looping)
 * in ticks.
 */
long
region_get_true_length_in_ticks (
  Region * region);

/**
 * Returns the length of the loop in frames.
 */
long
region_get_loop_length_in_frames (
  const Region * region);

/**
 * Returns the length of the loop in ticks.
 */
long
region_get_loop_length_in_ticks (
  const Region * region);

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
  const Region * region,
  const long     timeline_frames,
  const int      normalize);

/**
 * Returns the Track this Region is in.
 */
Track *
region_get_track (
  const Region * region);

/**
 * Sets the Track position to the Region and all its
 * members recursively.
 */
void
region_set_track_pos (
  Region *  region,
  const int pos);

/**
 * Returns the number of loops in the region,
 * optionally including incomplete ones.
 */
int
region_get_num_loops (
  Region * region,
  int      count_incomplete_loops);

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

ARRANGER_OBJ_DECLARE_POS_GETTER (
  Region, region, clip_start_pos);

ARRANGER_OBJ_DECLARE_POS_SETTER (
  Region, region, clip_start_pos);

ARRANGER_OBJ_DECLARE_POS_GETTER (
  Region, region, loop_start_pos);

ARRANGER_OBJ_DECLARE_POS_SETTER (
  Region, region, loop_start_pos);

ARRANGER_OBJ_DECLARE_POS_GETTER (
  Region, region, loop_end_pos);

ARRANGER_OBJ_DECLARE_POS_SETTER (
  Region, region, loop_end_pos);

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
 * Checks if position is valid then sets it.
 */
void
region_set_true_end_pos (
  Region * region,
  const Position * pos,
  ArrangerObjectUpdateFlag update_flag);

/**
 * Checks if position is valid then sets it.
 */
void
region_set_loop_end_pos (
  Region * region,
  const Position * pos,
  ArrangerObjectUpdateFlag update_flag);

/**
 * Checks if position is valid then sets it.
 */
void
region_set_loop_start_pos (
  Region * region,
  const Position * pos,
  ArrangerObjectUpdateFlag update_flag);

/**
 * Checks if position is valid then sets it.
 */
void
region_set_clip_start_pos (
  Region * region,
  const Position * pos,
  ArrangerObjectUpdateFlag update_flag);

/**
 * Moves the children of the region by the given
 * amount of ticks.
 */
void
region_add_ticks_to_children (
  Region *   self,
  const long ticks);

/**
 * Resizes the object on the left side or right
 * side by given amount of ticks.
 *
 * @param left 1 to resize left side, 0 to resize
 *   right side.
 * @param ticks Number of ticks to resize.
 * @param loop Whether this is a loop-resize (1) or
 *   a normal resize (0).
 * @param update_flag ArrangerObjectUpdateFlag.
 */
void
region_resize (
  Region *   region,
  const int  left,
  const long ticks,
  const int  loop,
  ArrangerObjectUpdateFlag update_flag);

/**
 * Updates the frames of each position in each child
 * of the region recursively.
 */
void
region_update_frames (
  Region * self);

/**
 * Returns if Region is (should be) visible.
 */
#define region_should_be_visible(mn) \
  arranger_object_info_should_be_visible ( \
    mn->obj_info)

/**
 * Returns if the position is inside the region
 * or not.
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
 */
int
region_is_hit_by_range (
  const Region * region,
  const long     gframes_start,
  const long     gframes_end,
  const int      end_inclusive);

void
region_unpack (Region * region);

/**
 * Returns the region at the given position in the
 * given Track.
 *
 * @param track The track to look in.
 * @param pos The position.
 *
 * FIXME with lanes there can be multiple positions.
 */
Region *
region_at_position (
  Track    * track,
  Position * pos);

/**
 * Generates the filename for this region.
 *
 * MUST be free'd.
 */
char *
region_generate_filename (Region * region);

/**
 * Returns the region with the earliest start point.
 */
Region *
region_get_start_region (Region ** regions,
                         int       num_regions);

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
 * Clone region.
 *
 * Creates a new region and either links to the
 * original or copies every field.
 */
Region *
region_clone (
  Region *        region,
  RegionCloneFlag flag);

/**
 * Disconnects the region and anything using it.
 *
 * Does not free the Region or its children's
 * resources.
 */
void
region_disconnect (
  Region * self);

/**
 * Frees each Region stored in obj_info.
 */
void
region_free_all (
  Region * region);

/**
 * Frees a single Region and its components.
 */
void
region_free (Region * region);

SERIALIZE_INC (Region, region)
DESERIALIZE_INC (Region, region)
PRINT_YAML_INC (Region, region)

/**
 * @}
 */

#endif // __AUDIO_REGION_H__
