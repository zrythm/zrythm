/*
 * Copyright (C) 2020-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/backend/clipboard.h"
#include "utils/flags.h"
#include "utils/objects.h"

/**
 * Creates a new Clipboard instance for the given
 * arranger selections.
 */
Clipboard *
clipboard_new_for_arranger_selections (
  ArrangerSelections * sel,
  bool                 clone)
{
  Clipboard * self = object_new (Clipboard);

  if (clone)
    {
      sel = arranger_selections_clone (sel);
    }

  switch (sel->type)
    {
    case ARRANGER_SELECTIONS_TYPE_AUTOMATION:
      self->automation_sel =
        (AutomationSelections *) sel;
      self->type =
        CLIPBOARD_TYPE_AUTOMATION_SELECTIONS;
      break;
    case ARRANGER_SELECTIONS_TYPE_TIMELINE:
      self->timeline_sel =
        (TimelineSelections *) sel;
      self->type =
        CLIPBOARD_TYPE_TIMELINE_SELECTIONS;
      break;
    case ARRANGER_SELECTIONS_TYPE_MIDI:
      self->ma_sel = (MidiArrangerSelections *) sel;
      self->type = CLIPBOARD_TYPE_MIDI_SELECTIONS;
      break;
    case ARRANGER_SELECTIONS_TYPE_CHORD:
      self->chord_sel = (ChordSelections *) sel;
      self->type = CLIPBOARD_TYPE_CHORD_SELECTIONS;
      break;
    default:
      g_return_val_if_reached (NULL);
    }

  return self;
}

Clipboard *
clipboard_new_for_mixer_selections (
  MixerSelections * sel,
  bool              clone)
{
  Clipboard * self = object_new (Clipboard);

  if (clone)
    {
      sel = mixer_selections_clone (sel, F_PROJECT);
    }

  self->mixer_sel = sel;
  self->type = CLIPBOARD_TYPE_MIXER_SELECTIONS;

  return self;
}

Clipboard *
clipboard_new_for_tracklist_selections (
  TracklistSelections * sel,
  bool                  clone)
{
  Clipboard * self = object_new (Clipboard);

  if (clone)
    {
      GError * err = NULL;
      sel = tracklist_selections_clone (sel, &err);
      if (!sel)
        {
          clipboard_free (self);
          g_warning (
            "failed to clone tracklist selections: "
            "%s",
            err->message);
          g_error_free_and_null (err);
          return NULL;
        }
    }

  self->tracklist_sel = sel;
  self->type = CLIPBOARD_TYPE_TRACKLIST_SELECTIONS;

  return self;
}

/**
 * Gets the ArrangerSelections, if this clipboard
 * contains arranger selections.
 */
ArrangerSelections *
clipboard_get_selections (Clipboard * self)
{
#define RETURN_IF_EXISTS(selections) \
  if (self->selections) \
    { \
      return (ArrangerSelections *) \
        self->selections; \
    }

  RETURN_IF_EXISTS (ma_sel);
  RETURN_IF_EXISTS (timeline_sel);
  RETURN_IF_EXISTS (automation_sel);
  RETURN_IF_EXISTS (chord_sel);

#undef RETURN_IF_EXISTS

  g_return_val_if_reached (NULL);
}

/**
 * Frees the clipboard and all associated data.
 */
void
clipboard_free (Clipboard * self)
{
  switch (self->type)
    {
    case CLIPBOARD_TYPE_TIMELINE_SELECTIONS:
    case CLIPBOARD_TYPE_MIDI_SELECTIONS:
    case CLIPBOARD_TYPE_AUTOMATION_SELECTIONS:
    case CLIPBOARD_TYPE_CHORD_SELECTIONS:
      {
        ArrangerSelections * sel =
          clipboard_get_selections (self);
        g_return_if_fail (sel);
        arranger_selections_free_full (sel);
      }
      break;
    case CLIPBOARD_TYPE_MIXER_SELECTIONS:
      mixer_selections_free (self->mixer_sel);
      break;
    case CLIPBOARD_TYPE_TRACKLIST_SELECTIONS:
      tracklist_selections_free (
        self->tracklist_sel);
      break;
    }

  g_free_and_null (self);
}
