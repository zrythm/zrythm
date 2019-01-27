/*
 * audio/automation_tracklist.c - Tracklist backend
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include <stdlib.h>

#include "audio/automatable.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/channel.h"
#include "plugins/plugin.h"
#include "audio/track.h"
#include "utils/arrays.h"

static void
add_automation_track (AutomationTracklist * self,
                      AutomationTrack *     at)
{
  array_append ((void **) self->automation_tracks,
                &self->num_automation_tracks,
                (void *) at);
  at->visible = 0;
}

static void
delete_automation_track (AutomationTracklist * self,
                         AutomationTrack *     at)
{
  array_delete ((void **) self->automation_tracks,
                 &self->num_automation_tracks,
                 at);
  automation_track_free (at);
}

void
automation_tracklist_init (AutomationTracklist * self,
                           Track *               track)
{
  self->track = track;

  /* add all automation tracks */
  automation_tracklist_refresh (self);

  /* set fader visible */
  Automatable * fader = track_get_fader_automatable (self->track);
  AutomationTrack * fader_at =
    automatable_get_automation_track (fader);
  fader_at->visible = 1;
}

void
automation_tracklist_refresh (AutomationTracklist * self)
{
  Channel * channel = track_get_channel (self->track);

  /* remove unneeded automation tracks */
  for (int i = 0; i < self->num_automation_tracks; i++)
    {
      AutomationTrack * at = self->automation_tracks[i];
      int match = 0;
      for (int j = 0; j < channel->num_automatables; j++)
        {
          Automatable * _a = channel->automatables[j];
          AutomationTrack * _at =
            automatable_get_automation_track (_a);
          if (_at == at)
            {
              match = 1;
              break;
            }
        }

      if (match)
        continue;

      for (int j = 0; j < STRIP_SIZE; j++)
        {
          Channel * channel =
            track_get_channel (self->track);
          Plugin * plugin = channel->plugins[j];
          if (plugin)
            {
              for (int k = 0; k < plugin->num_automatables; k++)
                {
                  Automatable * _a = plugin->automatables[k];
                  AutomationTrack * _at =
                    automatable_get_automation_track (_a);
                  if (_at == at)
                    {
                      match = 1;
                      break;
                    }
                }
            }
          if (match)
            break;
        }

      if (match)
        {
          continue;
        }
      else /* this automation track doesn't belong anymore.
              delete it */
        {
          delete_automation_track (self,
                                   at);
          i--;
        }
    }

  /* create and add automation tracks for channel
   * automatables that don't have automation tracks */
  for (int i = 0; i < channel->num_automatables; i++)
    {
      Automatable * a = channel->automatables[i];
      AutomationTrack * at =
        automatable_get_automation_track (a);
      if (!at)
        {
          at = automation_track_new (a);
          add_automation_track (self, at);
        }
    }

  /* same for plugin automatables */
  for (int j = 0; j < STRIP_SIZE; j++)
    {
      Channel * channel =
        track_get_channel (self->track);
      Plugin * plugin = channel->plugins[j];
      if (plugin)
        {
          for (int i = 0; i < plugin->num_automatables; i++)
            {
              Automatable * a = plugin->automatables[i];
              AutomationTrack * at =
                automatable_get_automation_track (a);
              if (!at)
                {
                  at = automation_track_new (a);
                  add_automation_track (self, at);
                }
            }
        }
    }
}

void
automation_tracklist_get_visible_tracks (
  AutomationTracklist * self,
  AutomationTrack **    visible_tracks,
  int *                 num_visible)
{
  *num_visible = 0;
  for (int i = 0; self->num_automation_tracks; i++)
    {
      AutomationTrack * at = self->automation_tracks[i];
      if (at->visible)
        {
          array_append ((void **) visible_tracks,
                        num_visible,
                        (void *) at);
        }
    }
}

/**
 * Used when the add button is added and a new automation
 * track is requested to be shown.
 *
 * Marks the first invisible automation track as visible
 * and returns it.
 */
AutomationTrack *
automation_tracklist_fetch_first_invisible_at (
  AutomationTracklist * self)
{
  for (int i = 0; self->num_automation_tracks; i++)
    {
      AutomationTrack * at = self->automation_tracks[i];
      if (!at->visible)
        {
          at->visible = 1;
          return at;
        }
    }

  /* all visible */
  return NULL;
}
