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
 * Piano roll backend.
 *
 * This is meant to be serialized along with each
 * project.
 */

#include <stdlib.h>

#include "audio/channel.h"
#include "gui/backend/clip_editor.h"
#include "audio/track.h"
#include "project.h"
#include "utils/flags.h"

/**
 * Inits the ClipEditor after a Project is loaded.
 */
void
clip_editor_init_loaded (
  ClipEditor * self)
{
  self->region =
    region_find_by_name (self->region_name);
  piano_roll_init_loaded (&self->piano_roll);
}

/**
 * Sets the region and refreshes the widgets.
 *
 * To be called only from GTK threads.
 */
void
clip_editor_set_region (
  ClipEditor * self,
  Region *     region)
{
  int recalc_graph = 0;
  if (self->region && self->region->type ==
        REGION_TYPE_MIDI)
    {
      channel_reattach_midi_editor_manual_press_port (
        track_get_channel (
          region_get_track (self->region)),
        F_DISCONNECT, F_NO_RECALC_GRAPH);
      recalc_graph = 1;
    }
  if (region->type == REGION_TYPE_MIDI)
    {
      channel_reattach_midi_editor_manual_press_port (
        track_get_channel (
          region_get_track (region)),
        F_CONNECT, F_NO_RECALC_GRAPH);
      recalc_graph = 1;
    }

  if (recalc_graph)
    mixer_recalc_graph (MIXER);

  /* if first time showing a region, show the
   * event viewer as necessary */
  if (!self->region && region)
    {
      EVENTS_PUSH (
        ET_CLIP_EDITOR_FIRST_TIME_REGION_SELECTED,
        NULL);
    }

  self->region = region;

  if (self->region_name)
    {
      g_free (self->region_name);
      self->region_name = NULL;
    }
  self->region_name = g_strdup (region->name);

  self->region_changed = 1;

  EVENTS_PUSH (ET_CLIP_EDITOR_REGION_CHANGED,
               NULL);
}

/**
 * Returns the Region that widgets are expected
 * to use.
 */
Region *
clip_editor_get_region_for_widgets (
  ClipEditor * self)
{
  return self->region_cache;
}

void
clip_editor_init (ClipEditor * self)
{
  self->region_name = NULL;
  piano_roll_init (&self->piano_roll);
  audio_clip_editor_init (&self->audio_clip_editor);
  chord_editor_init (&self->chord_editor);
  automation_editor_init (&self->automation_editor);
}

