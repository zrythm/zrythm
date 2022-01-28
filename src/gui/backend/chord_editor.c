/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include <stdlib.h>

#include "audio/chord_descriptor.h"
#include "gui/backend/chord_editor.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "settings/chord_preset.h"
#include "utils/objects.h"
#include "zrythm_app.h"

void
chord_editor_init (
  ChordEditor * self)
{
  self->schema_version =
    CHORD_EDITOR_SCHEMA_VERSION;

  self->num_chords = 0;
  for (int i = 0; i < 12; i++)
    {
      self->chords[i] =
        chord_descriptor_new (
          NOTE_C + i, 1, NOTE_C + i, CHORD_TYPE_MAJ,
          CHORD_ACC_NONE, 0);
      self->num_chords++;
    }

  editor_settings_init (&self->editor_settings);
}

ChordEditor *
chord_editor_clone (
  ChordEditor * src)
{
  ChordEditor * self =
    object_new (ChordEditor);
  self->schema_version =
    CHORD_EDITOR_SCHEMA_VERSION;

  for (int i = 0; i < src->num_chords; i++)
    {
      self->chords[i] =
        chord_descriptor_clone (src->chords[i]);
    }
  self->num_chords = src->num_chords;

  self->editor_settings = src->editor_settings;

  return self;
}

void
chord_editor_apply_preset (
  ChordEditor * self,
  ChordPreset * pset)
{
  for (int i = 0; i < 12; i++)
    {
      ChordDescriptor * descr = self->chords[i];
      chord_descriptor_copy (
        descr, pset->descr[i]);
      chord_descriptor_update_notes (descr);
    }

  EVENTS_PUSH (ET_CHORDS_UPDATED, NULL);
}

void
chord_editor_apply_preset_from_scale (
  ChordEditor *     self,
  MusicalScaleType  scale,
  MusicalNote       root_note)
{
  g_debug (
    "applying preset from scale %s, root note %s",
    musical_scale_type_to_string (scale),
    chord_descriptor_note_to_string (root_note));
  const ChordType * triads =
    musical_scale_get_triad_types (scale, true);
  const bool * notes =
    musical_scale_get_notes (scale, true);
  int cur_chord = 0;
  for (int i = 0; i < 12; i++)
    {
      /* skip notes not in scale */
      if (!notes[i])
        continue;

      ChordDescriptor * descr =
        self->chords[cur_chord];
      descr->has_bass = false;
      descr->root_note =
        (MusicalNote)
        ((int) root_note + i);
      descr->bass_note =
        (MusicalNote)
        ((int) root_note + i);
      descr->type = triads[cur_chord];
      descr->accent = CHORD_ACC_NONE;
      descr->inversion = 0;
      chord_descriptor_update_notes (descr);

      cur_chord++;
    }

  /* fill remaining chords with default data */
  while (cur_chord < 12)
    {
      ChordDescriptor * descr =
        self->chords[cur_chord];
      descr->has_bass = false;
      descr->root_note = NOTE_C;
      descr->bass_note = NOTE_C;
      descr->type = CHORD_TYPE_NONE;
      descr->accent = CHORD_ACC_NONE;
      descr->inversion = 0;
      chord_descriptor_update_notes (descr);
      cur_chord++;
    }

  EVENTS_PUSH (ET_CHORDS_UPDATED, NULL);
}

void
chord_editor_transpose_chords (
  ChordEditor * self,
  bool          up)
{
  for (int i = 0; i < 12; i++)
    {
      ChordDescriptor * descr = self->chords[i];

      int add = (up ? 1 : 11);
      descr->root_note =
        (MusicalNote)
        ((int) descr->root_note + add);
      descr->bass_note =
        (MusicalNote)
        ((int) descr->bass_note + add);

      descr->root_note = descr->root_note % 12;
      descr->bass_note = descr->bass_note % 12;

      chord_descriptor_update_notes (descr);
    }

  EVENTS_PUSH (ET_CHORDS_UPDATED, NULL);
}

ChordEditor *
chord_editor_new (void)
{
  ChordEditor * self =
    object_new (ChordEditor);
  self->schema_version =
    AUDIO_CLIP_EDITOR_SCHEMA_VERSION;

  return self;
}

void
chord_editor_free (
  ChordEditor * self)
{
  for (int i = 0; i < self->num_chords; i++)
    {
      object_free_w_func_and_null (
        chord_descriptor_free, self->chords[i]);
    }

  object_zero_and_free (self);
}
