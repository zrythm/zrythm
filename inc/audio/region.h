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
#include "utils/yaml.h"

#include <gtk/gtk.h>

#define REGION_PRINTF_FILENAME "%d_%s_%s.mid"
#define region_set_track(region,track) \
  ((Region *)region)->track = (Track *) track; \
  ((Region *)region)->track_id = ((Track *) track)->id

typedef struct _RegionWidget RegionWidget;
typedef struct Channel Channel;
typedef struct Track Track;
typedef struct MidiNote MidiNote;
typedef struct _AudioClipWidget AudioClipWidget;

typedef enum RegionType
{
  REGION_TYPE_MIDI,
  REGION_TYPE_AUDIO
} RegionType;

typedef enum RegionCloneFlag
{
  /**
   * Create a completely new region with a new id.
   */
  REGION_CLONE_COPY,
  REGION_CLONE_LINK
} RegionCloneFlag;

typedef struct Region
{
  /**
   * ID for saving/loading projects
   */
  int          id;

  /**
   * Region name to be shown on the region.
   */
  char         * name;

  RegionType   type;

  /* GLOBAL POSITIONS ON THE TIMELINE */
  /* -------------------------------- */

  /** Start position in the timeline. */
  Position     start_pos;

  /** End position in the timeline. */
  Position     end_pos;

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

  /**
   * Owner track.
   */
  int            track_id;
  Track *        track; ///< cache

  /**
   * Linked parent region.
   *
   * Either the midi notes from this region, or the midi
   * notes from the linked region are used
   */
  int             linked_region_id;
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

  /**
   * MIDI notes.
   */
  int             midi_note_ids[200];
  MidiNote *      midi_notes[200];

  /**
   * MIDI note count.
   */
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
   * ID of region in use.
   *
   * Used when doing/undoing.
   */
  int             actual_id;

  /**
   * Transient or not.
   *
   * Transient regions are regions that are cloned
   * and used during moving, then discarded.  */
  int             transient;
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
	CYAML_FIELD_INT (
    "id", CYAML_FLAG_DEFAULT,
	  Region, id),
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
	CYAML_FIELD_INT (
    "track_id", CYAML_FLAG_DEFAULT,
    Region, track_id),
	CYAML_FIELD_INT (
    "linked_region_id", CYAML_FLAG_DEFAULT,
    Region, linked_region_id),
	CYAML_FIELD_INT (
    "muted", CYAML_FLAG_DEFAULT,
    Region, muted),
  CYAML_FIELD_SEQUENCE_COUNT (
    "midi_note_ids", CYAML_FLAG_DEFAULT,
    Region, midi_note_ids, num_midi_notes,
    &int_schema, 0, CYAML_UNLIMITED),
	CYAML_FIELD_INT (
    "actual_id", CYAML_FLAG_DEFAULT,
    Region, actual_id),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
region_schema = {
	CYAML_VALUE_MAPPING (CYAML_FLAG_POINTER,
			Region, region_fields_schema),
};

/**
 * Inits freshly loaded region.
 */
void
region_init_loaded (Region * region);

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
 */
void
region_timeline_pos_to_local (
  Region *   region, ///< the region
  Position * timeline_pos, ///< timeline position
  Position * local_pos, ///< position to fill
  int        normalize);

/**
 * Returns the number of loops in the region,
 * optionally including incomplete ones.
 */
int
region_get_num_loops (
  Region * region,
  int      count_incomplete_loops);

void
region_shift (
  Region * region,
  long ticks,
  int  delta);

/**
 * Only to be used by implementing structs.
 */
void
region_init (Region *   region,
             RegionType type,
             Track *    track,
             Position * start_pos,
             Position * end_pos);

/**
 * Clamps position then sets it.
 */
void
region_set_start_pos (
  Region * region,
  Position * pos);

/**
 * Checks if position is valid then sets it.
 */
void
region_set_end_pos (Region * region,
                    Position * end_pos);

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
int
region_is_visible (Region * self);

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
 * Returns the region at the given position in the given channel
 */
Region *
region_at_position (Track    * track, ///< the track to look in
                    Position * pos); ///< the position

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
 * Creates a new region and either links to the original or
 * copies every field.
 */
Region *
region_clone (Region *        region,
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

void
region_free (Region * region);

SERIALIZE_INC (Region, region)
DESERIALIZE_INC (Region, region)
PRINT_YAML_INC (Region, region)

#endif // __AUDIO_REGION_H__
