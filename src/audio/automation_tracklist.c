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

#include <stdlib.h>

#include "audio/automatable.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/channel.h"
#include "audio/track.h"
#include "gui/widgets/automation_tracklist.h"
#include "gui/widgets/automation_track.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/objects.h"

/**
 * Inits a loaded AutomationTracklist.
 */
void
automation_tracklist_init_loaded (
  AutomationTracklist * self)
{
  self->ats_size = (size_t) self->num_ats;
  int j;
  AutomationTrack * at;
  for (j = 0; j < self->num_ats; j++)
    {
      at = self->ats[j];
      at->track = self->track;
      automation_track_init_loaded (at);
    }
}

void
automation_tracklist_add_at (
  AutomationTracklist * self,
  AutomationTrack *     at)
{
  array_double_size_if_full (
    self->ats,
    self->num_ats,
    self->ats_size,
    AutomationTrack *);
  array_append (
    self->ats,
    self->num_ats,
    at);

  at->track = self->track;
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

  g_message ("initing automation tracklist...");

  self->ats_size = 12;
  self->ats =
    calloc (self->ats_size,
            sizeof (AutomationTrack *));

  /* add all automation tracks */
  //automation_tracklist_update (self);

  /* create a visible lane for the fader */
  /*Automatable * fader =*/
    /*track_get_fader_automatable (self->track);*/
  /*AutomationTrack * fader_at =*/
    /*automatable_get_automation_track (fader);*/
  /*fader_at->created = 1;*/
  /*fader_at->visible = 1;*/
}

/*void*/
/*automation_tracklist_update (*/
  /*AutomationTracklist * self)*/
/*{*/
  /*Channel * channel =*/
    /*track_get_channel (self->track);*/

  /*[> generate channel automatables if necessary <]*/
  /*if (!channel_get_automatable (*/
        /*channel,*/
        /*AUTOMATABLE_TYPE_CHANNEL_FADER))*/
    /*{*/
      /*AutomationTrack * at =*/
        /*automation_track_new (*/
          /*automatable_create_fader (channel));*/
      /*automation_tracklist_add_at (*/
        /*atl, at);*/
    /*}*/
  /*if (!channel_get_automatable (*/
        /*channel,*/
        /*AUTOMATABLE_TYPE_CHANNEL_PAN))*/
    /*{*/
      /*AutomationTrack * at =*/
        /*automation_track_new (*/
          /*automatable_create_pan (channel));*/
      /*automation_tracklist_add_at (*/
        /*atl, at);*/
    /*}*/
  /*if (!channel_get_automatable (*/
        /*channel,*/
        /*AUTOMATABLE_TYPE_CHANNEL_MUTE))*/
    /*{*/
      /*AutomationTrack * at =*/
        /*automation_track_new (*/
          /*automatable_create_mute (channel));*/
      /*automation_tracklist_add_at (*/
        /*atl, at);*/
    /*}*/

  /*[> remove unneeded automation tracks <]*/
  /*AutomationTrack * at, * _at;*/
  /*Automatable * _a;*/
  /*int match, i, j;*/
  /*for (i = 0; i < self->num_ats; i++)*/
    /*{*/
      /*at = self->ats[i];*/
      /*match = 0;*/
      /*for (j = 0; j < channel->num_automatables; j++)*/
        /*{*/
          /*_a = channel->automatables[j];*/
          /*_at =*/
            /*automatable_get_automation_track (_a);*/
          /*if (_at == at)*/
            /*{*/
              /*match = 1;*/
              /*break;*/
            /*}*/
        /*}*/

      /*if (match)*/
        /*continue;*/

      /*for (j = 0; j < STRIP_SIZE; j++)*/
        /*{*/
          /*Channel * channel =*/
            /*track_get_channel (self->track);*/
          /*Plugin * plugin = channel->plugins[j];*/
          /*if (plugin)*/
            /*{*/
              /*for (int k = 0; k < plugin->num_automatables; k++)*/
                /*{*/
                  /*Automatable * _a = plugin->automatables[k];*/
                  /*AutomationTrack * _at =*/
                    /*automatable_get_automation_track (_a);*/
                  /*if (_at == at)*/
                    /*{*/
                      /*match = 1;*/
                      /*break;*/
                    /*}*/
                /*}*/
            /*}*/
          /*if (match)*/
            /*break;*/
        /*}*/

      /*if (match)*/
        /*{*/
          /*continue;*/
        /*}*/
      /*else  this automation track doesn't belong anymore.*/
              /*delete it */
        /*{*/
          /*automation_tracklist_delete_at (*/
            /*self, at, F_FREE);*/
          /*i--;*/
        /*}*/
    /*}*/

  /* create and add automation tracks for channel
   * automatables that don't have automation tracks */
  /*for (int i = 0; i < channel->num_automatables; i++)*/
    /*{*/
      /*Automatable * a = channel->automatables[i];*/
      /*AutomationTrack * at =*/
        /*automatable_get_automation_track (a);*/
      /*g_message ("at %p", at);*/
      /*if (!at)*/
        /*{*/
          /*at = automation_track_new (a);*/
          /*automation_tracklist_add_at (*/
            /*self, at);*/
        /*}*/
    /*}*/

  /*[> same for plugin automatables <]*/
  /*for (int j = 0; j < STRIP_SIZE; j++)*/
    /*{*/
      /*Channel * channel =*/
        /*track_get_channel (self->track);*/
      /*Plugin * plugin = channel->plugins[j];*/
      /*if (plugin)*/
        /*{*/
          /*for (int i = 0; i < plugin->num_automatables; i++)*/
            /*{*/
              /*Automatable * a = plugin->automatables[i];*/
              /*AutomationTrack * at =*/
                /*automatable_get_automation_track (a);*/
              /*if (!at)*/
                /*{*/
                  /*at = automation_track_new (a);*/
                  /*automation_tracklist_add_at (*/
                    /*self, at);*/
                /*}*/
            /*}*/
        /*}*/
    /*}*/
