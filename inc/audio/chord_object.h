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
 * Chord object in the TimelineArranger.
 */

#ifndef __AUDIO_CHORD_OBJECT_H__
#define __AUDIO_CHORD_OBJECT_H__

#include <stdint.h>

#include "audio/chord_descriptor.h"
#include "audio/position.h"
#include "gui/backend/arranger_object.h"
#include "gui/backend/arranger_object_info.h"
#include "utils/yaml.h"

/**
 * @addtogroup audio
 *
 * @{
 */

#define chord_object_is_main(c) \
  arranger_object_info_is_main ( \
    &c->obj_info)

#define chord_object_is_transient(r) \
  arranger_object_info_is_transient ( \
    &r->obj_info)

/** Gets the main counterpart of the ChordObject. */
#define chord_object_get_main_chord_object(r) \
  ((ChordObject *) r->obj_info.main)

/** Gets the transient counterpart of the ChordObject. */
#define chord_object_get_main_trans_chord_object(r) \
  ((ChordObject *) r->obj_info.main_trans)

typedef enum ChordObjectCloneFlag
{
  CHORD_OBJECT_CLONE_COPY_MAIN,
  CHORD_OBJECT_CLONE_COPY,
} ChordObjectCloneFlag;

typedef struct _ChordObjectWidget ChordObjectWidget;
typedef struct ChordDescriptor ChordDescriptor;

/**
 * A ChordObject to be shown in the
 * TimelineArrangerWidget.
 */
typedef struct ChordObject
{
  /** ChordObject object position (if used in chord
   * Track). */
  Position            pos;

  /** Cache, used in runtime operations. */
  Position            cache_pos;

  /** The index of the chord it belongs to
   * (0 topmost). */
  int                 index;

  /** Pointer back to parent. */
  Region *            region;

  /** Used in clones to identify a region instead of
   * cloning the whole Region. */
  char *              region_name;

  ChordObjectWidget * widget;

  ArrangerObjectInfo  obj_info;
} ChordObject;

static const cyaml_schema_field_t
  chord_object_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "pos", CYAML_FLAG_DEFAULT,
    ChordObject, pos, position_fields_schema),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
chord_object_schema = {
	CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
			ChordObject, chord_object_fields_schema),
};


ARRANGER_OBJ_DECLARE_MOVABLE (
  ChordObject, chord_object);

/**
 * Init the ChordObject after the Project is loaded.
 */
void
chord_object_init_loaded (
  ChordObject * self);

/**
 * Creates a ChordObject.
 */
ChordObject *
chord_object_new (
  Region * region,
  int index,
  int is_main);

static inline int
chord_object_is_equal (
  ChordObject * a,
  ChordObject * b)
{
  return
    !position_compare(&a->pos, &b->pos) &&
    a->index == b->index;
}

/**
 * Sets the Track of the chord.
 */
void
chord_object_set_region (
  ChordObject * self,
  Region *      region);

/**
 * Returns the ChordDescriptor associated with this
 * ChordObject.
 */
ChordDescriptor *
chord_object_get_chord_descriptor (
  ChordObject * self);

/**
 * Updates the frames of each position in each child
 * of the ChordObject recursively.
 */
void
chord_object_update_frames (
  ChordObject * self);

/**
 * Finds the ChordObject in the project
 * corresponding to the given one.
 */
ChordObject *
chord_object_find (
  ChordObject * clone);

/**
 * Finds the ChordObject in the project
 * corresponding to the given one's position.
 */
ChordObject *
chord_object_find_by_pos (
  ChordObject * clone);

/**
 * Clones the given chord.
 */
ChordObject *
chord_object_clone (
  ChordObject * src,
  ChordObjectCloneFlag flag);

/**
 * Frees the ChordObject.
 */
void
chord_object_free (
  ChordObject * self);

/**
 * @}
 */

#endif
