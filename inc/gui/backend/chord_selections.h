/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * API for selections in the piano roll.
 */

#ifndef __GUI_BACKEND_CHORD_SELECTIONS_H__
#define __GUI_BACKEND_CHORD_SELECTIONS_H__

#include "audio/chord_object.h"
#include "gui/backend/arranger_selections.h"
#include "utils/yaml.h"

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define CHORD_SELECTIONS \
  (&PROJECT->chord_selections)

/**
 * Selections to be used for the ChordArrangerWidget's
 * current selections, copying, undoing, etc.
 */
typedef struct ChordSelections
{
  /** Base struct. */
  ArrangerSelections  base;

  /** Selected ChordObject's.
   *
   * These are used for
   * serialization/deserialization only. */
  ChordObject **      chord_objects;
  int                 num_chord_objects;
  size_t              chord_objects_size;

} ChordSelections;

static const cyaml_schema_field_t
  chord_selections_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "base", CYAML_FLAG_DEFAULT,
    ChordSelections, base,
    arranger_selections_fields_schema),
  CYAML_FIELD_SEQUENCE_COUNT (
    "chord_objects",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    ChordSelections, chord_objects,
    num_chord_objects,
    &chord_object_schema, 0, CYAML_UNLIMITED),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
chord_selections_schema = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    ChordSelections,
    chord_selections_fields_schema),
};

/**
 * Returns if the selections can be pasted.
 *
 * @param pos Position to paste to.
 * @param region ZRegion to paste to.
 */
int
chord_selections_can_be_pasted (
  ChordSelections * ts,
  Position *        pos,
  ZRegion *          region);

void
chord_selections_paste_to_pos (
  ChordSelections * ts,
  Position *        playhead);

SERIALIZE_INC (ChordSelections,
               chord_selections)
DESERIALIZE_INC (ChordSelections,
                 chord_selections)
PRINT_YAML_INC (ChordSelections,
                chord_selections)

/**
* @}
*/

#endif
