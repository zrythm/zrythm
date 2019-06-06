/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * A region in the timeline.
 */
#ifndef __AUDIO_REGION_H__
#define __AUDIO_REGION_H__

#include "audio/midi_note.h"
#include "audio/midi_region.h"
#include "audio/position.h"
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

/** Gets the TrackLane counterpart of the Region. */
#define region_get_lane_region(r) \
  ((Region *) r->obj_info.lane)

/** Gets the non-TrackLane counterpart of the Region. */
#define region_get_main_region(r) \
  ((Region *) r->obj_info.main)

/** Gets the TrackLane counterpart of the Region
 * (transient). */
#define region_get_lane_trans_region(r) \
  ((Region *) r->obj_info.lane_trans)

/** Gets the non-TrackLane counterpart of the Region
 * (transient). */
#define region_get_main_trans_region(r) \
  ((Region *) r->obj_info.main_trans)

/**
 * Type of Region.
 */
typedef enum RegionType
{
  REGION_TYPE_MIDI,
  REGION_TYPE_AUDIO
} RegionType;

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

  /** Position that the original region ends in,
   * without any loops or modifications. */
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

  /** End position of the clip loop.
   *
   * Once this is reached, the clip will go back to
   * the clip  loop start position.
   *
   */
  Position     loop_end_pos;

  /* ---------------------------------------- */

  /**
   * Region widget.
   */
  RegionWidget * widget;

  /** Owner track. */
  int            track_pos;

  /** Owner lane. */
  int            lane_pos;
  TrackLane *    lane;

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
  MidiNote *      midi_notes[800];
  int             num_midi_notes;

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

  /**
   * Buffer holding samples/frames.
   */
  float *             buff;
  long                buff_size;

  /** Number of channels in the audio buffer. */
  int                 channels;

  /**
   * Original filename.
   */
  char *              filename;

  /* ==== AUDIO REGION END ==== */

  /**
   * Info on whether this Region is transient/lane
   * and pointers to transient/lane equivalents.
   */
  ArrangerObjectInfo  obj_info;
} Region;

static const cyaml_strval_t
region_type_strings[] =
{
	{ "Midi",          REGION_TYPE_MIDI    },
	{ "Audio",         REGION_TYPE_AUDIO   },
};

static const cyaml_schema_field_t
  region_fields_schema[] =
{
  CYAML_FIELD_STRING_PTR (
    "name", CYAML_FLAG_POINTER,
    Region, name,
   	0, CYAML_UNLIMITED),
  CYAML_FIELD_ENUM (
    "type", CYAML_FLAG_DEFAULT,
    Region, type, region_type_strings,
    CYAML_ARRAY_LEN (region_type_strings)),
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
  CYAML_FIELD_STRING_PTR (
    "filename",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    Region, filename,
   	0, CYAML_UNLIMITED),
	CYAML_FIELD_STRING_PTR (
    "linked_region_name", CYAML_FLAG_POINTER,
    Region, linked_region_name,
    0, CYAML_UNLIMITED),
	CYAML_FIELD_INT (
    "muted", CYAML_FLAG_DEFAULT,
    Region, muted),
  CYAML_FIELD_SEQUENCE_COUNT (
    "midi_notes", CYAML_FLAG_DEFAULT,
    Region, midi_notes, num_midi_notes,
    &midi_note_schema, 0, CYAML_UNLIMITED),
	CYAML_FIELD_INT (
    "track_pos", CYAML_FLAG_DEFAULT,
    Region, track_pos),

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
  const RegionType type,
  const Position * start_pos,
  const Position * end_pos,
  const int        is_main);

/**
 * Inits freshly loaded region.
 */
void
region_init_loaded (Region * region);

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
 * Generate a RegionWidget for the Region and all
 * its counterparts.
 */
void
region_gen_widget (
  Region * region);

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
  Region * region);

/**
 * Returns the length of the loop in ticks.
 */
long
region_get_loop_length_in_ticks (
  Region * region);

/**
 * Converts a position on the timeline (global)
 * to a local position (in the clip).
 *
 * If normalize is 1 it will only return a position
 * from 0.0.0.0 to loop_end (it will traverse the
 * loops to find the appropriate position),
 * otherwise it may exceed loop_end.
 *
 * @param timeline_pos Timeline position.
 * @param local_pos Position to fill.
 */
void
region_timeline_pos_to_local (
  Region *   region,
  Position * timeline_pos,
  Position * local_pos,
  int        normalize);

/**
 * Returns the Track this Region is in.
 */
Track *
region_get_track (
  Region * region);

/**
 * Returns the number of loops in the region,
 * optionally including incomplete ones.
 */
int
region_get_num_loops (
  Region * region,
  int      count_incomplete_loops);

/**
 * Shifts the Region by given number of ticks on x,
 * and delta number of visible tracks on y.
 */
void
region_shift (
  Region * self,
  long ticks,
  int  delta);

/**
 * Resizes the region on the left side or right side
 * by given amount of ticks.
 *
 * @param left 1 to resize left side, 0 to resize right
 *   side.
 * @param ticks Number of ticks to resize.
 */
void
region_resize (
  Region * r,
  int      left,
  long     ticks);

/**
 * Clamps position then sets it to its counterparts.
 *
 * To be used only when resizing. For moving,
 * use region_move().
 *
 * @param trans_only Only set the transient
 *   Position's.
 * @param validate Validate the Position before
 *   setting.
 */
void
region_set_start_pos (
  Region * region,
  Position * pos,
  int        trans_only,
  int        validate);

/**
 * Getter for start pos.
 */
void
region_get_start_pos (
  Region * region,
  Position * pos);

/**
 * Sets the track lane.
 */
void
region_set_lane (
  Region * region,
  TrackLane * lane);

void
region_set_cache_end_pos (
  Region * region,
  const Position * pos);

void
region_set_cache_start_pos (
  Region * region,
  Position * pos);

/**
 * Clamps position then sets it.
 *
 * To be used only when resizing. For moving,
 * use region_move().
 *
 * @param trans_only Only set the Position to the
 *   counterparts.
 * @param validate Validate the Position before
 *   setting.
 */
void
region_set_end_pos (
  Region * region,
  Position * pos,
  int        trans_only,
  int        validate);

/**
 * Moves the Region by the given amount of ticks.
 *
 * @param use_cached_pos Add the ticks to the cached
 *   Position instead of its current Position.
 * @param trans_only Only do transients.
 * @return Whether moved or not.
 */
int
region_move (
  Region * region,
  long     ticks,
  int      use_cached_pos,
  int      trans_only);

/**
 * Checks if position is valid then sets it.
 */
void
region_set_true_end_pos (Region * region,
                         Position * end_pos);

/**
 * Checks if position is valid then sets it.
 */
void
region_set_loop_end_pos (Region * region,
                         Position * pos);

/**
 * Checks if position is valid then sets it.
 */
void
region_set_loop_start_pos (Region * region,
                         Position * pos);

/**
 * Checks if position is valid then sets it.
 */
void
region_set_clip_start_pos (Region * region,
                         Position * pos);

/**
 * Returns if Region is in MidiArrangerSelections.
 */
int
region_is_selected (Region * self);

/**
 * Returns if Region is (should be) visible.
 */
#define region_should_be_visible(mn) \
  arranger_object_info_should_be_visible ( \
    mn->obj_info)

/**
 * Returns if the position is inside the region
 * or not.
 */
int
region_is_hit (
  Region *   region,
  Position * pos); ///< global position

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
