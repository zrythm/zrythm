// SPDX-FileCopyrightText: © 2018-2019, 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <math.h>
#include <stdlib.h>

#include "dsp/chord_track.h"
#include "dsp/position.h"
#include "dsp/scale_object.h"
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
scale_object_is_equal (ScaleObject * a, ScaleObject * b)
{
  ArrangerObject * obj_a = (ArrangerObject *) a;
  ArrangerObject * obj_b = (ArrangerObject *) b;
  return position_is_equal_ticks (&obj_a->pos, &obj_b->pos)
         && a->index == b->index
         && musical_scale_is_equal (a->scale, b->scale);
}