/*}*/

/**
 * Updates the Track position of the Automatable's
 * and AutomationTrack's.
 *
 * @param track The Track to update to.
 */
void
automation_tracklist_update_track_pos (
  AutomationTracklist * self,
  Track *               track)
{
  int i;
  for (i = 0; i < self->num_ats; i++)
    {
      self->ats[i]->track = track;
      self->ats[i]->automatable->track = track;
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
  AutomationTrack * src_at;
  dest->ats_size = (size_t) src->num_ats;
  dest->num_ats = src->num_ats;
  dest->ats =
    malloc (dest->ats_size *
            sizeof (AutomationTrack *));
  int i;
  for (i = 0; i < src->num_ats; i++)
    {
      src_at = src->ats[i];
      dest->ats[i] =
        automation_track_clone (
          src_at);
    }

  /* TODO create same automation lanes */
}

/**
 * Updates the frames of each position in each child
 * of the automation tracklist recursively.
 */
void
automation_tracklist_update_frames (
  AutomationTracklist * self)
{
  for (int i = 0; i < self->num_ats; i++)
    {
      automation_track_update_frames (
        self->ats[i]);
    }
}

void
automation_tracklist_get_visible_tracks (
  AutomationTracklist * self,
  AutomationTrack **    visible_tracks,
  int *                 num_visible)
{
  *num_visible = 0;
  AutomationTrack * at;
  for (int i = 0;
       i < self->num_ats; i++)
    {
      at = self->ats[i];
      if (at->created && at->visible)
        {
          array_append (visible_tracks,
                        *num_visible,
                        at);
        }
    }
}

/**
 * Returns the AutomationTrack corresponding to the
 * given Automatable.
 */
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
 * Used when the add button is added and a new
 * automation track is requested to be shown.
 *
 * Marks the first invisible automation track as
 * visible, or marks an uncreated one as created
 * if all invisible ones are visible, and returns
 * it.
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
      if (at->created && !at->visible)
        {
          return at;
        }
    }

  for (i = 0; i < self->num_ats; i++)
    {
      at = self->ats[i];
      if (!at->created)
        {
          return at;
        }
    }

  /* all visible */
  return NULL;
}

/**
 * Removes the AutomationTrack from the
 * AutomationTracklist, optionally freeing it.
 */
void
automation_tracklist_remove_at (
  AutomationTracklist * self,
  AutomationTrack *     at,
  int                   free)
{
  array_delete (
    self->ats, self->num_ats, at);

  if (free)
    free_later (at, automation_track_free);

  EVENTS_PUSH (
    ET_AUTOMATION_TRACKLIST_AT_REMOVED, self)
}

/**
 * Returns the number of visible AutomationTrack's.
 */
int
automation_tracklist_get_num_visible (
  AutomationTracklist * self)
{
  AutomationTrack * at;
  int i;
  int count = 0;
  for (i = 0; i < self->num_ats; i++)
    {
      at = self->ats[i];
      if (at->created && at->visible)
        {
          count++;
        }
    }

  return count;
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
}
