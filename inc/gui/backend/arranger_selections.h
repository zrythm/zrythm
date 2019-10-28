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
 * Common data structures and functions for
 * *ArrangerSelections.
 */

#ifndef __GUI_BACKEND_ARRANGER_SELECTIONS_H__
#define __GUI_BACKEND_ARRANGER_SELECTIONS_H__

#include "utils/yaml.h"

typedef struct ArrangerObject ArrangerObject;
typedef struct Position Position;
typedef enum ArrangerObjectUpdateFlag
  ArrangerObjectUpdateFlag;

/**
 * @addtogroup gui_backend
 *
 * @{
 */

typedef enum ArrangerSelectionsType
{
  ARRANGER_SELECTIONS_TYPE_CHORD,
  ARRANGER_SELECTIONS_TYPE_TIMELINE,
  ARRANGER_SELECTIONS_TYPE_MIDI,
  ARRANGER_SELECTIONS_TYPE_AUTOMATION,
} ArrangerSelectionsType;

typedef struct ArrangerSelections
{
  /** Type of selections. */
  ArrangerSelectionsType type;
} ArrangerSelections;

static const cyaml_strval_t
arranger_selections_type_strings[] =
{
	{ "Chord",
    ARRANGER_SELECTIONS_TYPE_CHORD    },
	{ "Timeline",
    ARRANGER_SELECTIONS_TYPE_TIMELINE   },
	{ "MIDI",
    ARRANGER_SELECTIONS_TYPE_MIDI },
	{ "Automation",
    ARRANGER_SELECTIONS_TYPE_AUTOMATION   },
};

static const cyaml_schema_field_t
arranger_selections_fields_schema[] =
{
  CYAML_FIELD_ENUM (
    "type", CYAML_FLAG_DEFAULT,
    ArrangerSelections, type,
    arranger_selections_type_strings,
    CYAML_ARRAY_LEN (
      arranger_selections_type_strings)),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
arranger_selections_schema = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER, ArrangerSelections,
    arranger_selections_fields_schema),
};

/**
 * Inits the selections after loading a project.
 */
void
arranger_selections_init_loaded (
  ArrangerSelections * self);

/**
 * Initializes the selections.
 */
void
arranger_selections_init (
  ArrangerSelections *   self,
  ArrangerSelectionsType type);

/**
 * Appends the given object to the selections.
 */
void
arranger_selections_add_object (
  ArrangerSelections * self,
  ArrangerObject *     obj);

/**
 * Resets the given counterparts from the other
 * counterparts.
 *
 * @param reset_trans 1 to reset the transient
 *   from main, 0 to reset main from transient.
 */
void
arranger_selections_reset_counterparts (
  ArrangerSelections * self,
  const int            reset_trans);

/**
 * Updates the visibility and selection state for
 * all objects in the selections.
 *
 * Eg, when moving or resizing, it hides the
 * original objects and only shows the transients.
 * When copy- moving, it shows both.
 */
void
arranger_selections_update_widget_visibility (
  ArrangerSelections * self);

/**
 * Clone the struct for copying, undoing, etc.
 */
ArrangerSelections *
arranger_selections_clone (
  ArrangerSelections * self);

/**
 * Returns if there are any selections.
 */
int
arranger_selections_has_any (
  ArrangerSelections * self);

/**
 * Returns the position of the leftmost object.
 *
 * @param transient If 1, the transient objects are
 *   checked instead.
 * @param pos The return value will be stored here.
 * @param global Return global (timeline) Position,
 *   otherwise returns the local (from the start
 *   of the Region) Position.
 */
void
arranger_selections_get_start_pos (
  ArrangerSelections * self,
  Position *           pos,
  int                  transient,
  int                  global);

/**
 * Returns the end position of the rightmost object.
 *
 * @param pos The return value will be stored here.
 * @param transient If 1, the transient objects are
 * checked instead.
 * @param global Return global (timeline) Position,
 *   otherwise returns the local (from the start
 *   of the Region) Position.
 */
void
arranger_selections_get_end_pos (
  ArrangerSelections * self,
  Position *           pos,
  int                  transient,
  int                  global);

/**
 * Gets first object.
 *
 * @param transient If 1, transient objects are
 *   checked instead.
 */
ArrangerObject *
arranger_selections_get_first_object (
  ArrangerSelections * self,
  const int            transient);

/**
 * Gets last object.
 *
 * @param transient If 1, transient objects are
 *   checked instead.
 */
ArrangerObject *
arranger_selections_get_last_object (
  ArrangerSelections * self,
  const int            transient);

/**
 * Pastes the given selections to the given Position.
 */
//void
//arranger_selections_paste_to_pos (
  //ArrangerSelections * self,
  //Position *           pos);

/**
 * Sets the cache Position's for each object in
 * the selection.
 *
 * Used by the ArrangerWidget's.
 */
void
arranger_selections_set_cache_poses (
  ArrangerSelections * self);

/**
 * Returns all objects in the selections in a
 * newly allocated array that should be free'd.
 *
 * @param size A pointer to save the size into.
 */
ArrangerObject **
arranger_selections_get_all_objects (
  ArrangerSelections * self,
  int *                size);

/**
 * Moves the selections by the given
 * amount of ticks.
 *
 * @param use_cached_pos Add the ticks to the cached
 *   Position's instead of the current Position's.
 * @param ticks Ticks to add.
 * @param update_flag ArrangerObjectUpdateFlag.
 */
void
arranger_selections_add_ticks (
  ArrangerSelections *     self,
  const long               ticks,
  const int                use_cached_pos,
  ArrangerObjectUpdateFlag update_flag);

/**
 * Clears selections.
 */
void
arranger_selections_clear (
  ArrangerSelections * self);

/**
 * Code to run after deserializing.
 */
void
arranger_selections_post_deserialize (
  ArrangerSelections * self);

/**
 * Frees the selections but not the objects.
 */
void
arranger_selections_free (
  ArrangerSelections * self);

/**
 * Frees all the objects as well.
 *
 * To be used in actions where the selections are
 * all clones.
 */
void
arranger_selections_free_full (
  ArrangerSelections * self);

/**
 * Returns if the arranger object is in the
 * selections or  not.
 *
 * The object must be the main object (see
 * ArrangerObjectInfo).
 */
int
arranger_selections_contains_object (
  ArrangerSelections * self,
  ArrangerObject *     obj);

/**
 * Removes the arranger object from the selections.
 */
void
arranger_selections_remove_object (
  ArrangerSelections * self,
  ArrangerObject *     obj);

/**
* @}
*/

#endif
