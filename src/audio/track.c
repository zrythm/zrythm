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

#include "actions/edit_tracks_action.h"
#include "actions/undo_manager.h"
#include "audio/audio_track.h"
#include "audio/automation_point.h"
#include "audio/automation_track.h"
#include "audio/audio_bus_track.h"
#include "audio/channel.h"
#include "audio/chord_track.h"
#include "audio/audio_group_track.h"
#include "audio/instrument_track.h"
#include "audio/marker_track.h"
#include "audio/master_track.h"
#include "audio/midi_bus_track.h"
#include "audio/midi_group_track.h"
#include "audio/midi_track.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "gui/backend/events.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/audio_region.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_region.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/track.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/string.h"

#include <glib/gi18n.h>

void
track_init_loaded (Track * track)
{
  int i,j;
  TrackLane * lane;
  for (j = 0; j < track->num_lanes; j++)
    {
      lane = track->lanes[j];
      lane->track = track;
      track_lane_init_loaded (lane);
    }
  ScaleObject * scale;
  for (i = 0; i < track->num_scales; i++)
    {
      scale = track->scales[i];
      scale->track = track;
      arranger_object_init_loaded (
        (ArrangerObject *) scale);
    }
  Marker * marker;
  for (i = 0; i < track->num_markers; i++)
    {
      marker = track->markers[i];
      marker->track = track;
      arranger_object_init_loaded (
        (ArrangerObject *) marker);
    }
  Region * region;
  for (i = 0; i < track->num_chord_regions; i++)
    {
      region = track->chord_regions[i];
      region->track_pos = track->pos;
      arranger_object_init_loaded (
        (ArrangerObject *) region);
    }

  /* init loaded channel */
  Channel * ch;
  if (track->channel)
    {
      ch = track->channel;
      ch->track = track;
      channel_init_loaded (ch);
      g_message (
        "track %s (ch %p) %p %p", track->name,
        track->channel, ch->track, track);
    }

  /* set track to automation tracklist */
  AutomationTracklist * atl;
  atl = &track->automation_tracklist;
  atl->track = track;
  automation_tracklist_init_loaded (atl);
}

/**
 * Adds a new TrackLane to the Track.
 */
static void
track_add_lane (
  Track * self)
{
  g_return_if_fail (self);
  array_double_size_if_full (
    self->lanes, self->num_lanes,
    self->lanes_size, TrackLane *);
  g_message ("-----num lanes %d, lanes size %d",
    self->num_lanes, self->lanes_size);
  self->lanes[self->num_lanes++] =
    track_lane_new (self, self->num_lanes);

  EVENTS_PUSH (ET_TRACK_LANE_ADDED,
               self->lanes[self->num_lanes - 1]);
}

/**
 * Inits the Track, optionally adding a single
 * lane.
 *
 * @param add_lane Add a lane. This should be used
 *   for new Tracks. When cloning, the lanes should
 *   be cloned so this should be 0.
 */
void
track_init (
  Track *   self,
  const int add_lane)
{
  self->visible = 1;
  self->main_height = TRACK_DEF_HEIGHT;
  self->midi_ch = 1;
  track_add_lane (self);
}

/**
 * Creates a track with the given label and returns
 * it.
 *
 * If the TrackType is one that needs a Channel,
 * then a Channel is also created for the track.
 *
 * @param with_lane Init the Track with a lane.
 */
