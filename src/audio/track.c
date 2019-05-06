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

#include "actions/edit_tracks_action.h"
#include "actions/undo_manager.h"
#include "audio/audio_track.h"
#include "audio/automation_point.h"
#include "audio/automation_track.h"
#include "audio/bus_track.h"
#include "audio/channel.h"
#include "audio/chord_track.h"
#include "audio/group_track.h"
#include "audio/instrument_track.h"
#include "audio/master_track.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "gui/backend/events.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/track.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/objects.h"

#include <glib/gi18n.h>

void
track_init_loaded (Track * track)
{
  track->channel =
    project_get_channel (track->channel_id);

  int i;
  for (i = 0; i < track->num_regions; i++)
    track->regions[i] =
      project_get_region (
        track->region_ids[i]);
  for (i = 0; i < track->num_chords; i++)
    track->chords[i] =
      project_get_chord (
        track->chord_ids[i]);

  track->widget = track_widget_new (track);

  if (track->type != TRACK_TYPE_CHORD)
    automation_tracklist_init_loaded (
      &track->automation_tracklist);
}

void
track_init (Track * track)
{
  track->visible = 1;
  track->handle_pos = 1;
}

/**
 * Creates a track with the given label and returns
 * it.
 *
 * If the TrackType is one that needs a Channel,
 * then a Channel is also created for the track.
 */
Track *
track_new (
  TrackType type,
  char * label)
{
  Track * track =
    calloc (1, sizeof (Track));

  track_init (track);
  project_add_track (track);

  track->name = g_strdup (label);

  ChannelType ct;
  switch (type)
    {
    case TRACK_TYPE_INSTRUMENT:
      instrument_track_init (track);
      ct = CT_MIDI;
      break;
    case TRACK_TYPE_AUDIO:
      audio_track_init (track);
      ct = CT_AUDIO;
      break;
    case TRACK_TYPE_MASTER:
      master_track_init (track);
      ct = CT_MASTER;
      break;
    case TRACK_TYPE_BUS:
      bus_track_init (track);
      ct = CT_BUS;
      break;
    case TRACK_TYPE_GROUP:
      group_track_init (track);
      ct = CT_BUS;
      break;
    case TRACK_TYPE_CHORD:
      break;
    default:
      g_return_val_if_reached (NULL);
    }

  /* if should have channel */
  if (type != TRACK_TYPE_CHORD)
    {
      Channel * ch =
        channel_new (ct, label, F_ADD_TO_PROJ);
      track->channel = ch;

      track->channel_id = ch->id;
      ch->track = track;
      ch->track_id = track->id;

      channel_generate_automatables (ch);
    }

  automation_tracklist_init (
    &track->automation_tracklist,
    track);

  track->widget = track_widget_new (track);

  return track;
}

/**
 * Clones the track and returns the clone.
 */
Track *
track_clone (Track * track)
{
  int i;
  Track * new_track =
    track_new (
      track->type,
      track->name);

#define COPY_MEMBER(a) \
  new_track->a = track->a

  COPY_MEMBER (id);
  COPY_MEMBER (type);
  COPY_MEMBER (bot_paned_visible);
  COPY_MEMBER (visible);
  COPY_MEMBER (handle_pos);
  COPY_MEMBER (mute);
  COPY_MEMBER (solo);
  COPY_MEMBER (recording);
  COPY_MEMBER (active);
  COPY_MEMBER (color.red);
  COPY_MEMBER (color.green);
  COPY_MEMBER (color.blue);
  COPY_MEMBER (color.alpha);
  COPY_MEMBER (pos);

  Channel * ch = channel_clone (track->channel);
  track->channel = ch;
  track->channel_id = ch->id;
  ch->track = track;
  ch->track_id = track->id;

  Region * region, * new_region;
  for (i = 0; i < track->num_regions; i++)
    {
      /* clone region */
      region =
        project_get_region (track->region_ids[i]);
      new_region =
        region_clone (region, REGION_CLONE_COPY);

      /* add to new track */
      array_double_append (
        new_track->regions,
        new_track->region_ids,
        new_track->num_regions,
        new_region,
        new_region->id);
    }

  automation_tracklist_clone (
    &track->automation_tracklist,
    &new_track->automation_tracklist);

#undef COPY_MEMBER

  new_track->actual_id = track->id;

  return new_track;
}

