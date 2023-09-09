// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
 */

#include <stdlib.h>

#include "dsp/foldable_track.h"
#include "dsp/track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "zrythm.h"
#include "zrythm_app.h"

void
foldable_track_init (Track * self)
{
  if (self->type == TRACK_TYPE_FOLDER)
    {
      /* GTK color picker color */
      gdk_rgba_parse (&self->color, "#865E3C");
      self->icon_name = g_strdup ("fluentui-folder-regular");
    }
}

/**
 * Used to check if soloed/muted/etc.
 */
bool
foldable_track_is_status (Track * self, FoldableTrackMixerStatus status)
{
  g_return_val_if_fail (self->tracklist, false);
  bool all_soloed = self->size > 1;
  bool has_channel_tracks = false;
  for (int i = 1; i < self->size; i++)
    {
      int     pos = self->pos + i;
      Track * child = tracklist_get_track (self->tracklist, pos);
      g_return_val_if_fail (IS_TRACK_AND_NONNULL (child), false);

      if (track_type_has_channel (child->type))
        has_channel_tracks = true;
      else
        continue;

      switch (status)
        {
        case FOLDABLE_TRACK_MIXER_STATUS_MUTED:
          if (!track_get_muted (child))
            return false;
          break;
        case FOLDABLE_TRACK_MIXER_STATUS_SOLOED:
          if (!track_get_soloed (child))
            return false;
          break;
        case FOLDABLE_TRACK_MIXER_STATUS_IMPLIED_SOLOED:
          if (!track_get_implied_soloed (child))
            return false;
          break;
        case FOLDABLE_TRACK_MIXER_STATUS_LISTENED:
          if (!track_get_listened (child))
            return false;
          break;
        }
    }
  return has_channel_tracks && all_soloed;
}

/**
 * Returns whether @p child is a folder child of
 * @p self.
 */
bool
foldable_track_is_direct_child (Track * self, Track * child)
{
  GPtrArray * parents = g_ptr_array_new ();
  track_add_folder_parents (child, parents, true);

  bool match = parents->len > 0;
  if (match)
    {
      Track * parent = g_ptr_array_index (parents, 0);
      if (parent != self)
        {
          match = false;
        }
    }
  g_ptr_array_unref (parents);

  return match;
}

/**
 * Returns whether @p child is a folder child of
 * @p self.
 */
bool
foldable_track_is_child (Track * self, Track * child)
{
  GPtrArray * parents = g_ptr_array_new ();
  track_add_folder_parents (child, parents, false);

  bool match = false;
  for (size_t i = 0; i < parents->len; i++)
    {
      Track * parent = g_ptr_array_index (parents, i);
      if (parent == self)
        {
          match = true;
          break;
        }
    }
  g_ptr_array_unref (parents);

  return match;
}

/**
 * Adds to the size recursively.
 *
 * This must only be called from the lowest-level
 * foldable track.
 */
void
foldable_track_add_to_size (Track * self, int delta)
{
  GPtrArray * parents = g_ptr_array_new ();
  track_add_folder_parents (self, parents, false);

  self->size += delta;
  g_debug ("new %s size: %d (added %d)", self->name, self->size, delta);
  for (size_t i = 0; i < parents->len; i++)
    {
      Track * parent = g_ptr_array_index (parents, i);
      parent->size += delta;
      g_debug ("new %s size: %d (added %d)", parent->name, parent->size, delta);
    }
  g_ptr_array_unref (parents);
}
