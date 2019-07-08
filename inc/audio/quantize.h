/*
 * Copyright (C) 2019 Alexandros Theodotou
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
 * Quantize info.
 */

#ifndef __AUDIO_QUANTIZE_H__
#define __AUDIO_QUANTIZE_H__

#include "audio/position.h"
#include "audio/snap_grid.h"

#include "utils/yaml.h"

#define QUANTIZE_IS_MIDI(q) \
  (&ZRYTHM->quantize_midi == q)
#define QUANTIZE_IS_TIMELINE(q) \
  (&ZRYTHM->quantize_timeline == q)
#define QUANTIZE_TIMELINE \
  (&PROJECT->quantize_timeline)
#define QUANTIZE_MIDI \
  (&PROJECT->quantize_midi)

typedef struct Quantize
{
  /**
   * Quantize to grid.
   *
   * If this is on, the other members are ignored.
   */
  int              use_grid;

  /**
   * Same as snap grid.
   */
  NoteLength       note_length;
  NoteType         note_type;

  /**
   * Snap points to be used by the grid and by position
   * to calculate previous/next snap point.
   */
  Position         snap_points[MAX_SNAP_POINTS];
  int              num_snap_points;
} Quantize;

static const cyaml_schema_field_t
  quantize_fields_schema[] =
{
  CYAML_FIELD_INT (
    "use_grid", CYAML_FLAG_DEFAULT,
    Quantize, use_grid),
  CYAML_FIELD_ENUM (
    "note_length", CYAML_FLAG_DEFAULT,
    Quantize, note_length, note_length_strings,
    CYAML_ARRAY_LEN (note_length_strings)),
  CYAML_FIELD_ENUM (
    "note_type", CYAML_FLAG_DEFAULT,
    Quantize, note_type, note_type_strings,
    CYAML_ARRAY_LEN (note_type_strings)),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
quantize_schema = {
	CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
	  Quantize, quantize_fields_schema),
};

/**
 * Initializes the quantize struct.
 */
void
quantize_init (Quantize *   self,
               NoteLength   note_length);

/**
 * Updates quantize snap points.
 */
void
quantize_update_snap_points (Quantize * self);

/**
 * Sets note length and re-calculates snap points.
 */
//void
//quantize_set_note_length (Quantize * self,
                          //NoteLength note_length);

/**
 * Sets note type and re-calculates snap points.
 */
//void
//quantize_set_note_type (Quantize *self,
                        //NoteType   note_type);

#endif
