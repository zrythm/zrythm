/*
 * audio/region.h - A region in the timeline having a start
 *   and an end
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#ifndef __AUDIO_REGION_H__
#define __AUDIO_REGION_H__

#include "audio/audio_clip.h"
#include "audio/midi_note.h"
#include "audio/midi_region.h"
#include "audio/position.h"
#include "utils/yaml.h"

#define REGION_PRINTF_FILENAME "%d_%s_%s.mid"
#define region_set_track(region,track) \
  ((Region *)region)->track = (Track *) track; \
  ((Region *)region)->track_id = ((Track *) track)->id

typedef struct _RegionWidget RegionWidget;
typedef struct Channel Channel;
typedef struct Track Track;
typedef struct MidiNote MidiNote;
typedef struct MidiRegion MidiRegion;
typedef struct AudioRegion AudioRegion;

typedef enum RegionType
{
  REGION_TYPE_MIDI,
  REGION_TYPE_AUDIO
} RegionType;

typedef enum RegionCloneFlag
{
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
  Position     start_pos; ///< start position
  Position     end_pos; ///< end position

  /**
   * Position where the first unit of repeation ends.
   *
   * If end pos > unit_end_pos, then region is repeating.
   */
  Position        unit_end_pos;

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

  int                      selected;

  /**
   * This is a hack to make the region serialize its
   * subtype.
   *
   * Only useful for serialization.
   */
  MidiRegion *         midi_region;
  AudioRegion *        audio_region;
} Region;

typedef struct MidiRegion
{
  Region          parent;

  /**
   * MIDI notes.
   */
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

  /**
   * Dummy member because serialization skips the midi
   * region if it is all 0's.
   */
  int             dummy;
} MidiRegion;

typedef struct AudioRegion
{
  Region              parent;

  AudioClip *         audio_clip;

  int                 dummy;
} AudioRegion;

static const cyaml_strval_t region_type_strings[] = {
	{ "Midi",          REGION_TYPE_MIDI    },
	{ "Audio",         REGION_TYPE_AUDIO   },
};

static const cyaml_schema_field_t
  midi_region_fields_schema[] =
{
  CYAML_FIELD_SEQUENCE_COUNT (
    /* default because it is an array of pointers, not a
     * pointer to an array */
    "midi_notes", CYAML_FLAG_DEFAULT,
      MidiRegion, midi_notes, num_midi_notes,
      &midi_note_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_INT (
      "dummy", CYAML_FLAG_DEFAULT,
      MidiRegion, dummy),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
midi_region_schema = {
	CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
			MidiRegion, midi_region_fields_schema),
};

static const cyaml_schema_field_t
  audio_region_fields_schema[] =
{
  CYAML_FIELD_INT (
      "dummy", CYAML_FLAG_DEFAULT,
      AudioRegion, dummy),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
audio_region_schema = {
	CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
			AudioRegion, audio_region_fields_schema),
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
      "unit_end_pos", CYAML_FLAG_DEFAULT,
      Region, unit_end_pos, position_fields_schema),
	CYAML_FIELD_INT (
			"track_id", CYAML_FLAG_DEFAULT,
			Region, track_id),
	CYAML_FIELD_INT (
			"linked_region_id", CYAML_FLAG_DEFAULT,
			Region, linked_region_id),
	CYAML_FIELD_INT (
			"selected", CYAML_FLAG_DEFAULT,
			Region, selected),
  CYAML_FIELD_MAPPING_PTR (
    "midi_region", CYAML_FLAG_POINTER,
    Region, midi_region, midi_region_fields_schema),
  CYAML_FIELD_MAPPING_PTR (
    "audio_region", CYAML_FLAG_POINTER,
    Region, audio_region, audio_region_fields_schema),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
region_schema = {
	CYAML_VALUE_MAPPING (CYAML_FLAG_POINTER,
			Region, region_fields_schema),
};

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
region_set_start_pos (Region * region,
                      Position * pos,
                      int      moved); ///< region moved or not (to move notes as
                                          ///< well)

/**
 * Checks if position is valid then sets it.
 */
void
region_set_end_pos (Region * region,
                    Position * end_pos);

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
 * Clone region.
 *
 * Creates a new region and either links to the original or
 * copies every field.
 */
Region *
region_clone (Region *        region,
              RegionCloneFlag flag);

SERIALIZE_INC (Region, region)
SERIALIZE_INC (MidiRegion, midi_region)
SERIALIZE_INC (AudioRegion, audio_region)
DESERIALIZE_INC (Region, region)
DESERIALIZE_INC (MidiRegion, midi_region)
DESERIALIZE_INC (AudioRegion, audio_region)
PRINT_YAML_INC (Region, region)
PRINT_YAML_INC (MidiRegion, midi_region)
PRINT_YAML_INC (AudioRegion, audio_region)

#endif // __AUDIO_REGION_H__