Track *
track_new (
  TrackType type,
  char * label,
  const int with_lane)
{
  Track * track =
    calloc (1, sizeof (Track));

  track_init (track, with_lane);

  track->name = g_strdup (label);

  switch (type)
    {
    case TRACK_TYPE_INSTRUMENT:
      track->in_signal_type =
        TYPE_EVENT;
      track->out_signal_type =
        TYPE_AUDIO;
      instrument_track_init (track);
      break;
    case TRACK_TYPE_AUDIO:
      track->in_signal_type =
        TYPE_AUDIO;
      track->out_signal_type =
        TYPE_AUDIO;
      audio_track_init (track);
      break;
    case TRACK_TYPE_MASTER:
      track->in_signal_type =
        TYPE_AUDIO;
      track->out_signal_type =
        TYPE_AUDIO;
      master_track_init (track);
      break;
    case TRACK_TYPE_AUDIO_BUS:
      track->in_signal_type =
        TYPE_AUDIO;
      track->out_signal_type =
        TYPE_AUDIO;
      audio_bus_track_init (track);
      break;
    case TRACK_TYPE_MIDI_BUS:
      track->in_signal_type =
        TYPE_EVENT;
      track->out_signal_type =
        TYPE_EVENT;
      midi_bus_track_init (track);
      break;
    case TRACK_TYPE_AUDIO_GROUP:
      track->in_signal_type =
        TYPE_AUDIO;
      track->out_signal_type =
        TYPE_AUDIO;
      audio_group_track_init (track);
      break;
    case TRACK_TYPE_MIDI_GROUP:
      track->in_signal_type =
        TYPE_EVENT;
      track->out_signal_type =
        TYPE_EVENT;
      midi_group_track_init (track);
      break;
    case TRACK_TYPE_MIDI:
      track->in_signal_type =
        TYPE_EVENT;
      track->out_signal_type =
        TYPE_EVENT;
      midi_track_init (track);
      break;
    case TRACK_TYPE_CHORD:
      track->in_signal_type =
        TYPE_EVENT;
      track->out_signal_type =
        TYPE_EVENT;
      chord_track_init (track);
      break;
    case TRACK_TYPE_MARKER:
      marker_track_init (track);
      track->in_signal_type =
        TYPE_UNKNOWN;
      track->out_signal_type =
        TYPE_UNKNOWN;
      break;
    default:
      g_return_val_if_reached (NULL);
    }

  automation_tracklist_init (
    &track->automation_tracklist,
    track);

  /* if should have channel */
  Channel * ch;
  switch (type)
    {
    case TRACK_TYPE_MASTER:
    case TRACK_TYPE_INSTRUMENT:
    case TRACK_TYPE_AUDIO:
    case TRACK_TYPE_MIDI:
    case TRACK_TYPE_AUDIO_BUS:
    case TRACK_TYPE_AUDIO_GROUP:
    case TRACK_TYPE_MIDI_BUS:
    case TRACK_TYPE_MIDI_GROUP:
      ch =
        channel_new (track);
      track->channel = ch;

      ch->track = track;

      channel_generate_automation_tracks (ch);
      break;
    case TRACK_TYPE_CHORD:
    case TRACK_TYPE_MARKER:
      break;
    }

  return track;
}

/**
 * Clones the track and returns the clone.
 */
Track *
track_clone (Track * track)
{
  int j;
  Track * new_track =
    track_new (
      track->type, track->name, F_WITHOUT_LANE);

#define COPY_MEMBER(a) \
  new_track->a = track->a

  COPY_MEMBER (type);
  COPY_MEMBER (automation_visible);
  COPY_MEMBER (visible);
  COPY_MEMBER (main_height);
  COPY_MEMBER (mute);
  COPY_MEMBER (solo);
  COPY_MEMBER (recording);
  COPY_MEMBER (pinned);
  COPY_MEMBER (active);
  COPY_MEMBER (color);
  COPY_MEMBER (pos);
  COPY_MEMBER (midi_ch);

  if (track->channel)
    {
      Channel * ch =
        channel_clone (track->channel, new_track);
      new_track->channel = ch;
      ch->track = new_track;
    }

  TrackLane * lane, * new_lane;
  new_track->num_lanes = track->num_lanes;
  new_track->lanes =
    realloc (
      new_track->lanes,
      sizeof (TrackLane *) *
        (size_t) track->num_lanes);
  for (j = 0; j < track->num_lanes; j++)
    {
      /* clone lane */
       lane = track->lanes[j];
       new_lane =
         track_lane_clone (lane);
       new_lane->track = new_track;
       new_track->lanes[j] = new_lane;
    }

  automation_tracklist_clone (
    &track->automation_tracklist,
    &new_track->automation_tracklist);

#undef COPY_MEMBER

  return new_track;
}

