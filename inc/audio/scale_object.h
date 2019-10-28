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
 * Scale object inside the chord Track in the
 * TimelineArranger.
 */

#ifndef __AUDIO_SCALE_OBJECT_H__
#define __AUDIO_SCALE_OBJECT_H__

#include <stdint.h>

#include "audio/scale.h"
#include "audio/position.h"
#include "gui/backend/arranger_object.h"
#include "utils/yaml.h"

typedef struct MusicalScale MusicalScale;

/**
 * @addtogroup audio
 *
 * @{
 */

#define scale_object_is_main(c) \
  arranger_object_is_main ( \
    (ArrangerObject *) c)
#define scale_object_is_transient(r) \
  arranger_object_is_transient ( \
    (ArrangerObject *) r)
#define scale_object_get_main(r) \
  ((ScaleObject *) \
   arranger_object_get_main ( \
     (ArrangerObject *) r))
#define scale_object_get_main_trans(r) \
  ((ScaleObject *) \
   arranger_object_get_main_trans ( \
     (ArrangerObject *) r))
#define scale_object_is_selected(r) \
  arranger_object_is_selected ( \
    (ArrangerObject *) r)
#define scale_object_get_visible_counterpart(r) \
  ((ScaleObject *) \
   arranger_object_get_visible_counterpart ( \
     (ArrangerObject *) r))

/**
 * A ScaleObject to be shown in the
 * TimelineArrangerWidget.
 */
typedef struct ScaleObject
{
  /** Base struct. */
  ArrangerObject  base;

  MusicalScale *  scale;

  /** Position of Track this ScaleObject is in. */
  int             track_pos;

  /** Cache. */
  Track *         track;
} ScaleObject;

static const cyaml_schema_field_t
  scale_object_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "base", CYAML_FLAG_DEFAULT,
    ScaleObject, base,
    arranger_object_fields_schema),
  CYAML_FIELD_MAPPING_PTR (
    "scale", CYAML_FLAG_POINTER,
    ScaleObject, scale,
    musical_scale_fields_schema),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
scale_object_schema = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    ScaleObject, scale_object_fields_schema),
};

/**
 * Creates a ScaleObject.
 */
ScaleObject *
scale_object_new (
  MusicalScale * descr,
  int            is_main);

int
scale_object_is_equal (
  ScaleObject * a,
  ScaleObject * b);

/**
 * Sets the Track of the scale.
 */
void
scale_object_set_track (
  ScaleObject * self,
  Track *  track);

/**
 * @}
 */

#endif
