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
#include "utils/flags.h"
#include "utils/objects.h"

void
automation_tracklist_init_loaded (
  AutomationTracklist * self)
{
  /* TODO */
  /*for (int i = 0; i < self->num_automation_tracks;*/
       /*i++)*/
    /*self->automation_tracks[i] =*/
      /*project_get_automation_track (*/
        /*self->at_ids[i]);*/

  /*for (int i = 0; i < self->num_automation_lanes;*/
       /*i++)*/
    /*self->automation_lanes[i] =*/
      /*project_get_automation_lane (*/
        /*self->al_ids[i]);*/

  /*self->track =*/
    /*project_get_track (self->track_id);*/
  /*g_message ("inited %s",*/
             /*self->track->name);*/
}

void
automation_tracklist_add_at (
  AutomationTracklist * self,
  AutomationTrack *     at)
{
  array_append (
    self->ats,
    self->num_ats,
    at);

  at->track = self->track;
}

void
automation_tracklist_add_al (
  AutomationTracklist * self,
  AutomationLane *      al)
{
  array_append (
    self->als,
    self->num_als,
    al);
}

void
automation_tracklist_delete_at (
  AutomationTracklist * self,
  AutomationTrack *     at,
  int                   free)
{
  array_delete (
    self->ats,
    self->num_ats,
    at);

  if (free)
    free_later (at, automation_track_free);
}

void
automation_tracklist_delete_al (
  AutomationTracklist * self,
  AutomationLane *      al,
  int                   free)
{
  array_delete (
    self->als,
    self->num_als,
    al);

  if (free)
    free_later (al, automation_lane_free);
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

  /* add all automation tracks */
  automation_tracklist_update (self);

  /* create a visible lane for the fader */
  Automatable * fader =
    track_get_fader_automatable (self->track);
  AutomationTrack * fader_at =
    automatable_get_automation_track (fader);
  AutomationLane * fader_al =
    automation_lane_new (fader_at);
  automation_tracklist_add_al (
    self, fader_al);
}

void
automation_tracklist_update (
  AutomationTracklist * self)
{
  Channel * channel =
    track_get_channel (self->track);

  /* remove unneeded automation tracks */
  AutomationTrack * at, * _at;
  Automatable * _a;
  int match, i, j;
  for (i = 0; i < self->num_ats; i++)
    {
      at = self->ats[i];
      match = 0;
      for (j = 0;  j < channel->num_automatables; j++)
        {
          _a = channel->automatables[j];
          _at =
            automatable_get_automation_track (_a);
          if (_at == at)
            {
              match = 1;
              break;
            }
        }

      if (match)
        continue;

      for (j = 0; j < STRIP_SIZE; j++)
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
          automation_tracklist_delete_at (
            self, at, F_FREE);
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
          automation_tracklist_add_at (
            self, at);
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
                  automation_tracklist_add_at (
                    self, at);
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
  AutomationTrack * src_at, * dest_at;
  AutomationPoint * src_ap, * dest_ap;
  AutomationCurve * src_ac, * dest_ac;
  int i, j;
  for (i = 0; i < src->num_ats; i++)
    {
      src_at = src->ats[i];
      dest_at = dest->ats[i];

      /* add automation points */
      for (j = 0; j < src_at->num_aps; j++)
        {
          src_ap = src_at->aps[j];
          dest_ap =
            automation_point_new_float (
              dest_at,
              src_ap->fvalue,
              &src_ap->pos);
          automation_track_add_ap (
            dest_at, dest_ap, 0);
        }

      /* add automation curves */
      for (j = 0; j < src_at->num_acs; j++)
        {
          src_ac = src_at->acs[j];
          dest_ac =
            automation_curve_new (
              dest_at, &src_ac->pos);
          automation_track_add_ac (
            dest_at, dest_ac);
        }
    }

  /* TODO create same automation lanes */
}

void
automation_tracklist_get_visible_tracks (
  AutomationTracklist * self,
  AutomationTrack **    visible_tracks,
  int *                 num_visible)
{
  *num_visible = 0;
  AutomationLane * al;
  for (int i = 0;
       i < self->num_als; i++)
    {
      al = self->als[i];
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
  AutomationTrack * at;
  for (int i = 0; i < self->num_ats; i++)
    {
      at = self->ats[i];
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
  AutomationTrack * at;
  int i;
  for (i = 0; i < self->num_ats; i++)
    {
      at = self->ats[i];
      if (at->al && !at->al->visible)
        {
          return at;
        }
    }

  for (i = 0; i < self->num_ats; i++)
    {
      at = self->ats[i];
      if (!at->al)
        {
          return at;
        }
    }

  /* all visible */
  return NULL;
}

/*static void*/
/*remove_automation_track (*/
  /*AutomationTracklist * self,*/
  /*AutomationTrack *     at)*/
/*{*/
  /*array_double_delete (*/
    /*self->automation_tracks,*/
    /*self->at_ids,*/
    /*self->num_automation_tracks,*/
    /*at, at->id);*/
  /*project_remove_automation_track (at);*/
  /*automation_track_free (at);*/
/*}*/

/*static void*/
/*remove_automation_lane (*/
  /*AutomationTracklist * self,*/
  /*AutomationLane *     al)*/
/*{*/
  /*array_double_delete (*/
    /*self->automation_lanes,*/
    /*self->al_ids,*/
    /*self->num_automation_lanes,*/
    /*al, al->id);*/
  /*project_remove_automation_lane (al);*/
  /*automation_lane_free (al);*/
/*}*/

void
automation_tracklist_free_members (
  AutomationTracklist * self)
{
  int i, size;
  size = self->num_ats;
  self->num_ats = 0;
  AutomationTrack * at;
  for (i = 0; i < size; i++)
    {
      at = self->ats[i];
      /*g_message ("removing %d %s",*/
                 /*at->id,*/
                 /*at->automatable->label);*/
      /*g_message ("actual automation track index %d",*/
        /*PROJECT->automation_tracks[at->id]);*/
      automation_track_free (
        at);
    }

  size = self->num_als;
  self->num_als = 0;
  for (i = 0; i < size; i++)
    {
      automation_lane_free (
        self->als[i]);
    }
}