/**
 * Sets recording and connects/disconnects the JACK ports.
 */
void
track_set_recording (Track *   track,
                     int       recording)
{
  Channel * channel =
    track_get_channel (track);

  if (!channel)
    {
      ui_show_notification_idle (
        "Recording not implemented yet for this "
        "track.");
      return;
    }
  if (channel->type == CT_AUDIO)
    {
      /* TODO connect L and R audio ports for recording */
      if (recording)
        {
          port_connect (
            AUDIO_ENGINE->stereo_in->l,
            channel->stereo_in->l);
          port_connect (
            AUDIO_ENGINE->stereo_in->r,
            channel->stereo_in->r);
        }
      else
        {
          port_disconnect (
            AUDIO_ENGINE->stereo_in->l,
            channel->stereo_in->l);
          port_disconnect (
            AUDIO_ENGINE->stereo_in->r,
            channel->stereo_in->r);
        }
    }
  else if (channel->type == CT_MIDI)
    {
      /* find first plugin */
      Plugin * plugin = channel_get_first_plugin (channel);

      if (plugin)
        {
          /* Connect/Disconnect MIDI port to the plugin */
          for (int i = 0; i < plugin->num_in_ports; i++)
            {
              Port * port = plugin->in_ports[i];
              if (port->type == TYPE_EVENT &&
                  port->flow == FLOW_INPUT)
                {
                  g_message ("%d MIDI In port: %s",
                             i, port->label);
                  if (recording)
                    port_connect (
                      AUDIO_ENGINE->midi_in, port);
                  else
                    port_disconnect (
                      AUDIO_ENGINE->midi_in, port);
                }
            }
        }
    }

  track->recording = recording;

  EVENTS_PUSH (ET_TRACK_STATE_CHANGED,
               track);
}

/**
 * Sets track muted and optionally adds the action
 * to the undo stack.
 */
void
track_set_muted (Track * track,
                 int     mute,
                 int     trigger_undo)
{
  UndoableAction * action =
    edit_tracks_action_new (
      EDIT_TRACK_ACTION_TYPE_MUTE,
      track,
      TRACKLIST_SELECTIONS,
      0.f, 0.f, 0, mute);
  g_message ("setting mute to %d",
             mute);
  if (trigger_undo)
    {
      undo_manager_perform (UNDO_MANAGER,
                            action);
    }
  else
    {
      edit_tracks_action_do (
        (EditTracksAction *) action);
    }
}

/**
 * Sets track soloed, updates UI and optionally
 * adds the action to the undo stack.
 */
void
track_set_soloed (Track * track,
                  int     solo,
                  int     trigger_undo)
{
  UndoableAction * action =
    edit_tracks_action_new (
      EDIT_TRACK_ACTION_TYPE_SOLO,
      track,
      TRACKLIST_SELECTIONS,
      0.f, 0.f, solo, 0);
  if (trigger_undo)
    {
      undo_manager_perform (UNDO_MANAGER,
                            action);
    }
  else
    {
      edit_tracks_action_do (
        (EditTracksAction *) action);
    }
}

/**
 * Returns if Track is in TracklistSelections.
 */
int
track_is_selected (Track * self)
{
  if (tracklist_selections_contains_track (
        TRACKLIST_SELECTIONS, self))
    return 1;

  return 0;
}

/**
 * Returns the last region in the track, or NULL.
 */
Region *
track_get_last_region (
  Track * track)
{
  int i;
  Region * last_region = NULL, * r;
  Position tmp;
  position_init (&tmp);

  if (track->type == TRACK_TYPE_AUDIO)
    {
      AudioTrack * at = (AudioTrack *) track;
      for (i = 0; i < at->num_regions; i++)
        {
          r = (Region *) at->regions[i];
          if (position_compare (
                &r->end_pos,
                &tmp) > 0)
            {
              last_region = r;
              position_set_to_pos (
                &tmp, &r->end_pos);
            }
        }
    }
  else if (track->type == TRACK_TYPE_INSTRUMENT)
    {
      InstrumentTrack * at =
        (InstrumentTrack *) track;
      for (i = 0; i < at->num_regions; i++)
        {
          r = (Region *) at->regions[i];
          if (position_compare (
                &r->end_pos,
                &tmp) > 0)
            {
              last_region = r;
              position_set_to_pos (
                &tmp, &r->end_pos);
            }
        }
    }
  return last_region;
}