/**
 * Appends the Track to the selections.
 *
 * @param exclusive Select only this track.
 * @param fire_events Fire events to update the
 *   UI.
 */
void
track_select (
  Track * self,
  int     select,
  int     exclusive,
  int     fire_events)
{
  if (select)
    {
      if (exclusive)
        {
          tracklist_selections_select_single (
            TRACKLIST_SELECTIONS, self);
        }
      else
        {
          tracklist_selections_add_track (
            TRACKLIST_SELECTIONS, self);
        }
    }
  else
    {
      tracklist_selections_remove_track (
        TRACKLIST_SELECTIONS, self);
    }

  if (fire_events)
    {
      EVENTS_PUSH (ET_TRACK_CHANGED,
                   self);
    }
}

/**
 * Sets recording and connects/disconnects the
 * JACK ports.
 */
void
track_set_recording (
  Track *   track,
  int       recording)
{
  Channel * channel =
    track_get_channel (track);

  if (!channel)
    {
      g_message (
        "Recording not implemented yet for this "
        "track.");
      return;
    }

  /*if (recording)*/
    /*{*/
      /* FIXME just enable the connections */
      /*port_connect (*/
        /*AUDIO_ENGINE->stereo_in->l,*/
        /*channel->stereo_in->l, 1);*/
      /*port_connect (*/
        /*AUDIO_ENGINE->stereo_in->r,*/
        /*channel->stereo_in->r, 1);*/
      /*port_connect (*/
        /*AUDIO_ENGINE->midi_in,*/
        /*channel->midi_in, 1);*/
    /*}*/
  /*else*/
    /*{*/
      /* FIXME just disable the connections */
      /*port_disconnect (*/
        /*AUDIO_ENGINE->stereo_in->l,*/
        /*channel->stereo_in->l);*/
      /*port_disconnect (*/
        /*AUDIO_ENGINE->stereo_in->r,*/
        /*channel->stereo_in->r);*/
      /*port_disconnect (*/
        /*AUDIO_ENGINE->midi_in,*/
        /*channel->midi_in);*/
    /*}*/

  track->recording = recording;

  EVENTS_PUSH (ET_TRACK_STATE_CHANGED,
               track);
}

/**
 * Sets track muted and optionally adds the action
 * to the undo stack.
 */
void
track_set_muted (
  Track * track,
  int     mute,
  int     trigger_undo)
{
  if (trigger_undo)
    {
      TracklistSelections tls;
      tls.tracks[0] = track;
      tls.num_tracks = 1;
      UndoableAction * action =
        edit_tracks_action_new (
          EDIT_TRACK_ACTION_TYPE_MUTE,
          track,
          &tls,
          0.f, 0.f, 0, mute);
      undo_manager_perform (UNDO_MANAGER,
                            action);
    }
  else
    {
      track->mute = mute;
      EVENTS_PUSH (
        ET_TRACK_STATE_CHANGED, track);
    }
}

/**
 * Returns the Track from the Project matching
 * \p name.
 *
 * @param name Name to search for.
 */
Track *
track_get_from_name (
  const char * name)
{
  g_return_val_if_fail (name, NULL);

  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];
      if (string_is_equal (track->name, name, 1))
        {
          return track;
        }
    }

  g_return_val_if_reached (NULL);
}

/**
 * Fills in the array with all the velocities in
 * the project that are within or outside the
 * range given.
 *
 * @param inside Whether to find velocities inside
 *   the range (1) or outside (0).
 */
