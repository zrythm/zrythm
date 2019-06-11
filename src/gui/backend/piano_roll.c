/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Piano roll backend.
 *
 * This is meant to be serialized along with each project.
 */

#include <stdlib.h>

#include "audio/channel.h"
#include "gui/backend/piano_roll.h"
#include "audio/track.h"
#include "gui/widgets/piano_roll.h"
#include "project.h"

DRUM_LABELS;

/**
 * Returns the MidiNoteDescriptor matching the value
 * (0-127).
 */
const MidiNoteDescriptor *
piano_roll_find_midi_note_descriptor_by_val (
  PianoRoll * self,
  int         val)
{
  MidiNoteDescriptor * descr;
  for (int i = 0; i < 128; i++)
    {
      if (self->drum_mode)
        descr = &self->drum_descriptors[i];
      else
        descr = &self->piano_descriptors[i];

      if (descr->value == val)
        return descr;
    }
  g_return_val_if_reached (NULL);
}

void
midi_note_descriptor_set_custom_name (
  MidiNoteDescriptor * descr,
  char *               str)
{
  descr->custom_name = g_strdup (str);
}

void
piano_roll_set_highlighting (
  PianoRoll * self,
  PianoRollHighlighting highlighting)
{
  self->highlighting = highlighting;

  EVENTS_PUSH (ET_PIANO_ROLL_HIGHLIGHTING_CHANGED,
               NULL);
}

void
piano_roll_init (PianoRoll * self)
{
  self->notes_zoom = 3;

  self->midi_modifier = MIDI_MODIFIER_VELOCITY;

  self->highlighting = PR_HIGHLIGHT_BOTH;

  MidiNoteDescriptor * descr;
  int idx = 0;
  for (int i = 127; i >= 0; i--)
    {
      /* do piano */
      descr = &self->piano_descriptors[idx];

      descr->index = idx;
      descr->value = i;
      descr->marked = 0;
      descr->visible = 1;
      descr->custom_name = "";

      descr->note_name =
        g_strdup_printf (
          "%s%d",
          chord_descriptor_note_to_string (i % 12),
          i / 12 - 2);
      descr->note_name_pango =
        g_strdup_printf (
          "%s<sup>%d</sup>",
          chord_descriptor_note_to_string (i % 12),
          i / 12 - 2);
      idx++;
    }

  /* do drum - put 35 to 81 first */
  idx = 0;
  for (int i = 35; i <= 81; i++)
    {
      descr = &self->drum_descriptors[idx];

      descr->index = idx;
      descr->value = i;
      descr->marked = 0;
      descr->visible = 1;
      descr->custom_name =
        g_strdup (drum_labels[idx]);

      descr->note_name =
        g_strdup_printf (
          "%s%d",
          chord_descriptor_note_to_string (i % 12),
          i / 12 - 2);
      descr->note_name_pango =
        g_strdup_printf (
          "%s<sup>%d</sup>",
          chord_descriptor_note_to_string (i % 12),
          i / 12 - 2);
      idx++;
    }
  for (int i = 0; i < 128; i++)
    {
      if (i >= 35 && i <= 81)
        continue;

      descr = &self->drum_descriptors[idx];

      descr->index = idx;
      descr->value = i;
      descr->marked = 0;
      descr->visible = 1;
      descr->custom_name =
        g_strdup_printf (
          "%s%d",
          chord_descriptor_note_to_string (i % 12),
          i / 12 - 2);

      descr->note_name =
        g_strdup_printf (
          "%s%d",
          chord_descriptor_note_to_string (i % 12),
          i / 12 - 2);
      descr->note_name_pango =
        g_strdup_printf (
          "%s<sup>%d</sup>",
          chord_descriptor_note_to_string (i % 12),
          i / 12 - 2);
      idx++;
    }

  g_warn_if_fail (idx == 128);
}
