/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
#include "project.h"
#include "utils/objects.h"

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
