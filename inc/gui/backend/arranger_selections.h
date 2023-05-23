// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Common data structures and functions for
 * *ArrangerSelections.
 */

#ifndef __GUI_BACKEND_ARRANGER_SELECTIONS_H__
#define __GUI_BACKEND_ARRANGER_SELECTIONS_H__

#include <stdbool.h>

#include "utils/yaml.h"

typedef struct ArrangerObject ArrangerObject;
typedef struct Position       Position;
typedef struct AudioClip      AudioClip;
typedef struct UndoableAction UndoableAction;
typedef enum ArrangerSelectionsActionEditType
                       ArrangerSelectionsActionEditType;
typedef struct ZRegion ZRegion;

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define ARRANGER_SELECTIONS_SCHEMA_VERSION 1

#define ARRANGER_SELECTIONS_MAGIC 35867752
#define IS_ARRANGER_SELECTIONS(x) \
  (((ArrangerSelections *) x)->magic \
   == ARRANGER_SELECTIONS_MAGIC)
#define IS_ARRANGER_SELECTIONS_AND_NONNULL(x) \
  (x && IS_ARRANGER_SELECTIONS (x))
#define ARRANGER_SELECTIONS(x) arranger_selections_cast (x)

#define ARRANGER_SELECTIONS_DEFAULT_NUDGE_TICKS 0.1

typedef enum ArrangerSelectionsType
{
  ARRANGER_SELECTIONS_TYPE_NONE,
  ARRANGER_SELECTIONS_TYPE_CHORD,
  ARRANGER_SELECTIONS_TYPE_TIMELINE,
  ARRANGER_SELECTIONS_TYPE_MIDI,
  ARRANGER_SELECTIONS_TYPE_AUTOMATION,
  ARRANGER_SELECTIONS_TYPE_AUDIO,
} ArrangerSelectionsType;

static const cyaml_strval_t arranger_selections_type_strings[] = {
  {"None",        ARRANGER_SELECTIONS_TYPE_NONE      },
  { "Chord",      ARRANGER_SELECTIONS_TYPE_CHORD     },
  { "Timeline",   ARRANGER_SELECTIONS_TYPE_TIMELINE  },
  { "MIDI",       ARRANGER_SELECTIONS_TYPE_MIDI      },
  { "Automation", ARRANGER_SELECTIONS_TYPE_AUTOMATION},
  { "Audio",      ARRANGER_SELECTIONS_TYPE_AUDIO     },
};

typedef struct ArrangerSelections
{
  int schema_version;

  /** Type of selections. */
  ArrangerSelectionsType type;

  int magic;
} ArrangerSelections;

