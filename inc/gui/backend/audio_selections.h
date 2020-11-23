/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * API for selections in the AudioArrangerWidget.
 */

#ifndef __GUI_BACKEND_AUDIO_SELECTIONS_H__
#define __GUI_BACKEND_AUDIO_SELECTIONS_H__

#include "gui/backend/arranger_selections.h"
#include "utils/yaml.h"

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define AUDIO_SELECTIONS \
  (&PROJECT->audio_selections)

/**
 * Selections to be used for the AudioArrangerWidget's
 * current selections, copying, undoing, etc.
 */
typedef struct AudioSelections
{
  ArrangerSelections base;

  /** Whether or not a selection exists. */
  bool               has_selection;

  /**
   * Selected range.
   *
   * The start position must always be before the
   * end position.
   */
  Position           sel_start;
  Position           sel_end;

} AudioSelections;

static const cyaml_schema_field_t
  audio_selections_fields_schema[] =
{
  YAML_FIELD_MAPPING_EMBEDDED (
    AudioSelections, base,
    arranger_selections_fields_schema),
  YAML_FIELD_INT (
    AudioSelections, has_selection),
  YAML_FIELD_MAPPING_EMBEDDED (
    AudioSelections, sel_start,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    AudioSelections, sel_end,
    position_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
audio_selections_schema = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    AudioSelections,
    audio_selections_fields_schema),
};

/**
 * Sets whether a range selection exists and sends
 * events to update the UI.
 */
void
audio_selections_set_has_range (
  AudioSelections * self,
  bool              has_range);

/**
 * Returns if the selections can be pasted.
 *
 * @param pos Position to paste to.
 * @param region ZRegion to paste to.
 */
bool
audio_selections_can_be_pasted (
  AudioSelections * ts,
  Position *        pos,
  ZRegion *         r);

SERIALIZE_INC (AudioSelections,
               audio_selections)
DESERIALIZE_INC (AudioSelections,
                 audio_selections)
PRINT_YAML_INC (AudioSelections,
                audio_selections)

/**
* @}
*/

#endif
