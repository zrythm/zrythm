/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * Chord clip editor backend.
 *
 * This is meant to be serialized along with each
 * project.
 */

#include <stdlib.h>

#include "audio/chord_descriptor.h"
#include "gui/backend/chord_editor.h"
#include "project.h"

void
chord_editor_init (
  ChordEditor * self)
{
  self->chords[0] =
    chord_descriptor_new (
      NOTE_A, 1, NOTE_A, CHORD_TYPE_MIN,
      CHORD_ACC_7, 0);
  self->chords[1] =
    chord_descriptor_new (
      NOTE_B, 1, NOTE_B, CHORD_TYPE_MIN,
      CHORD_ACC_7, 0);
  self->chords[2] =
    chord_descriptor_new (
      NOTE_B, 1, NOTE_B, CHORD_TYPE_MIN,
      CHORD_ACC_7, 0);
  self->chords[3] =
    chord_descriptor_new (
      NOTE_B, 1, NOTE_B, CHORD_TYPE_MIN,
      CHORD_ACC_7, 0);
  self->num_chords = 4;
}

