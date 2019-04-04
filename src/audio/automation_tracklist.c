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

#include <stdlib.h>

#include "audio/automatable.h"
#include "audio/automation_lane.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/channel.h"
#include "audio/track.h"
#include "gui/widgets/automation_lane.h"
#include "gui/widgets/automation_tracklist.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/arrays.h"

void
automation_tracklist_init_loaded (
  AutomationTracklist * self)
{
  for (int i = 0; i < self->num_automation_tracks;
       i++)
    self->automation_tracks[i] =
      project_get_automation_track (
        self->at_ids[i]);

  for (int i = 0; i < self->num_automation_lanes;
       i++)
    self->automation_lanes[i] =
      project_get_automation_lane (
        self->al_ids[i]);

  self->track =
    project_get_track (self->track_id);
  g_message ("inited %s",
             self->track->name);
}

static void
add_automation_track (AutomationTracklist * self,
                      AutomationTrack *     at)
{
  array_append (self->automation_tracks,
                self->num_automation_tracks,
                at);
  int size = self->num_automation_tracks - 1;
  array_append (self->at_ids,
                size,
                at->id);
}

void
automation_tracklist_add_automation_lane (
  AutomationTracklist * self,
  AutomationLane *      al)
{
  array_append (self->automation_lanes,
                self->num_automation_lanes,
                al);
  int size = self->num_automation_lanes - 1;
  array_append (self->al_ids,
                size,
                al->id);
}

static void
delete_automation_track (AutomationTracklist * self,
                         AutomationTrack *     at)
{
  array_delete (self->automation_tracks,
                self->num_automation_tracks,
                at);
  automation_track_free (at);
}

/**
 * Adds all automation tracks and sets fader as
 * visible.
 */
void
automation_tracklist_init (
  AutomationTracklist * self,
  Track *               track)
{
  self->track = track;
  self->track_id = track->id;

  /* add all automation tracks */
  automation_tracklist_update (self);

  /* create a visible lane for the fader */
  Automatable * fader =
    track_get_fader_automatable (self->track);
  AutomationTrack * fader_at =
    automatable_get_automation_track (fader);
  AutomationLane * fader_al =
    automation_lane_new (fader_at);
  automation_tracklist_add_automation_lane (
    self, fader_al);
}

void
automation_tracklist_update (
  AutomationTracklist * self)
{
  Channel * channel =
    track_get_channel (self->track);

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

/**
 * Clones the automation tracklist elements from
 * src to dest.
 */
void
automation_tracklist_clone (
  AutomationTracklist * src,
  AutomationTracklist * dest)
{
  /* TODO */
  g_warn_if_reached ();
}

void
automation_tracklist_get_visible_tracks (
  AutomationTracklist * self,
  AutomationTrack **    visible_tracks,
  int *                 num_visible)
{
  *num_visible = 0;
  for (int i = 0;
       i < self->num_automation_lanes; i++)
    {
      AutomationLane * al =
        self->automation_lanes[i];
      if (al->visible)
        {
          array_append (visible_tracks,
                        *num_visible,
                        al->at);
        }
    }
}

AutomationTrack *
automation_tracklist_get_at_from_automatable (
  AutomationTracklist * self,
  Automatable *         a)
{
  for (int i = 0; i < self->num_automation_tracks;
       i++)
    {
      AutomationTrack * at =
        self->automation_tracks[i];
      if (at->automatable == a)
        {
          return at;
        }
    }
  g_warn_if_reached ();
  return NULL;
}

/**
 * Used when the add button is added and a new automation
 * track is requested to be shown.
 *
 * Marks the first invisible automation track as visible
 * and returns it.
 */
AutomationTrack *
automation_tracklist_get_first_invisible_at (
  AutomationTracklist * self)
{
  /* prioritize automation tracks with existing
   * lanes */
  for (int i = 0;
       i < self->num_automation_tracks; i++)
    {
      AutomationTrack * at =
        self->automation_tracks[i];
      if (at->al && !at->al->visible)
        {
          return at;
        }
    }

  for (int i = 0;
       i < self->num_automation_tracks; i++)
    {
      AutomationTrack * at = self->automation_tracks[i];
      if (!at->al)
        {
          return at;
        }
    }

  /* all visible */
  return NULL;
}

static void
remove_automation_track (
  AutomationTracklist * self,
  AutomationTrack *     at)
{
  array_delete (self->automation_tracks,
                self->num_automation_tracks,
                at);
  int size = self->num_automation_tracks + 1;
  array_delete (self->automation_tracks,
                size,
                at->id);
  project_remove_automation_track (at);
  automation_track_free (at);
}

static void
remove_automation_lane (
  AutomationTracklist * self,
  AutomationLane *     al)
{
  array_delete (self->automation_lanes,
                self->num_automation_lanes,
                al);
  int size = self->num_automation_lanes + 1;
  array_delete (self->automation_lanes,
                size,
                al->id);
  project_remove_automation_lane (al);
  automation_lane_free (al);
}

void
automation_tracklist_free_members (
  AutomationTracklist * self)
{
  int i, size;
  size = self->num_automation_tracks;
  self->num_automation_tracks = 0;
  AutomationTrack * at;
  for (i = 0; i < size; i++)
    {
      at = self->automation_tracks[i];
      /*g_message ("removing %d %s",*/
                 /*at->id,*/
                 /*at->automatable->label);*/
      /*g_message ("actual automation track index %d",*/
        /*PROJECT->automation_tracks[at->id]);*/
      project_remove_automation_track (
        at);
      automation_track_free (
        at);
    }

  size = self->num_automation_lanes;
  self->num_automation_lanes = 0;
  for (i = 0; i < size; i++)
    {
      project_remove_automation_lane (
        self->automation_lanes[i]);
      automation_lane_free (
        self->automation_lanes[i]);
    }
}