void
track_get_velocities_in_range (
  const Track *    track,
  const Position * start_pos,
  const Position * end_pos,
  Velocity ***     velocities,
  int *            num_velocities,
  int *            velocities_size,
  int              inside)
{
  if (track->type != TRACK_TYPE_MIDI &&
      track->type != TRACK_TYPE_INSTRUMENT)
    return;

  TrackLane * lane;
  Region * region;
  MidiNote * mn;
  Position global_start_pos;
  for (int i = 0; i < track->num_lanes; i++)
    {
      lane = track->lanes[i];
      for (int j = 0; j < lane->num_regions; j++)
        {
          region = lane->regions[j];
          for (int k = 0;
               k < region->num_midi_notes; k++)
            {
              mn = region->midi_notes[k];
              midi_note_get_global_start_pos (
                mn, &global_start_pos);

#define ADD_VELOCITY \
  array_double_size_if_full ( \
    *velocities, *num_velocities, \
    *velocities_size, Velocity *); \
  (*velocities)[(* num_velocities)++] = \
    mn->vel

              if (inside &&
                  position_is_after_or_equal (
                    &global_start_pos, start_pos) &&
                  position_is_before_or_equal (
                    &global_start_pos, end_pos))
                {
                  ADD_VELOCITY;
                }
              else if (!inside &&
                  (position_is_before (
                    &global_start_pos, start_pos) ||
                  position_is_after (
                    &global_start_pos, end_pos)))
                {
                  ADD_VELOCITY;
                }
#undef ADD_VELOCITY
            }
        }
    }
}

/**
 * Returns if the given TrackType can host the
 * given RegionType.
 */
int
track_type_can_host_region_type (
  const TrackType  tt,
  const RegionType rt)
{
  switch (rt)
    {
    case REGION_TYPE_MIDI:
      return
        tt == TRACK_TYPE_MIDI ||
        tt == TRACK_TYPE_INSTRUMENT;
    case REGION_TYPE_AUDIO:
      return
        tt == TRACK_TYPE_AUDIO;
    case REGION_TYPE_AUTOMATION:
      return
        tt != TRACK_TYPE_CHORD &&
        tt != TRACK_TYPE_MARKER;
    case REGION_TYPE_CHORD:
      return
        tt == TRACK_TYPE_CHORD;
    }
  g_return_val_if_reached (-1);
}

/**
 * Returns the full visible height (main height +
 * height of all visible automation tracks + height
 * of all visible lanes).
 */
int
track_get_full_visible_height (
  Track * self)
{
  int height = self->main_height;

  if (self->lanes_visible)
    {
      for (int i = 0; i < self->num_lanes; i++)
        {
          TrackLane * lane = self->lanes[i];
          height += lane->height;
        }
    }
  if (self->automation_visible)
    {
      AutomationTracklist * atl =
        track_get_automation_tracklist (self);
      if (atl)
        {
          for (int i = 0; i < atl->num_ats; i++)
            {
              AutomationTrack * at = atl->ats[i];
              if (at->visible)
                height += at->height;
            }
        }
    }
  return height;
}

/**
 * Sets track soloed, updates UI and optionally
 * adds the action to the undo stack.
 */
