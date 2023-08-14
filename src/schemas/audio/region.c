// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/region.h"
#include "utils/objects.h"

#include "schemas/audio/region.h"

ZRegion *
region_upgrade_from_v1 (ZRegion_v1 * old)
{
  if (!old)
    return NULL;

  ZRegion * self = object_new (ZRegion);

  self->schema_version = REGION_SCHEMA_VERSION;
  ArrangerObject * base =
    arranger_object_upgrade_from_v1 (&old->base);
  self->base = *base;
  RegionIdentifier * id =
    region_identifier_upgrade_from_v1 (&old->id);
  self->id = *id;
  self->name = old->name;
  self->pool_id = old->pool_id;
  self->gain = old->gain;
  self->musical_mode = (RegionMusicalMode) old->musical_mode;
  self->midi_notes = g_malloc_n (
    (size_t) old->num_midi_notes, sizeof (AutomationPoint *));
  self->num_midi_notes = old->num_midi_notes;
  for (int i = 0; i < old->num_midi_notes; i++)
    {
      self->midi_notes[i] =
        midi_note_upgrade_from_v1 (old->midi_notes[i]);
    }
  self->aps = g_malloc_n (
    (size_t) old->num_aps, sizeof (AutomationPoint *));
  self->num_aps = old->num_aps;
  for (int i = 0; i < old->num_aps; i++)
    {
      self->aps[i] =
        automation_point_upgrade_from_v1 (old->aps[i]);
    }
  self->chord_objects = g_malloc_n (
    (size_t) old->num_chord_objects,
    sizeof (AutomationPoint *));
  self->num_chord_objects = old->num_chord_objects;
  for (int i = 0; i < old->num_chord_objects; i++)
    {
      self->chord_objects[i] =
        chord_object_upgrade_from_v1 (old->chord_objects[i]);
    }

  return self;
}
