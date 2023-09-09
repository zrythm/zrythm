// SPDX-FileCopyrightText: © 2019-2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/marker.h"
#include "dsp/marker_track.h"
#include "gui/widgets/marker.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/string.h"

/**
 * Creates a Marker.
 */
Marker *
marker_new (const char * name)
{
  Marker * self = object_new (Marker);

  self->schema_version = MARKER_SCHEMA_VERSION;

  ArrangerObject * obj = (ArrangerObject *) self;
  obj->type = ARRANGER_OBJECT_TYPE_MARKER;

  self->name = g_strdup (name);
  self->type = MARKER_TYPE_CUSTOM;
  position_init (&obj->pos);

  arranger_object_init (obj);

  arranger_object_gen_escaped_name ((ArrangerObject *) self);

  return self;
}

void
marker_set_index (Marker * self, int index)
{
  self->index = index;
}

/**
 * Sets the Track of the Marker.
 */
void
marker_set_track_name_hash (Marker * marker, unsigned int track_name_hash)
{
  marker->track_name_hash = track_name_hash;
}

Marker *
marker_find_by_name (const char * name)
{
  for (int i = 0; i < P_MARKER_TRACK->num_markers; i++)
    {
      Marker * marker = P_MARKER_TRACK->markers[i];
      if (string_is_equal (name, marker->name))
        {
          return marker;
        }
    }

  return NULL;
}

/**
 * Returns if the two Marker's are equal.
 */
int
marker_is_equal (Marker * a, Marker * b)
{
  ArrangerObject * obj_a = (ArrangerObject *) a;
  ArrangerObject * obj_b = (ArrangerObject *) b;
  return position_is_equal (&obj_a->pos, &obj_b->pos)
         && string_is_equal (a->name, b->name);
}
