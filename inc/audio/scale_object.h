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
 * Scale object inside the chord Track in the
 * TimelineArranger.
 */

#ifndef __AUDIO_SCALE_OBJECT_H__
#define __AUDIO_SCALE_OBJECT_H__

#include <stdint.h>

#include "audio/scale.h"
#include "audio/position.h"
#include "gui/backend/arranger_object.h"
#include "gui/backend/arranger_object_info.h"
#include "utils/yaml.h"

/**
 * @addtogroup audio
 *
 * @{
 */

#define scale_object_is_main(c) \
  arranger_object_info_is_main ( \
    &c->obj_info)

#define scale_object_is_transient(r) \
  arranger_object_info_is_transient ( \
    &r->obj_info)

/** Gets the main counterpart of the ScaleObject. */
#define scale_object_get_main_scale_object(r) \
  ((ScaleObject *) r->obj_info.main)

/** Gets the transient counterpart of the ScaleObject. */
#define scale_object_get_main_trans_scale_object(r) \
  ((ScaleObject *) r->obj_info.main_trans)

typedef enum ScaleObjectCloneFlag
{
  SCALE_OBJECT_CLONE_COPY_MAIN,
  SCALE_OBJECT_CLONE_COPY,
} ScaleObjectCloneFlag;

typedef struct _ScaleObjectWidget ScaleObjectWidget;
typedef struct MusicalScale MusicalScale;

/**
 * A ScaleObject to be shown in the
 * TimelineArrangerWidget.
 */
typedef struct ScaleObject
{
  /** ScaleObject object position (if used in scale
   * Track). */
  Position       pos;

  /** Cache, used in runtime operations. */
  Position       cache_pos;

  MusicalScale * scale;

  /** Position of Track this ScaleObject is in. */
  int            track_pos;

  /** Cache. */
  Track *        track;

  ScaleObjectWidget *  widget;

  ArrangerObjectInfo   obj_info;
} ScaleObject;

static const cyaml_schema_field_t
  scale_object_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "pos", CYAML_FLAG_DEFAULT,
    ScaleObject, pos, position_fields_schema),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
scale_object_schema = {
  CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
    ScaleObject, scale_object_fields_schema),
};

/**
 * Init the ScaleObject after the Project is loaded.
 */
void
scale_object_init_loaded (
  ScaleObject * self);

/**
 * Creates a ScaleObject.
 */
ScaleObject *
scale_object_new (
  MusicalScale * descr,
  int            is_main);

static inline int
scale_object_is_equal (
  ScaleObject * a,
  ScaleObject * b)
{
  return
    !position_compare(&a->pos, &b->pos) &&
    musical_scale_is_equal (a->scale, b->scale);
}

/**
 * Returns the Track this ScaleObject is in.
 */
Track *
scale_object_get_track (
  ScaleObject * self);

/**
 * Sets the cache Position.
 */
void
scale_object_set_cache_pos (
  ScaleObject * scale,
  const Position * pos);

/**
 * Sets the Track of the scale.
 */
void
scale_object_set_track (
  ScaleObject * self,
  Track *  track);

/**
 * Generates a widget for the scale.
 */
void
scale_object_gen_widget (
  ScaleObject * self);

/**
 * Finds the ScaleObject in the project
 * corresponding to the given one.
 */
ScaleObject *
scale_object_find (
  ScaleObject * clone);

/**
 * Clones the given scale.
 */
ScaleObject *
scale_object_clone (
  ScaleObject * src,
  ScaleObjectCloneFlag flag);

/**
 * Sets the ScaleObject position.
 *
 * @param trans_only Only do transients.
 */
void
scale_object_set_pos (
  ScaleObject *   self,
  const Position * pos,
  int        trans_only);

DECLARE_IS_ARRANGER_OBJ_SELECTED (
  ScaleObject, scale_object);

/**
 * Shifts the ScaleObject by given number of ticks
 * on x.
 */
void
scale_object_shift (
  ScaleObject * self,
  long ticks);

/**
 * Moves the ScaleObject by the given amount of
 * ticks.
 *
 * @param use_cached_pos Add the ticks to the cached
 *   Position instead of its current Position.
 * @param trans_only Only move transients.
 * @return Whether moved or not.
 */
int
scale_object_move (
  ScaleObject * scale,
  long     ticks,
  int      use_cached_pos,
  int      trans_only);

/**
 * Frees the ScaleObject.
 */
void
scale_object_free (
  ScaleObject * self);

/**
 * @}
 */

#endif