void
track_set_soloed (
  Track * track,
  int     solo,
  int     trigger_undo)
{
  TracklistSelections tls;
  tls.tracks[0] = track;
  tls.num_tracks = 1;
  UndoableAction * action =
    edit_tracks_action_new (
      EDIT_TRACK_ACTION_TYPE_SOLO,
      track,
      &tls,
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
 * Writes the track to the given MIDI file.
 */
void
track_write_to_midi_file (
  const Track * self,
  MIDI_FILE *   mf)
{
  g_return_if_fail (
    track_has_piano_roll (self));

  TrackLane * lane;
  for (int i = 0; i < self->num_lanes; i++)
    {
      lane = self->lanes[i];

      track_lane_write_to_midi_file (
        lane, mf);
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
 *
 * FIXME cache.
 */
Region *
track_get_last_region (
  Track * track)
{
  int i, j;
  Region * last_region = NULL, * r;
  ArrangerObject * r_obj;
  Position tmp;
  position_init (&tmp);

  if (track->type == TRACK_TYPE_AUDIO ||
      track->type == TRACK_TYPE_INSTRUMENT)
    {
      TrackLane * lane;
      for (i = 0; i < track->num_lanes; i++)
        {
          lane = track->lanes[i];

          for (j = 0; j < lane->num_regions; j++)
            {
              r = lane->regions[j];
              r_obj = (ArrangerObject *) r;
              if (position_is_after (
                    &r_obj->end_pos, &tmp))
                {
                  last_region = r;
                  position_set_to_pos (
                    &tmp, &r_obj->end_pos);
                }
            }
        }
    }

  AutomationTracklist * atl =
    &track->automation_tracklist;
  AutomationTrack * at;
  for (i = 0; i < atl->num_ats; i++)
    {
      at = atl->ats[i];
      r = automation_track_get_last_region (at);
      r_obj = (ArrangerObject *) r;

      if (!r)
        continue;

      if (position_is_after (
            &r_obj->end_pos, &tmp))
        {
          last_region = r;
          position_set_to_pos (
            &tmp, &r_obj->end_pos);
        }
    }

  return last_region;
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
    SETUP_TRACK (AUDIO_BUS, audio_bus);
    SETUP_TRACK (AUDIO_GROUP, audio_group);
    case TRACK_TYPE_CHORD:
    /* TODO */
    default:
      break;
    }

#undef SETUP_TRACK
}

/**
 * Adds a Region to the given lane of the track.
 *
 * The Region must be the main region (see
 * ArrangerObjectInfo).
 *
 * @param at The AutomationTrack of this Region, if
 *   automation region.
 * @param lane_pos The position of the lane to add
 *   to, if applicable.
 * @param gen_name Generate a unique region name or
 *   not. This will be 0 if the caller already
 *   generated a unique name.
 */
void
track_add_region (
  Track * track,
  Region * region,
  AutomationTrack * at,
  int      lane_pos,
  int      gen_name,
  int      fire_events)
{
  if (region->type == REGION_TYPE_AUTOMATION)
    {
      track = at->track;
    }
  g_warn_if_fail (track);

  if (gen_name)
    {
      region_gen_name (
        region, NULL, at, track);
    }

  int add_lane = 0, add_at = 0, add_chord = 0;
  switch (region->type)
    {
    case REGION_TYPE_MIDI:
      add_lane = 1;
      break;
    case REGION_TYPE_AUDIO:
      add_lane = 1;
      break;
    case REGION_TYPE_AUTOMATION:
      add_at = 1;
      break;
    case REGION_TYPE_CHORD:
      add_chord = 1;
      break;
    }

  if (add_lane)
    {
      /* enable extra lane if necessary */
      track_create_missing_lanes (track, lane_pos);

      g_warn_if_fail (track->lanes[lane_pos]);
      track_lane_add_region (
        track->lanes[lane_pos],
        region);
    }

  if (add_at)
    {
      automation_track_add_region (
        at, region);
    }

  if (add_chord)
    {
      g_warn_if_fail (track == P_CHORD_TRACK);
      array_double_size_if_full (
        track->chord_regions,
        track->num_chord_regions,
        track->chord_regions_size, Region *);
      array_append (track->chord_regions,
                    track->num_chord_regions,
                    region);
    }

  if (fire_events)
    {
      EVENTS_PUSH (
        ET_ARRANGER_OBJECT_CREATED, region);
    }
}

/**
 * Creates missing TrackLane's until pos.
 */
void
track_create_missing_lanes (
  Track *   track,
  const int pos)
{
  while (track->num_lanes < pos + 2)
    {
      track_add_lane (track);
    }
}

/**
 * Removes the empty last lanes of the Track
 * (except the last one).
 */
void
track_remove_empty_last_lanes (
  Track * track)
{
  g_return_if_fail (track);
  g_message ("removing empty last lanes from %s",
             track->name);
  int removed = 0;
  for (int i = track->num_lanes - 1; i >= 1; i--)
    {
      g_message ("lane %d has %d regions",
                 i, track->lanes[i]->num_regions);
      if (track->lanes[i]->num_regions > 0)
        break;

      if (track->lanes[i]->num_regions == 0 &&
          track->lanes[i - 1]->num_regions == 0)
        {
          g_message ("removing lane %d", i);
          track->num_lanes--;
          free_later (
            track->lanes[i], track_lane_free);
          removed = 1;
        }
    }

  if (removed)
    {
      EVENTS_PUSH (ET_TRACK_LANE_REMOVED, NULL);
    }
}

/**
 * Returns if the Track should have a piano roll.
 */
int
track_has_piano_roll (
  const Track * track)
{
  return track->type == TRACK_TYPE_MIDI ||
    track->type == TRACK_TYPE_INSTRUMENT;
}

/**
 * Updates position in the tracklist and also
 * updates the information in the lanes.
 */
void
track_set_pos (
  Track * track,
  int     pos)
{
  track->pos = pos;

  for (int i = 0; i < track->num_lanes; i++)
    {
      track_lane_set_track_pos (
        track->lanes[i], pos, 1);
    }
  automation_tracklist_update_track_pos (
    &track->automation_tracklist, track);

  /* update port identifier track positions */
  if (track->channel)
    {
      Port * ports[80000];
      int    num_ports = 0;
      channel_append_all_ports (
        track->channel,
        ports, &num_ports, 1);

      for (int i = 0; i < num_ports; i++)
        {
          g_warn_if_fail (ports[i]);
          ports[i]->identifier.track_pos = pos;
        }
    }
}

/**
 * Set track lanes visible and fire events.
 */
void
track_set_lanes_visible (
  Track *   track,
  const int visible)
{
  track->lanes_visible = visible;

  EVENTS_PUSH (
    ET_TRACK_LANES_VISIBILITY_CHANGED, track);
}

/**
 * Set automation visible and fire events.
 */
void
track_set_automation_visible (
  Track *   track,
  const int visible)
{
  track->automation_visible = visible;

  EVENTS_PUSH (
    ET_TRACK_AUTOMATION_VISIBILITY_CHANGED, track);
}

/**
 * Only removes the region from the track.
 *
 * @pararm free Also free the Region.
 */
void
track_remove_region (
  Track *  track,
  Region * region,
  int      fire_events,
  int      free)
{
  region_disconnect (region);

  g_warn_if_fail (region->lane_pos >= 0);

  if (region_type_has_lane (region->type))
    {
      array_delete (
        track->lanes[region->lane_pos]->
          regions,
        track->lanes[region->lane_pos]->
          num_regions,
        region);
    }
  else if (region->type == REGION_TYPE_CHORD)
    {
      array_delete (
        track->chord_regions,
        track->num_chord_regions,
        region);
    }
  else if (region->type == REGION_TYPE_AUTOMATION)
    {
      AutomationTrack * at =
        region_get_automation_track (region);
      array_delete (
        at->regions, at->num_regions, region);
    }

  if (free)
    free_later (region, arranger_object_free);

  if (fire_events)
    {
      EVENTS_PUSH (
        ET_ARRANGER_OBJECT_REMOVED,
        ARRANGER_OBJECT_TYPE_REGION);
    }
}

/**
 * Adds and connects a Modulator to the Track.
 */
void
track_add_modulator (
  Track * track,
  Modulator * modulator)
{
  array_double_size_if_full (
    track->modulators, track->num_modulators,
    track->modulators_size, Modulator *);
  array_append (track->modulators,
                track->num_modulators,
                modulator);

  mixer_recalc_graph (MIXER);

  EVENTS_PUSH (ET_MODULATOR_ADDED, modulator);
}

/**
 * Wrapper for each track type.
 */
void
track_free (Track * track)
{
  if (track->name)
    g_free (track->name);

  /* remove regions */
  /* FIXME move inside *_track_free */
  int i;
  for (i = 0; i < track->num_lanes; i++)
    track_lane_free (track->lanes[i]);

  /* remove automation points, curves, tracks,
   * lanes*/
  /* FIXME move inside *_track_free */
  automation_tracklist_free_members (
    &track->automation_tracklist);

#define _FREE_TRACK(type_caps,sc) \
  case TRACK_TYPE_##type_caps: \
    sc##_track_free (track); \
    break

  switch (track->type)
    {
      _FREE_TRACK (INSTRUMENT, instrument);
      _FREE_TRACK (MASTER, master);
      _FREE_TRACK (AUDIO, audio);
      _FREE_TRACK (CHORD, chord);
      _FREE_TRACK (AUDIO_BUS, audio_bus);
      _FREE_TRACK (MIDI_BUS, audio_bus);
      _FREE_TRACK (AUDIO_GROUP, audio_group);
      _FREE_TRACK (MIDI_GROUP, midi_group);
    default:
      /* TODO */
      break;
    }

#undef _FREE_TRACK

  if (track->channel)
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
    case TRACK_TYPE_MARKER:
      break;
    case TRACK_TYPE_AUDIO_BUS:
    case TRACK_TYPE_AUDIO_GROUP:
    case TRACK_TYPE_MIDI_BUS:
    case TRACK_TYPE_MIDI_GROUP:
    case TRACK_TYPE_INSTRUMENT:
    case TRACK_TYPE_AUDIO:
    case TRACK_TYPE_MASTER:
    case TRACK_TYPE_MIDI:
        {
          return &track->automation_tracklist;
        }
    default:
      g_warn_if_reached ();
      break;
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
    case TRACK_TYPE_AUDIO_BUS:
    case TRACK_TYPE_AUDIO_GROUP:
    case TRACK_TYPE_MIDI_BUS:
    case TRACK_TYPE_MIDI_GROUP:
    case TRACK_TYPE_AUDIO:
    case TRACK_TYPE_MASTER:
    case TRACK_TYPE_INSTRUMENT:
        {
          ChannelTrack * bt = (ChannelTrack *) track;
          return channel_get_fader_automatable (bt->channel);
        }
    default:
      break;
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
  g_warn_if_fail (track);

  switch (track->type)
    {
    case TRACK_TYPE_MASTER:
    case TRACK_TYPE_INSTRUMENT:
    case TRACK_TYPE_AUDIO:
    case TRACK_TYPE_AUDIO_BUS:
    case TRACK_TYPE_AUDIO_GROUP:
    case TRACK_TYPE_MIDI_BUS:
    case TRACK_TYPE_MIDI_GROUP:
    case TRACK_TYPE_MIDI:
      return track->channel;
    default:
      return NULL;
    }
}

/**
 * Returns the region at the given position, or NULL.
 */
Region *
track_get_region_at_pos (
  const Track *    track,
  const Position * pos)
{
  int i, j;

  if (track->type == TRACK_TYPE_INSTRUMENT ||
      track->type == TRACK_TYPE_AUDIO ||
      track->type == TRACK_TYPE_MIDI)
    {
      TrackLane * lane;
      Region * r;
      ArrangerObject * r_obj;
      for (i = 0; i < track->num_lanes; i++)
        {
          lane = track->lanes[i];

          for (j = 0; j < lane->num_regions; j++)
            {
              r = lane->regions[j];
              r_obj = (ArrangerObject *) r;
              if (pos->frames >=
                    r_obj->pos.frames &&
                  pos->frames <=
                    r_obj->end_pos.frames)
                return r;
            }
        }
    }
  else if (track->type == TRACK_TYPE_CHORD)
    {
      Region * r;
      ArrangerObject * r_obj;
      for (j = 0; j < track->num_chord_regions; j++)
        {
          r = track->chord_regions[j];
          r_obj = (ArrangerObject *) r;
          if (position_is_after_or_equal (
                pos, &r_obj->pos) &&
              position_is_before_or_equal (
                pos, &r_obj->end_pos))
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
    case TRACK_TYPE_MIDI:
      return g_strdup (
        _("MIDI"));
    case TRACK_TYPE_AUDIO_BUS:
      return g_strdup (
        _("Audio Bus"));
    case TRACK_TYPE_MIDI_BUS:
      return g_strdup (
        _("MIDI Bus"));
    case TRACK_TYPE_MASTER:
      return g_strdup (
        _("Master"));
    case TRACK_TYPE_CHORD:
      return g_strdup (
        _("Chord"));
    case TRACK_TYPE_AUDIO_GROUP:
      return g_strdup (
        _("Audio Group"));
    case TRACK_TYPE_MIDI_GROUP:
      return g_strdup (
        _("MIDI Group"));
    case TRACK_TYPE_MARKER:
      return g_strdup (
        _("Marker"));
    default:
      g_warn_if_reached ();
      return NULL;
    }
}

/**
 * Returns the FaderType corresponding to the given
 * Track.
 */
FaderType
track_get_fader_type (
  const Track * track)
{
  switch (track->type)
    {
    case TRACK_TYPE_MIDI:
    case TRACK_TYPE_MIDI_BUS:
    case TRACK_TYPE_CHORD:
    case TRACK_TYPE_MIDI_GROUP:
      return FADER_TYPE_MIDI_CHANNEL;
    case TRACK_TYPE_INSTRUMENT:
    case TRACK_TYPE_AUDIO:
    case TRACK_TYPE_AUDIO_BUS:
    case TRACK_TYPE_MASTER:
    case TRACK_TYPE_AUDIO_GROUP:
      return FADER_TYPE_AUDIO_CHANNEL;
    case TRACK_TYPE_MARKER:
      return FADER_TYPE_NONE;
    default:
      g_return_val_if_reached (FADER_TYPE_NONE);
    }
}

/**
 * Returns the PassthroughProcessorType
 * corresponding to the given Track.
 */
PassthroughProcessorType
track_get_passthrough_processor_type (
  const Track * track)
{
  switch (track->type)
    {
    case TRACK_TYPE_MIDI:
    case TRACK_TYPE_MIDI_BUS:
    case TRACK_TYPE_CHORD:
    case TRACK_TYPE_MIDI_GROUP:
      return PP_TYPE_MIDI_CHANNEL;
    case TRACK_TYPE_INSTRUMENT:
    case TRACK_TYPE_AUDIO:
    case TRACK_TYPE_AUDIO_BUS:
    case TRACK_TYPE_MASTER:
    case TRACK_TYPE_AUDIO_GROUP:
      return PP_TYPE_AUDIO_CHANNEL;
    case TRACK_TYPE_MARKER:
      return PP_TYPE_NONE;
    default:
      g_return_val_if_reached (PP_TYPE_NONE);
    }
}

/**
 * Updates the frames of each position in each child
 * of the track recursively.
 */
void
track_update_frames (
  Track * self)
{
  int i;
  for (i = 0; i < self->num_lanes; i++)
    {
      track_lane_update_frames (self->lanes[i]);
    }
  for (i = 0; i < self->num_chord_regions; i++)
    {
      arranger_object_update_frames (
        (ArrangerObject *)
        self->chord_regions[i]);
    }
  for (i = 0; i < self->num_scales; i++)
    {
      arranger_object_update_frames (
        (ArrangerObject *)
        self->scales[i]);
    }
  for (i = 0; i < self->num_markers; i++)
    {
      arranger_object_update_frames (
        (ArrangerObject *)
        self->markers[i]);
    }

  automation_tracklist_update_frames (
    &self->automation_tracklist);
}

/**
 * Wrapper to get the track name.
 */
const char *
track_get_name (Track * track)
{
  if (DEBUGGING)
    {
      return
        g_strdup_printf (
          "%d %s",
          track->pos, track->name);
    }
  return track->name;
}

/**
 * Setter for the track name.
 *
 * If the track name is duplicate, it discards the
 * new name.
 *
 * Must only be called from the GTK thread.
 */
void
track_set_name (
  Track * track,
  const char *  name)
{
  if (tracklist_track_name_is_unique (
        TRACKLIST, name))
    {
      if (track->name)
        g_free (track->name);
      track->name =
        g_strdup (name);

      if (track->channel)
        {
          /* update external ports */
          Port * ports[10000];
          int    num_ports = 0;
          channel_append_all_ports (
            track->channel, ports, &num_ports, 1);
          Port * port;
          for (int i = 0; i < num_ports; i++)
            {
              port = ports[i];

              if (port_is_exposed_to_backend (
                    port))
                {
                  port_rename_backend (port);
                }
            }
        }

      EVENTS_PUSH (ET_TRACK_NAME_CHANGED,
                   track);
    }
}
