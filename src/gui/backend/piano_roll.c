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

/**
 * \file
 *
 * Piano roll backend.
 *
 * This is meant to be serialized along with each project.
 */

#include <stdlib.h>

#include "audio/channel.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/piano_roll.h"
#include "audio/track.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"

DRUM_LABELS;

/**
 * Inits the descriptors to the default values.
 *
 * FIXME move them to tracks since each track
 * might have different arrangement of drums.
 */
static void
init_descriptors (
  PianoRoll * self)
{
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
      descr->custom_name = g_strdup ("");

      descr->note_name =
        g_strdup_printf (
          "%s%d",
          chord_descriptor_note_to_string (i % 12),
          i / 12 - 1);
      descr->note_name_pango =
        g_strdup_printf (
          "%s<sup>%d</sup>",
          chord_descriptor_note_to_string (i % 12),
          i / 12 - 1);
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
          i / 12 - 1);
      descr->note_name_pango =
        g_strdup_printf (
          "%s<sup>%d</sup>",
          chord_descriptor_note_to_string (i % 12),
          i / 12 - 1);
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
          i / 12 - 1);

      descr->note_name =
        g_strdup_printf (
          "%s%d",
          chord_descriptor_note_to_string (i % 12),
          i / 12 - 1);
      descr->note_name_pango =
        g_strdup_printf (
          "%s<sup>%d</sup>",
          chord_descriptor_note_to_string (i % 12),
          i / 12 - 1);
      idx++;
    }

  g_warn_if_fail (idx == 128);
}

/* 1 = black */
static const int notes[12] = {
    0,
    1,
    0,
    1,
    0,
    0,
    1,
    0,
    1,
    0,
    1,
    0 };

/**
 * Returns if the key is black.
 */
int
piano_roll_is_key_black (
  int        note)
{
  return notes[note % 12] == 1;
}

/**
 * Adds the note if it doesn't exist in the array.
 */
void
piano_roll_add_current_note (
  PianoRoll * self,
  int         note)
{
  if (!array_contains_int (
         self->current_notes,
         self->num_current_notes,
         note))
    {
      array_append (
        self->current_notes,
        self->num_current_notes,
        note);
    }
}

/**
 * Removes the note if it exists in the array.
 */
void
piano_roll_remove_current_note (
  PianoRoll * self,
  int         note)
{
  if (array_contains_int (
      self->current_notes,
      self->num_current_notes,
      note))
    {
      array_delete_primitive (
        self->current_notes,
        self->num_current_notes,
        note);
    }
}

/**
 * Returns 1 if it contains the given note, 0
 * otherwise.
 */
int
piano_roll_contains_current_note (
  PianoRoll * self,
  int         note)
{
  return
    array_contains_int (
      self->current_notes,
      self->num_current_notes, note);
}

/**
 * Inits the PianoRoll after a Project has been
 * loaded.
 */
void
piano_roll_init_loaded (
  PianoRoll * self)
{
  if (!ZRYTHM_TESTING)
    {
      self->highlighting =
        g_settings_get_enum (
          S_UI, "piano-roll-highlight");
    }

  init_descriptors (self);
}


/**
 * Returns the MidiNoteDescriptor matching the value
 * (0-127).
 */
const MidiNoteDescriptor *
piano_roll_find_midi_note_descriptor_by_val (
  PianoRoll *   self,
  const uint8_t val)
{
  g_return_val_if_fail (val < 128, NULL);

  MidiNoteDescriptor * descr;
  for (int i = 0; i < 128; i++)
    {
      if (self->drum_mode)
        descr = &self->drum_descriptors[i];
      else
        descr = &self->piano_descriptors[i];

      if (descr->value == (int) val)
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

  g_settings_set_enum (
    S_UI, "piano-roll-highlight",
    highlighting);

  EVENTS_PUSH (ET_PIANO_ROLL_HIGHLIGHTING_CHANGED,
               NULL);
}

/**
 * Returns the current track whose regions are
 * being shown in the piano roll.
 */
Track *
piano_roll_get_current_track (
  const PianoRoll * self)
{
  /* TODO */
  return NULL;
}

void
piano_roll_set_notes_zoom (
  PianoRoll * self,
  float         notes_zoom,
  int         fire_events)
{
  if (notes_zoom < 1.f || notes_zoom > 4.5f)
    return;

  self->notes_zoom = notes_zoom;

  if (fire_events)
    {
      EVENTS_PUSH (
        ET_PIANO_ROLL_KEY_HEIGHT_CHANGED, NULL);
    }
}

/**
 * Sets the MIDI modifier.
 */
void
piano_roll_set_midi_modifier (
  PianoRoll * self,
  MidiModifier modifier)
{
  self->midi_modifier = modifier;

  g_settings_set_enum (
    S_UI, "piano-roll-midi-modifier",
    modifier);

  EVENTS_PUSH (ET_PIANO_ROLL_MIDI_MODIFIER_CHANGED,
               NULL);
}

void
piano_roll_init (PianoRoll * self)
{
  self->notes_zoom = 3.f;

  self->midi_modifier = MIDI_MODIFIER_VELOCITY;

  if (!ZRYTHM_TESTING)
    {
      self->highlighting =
        g_settings_get_enum (
          S_UI, "piano-roll-highlight");
      self->midi_modifier =
        g_settings_get_enum (
          S_UI, "piano-roll-midi-modifier");
    }

  init_descriptors (self);
}
