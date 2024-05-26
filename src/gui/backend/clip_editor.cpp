// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Piano roll backend.
 *
 * This is meant to be serialized along with each
 * project.
 */

#include <cstdlib>

#include "dsp/channel.h"
#include "dsp/router.h"
#include "dsp/track.h"
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
clip_editor_init_loaded (ClipEditor * self)
{
  g_message ("Initializing clip editor backend...");

  piano_roll_init_loaded (self->piano_roll);

  g_message ("Done initializing clip editor backend");
}

/**
 * Sets the region and refreshes the widgets.
 *
 * To be called only from GTK threads.
 */
void
clip_editor_set_region (ClipEditor * self, ZRegion * region, bool fire_events)
{
  if (region)
    {
      g_return_if_fail (IS_REGION (region));
    }

  g_message (
    "clip editor: setting region to %p (%s)", region,
    region ? region->name : NULL);

  /* if first time showing a region, show the
   * event viewer as necessary */
  if (fire_events && !self->has_region && region)
    {
      EVENTS_PUSH (EventType::ET_CLIP_EDITOR_FIRST_TIME_REGION_SELECTED, NULL);
    }

  /*
   * block until current DSP cycle finishes to
   * avoid potentially sending the events to
   * multiple tracks
   */
  if (gZrythm && ROUTER && engine_get_run (AUDIO_ENGINE))
    {
      g_debug (
        "clip editor region changed, waiting for "
        "graph access to make the change");
      zix_sem_wait (&ROUTER->graph_access);
    }

  if (region)
    {
      self->has_region = 1;
      region_identifier_copy (&self->region_id, &region->id);
      self->track = arranger_object_get_track ((ArrangerObject *) region);

      /* if audio region, also set it in
       * selections */
      region_identifier_copy (&AUDIO_SELECTIONS->region_id, &region->id);
    }
  else
    {
      self->has_region = 0;
      self->track = NULL;
    }

  self->region_changed = 1;

  if (ROUTER && engine_get_run (AUDIO_ENGINE))
    {
      zix_sem_post (&ROUTER->graph_access);
      g_debug ("clip editor region successfully changed");
    }

  if (fire_events && ZRYTHM_HAVE_UI && MAIN_WINDOW && MW_CLIP_EDITOR)
    {
      EVENTS_PUSH (EventType::ET_CLIP_EDITOR_REGION_CHANGED, NULL);

      /* setting the region potentially changes the active
       * arranger - process now to change the active arranger */
      event_manager_process_now (EVENT_MANAGER);
    }
}

ZRegion *
clip_editor_get_region (ClipEditor * self)
{
  if (router_is_processing_thread (ROUTER))
    {
      return self->region;
    }

  if (!self->has_region)
    return NULL;

  ZRegion * region = region_find (&self->region_id);
  return region;
}

Track *
clip_editor_get_track (ClipEditor * self)
{
  if (router_is_processing_thread (ROUTER))
    {
      return self->track;
    }

  if (!self->has_region)
    return NULL;

  ZRegion * region = clip_editor_get_region (self);
  g_return_val_if_fail (region, NULL);

  Track * track = arranger_object_get_track ((ArrangerObject *) region);
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
clip_editor_get_arranger_selections (ClipEditor * self)
{
  ZRegion * region = clip_editor_get_region (CLIP_EDITOR);
  if (!region)
    {
      return NULL;
    }

  ArrangerSelections * sel = region_get_arranger_selections (region);
  if (!sel)
    {
      return NULL;
    }

  return sel;
}

ClipEditor *
clip_editor_clone (const ClipEditor * src)
{
  ClipEditor * self = object_new (ClipEditor);

  region_identifier_copy (&self->region_id, &src->region_id);
  self->has_region = src->has_region;
  self->piano_roll = piano_roll_clone (src->piano_roll);
  self->automation_editor = automation_editor_clone (src->automation_editor);
  self->chord_editor = chord_editor_clone (src->chord_editor);
  self->audio_clip_editor = audio_clip_editor_clone (src->audio_clip_editor);

  return self;
}

/**
 * To be called when recalculating the graph.
 */
void
clip_editor_set_caches (ClipEditor * self)
{
  if (self->has_region)
    {
      self->region = clip_editor_get_region (self);
      self->track = clip_editor_get_track (self);
    }
  else
    {
      self->region = NULL;
      self->track = NULL;
    }
}

/**
 * Creates a new clip editor.
 */
ClipEditor *
clip_editor_new (void)
{
  ClipEditor * self = object_new (ClipEditor);

  self->piano_roll = piano_roll_new ();
  self->audio_clip_editor = audio_clip_editor_new ();
  self->chord_editor = chord_editor_new ();
  self->automation_editor = automation_editor_new ();

  return self;
}

/**
 * Inits the clip editor.
 */
void
clip_editor_init (ClipEditor * self)
{
  piano_roll_init (self->piano_roll);
  audio_clip_editor_init (self->audio_clip_editor);
  chord_editor_init (self->chord_editor);
  automation_editor_init (self->automation_editor);
}

void
clip_editor_free (ClipEditor * self)
{
  object_free_w_func_and_null (piano_roll_free, self->piano_roll);
  object_free_w_func_and_null (audio_clip_editor_free, self->audio_clip_editor);
  object_free_w_func_and_null (chord_editor_free, self->chord_editor);
  object_free_w_func_and_null (automation_editor_free, self->automation_editor);

  object_zero_and_free (self);
}