/**
 * Returns the last region in the track, or NULL.
 */
AutomationPoint *
track_get_last_automation_point (
  Track * track)
{
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  if (!atl)
    return NULL;

  int i, j;
  AutomationPoint * last_ap = NULL, * ap;
  AutomationTrack * at;
  Position tmp;
  position_init (&tmp);

  for (i = 0; i < atl->num_automation_tracks;
       i++)
    {
      at = atl->automation_tracks[i];

      for (j = 0; j < at->num_automation_points;
           j++)
        {
          ap = at->automation_points[j];

          if (position_compare (
                &ap->pos,
                &tmp) > 0)
            {
              last_ap = ap;
              position_set_to_pos (
                &tmp, &ap->pos);
            }
        }
    }

  return last_ap;
}

/**
 * Wrapper.
 */
void
track_setup (Track * track)
{
#define SETUP_TRACK(uc,sc) \
  case TRACK_TYPE_##uc: \
    sc##_track_setup (track); \
    break;

  switch (track->type)
    {
    SETUP_TRACK (INSTRUMENT, instrument);
    SETUP_TRACK (MASTER, master);
    SETUP_TRACK (AUDIO, audio);
    SETUP_TRACK (BUS, bus);
    SETUP_TRACK (GROUP, group);
    case TRACK_TYPE_CHORD:
      break;
    }

#undef SETUP_TRACK
}

static Region *
get_region_by_name (Track * track, char * name)
{
  Region * region;
  for (int i = 0; i < track->num_regions; i++)
    {
      region = track->regions[i];
      if (g_strcmp0 (region->name, name) == 0)
        return region;
    }
  return NULL;
}

/**
 * Wrapper.
 */
void
track_add_region (Track * track,
                  Region * region)
{
  g_warn_if_fail (
    (track->type == TRACK_TYPE_INSTRUMENT ||
    track->type == TRACK_TYPE_AUDIO) &&
    region->id >= 0);

  int count = 1;
  region->name = g_strdup (track->name);
  while (get_region_by_name (
          track, region->name))
    {
      g_free (region->name);
      region->name =
        g_strdup_printf ("%s %d",
                         track->name,
                         count++);
    }

  region_set_track (region, track);
  track->region_ids[track->num_regions] =
    region->id;
  array_append (track->regions,
                track->num_regions,
                region);

  EVENTS_PUSH (ET_REGION_CREATED,
               region);
}

/**
 * Only removes the region from the track.
 *
 * Does not free the Region.
 */
void
track_remove_region (
  Track * track,
  Region * region)
{
  region_disconnect (region);

  array_double_delete (
    track->regions,
    track->region_ids,
    track->num_regions,
    region,
    region->id);

  EVENTS_PUSH (ET_REGION_REMOVED, track);
}

void
track_disconnect (Track * track)
{
  /* remove regions */
  int i;
  for (i = 0; i < track->num_regions; i++)
    track_remove_region (
      track, track->regions[i]);

  /* remove chords */
  for (i = 0; i < track->num_chords; i++)
    chord_track_remove_chord (
      track, track->chords[i]);

  channel_disconnect (track->channel);
}

/**
 * Wrapper for each track type.
 */
void
track_free (Track * track)
{
  project_remove_track (track);

  /* remove regions */
  int i;
  for (i = 0; i < track->num_regions; i++)
    region_free (track->regions[i]);

  /* remove chords */
  for (i = 0; i < track->num_chords; i++)
    chord_free (track->chords[i]);

  /* remove automation points, curves, tracks,
   * lanes*/
  automation_tracklist_free_members (
    &track->automation_tracklist);

  switch (track->type)
    {
    case TRACK_TYPE_INSTRUMENT:
      instrument_track_free (
        (InstrumentTrack *) track);
      break;
    case TRACK_TYPE_MASTER:
      master_track_free (
        (MasterTrack *) track);
      break;
    case TRACK_TYPE_AUDIO:
      audio_track_free (
        (AudioTrack *) track);
      break;
    case TRACK_TYPE_CHORD:
      chord_track_free (
        (ChordTrack *) track);
      break;
    case TRACK_TYPE_BUS:
      bus_track_free (
        (BusTrack *) track);
      break;
    case TRACK_TYPE_GROUP:
      group_track_free (track);
      break;
    }

  channel_free (track->channel);

  if (track->widget && GTK_IS_WIDGET (track->widget))
    gtk_widget_destroy (
      GTK_WIDGET (track->widget));

  free (track);
}

