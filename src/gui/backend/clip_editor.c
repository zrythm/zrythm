/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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
#include "audio/router.h"
#include "audio/track.h"
#include "gui/backend/clip_editor.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/arranger_object.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "zrythm.h"
#include "zrythm_app.h"

/**
 * Inits the ClipEditor after a Project is loaded.
 */
void
clip_editor_init_loaded (
  ClipEditor * self)
{
  g_message ("%s: initing...", __func__);

  piano_roll_init_loaded (&self->piano_roll);

  g_message ("%s: done", __func__);
}

/**
 * Sets the region and refreshes the widgets.
 *
 * To be called only from GTK threads.
 */
void
clip_editor_set_region (
  ClipEditor * self,
  ZRegion *    region,
  bool         fire_events)
{
  int recalc_graph = 0;
  if (self->has_region)
    {
      if (self->region_id.type == REGION_TYPE_MIDI)
        {
          Track * track =
            TRACKLIST->tracks[
              self->region_id.track_pos];
          channel_reattach_midi_editor_manual_press_port (
            track_get_channel (track),
            F_NO_CONNECT, F_NO_RECALC_GRAPH);
          recalc_graph = 1;
        }
    }
  if (region) /* new region exists */
    {
      if (region->id.type == REGION_TYPE_MIDI)
        {
          Track * track =
            arranger_object_get_track (
              (ArrangerObject *) region);
          channel_reattach_midi_editor_manual_press_port (
            track_get_channel (track),
            F_CONNECT, F_NO_RECALC_GRAPH);
          recalc_graph = 1;
        }
    }
  else /* new region is NULL */
    {
    }

  if (recalc_graph)
    router_recalc_graph (ROUTER, F_NOT_SOFT);

  /* if first time showing a region, show the
   * event viewer as necessary */
  if (fire_events && !self->has_region && region)
    {
      EVENTS_PUSH (
        ET_CLIP_EDITOR_FIRST_TIME_REGION_SELECTED,
        NULL);
    }

  if (region)
    {
      self->has_region = 1;
      region_identifier_copy (
        &self->region_id, &region->id);
    }
  else
    {
      self->has_region = 0;
    }

  self->region_changed = 1;

  if (fire_events && ZRYTHM_HAVE_UI &&
      MAIN_WINDOW && MW_CLIP_EDITOR)
    {
      clip_editor_widget_on_region_changed (
        MW_CLIP_EDITOR);
    }
  /*EVENTS_PUSH (*/
    /*ET_CLIP_EDITOR_REGION_CHANGED, NULL);*/
}

/**
 * Causes the selected ZRegion to be redrawin in the
 * UI, if any.
 */
void
clip_editor_redraw_region (
  ClipEditor * self)
{
  if (self->has_region)
    {
      ZRegion * region =
        clip_editor_get_region (self);
      arranger_object_queue_redraw (
        (ArrangerObject *) region);
    }
}

ZRegion *
clip_editor_get_region (
  ClipEditor * self)
{
  if (!self->has_region)
    return NULL;

  ZRegion * region = region_find (&self->region_id);
  return region;
}

Track *
clip_editor_get_track (
  ClipEditor * self)
{
  ZRegion * region = clip_editor_get_region (self);
  g_return_val_if_fail (region, NULL);

  Track * track =
    arranger_object_get_track (
      (ArrangerObject *) region);
  g_return_val_if_fail (track, NULL);

  return track;
}

#if 0
/**
 * Returns the ZRegion that widgets are expected
 * to use.
 */
ZRegion *
clip_editor_get_region_for_widgets (
  ClipEditor * self)
{
  ZRegion * region =
    region_find (&self->region_id_cache);
  return region;
}
#endif

ArrangerSelections *
clip_editor_get_arranger_selections (
  ClipEditor * self)
{
  ZRegion * region =
    clip_editor_get_region (CLIP_EDITOR);
  if (!region)
    {
      return NULL;
    }

  ArrangerSelections * sel =
    region_get_arranger_selections (region);
  if (!sel)
    {
      return NULL;
    }

  return sel;
}

/**
 * Creates a new clip editor.
 */
ClipEditor *
clip_editor_new (void)
{
  ClipEditor * self = object_new (ClipEditor);

  return self;
}

/**
 * Inits the clip editor.
 */
void
clip_editor_init (
  ClipEditor * self)
{
  piano_roll_init (&self->piano_roll);
  audio_clip_editor_init (&self->audio_clip_editor);
  chord_editor_init (&self->chord_editor);
  automation_editor_init (&self->automation_editor);
}

void
clip_editor_free (
  ClipEditor * self)
{
  object_zero_and_free (self);
}
