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

#include <math.h>
#include <stdlib.h>

#include "audio/chord_track.h"
#include "audio/position.h"
#include "audio/scale_object.h"
#include "gui/backend/arranger_object.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/objects.h"

/**
 * Creates a ScaleObject.
 */
ScaleObject *
scale_object_new (MusicalScale * descr)
{
  ScaleObject * self = object_new (ScaleObject);

  self->magic = SCALE_OBJECT_MAGIC;
  self->schema_version = SCALE_OBJECT_SCHEMA_VERSION;

  ArrangerObject * obj = (ArrangerObject *) self;
  obj->type = ARRANGER_OBJECT_TYPE_SCALE_OBJECT;

  self->scale = descr;

  arranger_object_init (obj);

  return self;
}

void
scale_object_set_index (ScaleObject * self, int index)
{
  self->index = index;
}

int
scale_object_is_equal (
  ScaleObject * a,
  ScaleObject * b)
{
  ArrangerObject * obj_a = (ArrangerObject *) a;
  ArrangerObject * obj_b = (ArrangerObject *) b;
  return position_is_equal_ticks (
           &obj_a->pos, &obj_b->pos)
         && a->index == b->index
         && musical_scale_is_equal (
           a->scale, b->scale);
}