/**
 * Returns the automation tracklist if the track type has one,
 * or NULL if it doesn't (like chord tracks).
 */
AutomationTracklist *
track_get_automation_tracklist (Track * track)
{
  switch (track->type)
    {
    case TRACK_TYPE_CHORD:
      break;
    case TRACK_TYPE_BUS:
    case TRACK_TYPE_GROUP:
    case TRACK_TYPE_INSTRUMENT:
    case TRACK_TYPE_AUDIO:
    case TRACK_TYPE_MASTER:
        {
          ChannelTrack * bt = (ChannelTrack *) track;
          return &bt->automation_tracklist;
        }
    }

  return NULL;
}

/**
 * Wrapper for track types that have fader automatables.
 *
 * Otherwise returns NULL.
 */
Automatable *
track_get_fader_automatable (Track * track)
{
  switch (track->type)
    {
    case TRACK_TYPE_CHORD:
      break;
    case TRACK_TYPE_BUS:
    case TRACK_TYPE_GROUP:
    case TRACK_TYPE_AUDIO:
    case TRACK_TYPE_MASTER:
    case TRACK_TYPE_INSTRUMENT:
        {
          ChannelTrack * bt = (ChannelTrack *) track;
          return channel_get_fader_automatable (bt->channel);
        }
    }

  return NULL;
}

/**
 * Returns the channel of the track, if the track type has
 * a channel,
 * or NULL if it doesn't.
 */
Channel *
track_get_channel (Track * track)
{
  switch (track->type)
    {
    case TRACK_TYPE_CHORD:
      break;
    case TRACK_TYPE_MASTER:
    case TRACK_TYPE_INSTRUMENT:
    case TRACK_TYPE_AUDIO:
    case TRACK_TYPE_BUS:
    case TRACK_TYPE_GROUP:
      return track->channel;
    }

  return NULL;
}

/**
 * Returns the region at the given position, or NULL.
 */
Region *
track_get_region_at_pos (
  Track *    track,
  Position * pos)
{
  if (track->type == TRACK_TYPE_INSTRUMENT)
    {
      InstrumentTrack * it = (InstrumentTrack *) track;
      for (int i = 0; i < it->num_regions; i++)
        {
          Region * r = (Region *) it->regions[i];
          if (position_compare (pos,
                                &r->start_pos) >= 0 &&
              position_compare (pos,
                                &r->end_pos) <= 0)
            return r;
        }
    }
  else if (track->type == TRACK_TYPE_AUDIO)
    {
      AudioTrack * at = (AudioTrack *) track;
      for (int i = 0; i < at->num_regions; i++)
        {
          Region * r = (Region *) at->regions[i];
          if (position_compare (pos,
                                &r->start_pos) >= 0 &&
              position_compare (pos,
                                &r->end_pos) <= 0)
            return r;
        }
    }
  return NULL;
}

char *
track_stringize_type (
  TrackType type)
{
  switch (type)
    {
    case TRACK_TYPE_INSTRUMENT:
      return g_strdup (
        _("Instrument"));
    case TRACK_TYPE_AUDIO:
      return g_strdup (
        _("Audio"));
    case TRACK_TYPE_BUS:
      return g_strdup (
        _("Bus"));
    case TRACK_TYPE_MASTER:
      return g_strdup (
        _("Master"));
    case TRACK_TYPE_CHORD:
      return g_strdup (
        _("Chord"));
    case TRACK_TYPE_GROUP:
      return g_strdup (
        _("Group"));
    default:
      g_warn_if_reached ();
      return NULL;
    }
}

/**
 * Wrapper to get the track name.
 */
char *
track_get_name (Track * track)
{
  return track->name;
}
