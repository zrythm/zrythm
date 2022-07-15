/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/position.h"
#include "audio/scale.h"
#include "gui/backend/arranger_object.h"
#include "utils/yaml.h"

typedef struct MusicalScale MusicalScale;

/**
 * @addtogroup audio
 *
 * @{
 */

#define SCALE_OBJECT_SCHEMA_VERSION 1

#define scale_object_is_selected(r) \
  arranger_object_is_selected ((ArrangerObject *) r)

#define SCALE_OBJECT_MAGIC 13187994
#define IS_SCALE_OBJECT(tr) \
  (tr && tr->magic == SCALE_OBJECT_MAGIC)

/**
 * A ScaleObject to be shown in the
 * TimelineArrangerWidget.
 */
typedef struct ScaleObject
{
  /** Base struct. */
  ArrangerObject base;

  int schema_version;

  MusicalScale * scale;

  int index;

  int magic;

  /** Cache layout for drawing the name. */
  PangoLayout * layout;
} ScaleObject;

static const cyaml_schema_field_t scale_object_fields_schema[] = {
  YAML_FIELD_MAPPING_EMBEDDED (
    ScaleObject,
    base,
    arranger_object_fields_schema),
  YAML_FIELD_INT (ScaleObject, index),
  YAML_FIELD_MAPPING_PTR (
    ScaleObject,
    scale,
    musical_scale_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t scale_object_schema = {
  YAML_VALUE_PTR (ScaleObject, scale_object_fields_schema),
};

/**
 * Creates a ScaleObject.
 */
ScaleObject *
scale_object_new (MusicalScale * descr);

void
scale_object_set_index (ScaleObject * self, int index);

int
scale_object_is_equal (ScaleObject * a, ScaleObject * b);

/**
 * @}
 */

#endif
