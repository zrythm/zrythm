// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/midi_note.h"
#include "utils/objects.h"

#include "schemas/audio/midi_note.h"

MidiNote *
midi_note_upgrade_from_v1 (MidiNote_v1 * old)
{
  if (!old)
    return NULL;

  MidiNote * self = object_new (MidiNote);

  self->schema_version = MIDI_NOTE_SCHEMA_VERSION;

#define UPDATE(name) self->name = old->name

  ArrangerObject * base =
    arranger_object_upgrade_from_v1 (&old->base);
  self->base = *base;

  self->vel = velocity_upgrade_from_v1 (old->vel);
  UPDATE (val);
  UPDATE (muted);
  UPDATE (pos);

  return self;
}