static const cyaml_schema_field_t
  arranger_selections_fields_schema[] = {
    YAML_FIELD_INT (ArrangerSelections, schema_version),
    YAML_FIELD_ENUM (
      ArrangerSelections,
      type,
      arranger_selections_type_strings),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t arranger_selections_schema = {
  YAML_VALUE_PTR (
    ArrangerSelections,
    arranger_selections_fields_schema),
};

typedef enum ArrangerSelectionsProperty
{
  ARRANGER_SELECTIONS_PROPERTY_HAS_LENGTH,
  ARRANGER_SELECTIONS_PROPERTY_CAN_LOOP,
  ARRANGER_SELECTIONS_PROPERTY_HAS_LOOPED,
  ARRANGER_SELECTIONS_PROPERTY_CAN_FADE,
} ArrangerSelectionsProperty;

static inline ArrangerSelections *
arranger_selections_cast (void * sel)
{
  if (!IS_ARRANGER_SELECTIONS ((ArrangerSelections *) sel))
    {
      g_warning ("%s", __func__);
    }
  return (ArrangerSelections *) sel;
}

/**
 * Inits the selections after loading a project.
 *
 * @param project Whether these are project
 *   selections (as opposed to clones).
 * @param action To be passed when this is called from an
 *   undoable action.
 */
NONNULL_ARGS (1)
void arranger_selections_init_loaded (
  ArrangerSelections * self,
  bool                 project,
  UndoableAction *     action);

/**
 * Initializes the selections.
 */
NONNULL void
arranger_selections_init (
  ArrangerSelections *   self,
  ArrangerSelectionsType type);

/**
 * Creates new arranger selections.
 */
ArrangerSelections *
arranger_selections_new (ArrangerSelectionsType type);

/**
 * Verify that the objects are not invalid.
 */
NONNULL bool
arranger_selections_verify (ArrangerSelections * self);

/**
 * Appends the given object to the selections.
 */
NONNULL void
arranger_selections_add_object (
  ArrangerSelections * self,
  ArrangerObject *     obj);

/**
 * Sets the values of each object in the dest
 * selections to the values in the src selections.
 */
NONNULL void
arranger_selections_set_from_selections (
  ArrangerSelections * dest,
  ArrangerSelections * src);

/**
 * Sorts the selections by their indices (eg, for
 * regions, their track indices, then the lane
 * indices, then the index in the lane).
 *
 * @note Only works for objects whose tracks exist.
 *
 * @param desc Descending or not.
 */
NONNULL void
arranger_selections_sort_by_indices (
  ArrangerSelections * sel,
  int                  desc);

/**
 * Clone the struct for copying, undoing, etc.
 */
NONNULL ArrangerSelections *
arranger_selections_clone (const ArrangerSelections * self);

/**
 * Returns if there are any selections.
 */
NONNULL bool
arranger_selections_has_any (ArrangerSelections * self);

/**
 * Returns the position of the leftmost object.
 *
 * @param pos[out] The return value will be stored
 *   here.
 * @param global Return global (timeline) Position,
 *   otherwise returns the local (from the start
 *   of the Region) Position.
 */
NONNULL void
arranger_selections_get_start_pos (
  const ArrangerSelections * self,
  Position *                 pos,
  const bool                 global);

/**
 * Returns the end position of the rightmost object.
 *
 * @param pos The return value will be stored here.
 * @param global Return global (timeline) Position,
 *   otherwise returns the local (from the start
 *   of the Region) Position.
 */
NONNULL void
arranger_selections_get_end_pos (
  ArrangerSelections * self,
  Position *           pos,
  int                  global);

/**
 * Returns the number of selected objects.
 */
NONNULL int
arranger_selections_get_num_objects (
  const ArrangerSelections * self);

/**
 * Gets first object.
 */
NONNULL ArrangerObject *
arranger_selections_get_first_object (
  const ArrangerSelections * self);

/**
 * Gets last object.
 *
 * @param ends_last Whether to get the object that ends last,
 *   otherwise the object that starts last.
 */
NONNULL ArrangerObject *
arranger_selections_get_last_object (
  const ArrangerSelections * self,
  bool                       ends_last);

/**
 * Pastes the given selections to the given
 * Position.
 */
NONNULL void
arranger_selections_paste_to_pos (
  ArrangerSelections * self,
  Position *           pos,
  bool                 undoable);

/**
 * Appends all objects in the given array.
 */
NONNULL void
arranger_selections_get_all_objects (
  const ArrangerSelections * self,
  GPtrArray *                arr);

#if 0
/**
 * Redraws each object in the arranger selections.
 */
NONNULL
void
arranger_selections_redraw (
  ArrangerSelections * self);
#endif

/**
 * Adds each object in the selection to the given region (if
 * applicable).
 *
 * @param clone Whether to clone each object instead of adding
 *   it directly.
 */
void
arranger_selections_add_to_region (
  ArrangerSelections * self,
  ZRegion *            region,
  bool                 clone);

/**
 * Moves the selections by the given
 * amount of ticks.
 *
 * @param ticks Ticks to add.
 */
NONNULL void
arranger_selections_add_ticks (
  ArrangerSelections * self,
  const double         ticks);

/**
 * Returns whether all the selections are on the
 * same lane (track lane or automation lane).
 */
NONNULL bool
arranger_selections_all_on_same_lane (
  ArrangerSelections * self);

/**
 * Selects all possible objects from the project.
 */
NONNULL void
arranger_selections_select_all (
  ArrangerSelections * self,
  bool                 fire_events);

/**
 * Clears selections.
 */
NONNULL void
arranger_selections_clear (
  ArrangerSelections * self,
  bool                 free,
  bool                 fire_events);

/**
 * Code to run after deserializing.
 */
NONNULL void
arranger_selections_post_deserialize (
  ArrangerSelections * self);

NONNULL bool
arranger_selections_validate (ArrangerSelections * self);

/**
 * Frees anything allocated by the selections
 * but not the objects or @ref self itself.
 */
NONNULL void
arranger_selections_free_members (ArrangerSelections * self);

/**
 * Frees the selections but not the objects.
 */
NONNULL void
arranger_selections_free (ArrangerSelections * self);

/**
 * Frees all the objects as well.
 *
 * To be used in actions where the selections are
 * all clones.
 */
NONNULL void
arranger_selections_free_full (ArrangerSelections * self);

/**
 * Returns if the arranger object is in the
 * selections or  not.
 *
 * The object must be the main object (see
 * ArrangerObjectInfo).
 */
NONNULL int
arranger_selections_contains_object (
  ArrangerSelections * self,
  ArrangerObject *     obj);

/**
 * Returns if the selections contain an undeletable
 * object (such as the start marker).
 */
NONNULL bool
arranger_selections_contains_undeletable_object (
  const ArrangerSelections * self);

/**
 * Returns if the selections contain an unclonable
 * object (such as the start marker).
 */
NONNULL bool
arranger_selections_contains_unclonable_object (
  const ArrangerSelections * self);

NONNULL bool
arranger_selections_contains_unrenamable_object (
  const ArrangerSelections * self);

/**
 * Checks whether an object matches the given
 * parameters.
 *
 * If a parameter should be checked, the has_*
 * argument must be true and the corresponding
 * argument must have the value to be checked
 * against.
 */
NONNULL bool
arranger_selections_contains_object_with_property (
  ArrangerSelections *       self,
  ArrangerSelectionsProperty property,
  bool                       value);

/**
 * Removes the arranger object from the selections.
 */
NONNULL void
arranger_selections_remove_object (
  ArrangerSelections * self,
  ArrangerObject *     obj);

/**
 * Merges the given selections into one region.
 *
 * @note All selections must be on the same lane.
 */
NONNULL void
arranger_selections_merge (ArrangerSelections * self);

/**
 * Returns if the selections can be pasted.
 */
NONNULL bool
arranger_selections_can_be_pasted (ArrangerSelections * self);

NONNULL bool
arranger_selections_contains_looped (
  ArrangerSelections * self);

NONNULL bool
arranger_selections_can_be_merged (ArrangerSelections * self);

NONNULL double
arranger_selections_get_length_in_ticks (
  ArrangerSelections * self);

NONNULL bool
arranger_selections_contains_clip (
  ArrangerSelections * self,
  AudioClip *          clip);

NONNULL bool
arranger_selections_can_split_at_pos (
  const ArrangerSelections * self,
  const Position *           pos);

NONNULL ArrangerSelections *
arranger_selections_get_for_type (ArrangerSelectionsType type);

/**
* @}
*/

#endif
